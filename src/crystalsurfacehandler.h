#pragma once
#include "surface.h"
#include <QObject>

class DeprecatedCrystal;
class JobParameters;
using CrystalSymops = QMap<SymopId, Vector3q>;

class CrystalSurfaceHandler : public QObject {
  friend QDataStream &operator<<(QDataStream &ds,
                                 const CrystalSurfaceHandler &);
  friend QDataStream &operator>>(QDataStream &, CrystalSurfaceHandler &);

  Q_OBJECT
public:
  CrystalSurfaceHandler();
  ~CrystalSurfaceHandler();
  inline int numberOfSurfaces() const { return m_surfaces.size(); }
  inline bool isCurrentSurfaceIndex(int index) const {
    return m_currentSurfaceIndex == index;
  }

  Surface *currentSurface() const;
  Surface *surfaceFromIndex(int index) const;
  inline int currentSurfaceIndex() const { return m_currentSurfaceIndex; }

  QVector<int> generateInternalFragment(DeprecatedCrystal *, int surfaceIndex,
                                        bool selectFragments = false) const;
  void generateExternalFragmentForSurface(DeprecatedCrystal *crystal,
                                          int surfaceIndex, int faceIndex,
                                          bool selectFragment = false) const;
  QVector<int>
  generateExternalFragmentsForSurface(DeprecatedCrystal *, int,
                                      bool selectFragments = false) const;

  bool loadSurfaceData(DeprecatedCrystal *, const JobParameters &);
  bool setCurrentSurfaceIndex(int surfaceIndex);
  void resetCurrentSurfaceIndex();
  void toggleSurfaceVisibility(int surfaceIndex);
  void setSurfaceVisibility(int surfaceIndex, bool visible);
  void deleteCurrentSurface();

  void setAllSurfaceVisibilities(bool visible);
  QStringList surfaceTitles() const;
  QVector<bool> surfaceVisibilities() const;
  QVector<QVector3D> surfaceCentroids() const;

  void updateAllSurfaceNoneProperties();

  int numberOfFacesToDrawForAllSurfaces() const;

  bool hasVisibleSurface() const;
  bool hasHiddenSurface() const;
  int numberOfVisibleSurfaces() const;

  int equivalentSurfaceIndex(const DeprecatedCrystal &,
                             const JobParameters &) const;
  int indexOfPropertyEquivalentToRequestedPropertyForSurface(
      int surfaceIndex, const JobParameters &) const;

  void setShowSurfaceInteriors(bool);
  void deleteSurface(Surface *);

  // cloning methods
  Surface *cloneSurface(const Surface &) const;
  Surface *cloneSurfaceWithCellShift(const DeprecatedCrystal &, const Surface &,
                                     const Vector3q &) const;
  Surface *cloneSurfaceForFragment(const DeprecatedCrystal &, const Surface &,
                                   int) const;

  bool cloneAlreadyExists(const DeprecatedCrystal &, const Surface &,
                          const CrystalSymops &) const;
  Surface *existingClone(const DeprecatedCrystal &, const Surface &,
                         const CrystalSymops &) const;

  void cloneCurrentSurfaceForFragmentList(DeprecatedCrystal &,
                                          const QVector<int> &);

  void cloneCurrentSurfaceForSelection(DeprecatedCrystal &);
  void cloneCurrentSurfaceForAllFragments(DeprecatedCrystal &);
  void cloneCurrentSurfaceWithCellShifts(DeprecatedCrystal &,
                                         const QVector3D &);

  // Methods to DELETE later
  inline QVector<Surface *> &surfaceList() { return m_surfaces; }
  inline const QVector<Surface *> &surfaceList() const { return m_surfaces; }

  void rebuildSurfaceParentCloneRelationship();

  QPair<QVector3D, QVector3D>
  positionsOfMinDistanceFragSurface(const DeprecatedCrystal &, int fragIndex,
                                    int surfaceIndex) const;
  QPair<QVector3D, QVector3D>
  positionsOfMinDistanceSurfaceSurface(int surfaceIndex1,
                                       int surfaceIndex2) const;
  QPair<QVector3D, QVector3D>
  positionsOfMinDistancePosSurface(const QVector3D &pos,
                                   int surfaceIndex) const;

signals:
  void newPropertyAddedToCurrentSurface();
  void surfaceVisibilitiesChanged();
  void surfacesChanged();

private:
  QVector<Surface *> m_surfaces;
  int m_currentSurfaceIndex{-1};
};
