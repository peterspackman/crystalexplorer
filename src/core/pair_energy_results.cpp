#include "pair_energy_results.h"

PairInteractionResult::PairInteractionResult(const QString& interactionModel, QObject* parent)
        : QObject(parent), m_interactionModel(interactionModel) {}

void PairInteractionResult::addComponent(const QString& component, double value) {
    m_components.append(qMakePair(component, value));
}

QList<QPair<QString, double>> PairInteractionResult::components() const {
    return m_components; 
}

QString PairInteractionResult::interactionModel() const {
    return m_interactionModel;
}

void PairInteractionResults::addPairInteractionResult(PairInteractionResult* result)
{
    qDebug() << "Adding result" << result;
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    m_pairInteractionResults.append(result);
    result->setParent(this);
    endInsertRows();
    emit resultAdded();
}

QList<PairInteractionResult*> PairInteractionResults::filterByModel(const QString& model) const
{
    QList<PairInteractionResult*> filteredResults;
    for (PairInteractionResult* result : m_pairInteractionResults) {
        if (result->interactionModel() == model) {
            filteredResults.append(result);
        }
    }
    return filteredResults;
}

void PairInteractionResults::removePairInteractionResult(PairInteractionResult* result)
{
    int index = m_pairInteractionResults.indexOf(result);
    if (index >= 0) {
        beginRemoveRows(QModelIndex(), index, index);
        m_pairInteractionResults.removeAt(index);
        endRemoveRows();
        emit resultRemoved();
    }
}

QList<PairInteractionResult*> PairInteractionResults::filterByComponent(const QString& component) const
{
    QList<PairInteractionResult*> filteredResults;
    for (PairInteractionResult* result : m_pairInteractionResults) {
        for (const auto& pair : result->components()) {
            if (pair.first == component) {
                filteredResults.append(result);
                break;
            }
        }
    }
    return filteredResults;
}

QList<PairInteractionResult*> PairInteractionResults::filterByModelAndComponent(const QString& model, const QString& component) const
{
    QList<PairInteractionResult*> filteredResults;
    for (PairInteractionResult* result : m_pairInteractionResults) {
        if (result->interactionModel() == model) {
            for (const auto& pair : result->components()) {
                if (pair.first == component) {
                    filteredResults.append(result);
                    break;
                }
            }
        }
    }
    return filteredResults;
}

PairInteractionResults::PairInteractionResults(QObject* parent) : QAbstractItemModel(parent) {}

QModelIndex PairInteractionResults::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent)) {
        return QModelIndex();
    }

    if (!parent.isValid()) {
        // Top-level item
        if (row < m_pairInteractionResults.size()) {
            return createIndex(row, column, m_pairInteractionResults[row]);
        }
    } else {
        // Child item
        PairInteractionResult* parentResult = static_cast<PairInteractionResult*>(parent.internalPointer());
        if (parentResult) {
            if (row < parentResult->components().size()) {
                return createIndex(row, column, &(parentResult->components()[row]));
            }
        }
    }

    return QModelIndex();
}

QModelIndex PairInteractionResults::parent(const QModelIndex& child) const
{
    if (!child.isValid()) {
        return QModelIndex();
    }

    if (PairInteractionResult* childResult = static_cast<PairInteractionResult*>(child.internalPointer())) {
        // Child item, find its parent result
        for (int i = 0; i < m_pairInteractionResults.size(); ++i) {
            if (m_pairInteractionResults[i] == childResult) {
                return createIndex(i, 0, m_pairInteractionResults[i]);
            }
        }
    }

    return QModelIndex();
}

int PairInteractionResults::rowCount(const QModelIndex& parent) const
{
    if (!parent.isValid()) {
        // Top-level item count
        if (m_pairInteractionResults.isEmpty()) {
            return 0; // Hide the pair interaction results if there are none
        } else {
            return 1; // Show a single top-level item for the pair interaction results
        }
    } else {
        // Child item count
        if (PairInteractionResult* parentResult = static_cast<PairInteractionResult*>(parent.internalPointer())) {
            return parentResult->components().size();
        }
    }

    return 0;
}

int PairInteractionResults::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return 2; // Assuming we have two columns: component name and value
}

QVariant PairInteractionResults::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    if (role == Qt::DisplayRole) {
        if (!index.parent().isValid()) {
            // Top-level item
            if (index.row() == 0) {
                return "Pair Interactions"; // Display a custom label for the top-level item
            }
        } else {
            // Child item (component)
            if (PairInteractionResult* result = static_cast<PairInteractionResult*>(index.parent().internalPointer())) {
                QPair<QString, double> component = result->components()[index.row()];
                if (index.column() == 0) {
                    return component.first; // Component name
                } else if (index.column() == 1) {
                    return component.second; // Component value
                }
            }
        }
    }

    return QVariant();
}
