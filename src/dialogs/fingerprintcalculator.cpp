#include "fingerprintcalculator.h"
#include "mesh.h"
#include "chemicalstructure.h"
#include "elementdata.h"
#include "isosurface.h"
#include <QObject>

FingerprintCalculator::FingerprintCalculator(Mesh *mesh) : m_mesh(mesh) {}

void FingerprintCalculator::setMesh(Mesh *mesh) {
    m_mesh = mesh;
}

QVector<double> FingerprintCalculator::calculateElementBreakdown(const QString &insideElement, 
                                                               const QStringList &elementSymbols) {
    QVector<double> result;
    if (!m_mesh)
        return result;

    // Get element assignment data
    auto *structure = qobject_cast<ChemicalStructure *>(m_mesh->parent());
    if (!structure)
        return result;
        
    auto insideNums = structure->atomicNumbersForIndices(m_mesh->atomsInside());
    auto outsideNums = structure->atomicNumbersForIndices(m_mesh->atomsOutside());
    QString diIdxName = isosurface::getSurfacePropertyDisplayName("di_idx");
    QString deIdxName = isosurface::getSurfacePropertyDisplayName("de_idx");
    Eigen::VectorXi di_idx = m_mesh->vertexProperty(diIdxName).cast<int>();
    Eigen::VectorXi de_idx = m_mesh->vertexProperty(deIdxName).cast<int>();
    
    if (di_idx.rows() == 0 || de_idx.rows() == 0)
        return result;

    int insideAtomicNum = ElementData::atomicNumberFromElementSymbol(insideElement);
    
    // Get di and de properties for sampling
    QString diName = isosurface::getSurfacePropertyDisplayName("di");
    QString deName = isosurface::getSurfacePropertyDisplayName("de");
    Eigen::VectorXd di = m_mesh->vertexProperty(diName).cast<double>();
    Eigen::VectorXd de = m_mesh->vertexProperty(deName).cast<double>();
    
    if (di.rows() == 0 || de.rows() == 0)
        return result;
    
    QVector<double> totalFilteredArea(elementSymbols.size(), 0.0);
    
    // Sample each face using barycentric coordinates
    for (int faceIdx = 0; faceIdx < m_mesh->numberOfFaces(); ++faceIdx) {
        Eigen::Vector3i faceIndices = m_mesh->faces().col(faceIdx);
        
        double faceArea = m_mesh->faceAreas()(faceIdx);
        int expectedSamplesThisFace = (m_samplesPerEdge + 1) * (m_samplesPerEdge + 2) / 2;
        
        for (int i = 0; i <= m_samplesPerEdge; ++i) {
            for (int j = 0; j <= m_samplesPerEdge - i; ++j) {
                double a = static_cast<double>(i) / m_samplesPerEdge;
                double b = static_cast<double>(j) / m_samplesPerEdge;
                double c = 1.0 - a - b;
                
                // Determine element assignment for this sample
                int v0 = faceIndices[0], v1 = faceIndices[1], v2 = faceIndices[2];
                
                if (v0 < di_idx.rows() && v1 < di_idx.rows() && v2 < di_idx.rows() &&
                    di_idx(v0) >= 0 && de_idx(v0) >= 0 && di_idx(v1) >= 0 && de_idx(v1) >= 0 && 
                    di_idx(v2) >= 0 && de_idx(v2) >= 0) {
                    
                    // Use dominant vertex for element assignment
                    int dominantVertex;
                    if (a >= b && a >= c) dominantVertex = v0;
                    else if (b >= c) dominantVertex = v1;
                    else dominantVertex = v2;
                    
                    int sampleInsideAtom = insideNums(di_idx(dominantVertex));
                    int sampleOutsideAtom = outsideNums(de_idx(dominantVertex));
                    
                    // Check if this sample matches the requested inside element
                    if (sampleInsideAtom == insideAtomicNum) {
                        // Find which outside element this sample belongs to
                        for (int elemIdx = 0; elemIdx < elementSymbols.size(); ++elemIdx) {
                            int outsideAtomicNum = ElementData::atomicNumberFromElementSymbol(elementSymbols[elemIdx]);
                            if (sampleOutsideAtom == outsideAtomicNum) {
                                double sampleArea = faceArea / expectedSamplesThisFace;
                                totalFilteredArea[elemIdx] += sampleArea;
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
    
    // Convert to percentages
    for (int i = 0; i < totalFilteredArea.size(); ++i) {
        double percentage = (totalFilteredArea[i] / m_mesh->surfaceArea()) * 100.0;
        result.append(percentage);
    }
    
    return result;
}