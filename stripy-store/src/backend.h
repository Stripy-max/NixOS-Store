#pragma once
#include <QHash>
#include <QObject>
#include <QProcess>
#include <QSet>
#include <QStringList>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>
#include "appmodel.h"

class Backend : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QObject *appModel READ appModel CONSTANT)
    Q_PROPERTY(QObject *featuredModel READ featuredModel CONSTANT)
    Q_PROPERTY(QObject *picksModel READ picksModel CONSTANT)
    Q_PROPERTY(QVariantList categories READ categories CONSTANT)
    Q_PROPERTY(QStringList catalogWarnings READ catalogWarnings CONSTANT)
    Q_PROPERTY(QString appVersion READ appVersion CONSTANT)

    // environment / integration health
    Q_PROPERTY(QString health READ health NOTIFY healthChanged)           // ok | needs-setup | unsupported
    Q_PROPERTY(QStringList problems READ problems NOTIFY healthChanged)
    Q_PROPERTY(bool canCreateAppsFile READ canCreateAppsFile NOTIFY healthChanged)
    Q_PROPERTY(QString scope READ scope NOTIFY healthChanged)
    Q_PROPERTY(QString flakeDir READ flakeDir NOTIFY healthChanged)
    Q_PROPERTY(QString host READ host NOTIFY healthChanged)
    Q_PROPERTY(QString pinInfo READ pinInfo NOTIFY pinInfoChanged)

    // current view
    Q_PROPERTY(QString view READ view NOTIFY viewChanged)
    Q_PROPERTY(QString viewTitle READ viewTitle NOTIFY viewChanged)
    Q_PROPERTY(QString sort READ sort NOTIFY viewChanged)
    Q_PROPERTY(QString query READ query NOTIFY viewChanged)
    Q_PROPERTY(bool searching READ searching NOTIFY searchingChanged)
    Q_PROPERTY(QString searchNotice READ searchNotice NOTIFY searchingChanged)
    Q_PROPERTY(int installedCount READ installedCount NOTIFY stateRevisionChanged)
    Q_PROPERTY(int pendingCount READ pendingCount NOTIFY stateRevisionChanged)

    // operations
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString opTitle READ opTitle NOTIFY busyChanged)
    Q_PROPERTY(QString stage READ stage NOTIFY stageChanged)
    Q_PROPERTY(QString log READ log NOTIFY logChanged)
    Q_PROPERTY(bool hasResult READ hasResult NOTIFY resultChanged)
    Q_PROPERTY(bool resultOk READ resultOk NOTIFY resultChanged)
    Q_PROPERTY(QString resultText READ resultText NOTIFY resultChanged)
    Q_PROPERTY(int stateRevision READ stateRevision NOTIFY stateRevisionChanged)
    Q_PROPERTY(QString logFilePath READ logFilePath CONSTANT)

    // preferences
    Q_PROPERTY(QString appearance READ appearance WRITE setAppearance NOTIFY appearanceChanged)
    Q_PROPERTY(bool confirmInstall READ confirmInstall WRITE setConfirmInstall NOTIFY confirmInstallChanged)

public:
    explicit Backend(QObject *parent = nullptr);

    QObject *appModel() { return m_model; }
    QObject *featuredModel() { return m_featured; }
    QObject *picksModel() { return m_picks; }
    QVariantList categories() const { return m_categories; }
    QStringList catalogWarnings() const { return m_catalogWarnings; }
    QString appVersion() const;
    QString health() const { return m_health; }
    QStringList problems() const { return m_problems; }
    bool canCreateAppsFile() const { return m_canCreateApps; }
    QString scope() const;
    QString flakeDir() const { return m_dir; }
    QString host() const { return m_host; }
    QString pinInfo() const { return m_pinInfo; }
    QString view() const { return m_view; }
    QString viewTitle() const;
    QString sort() const { return m_sort; }
    QString query() const { return m_query; }
    bool searching() const { return m_searching; }
    QString searchNotice() const { return m_searchNotice; }
    int installedCount() const;
    int pendingCount() const;
    bool busy() const { return m_busy; }
    QString opTitle() const { return m_opTitle; }
    QString stage() const { return m_stage; }
    QString log() const { return m_log; }
    bool hasResult() const { return m_hasResult; }
    bool resultOk() const { return m_resultOk; }
    QString resultText() const { return m_resultText; }
    int stateRevision() const { return m_stateRev; }
    QString logFilePath() const { return m_logFile; }
    QString appearance() const { return m_appearance; }
    void setAppearance(const QString &a);
    bool confirmInstall() const { return m_confirmInstall; }
    void setConfirmInstall(bool b);

    // navigation / browsing
    Q_INVOKABLE void showHome();
    Q_INVOKABLE void showCategory(const QString &id);
    Q_INVOKABLE void showInstalled();
    Q_INVOKABLE void search(const QString &text);
    Q_INVOKABLE void setSort(const QString &id);
    Q_INVOKABLE QVariantMap appInfo(const QString &attr) const;
    Q_INVOKABLE void fetchDetails(const QString &attr);
    Q_INVOKABLE void openHomepage(const QString &url);

    // operations (system scope)
    Q_INVOKABLE void install(const QString &attr);
    Q_INVOKABLE void remove(const QString &attr);
    Q_INVOKABLE void applyConfiguration();
    Q_INVOKABLE void updateSystem();
    Q_INVOKABLE void dismissResult();

    // setup
    Q_INVOKABLE QString setConfig(const QString &dir, const QString &host);  // returns "" or an error
    Q_INVOKABLE void createAppsFile();
    Q_INVOKABLE void recheck();
    Q_INVOKABLE QString stateOf(const QString &attr) const;
    Q_INVOKABLE QString failureOf(const QString &attr) const;

signals:
    void healthChanged();
    void pinInfoChanged();
    void viewChanged();
    void searchingChanged();
    void busyChanged();
    void stageChanged();
    void logChanged();
    void resultChanged();
    void stateRevisionChanged();
    void appearanceChanged();
    void confirmInstallChanged();
    void detailsReady(const QString &attr);

private:
    struct Step {
        QString label, program, failure;
        QStringList args, altArgs;
        bool privileged = false;
    };
    struct Fail { bool add = true; QString message; };

    // setup / files
    void loadSettings();
    bool loadCatalog();
    bool parseCatalog(const QByteArray &data, const QString &origin);
    void loadRequested();
    void loadApplied();
    void writeApplied();
    void readPin();
    QString appsPath() const;
    QString appliedPath() const;
    static QByteArray readFile(const QString &path);
    static bool writeAtomic(const QString &path, const QByteArray &data, QString *err);
    static QByteArray serialize(const QStringList &list);

    // browsing
    void refreshView();
    void sortEntries(QVector<AppEntry> &v) const;
    void rebuildHome();
    void cancelSearch();
    void startRemoteSearch(const QString &query);
    QString categoryName(const QString &id) const;

    // operations
    void changeRequested(const QString &attr, bool add);
    bool canOperate(QString *why) const;
    Step rebuildStep() const;
    void startOperation(const QString &kind, const QString &attr, const QString &title, const QList<Step> &steps);
    void launchStep(const QStringList &args);
    void onStepExit(int code, QProcess::ExitStatus st);
    void finishOperation(bool ok, QString message, bool quiet = false);
    void appendOutput(const QByteArray &chunk);
    void setStage(const QString &s);
    void postResult(bool ok, const QString &text);
    void bumpState();
    static QString explainFailure(const QString &log);
    void appendLogFile(bool ok, const QString &message) const;

    // configuration
    QString m_dir, m_host, m_nix, m_rebuild, m_pkexec;
    QString m_health = "unsupported", m_scopeNote, m_pinInfo;
    QStringList m_problems;
    bool m_canCreateApps = false;
    QString m_appearance = "system";
    bool m_confirmInstall = true;
    QString m_logFile;

    // catalog
    QVector<AppEntry> m_catalog;
    QHash<QString, AppEntry> m_known;           // catalog + every search hit seen
    QVariantList m_categories;
    QHash<QString, QString> m_catNames;
    QStringList m_catOrder, m_catalogWarnings;

    // state
    QStringList m_requested, m_applied;
    bool m_requestedValid = false, m_appliedKnown = false;
    QHash<QString, Fail> m_failed;
    int m_stateRev = 0;

    // view
    AppModel *m_model, *m_featured, *m_picks;
    QString m_view = "home", m_viewArg, m_query, m_sort = "recommended";
    QProcess *m_search = nullptr, *m_detail = nullptr;
    QTimer m_searchTimeout;
    QHash<QString, QVector<AppEntry>> m_searchCache;
    QHash<QString, QVariantMap> m_details;
    bool m_searching = false;
    QString m_searchNotice;

    // operation in flight
    bool m_busy = false, m_hasResult = false, m_resultOk = false, m_usedAlt = false;
    QString m_opKind, m_opAttr, m_opTitle, m_stage, m_log, m_resultText, m_okText;
    QList<Step> m_steps;
    int m_stepIdx = 0;
    QProcess *m_proc = nullptr;
    QByteArray m_lineBuf, m_wrote, m_prevContent;
};
