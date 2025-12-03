#include "volume.h"
#include <QDebug>

Volume::Volume(const Dimensions &dims, QObject *parent) : QObject(parent)  {
    setDimensions(dims);
}

size_t Volume::numberOfPoints() const {
    return m_dimensions.counts(0) * m_dimensions.counts(1) * m_dimensions.counts(2);
}

size_t Volume::count(Eigen::Index dim) const {
    return m_dimensions.counts(dim);
}

void Volume::setDimensions(const Volume::Dimensions &dims) {
    // Validate dimension counts are positive
    if (dims.counts(0) <= 0 || dims.counts(1) <= 0 || dims.counts(2) <= 0) {
        qWarning() << "Invalid volume dimensions: counts must be positive"
                   << dims.counts(0) << dims.counts(1) << dims.counts(2);
        return;
    }

    // Check compatibility with existing scalar values
    size_t expectedSize = dims.counts(0) * dims.counts(1) * dims.counts(2);
    if (m_scalarValues.size() > 0 && m_scalarValues.size() != expectedSize) {
        qWarning() << "Volume dimension size mismatch: expected" << expectedSize
                   << "but have" << m_scalarValues.size() << "scalar values";
        qWarning() << "Clearing existing scalar values";
        m_scalarValues.resize(0);
    }

    m_dimensions = dims;
}

[[nodiscard]] const Volume::Dimensions &Volume::dimensions() const {
    return m_dimensions;
}

void Volume::setDescription(const QString &desc) {
    m_description = desc;
}

QString Volume::description() const {
    return m_description;
}

void Volume::setScalarValues(const Volume::ScalarValues &values) {
    m_scalarValues = values;
}

[[nodiscard]] const Volume::ScalarValues &Volume::scalarValues() const {
    return m_scalarValues;
}
