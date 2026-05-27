#include "jsontablemodel.h"

#include <QJsonObject>

JsonTableModel::JsonTableModel(QObject* parent)
    : QAbstractTableModel(parent)
{
}

int JsonTableModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
    {
        return 0;
    }

    return m_rows.size();
}

int JsonTableModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid())
    {
        return 0;
    }

    return m_headers.size();
}

QVariant JsonTableModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
    {
        return QVariant();
    }

    const QJsonObject row = m_rows.at(index.row()).toObject();
    return row.value(m_keys.at(index.column())).toVariant();
}

QVariant JsonTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
    {
        return QVariant();
    }

    if (orientation == Qt::Horizontal)
    {
        return m_headers.at(section);
    }

    return section + 1;
}

void JsonTableModel::setRows(const QJsonArray& rows, const QStringList& headers, const QStringList& keys)
{
    beginResetModel();
    m_rows = rows;
    m_headers = headers;
    m_keys = keys;
    endResetModel();
}
