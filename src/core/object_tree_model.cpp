#include "object_tree_model.h"
#include "mesh.h"
#include <QEvent>
#include <QIcon>

namespace impl {
inline QModelIndex genericIndexFromObject(const QAbstractItemModel* model, QObject* object, const QModelIndex& parent = QModelIndex()) {
    if (!model || !object) return QModelIndex();

    QList<QModelIndex> matchingIndices = model->match(parent, Qt::UserRole + 1, QVariant::fromValue(static_cast<QObject*>(object)), 1, Qt::MatchRecursive);

    if (!matchingIndices.isEmpty()) {
        return matchingIndices.first();
    }

    return QModelIndex();
}
}


ObjectTreeModel::ObjectTreeModel(QObject *root, QObject *parent) : QAbstractItemModel(parent), m_root(root) {
    m_root->installEventFilter(this);
}

QModelIndex ObjectTreeModel::index(int row, int column, const QModelIndex &parent) const {
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    QObject *parentObject = !parent.isValid() ? m_root : static_cast<QObject*>(parent.internalPointer());
    QObject *childObject = parentObject->children().at(row);
    if (childObject)
        return createIndex(row, column, childObject);

    return QModelIndex();
}

QModelIndex ObjectTreeModel::parent(const QModelIndex &child) const {
    if (!child.isValid())
        return QModelIndex();

    QObject *childObject = static_cast<QObject*>(child.internalPointer());
    QObject *parentObject = childObject->parent();

    if (parentObject == m_root)
        return QModelIndex();

    QObject *grandparentObject = parentObject->parent();
    int row = grandparentObject->children().indexOf(parentObject);
    return createIndex(row, 0, parentObject);
}

int ObjectTreeModel::rowCount(const QModelIndex &parent) const {
    QObject *parentObject = !parent.isValid() ? m_root : static_cast<QObject*>(parent.internalPointer());
    return parentObject->children().count();
}

int ObjectTreeModel::columnCount(const QModelIndex &parent) const {
    Q_UNUSED(parent);
    return 2; // Adjust the number of columns as needed
}

QVariant ObjectTreeModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid())
        return QVariant();

    int col = index.column();
    QObject* itemObject = static_cast<QObject*>(index.internalPointer());
    if (role == Qt::DecorationRole) {
	if(col == 0) { // Visibility column
	    QVariant visibleProperty = itemObject->property("visible");
	    if (!visibleProperty.isNull()) {
		bool isVisible = visibleProperty.toBool();
		return QIcon(isVisible ? ":/images/tick.png" : ":/images/cross.png");
	    }
	}
	else if(col == 1) {
	    auto *mesh = qobject_cast<Mesh*>(itemObject);
	    if(mesh) {
		return QIcon(":/images/mesh.png");
	    }
	}
    }
    else if (role == Qt::DisplayRole && col == 1) {
	return QVariant(itemObject->objectName());
    } else if (role == Qt::UserRole) {
        return QVariant::fromValue(itemObject);
    }

    return QVariant();
}

bool ObjectTreeModel::eventFilter(QObject* obj, QEvent* event) {
    if (event->type() == QEvent::ChildAdded) {
        QChildEvent* childEvent = static_cast<QChildEvent*>(event);
        QObject* newChild = childEvent->child();
        if (newChild) {
            installEventFilterRecursive(newChild);  // Install event filter on the children of the new child

            QObject* parent = newChild->parent();
            QModelIndex parentIndex = indexFromObject(parent);
            int newRow = parent->children().indexOf(newChild);
            beginInsertRows(parentIndex, newRow, newRow);
            endInsertRows();
        }
    } else if (event->type() == QEvent::ChildRemoved) {
        QChildEvent* childEvent = static_cast<QChildEvent*>(event);
        QObject* removedChild = childEvent->child();
        if (removedChild) {
            removedChild->removeEventFilter(this);  // Stop monitoring the removed child
            removeEventFilterRecursive(removedChild);  // Remove event filter from the children of the removed child

            // Find the index of the removed child
            QModelIndex removedIndex = indexFromObject(removedChild);
            if (removedIndex.isValid()) {
                QModelIndex parentIndex = removedIndex.parent();
                int removedRow = removedIndex.row();
                beginRemoveRows(parentIndex, removedRow, removedRow);
                endRemoveRows();
            }
        }
    }
    return QAbstractItemModel::eventFilter(obj, event);
}

void ObjectTreeModel::installEventFilterRecursive(QObject* object) {
    object->installEventFilter(this);  // Monitor the new child
    emit childAdded(object);
    for (QObject* child : object->children()) {
        installEventFilterRecursive(child);
    }
}

void ObjectTreeModel::removeEventFilterRecursive(QObject* object) {
    object->removeEventFilter(this);
    emit childRemoved(object);
    for (QObject* child : object->children()) {
        removeEventFilterRecursive(child);
    }
}

QModelIndex ObjectTreeModel::indexFromObject(QObject* object, const QModelIndex& parent) const {
    if (!object)
        return QModelIndex();

    int rowCount = this->rowCount(parent);
    int columnCount = this->columnCount(parent);

    for (int row = 0; row < rowCount; ++row) {
        for (int column = 0; column < columnCount; ++column) {
            QModelIndex index = this->index(row, column, parent);
            if (!index.isValid())
                continue;

            QObject* indexObject = static_cast<QObject*>(index.internalPointer());
            if (indexObject == object)
                return index;

            QModelIndex childIndex = indexFromObject(object, index);
            if (childIndex.isValid())
                return childIndex;
        }
    }

    return QModelIndex();
}

QVariant ObjectTreeModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if(role != Qt::DisplayRole) return QVariant();

    if(orientation == Qt::Horizontal) {
	switch(section) {
	    case 0: return tr("Visibility");
	    case 1: return tr("Name");
	    default: return QVariant();
	}
    }
    return QVariant();
}
