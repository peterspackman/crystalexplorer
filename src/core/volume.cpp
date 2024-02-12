#include "volume.h"

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
    // TODO check compatibility
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
