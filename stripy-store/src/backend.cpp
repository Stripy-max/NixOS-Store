#include "backend.h"
#include "logic.h"

#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QSet>
#include <QSettings>
#include <QStandardPaths>
#include <QSysInfo>
#include <QUrl>
#include <algorithm>

namespace {
inline std::string S(const QString &s) { return s.toStdString(); }
constexpr int kMaxLog = 200000;
const char *kMetaExpr =
    "p: let m = p.meta or {}; "
    "f = x: if builtins.isAttrs x then (x.spdxId or x.shortName or x.fullName or \"\") else toString x; "
    "l = m.license or null; h = m.homepage or null; "
    "in { version = p.version or null; "
    "homepage = if builtins.isList h then (if h == [] then null else builtins.head h) else h; "
    "unfree = m.unfree or false; "
    "license = if l == null then null else if builtins.isList l then map f l else f l; }";
}

// ───────────────────────── construction & settings ─────────────────────────

Backend::Backend(QObject *parent) : QObject(parent)
{
    const QString data = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    m_logFile = data + "/operations.log";

    auto stateFn = [this](const QString &a) { return stateOf(a); };
    auto catFn = [this](const QString &id) { return categoryName(id); };
    m_model = new AppModel(stateFn, catFn, this);
    m_featured = new AppModel(stateFn, catFn, this);
    m_picks = new AppModel(stateFn, catFn, this);

    m_searchTimeout.setSingleShot(true);
    connect(&m_searchTimeout, &QTimer::timeout, this, [this] {
        if (m_search) {
            m_searchNotice = tr("The online search took too long and was stopped. Showing catalog results only.");
            m_search->kill();
        }
    });

    loadSettings();
    loadCatalog();
    recheck();
    rebuildHome();
    refreshView();
}

QString Backend::appVersion() const
{
#ifdef STRIPY_VERSION
    return QStringLiteral(STRIPY_VERSION);
#else
    return QStringLiteral("dev");
#endif
}

void Backend::loadSettings()
{
    QSettings s;
    m_appearance = s.value("appearance", "system").toString();
    if (m_appearance != "light" && m_appearance != "dark") m_appearance = "system";
    m_confirmInstall = s.value("confirmInstall", true).toBool();

    const QString home = QDir::homePath();
    QString dir = s.value("flakeDir").toString();
    if (dir.isEmpty()) dir = qEnvironmentVariable("STRIPY_DIR");
    if (dir.isEmpty()) {
        for (const QString &c : {home + "/stripyos", QString("/etc/stripyos")})
            if (QFileInfo::exists(c + "/flake.nix")) { dir = c; break; }
    }
    if (dir.isEmpty()) dir = home + "/stripyos";
    m_dir = QDir::cleanPath(dir);

    QString host = s.value("host").toString();
    if (host.isEmpty()) host = qEnvironmentVariable("STRIPY_HOST");
    if (host.isEmpty()) host = QSysInfo::machineHostName();
    m_host = host;
}

void Backend::setAppearance(const QString &a)
{
    const QString v = (a == "light" || a == "dark") ? a : "system";
    if (v == m_appearance) return;
    m_appearance = v;
    QSettings().setValue("appearance", v);
    emit appearanceChanged();
}

void Backend::setConfirmInstall(bool b)
{
    if (b == m_confirmInstall) return;
    m_confirmInstall = b;
    QSettings().setValue("confirmInstall", b);
    emit confirmInstallChanged();
}

QString Backend::setConfig(const QString &dir, const QString &host)
{
    const QString d = QDir::cleanPath(dir.trimmed());
    const QString h = host.trimmed();
    if (!stripy::isSafeFlakeDir(S(d))) return tr("The folder must be an absolute path without the characters # ? or quotes.");
    if (!stripy::isValidHost(S(h))) return tr("The configuration name may only contain letters, digits, '.', '_' and '-'.");
    if (m_busy) return tr("Wait for the running operation to finish.");
    QSettings s;
    s.setValue("flakeDir", d);
    s.setValue("host", h);
    m_dir = d;
    m_host = h;
    recheck();
    return QString();
}

// ───────────────────────── files ─────────────────────────

QString Backend::appsPath() const { return m_dir + "/apps.json"; }
QString Backend::appliedPath() const
{
    return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/applied.json";
}

QByteArray Backend::readFile(const QString &path)
{
    QFile f(path);
    return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();
}

bool Backend::writeAtomic(const QString &path, const QByteArray &data, QString *err)
{
    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly)) { if (err) *err = f.errorString(); return false; }
    if (f.write(data) != data.size()) { if (err) *err = f.errorString(); f.cancelWriting(); return false; }
    if (!f.commit()) { if (err) *err = f.errorString(); return false; }
    return true;
}

QByteArray Backend::serialize(const QStringList &list)
{
    QJsonArray a;
    for (const QString &s : list) a.append(s);
    return QJsonDocument(a).toJson(QJsonDocument::Indented);
}

void Backend::loadRequested()
{
    m_requested.clear();
    m_requestedValid = false;
    QFile f(appsPath());
    if (!f.open(QIODevice::ReadOnly)) return;
    QJsonParseError pe;
    const auto doc = QJsonDocument::fromJson(f.readAll(), &pe);
    if (pe.error != QJsonParseError::NoError || !doc.isArray()) return;
    QStringList out;
    for (const auto &v : doc.array()) {
        const QString a = v.toString();
        if (!stripy::isValidAttr(S(a))) return;        // refuse to touch a file we do not understand
        if (!out.contains(a)) out << a;
    }
    m_requested = out;
    m_requestedValid = true;
}

void Backend::loadApplied()
{
    m_applied.clear();
    m_appliedKnown = false;
    const auto doc = QJsonDocument::fromJson(readFile(appliedPath()));
    if (!doc.isObject()) return;
    const auto o = doc.object();
    if (o.value("dir").toString() != m_dir || o.value("host").toString() != m_host) return;
    for (const auto &v : o.value("apps").toArray()) {
        const QString a = v.toString();
        if (stripy::isValidAttr(S(a))) m_applied << a;
    }
    m_appliedKnown = true;
}

void Backend::writeApplied()
{
    QJsonObject o;
    o["dir"] = m_dir;
    o["host"] = m_host;
    QJsonArray a;
    for (const QString &s : m_applied) a.append(s);
    o["apps"] = a;
    QDir().mkpath(QFileInfo(appliedPath()).absolutePath());
    QString err;
    if (!writeAtomic(appliedPath(), QJsonDocument(o).toJson(), &err))
        m_log += "\n[warning] could not record applied state: " + err + "\n";
}

void Backend::readPin()
{
    m_pinInfo.clear();
    const auto doc = QJsonDocument::fromJson(readFile(m_dir + "/flake.lock"));
    if (doc.isObject()) {
        const auto nodes = doc.object().value("nodes").toObject();
        const auto in = nodes.value("root").toObject().value("inputs").toObject().value("nixpkgs");
        const QString node = in.isString() ? in.toString() : QStringLiteral("nixpkgs");
        const auto locked = nodes.value(node).toObject().value("locked").toObject();
        const QString rev = locked.value("rev").toString().left(7);
        const qint64 t = static_cast<qint64>(locked.value("lastModified").toDouble());
        if (!rev.isEmpty()) {
            m_pinInfo = "nixpkgs " + rev;
            if (t > 0) m_pinInfo += " · " + QDateTime::fromSecsSinceEpoch(t).toUTC().toString("yyyy-MM-dd");
        }
    }
    emit pinInfoChanged();
}

// ───────────────────────── catalog ─────────────────────────

bool Backend::loadCatalog()
{
    m_catalogWarnings.clear();
    QStringList candidates;
    const QString env = qEnvironmentVariable("STRIPY_CATALOG");
    if (!env.isEmpty()) candidates << env;
    candidates << m_dir + "/catalog.json";
    for (const QString &c : candidates) {
        if (!QFileInfo::exists(c)) continue;
        if (parseCatalog(readFile(c), c)) return true;
        m_catalogWarnings << tr("Ignored invalid catalog %1; using the built-in catalog.").arg(c);
    }
    QFile f(":/data/catalog.json");
    if (f.open(QIODevice::ReadOnly) && parseCatalog(f.readAll(), "built-in")) return true;
    m_catalogWarnings << tr("The built-in catalog could not be loaded. Search still works if Nix is available.");
    return false;
}

bool Backend::parseCatalog(const QByteArray &data, const QString &origin)
{
    QJsonParseError pe;
    const auto doc = QJsonDocument::fromJson(data, &pe);
    if (pe.error != QJsonParseError::NoError || !doc.isObject()) return false;
    const auto root = doc.object();
    if (root.value("schema").toInt() != 2) return false;

    QStringList warnings;
    QHash<QString, QString> names, icons;
    QStringList order;
    for (const auto &v : root.value("categories").toArray()) {
        const auto o = v.toObject();
        const QString id = o.value("id").toString().trimmed();
        const QString name = o.value("name").toString().trimmed();
        if (id.isEmpty() || name.isEmpty() || names.contains(id)) { warnings << tr("%1: skipped an invalid category").arg(origin); continue; }
        names[id] = name;
        icons[id] = o.value("icon").toString("applications-other");
        order << id;
    }

    QVector<AppEntry> apps;
    QSet<QString> seen;
    for (const auto &v : root.value("apps").toArray()) {
        const auto o = v.toObject();
        AppEntry e;
        e.attr = o.value("attr").toString().trimmed();
        if (!stripy::isValidAttr(S(e.attr)) || seen.contains(e.attr)) {
            warnings << tr("%1: skipped entry with invalid or duplicate attribute '%2'").arg(origin, e.attr);
            continue;
        }
        seen.insert(e.attr);
        e.name = o.value("name").toString().trimmed();
        if (e.name.isEmpty()) e.name = QString::fromStdString(stripy::humanName(S(e.attr)));
        e.description = o.value("description").toString().trimmed();
        e.version = o.value("version").toString();
        e.icon = o.value("icon").toString();
        for (const auto &c : o.value("categories").toArray())
            if (names.contains(c.toString()) && !e.categories.contains(c.toString())) e.categories << c.toString();
        for (const auto &k : o.value("keywords").toArray()) e.keywords << k.toString();
        const QString lic = o.value("license").toString();
        e.license = (lic == "free" || lic == "unfree") ? lic : QStringLiteral("unknown");
        const QString hp = o.value("homepage").toString();
        if (stripy::isHttpUrl(S(hp))) e.homepage = hp;
        else if (!hp.isEmpty()) warnings << tr("%1: ignored non-http homepage for %2").arg(origin, e.attr);
        e.rank = o.value("rank").toInt(1000);
        e.featured = o.value("featured").toBool();
        e.curated = true;
        apps << e;
    }
    if (apps.isEmpty()) return false;

    m_catalog = apps;
    m_catalogWarnings << warnings;
    m_catNames = names;
    m_catOrder = order;
    m_categories.clear();
    for (const QString &id : order) {
        int n = 0;
        for (const auto &e : apps) if (e.categories.contains(id)) ++n;
        if (n == 0) continue;                                  // never show empty categories
        m_categories.append(QVariantMap{{"id", id}, {"name", names[id]}, {"icon", icons[id]}, {"count", n}});
    }
    m_known.clear();
    for (const auto &e : apps) m_known.insert(e.attr, e);
    return true;
}

QString Backend::categoryName(const QString &id) const { return m_catNames.value(id); }

// ───────────────────────── health / integration check ─────────────────────────

void Backend::recheck()
{
    m_problems.clear();
    m_canCreateApps = false;

    auto find = [](const QString &name, const QStringList &preferred) {
        for (const QString &p : preferred)
            if (QFileInfo(p).isExecutable()) return p;
        return QStandardPaths::findExecutable(name);
    };
    m_nix = find("nix", {});
    m_rebuild = find("nixos-rebuild", {"/run/current-system/sw/bin/nixos-rebuild"});
    m_pkexec = find("pkexec", {"/run/wrappers/bin/pkexec"});

    bool unsupported = false, setup = false;
    if (m_nix.isEmpty()) { m_problems << tr("Nix was not found, so only the built-in catalog can be browsed."); unsupported = true; }
    if (m_rebuild.isEmpty()) { m_problems << tr("This system does not look like NixOS (nixos-rebuild was not found). Installing is disabled."); unsupported = true; }
    if (m_pkexec.isEmpty()) { m_problems << tr("pkexec (polkit) was not found, so administrator authorization is not possible."); unsupported = true; }

    if (!stripy::isSafeFlakeDir(S(m_dir))) {
        m_problems << tr("The configuration folder “%1” is not a valid absolute path.").arg(m_dir);
        setup = true;
    } else if (!QFileInfo::exists(m_dir + "/flake.nix")) {
        m_problems << tr("No flake.nix was found in %1. Choose your StripyOS configuration folder in Settings.").arg(m_dir);
        setup = true;
    } else {
        bool referenced = false;
        QDirIterator it(m_dir, {"*.nix"}, QDir::Files, QDirIterator::Subdirectories);
        int scanned = 0;
        while (it.hasNext() && scanned < 300 && !referenced) {
            const QString p = it.next();
            if (p.contains("/.git/") || p.contains("/result/")) continue;
            ++scanned;
            const QByteArray c = readFile(p);
            referenced = c.size() < 1000000 && (c.contains("stripy-apps") || c.contains("stripy.apps"));
        }
        if (!referenced) {
            m_problems << tr("The StripyOS apps module is not part of this configuration (no reference to stripy-apps.nix). "
                             "Without it, installing would not change the system. See nix/example-flake.nix.");
            setup = true;
        }
        if (!QFileInfo::exists(appsPath())) {
            m_problems << tr("apps.json does not exist yet in %1.").arg(m_dir);
            m_canCreateApps = true;
            setup = true;
            m_requested.clear();
            m_requestedValid = false;
        }
    }
    if (QFileInfo::exists(appsPath())) {
        loadRequested();
        if (!m_requestedValid) {
            m_problems << tr("apps.json could not be read (invalid JSON or invalid package names). It was left untouched.");
            setup = true;
        }
    }
    if (!stripy::isValidHost(S(m_host))) {
        m_problems << tr("The configuration name “%1” is not valid. Set it in Settings.").arg(m_host);
        setup = true;
    }
    m_health = unsupported ? "unsupported" : setup ? "needs-setup" : "ok";
    loadApplied();
    readPin();
    emit healthChanged();
    bumpState();
    refreshView();
}

void Backend::createAppsFile()
{
    if (m_busy || !stripy::isSafeFlakeDir(S(m_dir)) || !QFileInfo::exists(m_dir + "/flake.nix")) return;
    if (QFileInfo::exists(appsPath())) return;                  // never overwrite
    QString err;
    if (!writeAtomic(appsPath(), "[]\n", &err)) {
        postResult(false, tr("Could not create apps.json: %1").arg(err));
        return;
    }
    recheck();
}

QString Backend::scope() const
{
    return tr("System-wide, for all users · configuration %1#%2").arg(m_dir, m_host);
}

int Backend::installedCount() const
{
    QSet<QString> s;
    for (const auto &a : m_requested) s.insert(a);
    for (const auto &a : m_applied) s.insert(a);
    return s.size();
}

int Backend::pendingCount() const
{
    QSet<QString> all;
    for (const auto &a : m_requested) all.insert(a);
    for (const auto &a : m_applied) all.insert(a);
    int n = 0;
    for (const auto &a : all) {
        const QString s = stateOf(a);
        if (s.startsWith("pending") || s == "unverified") ++n;
    }
    return n;
}

void Backend::bumpState()
{
    ++m_stateRev;
    m_model->refreshStates();
    m_featured->refreshStates();
    m_picks->refreshStates();
    emit stateRevisionChanged();
}

QString Backend::stateOf(const QString &attr) const
{
    if (m_busy && attr == m_opAttr && (m_opKind == "install" || m_opKind == "remove"))
        return m_opKind == "remove" ? "removing" : "installing";
    const auto f = m_failed.constFind(attr);
    if (f != m_failed.constEnd()) return f->add ? "failed-install" : "failed-remove";
    const bool req = m_requested.contains(attr);
    if (!m_appliedKnown) return req ? "unverified" : "none";
    const bool app = m_applied.contains(attr);
    if (req && app) return "installed";
    if (req) return "pending-install";
    if (app) return "pending-remove";
    return "none";
}

QString Backend::failureOf(const QString &attr) const { return m_failed.value(attr).message; }

// ───────────────────────── browsing ─────────────────────────

void Backend::showHome()
{
    cancelSearch();
    m_view = "home"; m_viewArg.clear(); m_query.clear(); m_sort = "recommended";
    emit viewChanged();
}

void Backend::showCategory(const QString &id)
{
    cancelSearch();
    if (!m_catNames.contains(id)) return;
    m_view = "category"; m_viewArg = id; m_query.clear();
    if (m_sort == "relevance") m_sort = "recommended";
    emit viewChanged();
    refreshView();
}

void Backend::showInstalled()
{
    cancelSearch();
    m_view = "installed"; m_viewArg.clear(); m_query.clear();
    if (m_sort == "relevance") m_sort = "recommended";
    emit viewChanged();
    refreshView();
}

void Backend::setSort(const QString &id)
{
    static const QStringList ok = {"relevance", "recommended", "az", "za", "installed"};
    if (!ok.contains(id) || id == m_sort) return;
    if (id == "relevance" && m_view != "search") return;
    m_sort = id;
    emit viewChanged();
    refreshView();
}

QString Backend::viewTitle() const
{
    if (m_view == "category") return m_catNames.value(m_viewArg);
    if (m_view == "installed") return tr("Installed");
    if (m_view == "search") return m_query.isEmpty() ? tr("Search") : tr("Results for “%1”").arg(m_query);
    return tr("Discover");
}

void Backend::search(const QString &text)
{
    const QString q = text.simplified();
    cancelSearch();
    if (q.isEmpty()) { showHome(); return; }
    if (m_view != "search") m_sort = "relevance";
    m_view = "search"; m_viewArg.clear(); m_query = q;
    m_searchNotice.clear();
    emit viewChanged();
    refreshView();                                   // instant local results
    const QString key = q.toLower();
    if (q.size() < 3 || m_nix.isEmpty()) {
        if (m_nix.isEmpty()) { m_searchNotice = tr("Online search is unavailable because Nix was not found."); emit searchingChanged(); }
        return;
    }
    if (m_searchCache.contains(key)) return;         // already merged by refreshView()
    startRemoteSearch(q);
}

void Backend::cancelSearch()
{
    m_searchTimeout.stop();
    if (m_search) {
        QProcess *p = m_search;
        m_search = nullptr;                          // its finished() handler ignores stale processes
        p->kill();
    }
    if (m_searching || !m_searchNotice.isEmpty()) {
        m_searching = false;
        m_searchNotice.clear();
        emit searchingChanged();
    }
}

void Backend::startRemoteSearch(const QString &query)
{
    QStringList args = {"--extra-experimental-features", "nix-command flakes", "search", "nixpkgs"};
    const QStringList words = query.split(' ', Qt::SkipEmptyParts);
    for (int i = 0; i < words.size() && i < 4; ++i)
        args << QString::fromStdString(stripy::escapeRegex(S(words[i])));
    args << "--json";

    auto *p = new QProcess(this);
    m_search = p;
    m_searching = true;
    m_searchNotice.clear();
    emit searchingChanged();
    m_searchTimeout.start(90000);

    connect(p, &QProcess::errorOccurred, this, [this, p](QProcess::ProcessError e) {
        if (e != QProcess::FailedToStart) return;
        if (p == m_search) {
            m_search = nullptr;
            m_searching = false;
            m_searchNotice = tr("Online search could not be started.");
            emit searchingChanged();
        }
        p->deleteLater();
    });
    connect(p, &QProcess::finished, this, [this, p, query](int code, QProcess::ExitStatus st) {
        const bool current = (p == m_search);
        const QByteArray out = p->readAllStandardOutput();
        const QString err = QString::fromUtf8(p->readAllStandardError());
        p->deleteLater();
        if (!current) return;                        // superseded by a newer query
        m_search = nullptr;
        m_searchTimeout.stop();
        m_searching = false;
        if (code == 0 && st == QProcess::NormalExit) {
            QVector<AppEntry> hits;
            const auto obj = QJsonDocument::fromJson(out).object();
            for (auto it = obj.begin(); it != obj.end() && hits.size() < 60; ++it) {
                const QString attr = it.key().section('.', 2);       // legacyPackages.<system>.<attr>
                if (!stripy::isValidAttr(S(attr))) continue;
                const auto v = it.value().toObject();
                AppEntry e;
                e.attr = attr;
                const QString pname = v.value("pname").toString();
                e.name = QString::fromStdString(stripy::humanName(S(pname.isEmpty() ? attr : pname)));
                e.description = v.value("description").toString();
                e.version = v.value("version").toString();
                hits << e;
            }
            m_searchCache.insert(query.toLower(), hits);
            for (const auto &h : hits) if (!m_known.contains(h.attr)) m_known.insert(h.attr, h);
            if (m_view == "search" && m_query == query) refreshView();
        } else if (m_searchNotice.isEmpty()) {
            const QString first = err.section('\n', 0, 0).trimmed();
            const QString low = err.toLower();
            if (low.contains("resolve host") || low.contains("unable to download") || low.contains("network"))
                m_searchNotice = tr("No network connection. Showing catalog results only.");
            else
                m_searchNotice = tr("Online search failed (%1). Showing catalog results only.").arg(first.left(120));
        }
        emit searchingChanged();
    });
    p->start(m_nix, args);
}

void Backend::sortEntries(QVector<AppEntry> &v) const
{
    auto byName = [](const AppEntry &a, const AppEntry &b) { return a.name.compare(b.name, Qt::CaseInsensitive) < 0; };
    if (m_sort == "relevance") {
        std::stable_sort(v.begin(), v.end(), [&](const AppEntry &a, const AppEntry &b) {
            if (a.score != b.score) return a.score > b.score;
            if (a.curated != b.curated) return a.curated;
            return byName(a, b);
        });
    } else if (m_sort == "az") {
        std::stable_sort(v.begin(), v.end(), byName);
    } else if (m_sort == "za") {
        std::stable_sort(v.begin(), v.end(), [&](const AppEntry &a, const AppEntry &b) { return byName(b, a); });
    } else if (m_sort == "installed") {
        std::stable_sort(v.begin(), v.end(), [&](const AppEntry &a, const AppEntry &b) {
            const bool ia = stateOf(a.attr) != "none", ib = stateOf(b.attr) != "none";
            if (ia != ib) return ia;
            return byName(a, b);
        });
    } else {                                              // recommended
        std::stable_sort(v.begin(), v.end(), [&](const AppEntry &a, const AppEntry &b) {
            if (a.curated != b.curated) return a.curated;
            if (a.rank != b.rank) return a.rank < b.rank;
            return byName(a, b);
        });
    }
}

void Backend::refreshView()
{
    QVector<AppEntry> list;
    if (m_view == "category") {
        for (const auto &e : m_catalog) if (e.categories.contains(m_viewArg)) list << e;
    } else if (m_view == "installed") {
        QStringList attrs = m_requested;
        for (const auto &a : m_applied) if (!attrs.contains(a)) attrs << a;
        for (const auto &a : attrs) {
            AppEntry e = m_known.value(a);
            if (e.attr.isEmpty()) { e.attr = a; e.name = QString::fromStdString(stripy::humanName(S(a))); }
            list << e;
        }
    } else if (m_view == "search") {
        QSet<QString> have;
        for (auto e : m_catalog) {
            stripy::SearchFields f{S(e.name), S(e.attr), S(e.description), {}};
            for (const auto &k : e.keywords) f.keywords.push_back(S(k));
            e.score = stripy::matchScore(S(m_query), f);
            if (e.score > 0) { list << e; have.insert(e.attr); }
        }
        for (auto e : m_searchCache.value(m_query.toLower())) {
            if (have.contains(e.attr)) continue;
            e.score = 1;
            list << e;
        }
    } else {
        m_model->setEntries({});
        return;
    }
    sortEntries(list);
    m_model->setEntries(list);
}

void Backend::rebuildHome()
{
    auto order = [&](const AppEntry &e) {
        const int c = e.categories.isEmpty() ? 99 : int(m_catOrder.indexOf(e.categories.first()));
        return c;
    };
    QVector<AppEntry> feat, picks;
    for (const auto &e : m_catalog) if (e.featured) feat << e;
    std::stable_sort(feat.begin(), feat.end(), [&](const AppEntry &a, const AppEntry &b) {
        return order(a) != order(b) ? order(a) < order(b) : a.rank < b.rank;
    });
    if (feat.size() > 6) feat.resize(6);
    QSet<QString> used;
    for (const auto &e : feat) used.insert(e.attr);
    for (const QString &cat : m_catOrder) {
        const AppEntry *best = nullptr;
        for (const auto &e : m_catalog)
            if (e.categories.contains(cat) && !used.contains(e.attr) && (!best || e.rank < best->rank)) best = &e;
        if (best) { picks << *best; used.insert(best->attr); }
        if (picks.size() >= 8) break;
    }
    m_featured->setEntries(feat);
    m_picks->setEntries(picks);
}

QVariantMap Backend::appInfo(const QString &attr) const
{
    AppEntry e = m_known.value(attr);
    if (e.attr.isEmpty()) { e.attr = attr; e.name = QString::fromStdString(stripy::humanName(S(attr))); }
    QStringList cats;
    for (const auto &c : e.categories) cats << m_catNames.value(c);
    return {
        {"attr", e.attr}, {"name", e.name}, {"description", e.description}, {"version", e.version},
        {"homepage", e.homepage}, {"license", e.license}, {"categoriesText", cats.join(" · ")},
        {"curated", e.curated}, {"state", stateOf(attr)}, {"failure", failureOf(attr)},
        {"source", QStringLiteral("nixpkgs")},
        {"licenseText", m_details.value(attr).value("licenseText").toString()},
        {"detailsLoaded", m_details.contains(attr)},
    };
}

void Backend::fetchDetails(const QString &attr)
{
    if (!stripy::isValidAttr(S(attr)) || m_nix.isEmpty() || m_details.contains(attr)) return;
    if (m_detail) { QProcess *old = m_detail; m_detail = nullptr; old->kill(); }
    auto *p = new QProcess(this);
    m_detail = p;
    connect(p, &QProcess::errorOccurred, this, [this, p](QProcess::ProcessError e) {
        if (e == QProcess::FailedToStart) { if (p == m_detail) m_detail = nullptr; p->deleteLater(); }
    });
    connect(p, &QProcess::finished, this, [this, p, attr](int code, QProcess::ExitStatus st) {
        const bool current = (p == m_detail);
        const QByteArray out = p->readAllStandardOutput();
        p->deleteLater();
        if (!current) return;
        m_detail = nullptr;
        if (code != 0 || st != QProcess::NormalExit) return;     // details are optional; UI keeps what it has
        const auto o = QJsonDocument::fromJson(out).object();
        AppEntry e = m_known.value(attr);
        if (e.attr.isEmpty()) { e.attr = attr; e.name = QString::fromStdString(stripy::humanName(S(attr))); }
        const QString ver = o.value("version").toString();
        if (!ver.isEmpty()) e.version = ver;
        const QString hp = o.value("homepage").toString();
        if (stripy::isHttpUrl(S(hp))) e.homepage = hp;
        e.license = o.value("unfree").toBool() ? "unfree" : "free";
        m_known.insert(attr, e);
        m_details.insert(attr, o.toVariantMap());
        QStringList lic;
        const auto lv = o.value("license");
        if (lv.isArray()) for (const auto &x : lv.toArray()) lic << x.toString();
        else if (lv.isString()) lic << lv.toString();
        m_details[attr]["licenseText"] = lic.join(", ");
        emit detailsReady(attr);
    });
    p->start(m_nix, {"--extra-experimental-features", "nix-command flakes", "eval", "--json",
                     "nixpkgs#" + attr, "--apply", QString::fromUtf8(kMetaExpr)});
}

void Backend::openHomepage(const QString &url)
{
    if (stripy::isHttpUrl(S(url))) QDesktopServices::openUrl(QUrl(url));
}

// ───────────────────────── operations ─────────────────────────

void Backend::postResult(bool ok, const QString &text)
{
    m_hasResult = true;
    m_resultOk = ok;
    m_resultText = text;
    emit resultChanged();
}

void Backend::dismissResult()
{
    if (!m_hasResult) return;
    m_hasResult = false;
    emit resultChanged();
}

void Backend::setStage(const QString &s)
{
    if (s == m_stage) return;
    m_stage = s;
    emit stageChanged();
}

bool Backend::canOperate(QString *why) const
{
    if (m_busy) { *why = tr("Another operation is still running."); return false; }
    if (m_health != "ok") {
        *why = m_problems.isEmpty() ? tr("The system integration is not set up.") : m_problems.first();
        return false;
    }
    if (!m_requestedValid) { *why = tr("apps.json could not be read."); return false; }
    return true;
}

Backend::Step Backend::rebuildStep() const
{
    Step s;
    s.label = tr("Applying system configuration…");
    s.failure = tr("The system configuration could not be applied.");
    s.program = m_pkexec;
    s.args = {m_rebuild, "switch", "--flake", "path:" + m_dir + "#" + m_host};
    s.privileged = true;
    return s;
}

void Backend::install(const QString &attr) { changeRequested(attr, true); }
void Backend::remove(const QString &attr) { changeRequested(attr, false); }

void Backend::changeRequested(const QString &attr, bool add)
{
    if (!stripy::isValidAttr(S(attr))) { postResult(false, tr("“%1” is not a valid package name.").arg(attr.left(60))); return; }
    QString why;
    if (!canOperate(&why)) { postResult(false, why); return; }

    QStringList next = m_requested;
    if (add) { if (!next.contains(attr)) next << attr; }
    else next.removeAll(attr);

    m_wrote.clear();
    m_prevContent.clear();
    if (next != m_requested) {
        const QByteArray before = readFile(appsPath());
        const QByteArray content = serialize(next);
        QString err;
        if (!writeAtomic(appsPath(), content, &err)) {          // nothing is started if saving fails
            postResult(false, tr("Could not save the application list (%1). Nothing was changed.").arg(err));
            return;
        }
        m_prevContent = before;
        m_wrote = content;
        m_requested = next;
    }
    m_failed.remove(attr);

    const QString name = appInfo(attr).value("name").toString();
    m_okText = add ? tr("%1 was installed.").arg(name) : tr("%1 was removed.").arg(name);
    startOperation(add ? "install" : "remove", attr,
                   add ? tr("Installing %1").arg(name) : tr("Removing %1").arg(name), {rebuildStep()});
}

void Backend::applyConfiguration()
{
    QString why;
    if (!canOperate(&why)) { postResult(false, why); return; }
    m_wrote.clear(); m_prevContent.clear();
    m_okText = tr("The configuration was applied.");
    startOperation("apply", QString(), tr("Applying configuration"), {rebuildStep()});
}

void Backend::updateSystem()
{
    QString why;
    if (!canOperate(&why)) { postResult(false, why); return; }
    Step up;
    up.label = tr("Updating the package source (nixpkgs)…");
    up.failure = tr("The package source could not be updated.");
    up.program = m_nix;
    up.args = {"--extra-experimental-features", "nix-command flakes", "flake", "update", "--flake", "path:" + m_dir, "nixpkgs"};
    up.altArgs = {"--extra-experimental-features", "nix-command flakes", "flake", "lock", "--update-input", "nixpkgs", "path:" + m_dir};
    m_wrote.clear(); m_prevContent.clear();
    m_okText = tr("StripyOS was updated.");
    startOperation("update", QString(), tr("Updating StripyOS"), {up, rebuildStep()});
}

void Backend::startOperation(const QString &kind, const QString &attr, const QString &title, const QList<Step> &steps)
{
    m_busy = true;
    m_opKind = kind; m_opAttr = attr; m_opTitle = title;
    m_steps = steps; m_stepIdx = 0; m_usedAlt = false;
    m_log.clear(); m_lineBuf.clear();
    m_hasResult = false;
    m_stage = tr("Preparing…");
    emit busyChanged(); emit stageChanged(); emit logChanged(); emit resultChanged();
    bumpState();
    setStage(m_steps.first().label);
    launchStep(m_steps.first().args);
}

void Backend::launchStep(const QStringList &args)
{
    const Step &step = m_steps.at(m_stepIdx);
    auto *p = new QProcess(this);
    m_proc = p;
    p->setProcessChannelMode(QProcess::MergedChannels);
    connect(p, &QProcess::readyRead, this, [this, p] { if (p == m_proc) appendOutput(p->readAll()); });
    connect(p, &QProcess::errorOccurred, this, [this, p](QProcess::ProcessError e) {
        if (e != QProcess::FailedToStart || p != m_proc) return;
        const QString prog = p->program();
        m_proc = nullptr;
        p->deleteLater();
        finishOperation(false, tr("Could not start %1.").arg(QFileInfo(prog).fileName()));
    });
    connect(p, &QProcess::finished, this, [this, p](int code, QProcess::ExitStatus st) {
        if (p != m_proc) return;
        appendOutput(p->readAll());
        m_proc = nullptr;
        p->deleteLater();
        onStepExit(code, st);
    });
    p->start(step.program, args);
}

void Backend::appendOutput(const QByteArray &chunk)
{
    if (chunk.isEmpty()) return;
    m_log += QString::fromUtf8(chunk);
    if (m_log.size() > kMaxLog) m_log = m_log.right(kMaxLog);
    emit logChanged();

    m_lineBuf += chunk;
    for (;;) {
        const int a = m_lineBuf.indexOf('\n'), b = m_lineBuf.indexOf('\r');
        const int i = a < 0 ? b : (b < 0 ? a : qMin(a, b));
        if (i < 0) break;
        const QString stage = QString::fromStdString(stripy::stageForLine(m_lineBuf.left(i).toStdString()));
        m_lineBuf.remove(0, i + 1);
        if (!stage.isEmpty()) setStage(stage);        // only stages that really appeared in the output
    }
    if (m_lineBuf.size() > 8192) m_lineBuf.clear();
}

void Backend::onStepExit(int code, QProcess::ExitStatus st)
{
    const Step step = m_steps.at(m_stepIdx);
    if (st == QProcess::CrashExit) {
        finishOperation(false, tr("The operation was interrupted before it finished."));
        return;
    }
    if (code == 0) {
        if (m_stepIdx + 1 < m_steps.size()) {
            ++m_stepIdx;
            m_usedAlt = false;
            setStage(m_steps.at(m_stepIdx).label);
            launchStep(m_steps.at(m_stepIdx).args);
            return;
        }
        finishOperation(true, m_okText);
        return;
    }
    if (step.privileged && (code == 126 || code == 127)) {
        finishOperation(false, code == 126 ? tr("Authorization was cancelled. Nothing was changed.")
                                           : tr("Authorization failed. Nothing was changed."), true);
        return;
    }
    if (!step.altArgs.isEmpty() && !m_usedAlt) {          // older Nix syntax for the same operation
        m_usedAlt = true;
        launchStep(step.altArgs);
        return;
    }
    QString msg = step.failure;
    const QString why = explainFailure(m_log);
    if (!why.isEmpty()) msg += " " + why;
    finishOperation(false, msg);
}

QString Backend::explainFailure(const QString &log)
{
    const QString l = log.toLower();
    if (l.contains("no space left on device")) return QObject::tr("The disk is full.");
    if (l.contains("has an unfree license")) return QObject::tr("This package is non-free and the system policy does not allow it yet.");
    if (l.contains("could not resolve host") || l.contains("unable to download") || l.contains("download buffer error"))
        return QObject::tr("A network error occurred. Check your internet connection and try again.");
    if (l.contains("attribute '") && l.contains("missing"))
        return QObject::tr("The package no longer exists in the configured package source.");
    if (l.contains("stripy: package") && l.contains("not found"))
        return QObject::tr("The package was not found in the configured package source.");
    if (l.contains("error:") && l.contains("evaluat")) return QObject::tr("The configuration could not be evaluated.");
    if (l.contains("build of") && l.contains("failed")) return QObject::tr("A package failed to build.");
    return QString();
}

void Backend::finishOperation(bool ok, QString message, bool quiet)
{
    const QString kind = m_opKind, attr = m_opAttr;
    if (ok) {
        m_applied = m_requested;                // a successful rebuild applies the whole list
        m_appliedKnown = true;
        writeApplied();
        m_failed.clear();
    } else if (kind == "install" || kind == "remove") {
        if (!m_wrote.isEmpty()) {
            // Restore the list only if nobody else touched the file since we wrote it.
            if (readFile(appsPath()) == m_wrote) {
                QString err;
                if (writeAtomic(appsPath(), m_prevContent, &err))
                    message += " " + tr("The requested change to your application list was withdrawn.");
                else
                    message += " " + tr("The application list still contains the requested change (%1).").arg(err);
            } else {
                message += " " + tr("apps.json was modified by something else, so it was left as it is.");
            }
            loadRequested();
        }
        if (!quiet) {                       // a dismissed password prompt is not a failed install
            Fail f;
            f.add = (kind == "install");
            f.message = message;
            m_failed.insert(attr, f);
        }
    }
    if (kind == "update") readPin();

    appendLogFile(ok, message);
    m_busy = false;
    m_opKind.clear(); m_opAttr.clear();
    m_stage.clear();
    m_steps.clear();
    postResult(ok, message);
    emit busyChanged(); emit stageChanged();
    bumpState();
    refreshView();
}

void Backend::appendLogFile(bool ok, const QString &message) const
{
    QDir().mkpath(QFileInfo(m_logFile).absolutePath());
    QFile f(m_logFile);
    if (f.exists() && f.size() > 512 * 1024) f.remove();
    if (!f.open(QIODevice::Append | QIODevice::Text)) return;
    f.write(QStringLiteral("\n=== %1 · %2 %3 · %4 ===\n%5\n")
                .arg(QDateTime::currentDateTime().toString(Qt::ISODate), m_opKind, m_opAttr,
                     ok ? "ok" : "FAILED", message).toUtf8());
    f.write(m_log.toUtf8());
}
