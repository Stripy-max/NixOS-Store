#pragma once
#include <QAbstractListModel>
#include <QStringList>
#include <QVector>
#include <functional>

struct AppEntry {
    QString attr;            // exact nixpkgs attribute path, e.g. "kdePackages.kdenlive"
    QString name;            // display name
    QString description;
    QString version;         // only when known
    QString homepage;        // http(s) only
    QString icon;            // optional bundled icon file name (without extension)
    QStringList categories;  // category ids
    QStringList keywords;
    QString license = "unknown";   // "free" | "unfree" | "unknown"
    int rank = 100000;       // curated order (lower = earlier)
    bool featured = false;
    bool curated = false;    // comes from the catalog (vs. a raw nixpkgs search hit)
    int score = 0;           // search relevance
};

// One flat model reused for every list in the UI; per-row install state is
// asked from the backend so a state change only needs dataChanged().
class AppModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
public:
    enum Roles {
        AttrRole = Qt::UserRole + 1, NameRole, DescriptionRole, VersionRole, HomepageRole,
        IconNameRole, CategoriesTextRole, LicenseRole, InstallStateRole, CuratedRole
    };
    using Lookup = std::function<QString(const QString &)>;

    AppModel(Lookup stateOf, Lookup categoryName, QObject *parent = nullptr)
        : QAbstractListModel(parent), m_state(std::move(stateOf)), m_catName(std::move(categoryName)) {}

    int count() const { return m_e.size(); }
    int rowCount(const QModelIndex &parent = {}) const override { return parent.isValid() ? 0 : m_e.size(); }
    QHash<int, QByteArray> roleNames() const override;
    QVariant data(const QModelIndex &idx, int role) const override;

    void setEntries(QVector<AppEntry> e);
    void refreshStates();
    const QVector<AppEntry> &entries() const { return m_e; }

signals:
    void countChanged();

private:
    QVector<AppEntry> m_e;
    Lookup m_state, m_catName;
};
