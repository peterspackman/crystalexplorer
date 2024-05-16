#pragma once
#include <QObject>
#include <QAbstractItemModel>

class ObjectTreeModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit ObjectTreeModel(QObject *root, QObject *parent = nullptr);

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    QModelIndex indexFromObject(QObject* object, const QModelIndex& parent = QModelIndex()) const;

signals:
    void childAdded(QObject *);
    void childRemoved(QObject *);
private:
    void installEventFilterRecursive(QObject* object);
    void removeEventFilterRecursive(QObject* object);
    bool eventFilter(QObject* obj, QEvent* event) override;

    QObject *m_root;
};

