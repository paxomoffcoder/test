#ifndef JSONTABLEMODEL_H
#define JSONTABLEMODEL_H

#include <QAbstractTableModel>
#include <QJsonArray>

class JsonTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit JsonTableModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    void setRows(const QJsonArray& rows, const QStringList& headers, const QStringList& keys);

private:
    QJsonArray m_rows;
    QStringList m_headers;
    QStringList m_keys;
};

#endif // JSONTABLEMODEL_H
