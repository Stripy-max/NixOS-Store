#include "appmodel.h"

QHash<int, QByteArray> AppModel::roleNames() const
{
    return {
        {AttrRole, "attr"}, {NameRole, "name"}, {DescriptionRole, "description"},
        {VersionRole, "version"}, {HomepageRole, "homepage"}, {IconNameRole, "iconName"},
        {CategoriesTextRole, "categoriesText"}, {LicenseRole, "license"},
        {InstallStateRole, "installState"}, {CuratedRole, "curated"},
    };
}

QVariant AppModel::data(const QModelIndex &idx, int role) const
{
    if (!idx.isValid() || idx.row() < 0 || idx.row() >= m_e.size()) return {};
    const AppEntry &e = m_e.at(idx.row());
    switch (role) {
    case AttrRole: return e.attr;
    case NameRole: return e.name;
    case DescriptionRole: return e.description;
    case VersionRole: return e.version;
    case HomepageRole: return e.homepage;
    case IconNameRole: return e.icon;
    case LicenseRole: return e.license;
    case CuratedRole: return e.curated;
    case InstallStateRole: return m_state ? m_state(e.attr) : QString("none");
    case CategoriesTextRole: {
        QStringList names;
        for (const QString &id : e.categories) {
            const QString n = m_catName ? m_catName(id) : QString();
            if (!n.isEmpty()) names << n;
            if (names.size() == 2) break;
        }
        return names.join(QStringLiteral(" · "));
    }
    }
    return {};
}

void AppModel::setEntries(QVector<AppEntry> e)
{
    const int before = m_e.size();
    beginResetModel();
    m_e = std::move(e);
    endResetModel();
    if (before != m_e.size()) emit countChanged();
}

void AppModel::refreshStates()
{
    if (m_e.isEmpty()) return;
    emit dataChanged(index(0), index(m_e.size() - 1), {InstallStateRole});
}
