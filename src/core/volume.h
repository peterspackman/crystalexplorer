#pragma once
#include <QObject>
#include <occ/core/linear_algebra.h>

class Volume: public QObject {
  Q_OBJECT

public:
    using ScalarValues = occ::Vec;
    using VectorValues = occ::Mat3N;

    struct Dimensions{
	occ::Mat3 axes{occ::Mat3::Identity()};
	occ::IVec3 counts;
    };

    Volume(const Dimensions &dims, QObject *parent = nullptr);

    [[nodiscard]] const Dimensions &dimensions() const;
    void setDimensions(const Dimensions &dims);

    [[nodiscard]] size_t numberOfPoints() const;
    [[nodiscard]] size_t count(Eigen::Index dim) const;

    void setScalarValues(const ScalarValues &);
    [[nodiscard]] const ScalarValues &scalarValues() const;


    [[nodiscard]] QString description() const;
    void setDescription(const QString &);

private:
    QString m_description;
    Dimensions m_dimensions;
    ScalarValues m_scalarValues;
};
