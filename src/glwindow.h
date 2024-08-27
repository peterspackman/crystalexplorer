#pragma once
#include <QMatrix4x4>
#include <QOpenGLWidget>
#include <QVector3D>

#include "elementeditor.h"
#include "measurement.h"
#include "project.h"
#include "viewtoolbar.h" // for definition of AxisID

using cx::graphics::SelectionType;
// Uncomment the line below to enable (i) atom suppression
// (accessed via the context menu)
//#define ENABLE_ATOM_SUPPRESSION

const float ROT_MIN_VALUE = 0.0;
const float ROT_MAX_VALUE = 360.0;
const float ROT_RANGE = ROT_MAX_VALUE - ROT_MIN_VALUE;

const double VIEWING_VOLUME_FAR = 200.0;
const GLfloat PERSPECTIVE_NEAR_VALUE = 30.0;
const double CAMERA_DISTANCE = 60.0;

const float RADIUS_THRESHOLD = 0.000001f;
const float DEFAULT_SCALE = 0.25f;
const float SCALE_THRESHOLD = 0.01f;

enum MouseMode { translate, rotate, zoom };
static const Qt::CursorShape mouseModeCursor[] = {
    Qt::OpenHandCursor, Qt::ArrowCursor, Qt::SizeVerCursor};
static const Qt::CursorShape mouseModeCursorButtonHeld[] = {
    Qt::ClosedHandCursor, Qt::ArrowCursor, Qt::SizeVerCursor};
static const bool mouseModeAllowsSelection[] = {false, true, false};

enum class SelectionMode {
  Pick,
  Distance,
  Angle,
  Dihedral,
  OutOfPlaneBend,
  InPlaneBend
};


const int ANIMATION_REDRAW_WAIT_TIME = 16; // Aim for 60 fps

class GLWindow : public QOpenGLWidget, protected QOpenGLExtraFunctions {
  Q_OBJECT

public:
  GLWindow(QWidget *parent = 0);
  ~GLWindow() { delete animationTimer; }
  QColor backgroundColor() { return _backgroundColor; }
  Scene *currentScene() const { return scene; }

  QImage renderToImage(int scaleFactor, bool for_picking = false);
  QImage exportToImage(int scaleFactor = 1, const QColor & background = Qt::white);
  bool renderToPovRay(QTextStream &);

public slots:
  void showMessageOnGraphicsView(QString);
  void setCurrentCrystal(Project *);
  void redraw();
  void setMouseMode(MouseMode);
  void setSelectionMode(SelectionMode);
  void rotateAboutX(int);
  void rotateAboutY(int);
  void rotateAboutZ(int);
  void rescale(float);
  void viewMillerDirection(float, float, float);
  void updateBackgroundColor(QColor);
  void setPerspective(bool usePerspective, float perspectiveValue);
  void updateDepthFading();

  void updateDepthTest(bool);
  void updateFrontClippingPlane(float);

  void setAnimateScene(bool);
  void setAnimationSettings(double minorX, double minorY, double minorZ,
                            double minorSpeed, double majorX, double majorY,
                            double majorZ, double majorSpeed);

  void surfacePropertyChanged();
  void undoLastMeasurement();
  bool hasMeasurements();
  void getNewBackgroundColor();
  void saveOrientation();
  void switchToOrientation();
  void recenterScene();
  void resetViewAndRedraw();
  void updateSurfacesForFingerprintWindow();
  void screenGammaChanged();
  void materialChanged();
  void lightSettingsChanged();
  void textSettingsChanged();

signals:
  void scaleChanged(float);
  void transformationMatrixChanged();
  void backgroundColorChanged(QColor);
  void elementChanged(const Element *);
  void surfaceHideRequest(int);
  void surfaceDeleteRequest(int);
  void resetCurrentCrystal();
  void mouseDrag(QPointF);

protected:
  void initializeGL() override;
  void resizeGL(int, int) override;
  void paintGL() override;
  void mousePressEvent(QMouseEvent *) override;
  void mouseDoubleClickEvent(QMouseEvent *) override;
  void mouseReleaseEvent(QMouseEvent *) override;
  void mouseMoveEvent(QMouseEvent *) override;
  void wheelEvent(QWheelEvent *) override;
  void keyPressEvent(QKeyEvent *) override;
  void keyReleaseEvent(QKeyEvent *) override;
  bool event(QEvent *event) override;

private slots:
  void updateAtomColoring(ChemicalStructure::AtomColoring);
  void contextualSelectAtomsInsideSurface();
  void contextualSelectAtomsOutsideSurface();
  void contextualGenerateInternalFragment();
  void contextualGenerateExternalFragment();
  void contextualGenerateAllExternalFragments();
  void contextualHideSurface();
  void contextualDeleteSurface();
  void contextualShowSurfaceCaps();
  void contextualHideSurfaceCaps();
  void contextualResetCrystal();
  void contextualResetOrigin();
  void contextualSelectAll();
  void contextualDeselectAll();
  void contextualSelectSuppressedAtoms();
  void contextualEditElement();
  void contextualEditNonePropertyColor();
  void contextualResetNonePropertyColor();
  void contextualDeleteFragmentWithAtom();
  void contextualDeleteFragmentWithBond();
  void contextualHideUnitCellBox();
  void contextualShowUnitCellBox();
  void contextualCompleteAllFragments();
  void contextualRemoveIncompleteFragments();
  void contextualCompletePickedAtom();
  void contextualCompleteSelectedBond();
  void contextualToggleAtomicLabels();
  void contextualHideAllSurfaces();
  void contextualShowAllSurfaces();
  void contextualHideHydrogens();
  void contextualShowHydrogens();
  void contextualHideSuppressedAtoms();
  void contextualShowSuppressedAtoms();
  void contextualSuppressSelectedAtoms();
  void contextualUnsuppressSelectedAtoms();
  void contextualUnsuppressAllAtoms();
  void contextualBondSelectedAtoms();
  void contextualUnbondSelectedAtoms();
  void contextualColorSelection(bool fragments);
  void contextualResetCustomAtomColors();
  void contextualRemoveSelectedAtoms();
  void forcedRedraw();

protected slots:
  void messageLogged(const QOpenGLDebugMessage &msg);

private:
  void makeFrameBufferObject();
  void init();
  void initPointers();
  void setProjection(GLfloat, GLfloat);
  void setModelView();
  void drawScene(bool forPicking = false);
  void handleLeftMousePressForPicking(QMouseEvent *);
  void handleRightMousePress(QPoint);
  void handleObjectInformationDisplay(QPoint);
  void handleMousePressForMeasurement(MeasurementType, QMouseEvent *);
  QColor pickObjectAt(QPoint);
  void showSelectionSpecificContextMenu(const QPoint &, SelectionType);
  void showGeneralContextMenu(const QPoint &);
  void addGeneralActionsToContextMenu(QMenu *);
  void addColorBySubmenu(QMenu *);
  void updateScale(GLfloat, bool doEmit = true);
  float scaleEstimateFromCrystalRadius(float);
  void getViewAngleAndScaleFromScene();
  void applyRotationToTMatrix(GLfloat, GLfloat, GLfloat);
  void applyTranslationToTMatrix(GLfloat, GLfloat);
  void setRotationValues(GLfloat, GLfloat, GLfloat, bool doEmit = true);
  void viewDownVector(const occ::Vec3&);
  void applyAnimationRotation();
  void applyRotationAboutVectorToTMatrix(float theta, float n1, float n2,
                                         float n3);
  void setObjectInformationTextAndPosition(QString text, QPoint pos);
  void showMeasurementContextMenu(const QPoint &);
  void setBackgroundColor(QColor);
  void showSurfaceCaps(bool);
  void showHydrogens(bool);
  void showSuppressedAtoms(bool);
  void showMessage(const QString &);

  Scene *scene{nullptr};
  QMatrix4x4 m_projection, m_view, m_model;

  GLuint *selectionBuffer;

  GLfloat glCameraDistance;
  GLfloat frontClippingPlane;
  GLfloat perspectiveNearValue;
  bool usePerspectiveProjection;
  bool enableDepthTest{true};

  MouseMode mouseMode;
  MouseMode prevMouseMode;
  QPoint savedMousePosition;
  bool _leftMouseButtonHeld;
  bool _rightMouseButtonHeld;
  bool _hadHits;
  bool _mouseMoved;
  SelectionMode m_selectionMode;
  int numberOfSelections;
  Measurement m_currentMeasurement;
  bool m_I_keyHeld{false};
  bool m_shiftKeyHeld;
  bool m_singleMouseClick;
  bool m_doubleMouseClick;

  MeasurementObject m_firstSelectionForMeasurement;
  QMenu *contextMenu{nullptr};

  float m_depthFogEnabled;
  float m_fogDensity;
  float m_fogOffset;
  int m_width;
  int m_height;

  QColor _backgroundColor;

  bool animateScene;
  QTimer *animationTimer{nullptr};
  float _minorAxisX;
  float _minorAxisY;
  float _minorAxisZ;
  float _minorSpeed;
  float _majorAxisX;
  float _majorAxisY;
  float _majorAxisZ;
  float _majorSpeed;

  ElementEditor *_elementEditor{nullptr};
  QOpenGLDebugLogger *m_debugLogger{nullptr};
  bool m_picking{false};
  QImage m_pickingImage;
  QImage m_textLayer;

  QOpenGLFramebufferObject *m_framebuffer{nullptr};
  QOpenGLFramebufferObject *m_resolvedFramebuffer{nullptr};
  QOpenGLShaderProgram *m_postprocessShader{nullptr};
  QOpenGLVertexArrayObject m_quadVAO;
  QOpenGLBuffer m_quadVBO;
};
