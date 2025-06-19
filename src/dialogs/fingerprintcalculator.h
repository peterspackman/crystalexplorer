#pragma once

#include <QVector>
#include <QString>
#include <QStringList>

class Mesh;

class FingerprintCalculator {
public:
    explicit FingerprintCalculator(Mesh *mesh = nullptr);
    
    void setMesh(Mesh *mesh);
    QVector<double> calculateElementBreakdown(const QString &insideElement, 
                                            const QStringList &elementSymbols);

private:
    Mesh *m_mesh{nullptr};
    int m_samplesPerEdge{3}; // Default sampling resolution
};