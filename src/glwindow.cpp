#include <QColorDialog>
#include <QDebug>
#include <QInputDialog>
#include <QMenu>
#include <QMouseEvent>
#include <QToolTip>
#include <QVector2D>
#include <cmath>

#include "elementdata.h"
#include "glwindow.h"
#include "graphics.h"
#include "mathconstants.h"
#include "renderselection.h"
#include "settings.h"
#include <fmt/core.h>

GLWindow::GLWindow(QWidget *parent) : QOpenGLWidget(parent) { init(); }

void GLWindow::init() {

  initPointers();
  m_usePerspectiveProjection =
      settings::readSetting(settings::keys::USE_PERSPECTIVE_FLAG).toBool();
  _backgroundColor = QColor(
      settings::readSetting(settings::keys::BACKGROUND_COLOR).toString());
  enableDepthTest =
      settings::readSetting(settings::keys::ENABLE_DEPTH_TEST).toBool();
  m_depthFogEnabled =
      settings::readSetting(settings::keys::DEPTH_FOG_ENABLED).toBool();
  m_fogDensity =
      settings::readSetting(settings::keys::DEPTH_FOG_DENSITY).toFloat();
  m_fogOffset = 0.0;

  mouseMode = rotate;
  setMouseMode(mouseMode);

  _hadHits = false;
  _mouseMoved = false;

  setSelectionMode(SelectionMode::Pick);

  m_shiftKeyHeld = false;

  setFocusPolicy(Qt::StrongFocus);

  animateScene = false;
  animationTimer = new QTimer(this);
  connect(animationTimer, &QTimer::timeout, this, &GLWindow::redraw);
  _minorAxisX = 0;
  _minorAxisY = 0;
  _minorAxisZ = 0;
  _minorSpeed = 0;
  // The axis about which the minor axis rotates
  _majorAxisX = 0;
  _majorAxisY = 0;
  _majorAxisZ = 0;
  _majorSpeed = 0;

  m_singleMouseClick = true;
  _elementEditor = nullptr;
  m_textLayer = QImage(size(), QImage::Format_ARGB32);
}

void GLWindow::initPointers() {
  scene = nullptr;
  selectionBuffer = nullptr;
  contextMenu = nullptr;
}

void GLWindow::setMouseMode(MouseMode mode) {
  prevMouseMode = mouseMode;
  mouseMode = mode;
  setCursor(mouseModeCursor[mouseMode]);
}

void GLWindow::setSelectionMode(SelectionMode mode) {
  m_selectionMode = mode;

  switch (m_selectionMode) {
  case SelectionMode::Pick:
    if (scene) {
      scene->setSelectStatusForAllAtoms(false);
    }
    break;
  case SelectionMode::Distance:
  case SelectionMode::Angle:
  case SelectionMode::Dihedral:
  case SelectionMode::OutOfPlaneBend:
  case SelectionMode::InPlaneBend:
    numberOfSelections = 0;
    if (scene) {
      scene->setSelectStatusForAllAtoms(false);
    }
    // also want to get rid of any incomplete measurements here
    break;
  }
  redraw();
}

void GLWindow::undoLastMeasurement() {
  if (scene) {
    scene->removeLastMeasurement();
    redraw();
  }
}

bool GLWindow::hasMeasurements() {
  bool retval = false;
  if (scene) {
    retval = scene->hasMeasurements();
  }
  return retval;
}

void GLWindow::makeFrameBufferObject() {

  // Delete the old framebuffer
  if (m_framebuffer) {
    delete m_framebuffer;
    m_framebuffer = nullptr;
  }
  if (m_resolvedFramebuffer) {
    delete m_resolvedFramebuffer;
    m_resolvedFramebuffer = nullptr;
  }

  // Create the FBO
  QOpenGLFramebufferObjectFormat format;
  format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
  format.setTextureTarget(GL_TEXTURE_2D);
  format.setSamples(4);
  int w = std::max(1, static_cast<int>(width() * devicePixelRatio()));
  int h = std::max(1, static_cast<int>(height() * devicePixelRatio()));
  m_framebuffer = new QOpenGLFramebufferObject(w, h, format);
  m_resolvedFramebuffer = new QOpenGLFramebufferObject(w, h);
}

/*!
 Called once internally and automatically by Qt/OpenGL prior to first paintGL()
 */
void GLWindow::initializeGL() {
  initializeOpenGLFunctions();
  m_debugLogger = new QOpenGLDebugLogger(this);

  if (m_debugLogger->initialize()) {
    qDebug() << "GL_DEBUG Logger: " << m_debugLogger << "\n";
    connect(m_debugLogger, &QOpenGLDebugLogger::messageLogged, this,
            &GLWindow::messageLogged);
    m_debugLogger->startLogging();
  }

  makeFrameBufferObject();

  // Create the shader program
  m_postprocessShader = new QOpenGLShaderProgram();
  m_postprocessShader->addShaderFromSourceCode(QOpenGLShader::Vertex, R"(
      #version 330 core
      layout (location = 0) in vec2 aPos;
      layout (location = 1) in vec2 aTexCoords;

      out vec2 TexCoords;

      void main()
      {
          gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
          TexCoords = aTexCoords;
      }
  )");
  m_postprocessShader->addShaderFromSourceCode(QOpenGLShader::Fragment, R"(
      #version 330 core
      out vec4 FragColor;

      in vec2 TexCoords;

      uniform sampler2D screenTexture;

      void main()
      {
          FragColor = texture(screenTexture, TexCoords);
      }
  )");
  m_postprocessShader->bindAttributeLocation("aPos", 0);
  m_postprocessShader->bindAttributeLocation("aTexCoords", 1);
  m_postprocessShader->link();

  // Create the screen-filling quad
  m_quadVAO.create();
  m_quadVBO.create();
  m_quadVAO.bind();
  m_quadVBO.bind();

  GLfloat quadVertices[] = {// vertex attributes for a quad that fills the
                            // entire screen in Normalized Device Coordinates.
                            // positions   // texCoords
                            -1.0f, 1.0f, 0.0f, 1.0f,  -1.0f, -1.0f,
                            0.0f,  0.0f, 1.0f, -1.0f, 1.0f,  0.0f,

                            -1.0f, 1.0f, 0.0f, 1.0f,  1.0f,  -1.0f,
                            1.0f,  0.0f, 1.0f, 1.0f,  1.0f,  1.0f};
  m_quadVBO.allocate(quadVertices, sizeof(quadVertices));

  // Setup the vertex attributes pointers
  int positionLocation = m_postprocessShader->attributeLocation("aPos");
  int texCoordLocation = m_postprocessShader->attributeLocation("aTexCoords");

  m_postprocessShader->enableAttributeArray(positionLocation);
  m_postprocessShader->enableAttributeArray(texCoordLocation);

  m_postprocessShader->setAttributeBuffer(positionLocation, GL_FLOAT, 0, 2,
                                          4 * sizeof(GLfloat));
  m_postprocessShader->setAttributeBuffer(
      texCoordLocation, GL_FLOAT, 2 * sizeof(GLfloat), 2, 4 * sizeof(GLfloat));

  glClearColor(_backgroundColor.redF(), _backgroundColor.greenF(),
               _backgroundColor.blueF(), _backgroundColor.alphaF());
  glEnable(GL_BLEND);
  glEnable(GL_MULTISAMPLE);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glDepthFunc(GL_GREATER);

  // One of the following commands is necessary to stop the shading problems
  // that occur when rescaling.  Without one of the following, when the picture
  // is scaled, the normals are adjusted resulting in the image becoming darker
  // or lighter.  With one of the following, the normals are adjusted to make
  // sure they remain normalized.  "glEnable(GL_NORMALIZE)" is more robust but
  // more expensive as it calculates a square root.
  // "glEnable(GL_RESCALE_NORMAL)" works fine if the scaling is simple, as it is
  // in this program, and it is less expensive.  **However**, on Windows XP,
  // "GL_RESCALE_NORMAL" was not defined, whereas "GL_NORMALIZE" was.
  //
  /*
  #if defined(Q_OS_WIN)
    glEnable(GL_NORMALIZE);
  #else
    glEnable(GL_RESCALE_NORMAL);
  #endif
  */
}

void GLWindow::resizeGL(int width, int height) {
  setProjection(width, height);
  m_textLayer = QImage(size(), QImage::Format_ARGB32);
  makeFrameBufferObject();
}

void GLWindow::setAnimateScene(bool animate) {
  animateScene = animate;
  if (animateScene) {
    redraw();
  } else {
    animationTimer->stop();
  }
}

void GLWindow::setAnimationSettings(double minorX, double minorY, double minorZ,
                                    double minorSpeed, double majorX,
                                    double majorY, double majorZ,
                                    double majorSpeed) {
  // The minor axis about which the crystal rotates
  _minorAxisX = minorX;
  _minorAxisY = minorY;
  _minorAxisZ = minorZ;
  _minorSpeed = minorSpeed;
  // The major axis about which the minor axis rotates
  _majorAxisX = majorX;
  _majorAxisY = majorY;
  _majorAxisZ = majorZ;
  _majorSpeed = majorSpeed;
}

void GLWindow::setPerspective(bool usePerspective, float perspectiveValue) {
  m_usePerspectiveProjection = usePerspective;
  m_perspectiveNearValue = perspectiveValue;

  GLint viewport[4];
  makeCurrent();
  glGetIntegerv(GL_VIEWPORT, viewport);
  doneCurrent();
  GLint viewportWidth = viewport[2];
  GLint viewportHeight = viewport[3];

  setProjection(viewportWidth, viewportHeight);

  redraw();
}

void GLWindow::setProjection(GLfloat width, GLfloat height) {
  GLfloat left = -width / VIEWING_VOLUME_FAR;
  GLfloat right = width / VIEWING_VOLUME_FAR;
  GLfloat bottom = -height / VIEWING_VOLUME_FAR;
  GLfloat top = height / VIEWING_VOLUME_FAR;
  m_projection.setToIdentity();
  if (m_usePerspectiveProjection) {
    m_projection.frustum(left, right, bottom, top, VIEWING_VOLUME_FAR,
                         m_perspectiveNearValue);

  } else {
    m_projection.ortho(left, right, bottom, top, VIEWING_VOLUME_FAR,
                       m_frontClippingPlane);
  }
}

void GLWindow::updateFrontClippingPlane(float clippingPlane) {
  if (m_frontClippingPlane != clippingPlane) {
    m_frontClippingPlane = clippingPlane;
    setPerspective(
        false, 0.0); // Turn off perspective (currently no perspective version)
    redraw();
  }
}

void GLWindow::paintGL() {

  m_framebuffer->bind();
  if (enableDepthTest) {
    glEnable(GL_DEPTH_TEST);
  }

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glClearDepth(0);
  setModelView();
  drawScene(false);
  m_framebuffer->release();

  m_framebuffer->blitFramebuffer(
      m_resolvedFramebuffer, QRect(QPoint(), m_resolvedFramebuffer->size()),
      m_framebuffer, QRect(QPoint(), m_framebuffer->size()));
  glDisable(GL_DEPTH_TEST);

  QOpenGLFramebufferObject::bindDefault();
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, m_resolvedFramebuffer->texture());

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glClearDepth(0);
  // Draw the screen-filling quad
  m_postprocessShader->bind();
  m_postprocessShader->setUniformValue(
      "screenTexture", 0); // Assuming the texture is bound to texture unit 0
  m_postprocessShader->setUniformValue("resolution", width(), height());
  m_quadVAO.bind();
  glDrawArrays(GL_TRIANGLES, 0, 6);
  m_postprocessShader->release();
}

QImage GLWindow::exportToImage(int scaleFactor, const QColor &background) {
  makeCurrent();
  int w = width() * scaleFactor;
  int h = height() * scaleFactor;
  glViewport(0, 0, w, h);

  setModelView();
  QOpenGLFramebufferObject fbo(w, h,
                               QOpenGLFramebufferObject::CombinedDepthStencil);

  fbo.bind();
  if (enableDepthTest) {
    glEnable(GL_DEPTH_TEST);
  }
  glClearColor(background.redF(), background.greenF(), background.blueF(),
               background.alphaF());
  glClearDepth(0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glClearDepth(0);
  drawScene(false);
  fbo.release();

  QImage result(fbo.toImage());
  const QColor &color = scene->backgroundColor();
  glClearColor(color.redF(), color.greenF(), color.blueF(), color.alphaF());
  glClearDepth(0);

  doneCurrent();
  return result;
}

QImage GLWindow::renderToImage(int scaleFactor, bool for_picking) {
  makeCurrent();
  int w = width() * scaleFactor;
  int h = height() * scaleFactor;
  glViewport(0, 0, w, h);

  setModelView();
  QOpenGLFramebufferObject fbo(w, h,
                               QOpenGLFramebufferObject::CombinedDepthStencil);

  fbo.bind();
  if (enableDepthTest) {
    glEnable(GL_DEPTH_TEST);
  }
  if (for_picking) {
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClearDepth(0);
  }
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glClearDepth(0);
  drawScene(for_picking);
  fbo.release();

  QImage result(fbo.toImage());
  if (for_picking) {
    const QColor &color = scene->backgroundColor();
    glClearColor(color.redF(), color.greenF(), color.blueF(), color.alphaF());
    glClearDepth(0);
  }
  doneCurrent();
  return result;
}

bool GLWindow::renderToPovRay(QTextStream &ts) {
  qDebug() << "renderToPovRay";
  return false;
  /*
  if (!scene)
    return false;
  scene->exportToPovrayTextStream(ts);
  return true;
  */
}

void GLWindow::setModelView() {

  m_model.setToIdentity();
  m_view.setToIdentity();
  m_view.translate(0.0, 0.0, -m_cameraDistance);

  if (scene) {
    if (animateScene) {
      applyAnimationRotation();
    }

    GLfloat scale = scene->scale();
    m_view.scale(scale, scale, scale);
    m_view = m_view * scene->orientation().transformationMatrix();
    occ::Vec3 origin = scene->origin();

    m_view.translate(-origin.x(), -origin.y(), -origin.z());
    if (animateScene) {
      animationTimer->start(ANIMATION_REDRAW_WAIT_TIME);
    }
  }
}

void GLWindow::applyAnimationRotation() {
  /*
   Indices:

   T = [ 0   4   8   12 ]   R = [ 0  4  8  ]
   [ 1   5   9   13 ]       [ 1  5  9  ]
   [ 2   6   10  14 ]       [ 2  6  10 ]
   [ 3   7   11  15 ]
   */
  const float tiny = 0.01f;
  QMatrix4x4 R;
  R.setToIdentity();
  QVector3D majorAxis = QVector3D(_majorAxisX, _majorAxisY, _majorAxisZ);
  if (fabs(_majorSpeed) > tiny && fabs(majorAxis.length()) > tiny) {
    R.rotate(_majorSpeed, majorAxis);
  }
  QVector3D minorAxis = QVector3D(_minorAxisX, _minorAxisY, _minorAxisZ);
  if (fabs(_minorSpeed) > tiny && fabs(minorAxis.length()) > tiny) {
    // x = R * minorAxis
    minorAxis = minorAxis.normalized();
    minorAxis = R.mapVector(minorAxis);
    applyRotationAboutVectorToTMatrix(_minorSpeed * RAD_PER_DEG, minorAxis.x(),
                                      minorAxis.y(), minorAxis.z());
  }
}

void GLWindow::drawScene(bool forPicking) {
  // should only be called in paintGL
  //    QPainter painter(this);

  if (scene) {
    scene->setModelViewProjection(m_model, m_view, m_projection);
    if (forPicking) {
      scene->drawForPicking();
    } else {
      scene->draw();
    }
  }
}

bool GLWindow::event(QEvent *event) {
  //    if (event->type() == QEvent::ToolTip) {
  //        QHelpEvent *helpEvent = static_cast<QHelpEvent *>(event);
  //        QColor color = pickObjectAt(helpEvent->pos());
  //        if(true) {
  //            QToolTip::showText(helpEvent->globalPos(), color.name());
  //        } else {
  //            QToolTip::hideText();
  //            event->ignore();
  //        }
  //        return true;
  //    }
  return QWidget::event(event);
}

void GLWindow::keyPressEvent(QKeyEvent *event) {
  switch (event->key()) {
  case Qt::Key_Shift:
    m_shiftKeyHeld = true;
    event->accept();
    break;
  case Qt::Key_I:
    m_I_keyHeld = true;
    event->accept();
    if (!scene)
      return;
    handleObjectInformationDisplay(mapFromGlobal(QCursor::pos()));
    break;
  default:
    break;
  }
}

void GLWindow::keyReleaseEvent(QKeyEvent *event) {
  switch (event->key()) {
  case Qt::Key_Shift:
    m_shiftKeyHeld = false;
    event->accept();
    break;
  case Qt::Key_I:
    m_I_keyHeld = false;
    event->accept();
    break;
  default:
    break;
  }
}

void GLWindow::mousePressEvent(QMouseEvent *event) {
  m_singleMouseClick = true;

  if (event->button() == Qt::LeftButton) {
    _leftMouseButtonHeld = true;
    savedMousePosition = event->pos();

    switch (m_selectionMode) {
    case SelectionMode::Pick:
      handleLeftMousePressForPicking(event);
      break;
    case SelectionMode::Distance:
      handleMousePressForMeasurement(MeasurementType::Distance, event);
      break;
    case SelectionMode::Angle:
      handleMousePressForMeasurement(MeasurementType::Angle, event);
      break;
    case SelectionMode::Dihedral:
      handleMousePressForMeasurement(MeasurementType::Dihedral, event);
      break;
    case SelectionMode::OutOfPlaneBend:
      handleMousePressForMeasurement(MeasurementType::OutOfPlaneBend, event);
      break;
    case SelectionMode::InPlaneBend:
      handleMousePressForMeasurement(MeasurementType::InPlaneBend, event);
      break;
    }
  } else if (event->button() == Qt::RightButton) {
    _rightMouseButtonHeld = true;
    if (event->modifiers() == Qt::ControlModifier) {
      handleObjectInformationDisplay(event->pos());
    } else if (m_selectionMode == SelectionMode::Pick) {
      handleRightMousePress(event->pos());
    }
  }
  m_singleMouseClick = false;
}

void GLWindow::handleLeftMousePressForPicking(QMouseEvent *event) {
  if (scene == nullptr)
    return;
  setCursor(mouseModeCursorButtonHeld[mouseMode]);

  // ctrl-click or command-click on mac
  if (event->modifiers() == Qt::ControlModifier) {
    handleRightMousePress(event->pos());
    return;
  }
  if (mouseModeAllowsSelection[mouseMode]) {
    QColor color = pickObjectAt(event->pos());

    if (event->modifiers() == Qt::AltModifier) {
      _hadHits = scene->processHitsForSingleClickSelectionWithAltKey(color);
    } else {
      _hadHits = scene->processSelectionSingleClick(color);
      if (event->modifiers() == Qt::ShiftModifier) {
        _hadHits = scene->processSelectionDoubleClick(color);
      }
    }
    redraw();
  }
}

void GLWindow::handleRightMousePress(QPoint pos) {
  if (scene == nullptr)
    return;
  if (mouseModeAllowsSelection[mouseMode]) {
    QColor color = pickObjectAt(pos);
    SelectionType type = scene->decodeSelectionType(color);
    if (type != SelectionType::None) {
      showSelectionSpecificContextMenu(pos, type);
    } else {
      showGeneralContextMenu(pos);
    }
  }
}

void GLWindow::handleObjectInformationDisplay(QPoint pos) {
  if (scene == nullptr)
    return;
  if (mouseModeAllowsSelection[mouseMode]) {
    QColor color = pickObjectAt(pos);
    _hadHits = scene->processSelectionForInformation(color);
    if (_hadHits) {
      SelectionType type = scene->decodeSelectionType(color);
      switch (type) {
      case SelectionType::Atom: {
        const auto &atom = scene->selectedAtom();
        setObjectInformationTextAndPosition(
            getSelectionInformationLabelText(atom), pos);
        break;
      }
      case SelectionType::Bond: {
        const auto &bond = scene->selectedBond();
        setObjectInformationTextAndPosition(
            getSelectionInformationLabelText(bond), pos);
        break;
      }
      case SelectionType::Surface: {
        // TODO get surface info;
        auto selection = scene->selectedSurface();
        if (selection.surface) {
          setObjectInformationTextAndPosition(
              getSelectionInformationLabelText(selection), pos);
        }
        break;
      }
      default:
        break;
      }
    }
  }
  redraw();
}

void GLWindow::showMessage(const QString &message) {
  QToolTip::showText(mapToGlobal(QPoint(50, height() - 100)), message);
}

void GLWindow::showMessageOnGraphicsView(QString message) {
  showMessage(message);
}

void GLWindow::setObjectInformationTextAndPosition(QString text, QPoint pos) {
  if (!m_infoLabel) {
    m_infoLabel = new QLabel(this);
    m_infoLabel->setFrameStyle(QFrame::Panel);
    m_infoLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_infoLabel->setWindowFlags(Qt::ToolTip);
    m_infoLabel->setTextInteractionFlags(Qt::TextSelectableByMouse |
                                         Qt::TextSelectableByKeyboard);
  }

  m_infoLabel->setText(text);
  m_infoLabel->adjustSize();
  m_infoLabel->setFixedSize(m_infoLabel->size());

  QPoint globalPos = mapToGlobal(pos);
  m_infoLabel->move(globalPos + QPoint(10, 10));
  m_infoLabel->show();
}

void GLWindow::hideObjectInformation() {
  if (m_infoLabel) {
    m_infoLabel->hide();
  }
}

void GLWindow::handleMousePressForMeasurement(MeasurementType type,
                                              QMouseEvent *event) {
  if (scene == nullptr)
    return;

  QColor color = pickObjectAt(event->pos());

  auto selection = scene->processMeasurementSingleClick(
      color, event->modifiers().testFlag(Qt::ShiftModifier));

  // is valid position?
  if (selection.index == -1) {
    redraw();
    return;
  }
  if (type == MeasurementType::Distance) {
    if (numberOfSelections == 0) {

      m_currentMeasurement = Measurement(type);
      m_firstSelectionForMeasurement = selection;
      numberOfSelections++;

    } else if (numberOfSelections == Measurement::totalPositions(type) - 1) {

      if (type == MeasurementType::Distance) {

        // For single-click we assume a single atom or single surface
        // triangle has been selected.
        // For shift-click we assume a whole fragment or whole surface has
        // been selected.
        // In the latter case, we find the minimum distances.

        // Pair of minimum positions for calculating distance and plotting
        // distance line.
        auto d = scene->positionsForDistanceMeasurement(
            m_firstSelectionForMeasurement, selection);
        qDebug() << "Valid measurement: " << d.valid;

        if (d.valid) {
          m_currentMeasurement.addPosition(d.a);
          m_currentMeasurement.addPosition(d.b);
          scene->addMeasurement(m_currentMeasurement);
          scene->setSelectStatusForAllAtoms(false);
        }
      }
      numberOfSelections = 0;
    }

  } else {

    if (numberOfSelections == 0) {
      m_currentMeasurement = Measurement(type);
      m_currentMeasurement.addPosition(selection.position);
      numberOfSelections++;
    } else if (numberOfSelections == Measurement::totalPositions(type) - 1) {
      m_currentMeasurement.addPosition(selection.position);
      scene->addMeasurement(m_currentMeasurement);
      scene->setSelectStatusForAllAtoms(false);
      numberOfSelections = 0;
    } else {
      m_currentMeasurement.addPosition(selection.position);
      numberOfSelections++;
    }
  }

  redraw();
}

void GLWindow::mouseDoubleClickEvent(QMouseEvent *event) {
  if (scene == nullptr)
    return;
  if (event->button() == Qt::LeftButton) {
    savedMousePosition = event->pos();

    if (mouseModeAllowsSelection[mouseMode]) {
      QColor color = pickObjectAt(event->pos());
      _hadHits = scene->processSelectionDoubleClick(color);
      if (_hadHits) {
        redraw();
      }
    }
  }
}

/*
 \brief This context menu is shown when we right click on an atom or bond or
 surface
 */
void GLWindow::showSelectionSpecificContextMenu(const QPoint &pos,
                                                SelectionType selectionType) {
  if (contextMenu != nullptr) {
    delete contextMenu;
  }
  QMenu *contextMenu = new QMenu;

  switch (selectionType) {
  case SelectionType::Atom:
    contextMenu->addAction(tr("Complete Fragment"), this,
                           &GLWindow::contextualCompletePickedAtom);
    contextMenu->addAction(tr("Remove Fragment"), this,
                           &GLWindow::contextualDeleteFragmentWithAtom);
    contextMenu->addAction(tr("Edit Element"), this,
                           &GLWindow::contextualEditElement);
    break;
  case SelectionType::Bond:
    contextMenu->addAction(tr("Complete Fragment"), this,
                           &GLWindow::contextualCompleteSelectedBond);
    contextMenu->addAction(tr("Remove Fragment"), this,
                           &GLWindow::contextualDeleteFragmentWithBond);
    break;
  case SelectionType::Surface: {
    const auto &selection = scene->selectedSurface();
    if (selection.surface) {
      auto *mesh = selection.surface->mesh();
      if (!mesh)
        break;

      contextMenu->addAction(tr("Hide Surface"), this,
                             &GLWindow::contextualHideSurface);
      contextMenu->addAction(tr("Delete Surface"), this,
                             &GLWindow::contextualDeleteSurface);

      if (mesh->kind() == isosurface::Kind::Hirshfeld) {
        contextMenu->addAction(tr("Generate Internal Fragment"), this,
                               &GLWindow::contextualGenerateInternalFragment);
        contextMenu->addAction(tr("Generate External Fragment"), this,
                               &GLWindow::contextualGenerateExternalFragment);
        contextMenu->addAction(
            tr("Generate All External Fragments"), this,
            &GLWindow::contextualGenerateAllExternalFragments);
      }

      contextMenu->addAction(tr("Select Atoms Inside Surface"), this,
                             &GLWindow::contextualSelectAtomsInsideSurface);
      contextMenu->addAction(tr("Select Atoms Outside Surface"), this,
                             &GLWindow::contextualSelectAtomsOutsideSurface);
    }
    break;
  }
  default:
    break;
  }

  contextMenu->addSeparator();
  addGeneralActionsToContextMenu(contextMenu);

  if (!contextMenu->isEmpty()) {
    contextMenu->exec(this->mapToGlobal(pos));
  }
}

void GLWindow::contextualSelectAtomsInsideSurface() {
  Q_ASSERT(scene);
  scene->selectAtomsSeparatedBySurface(true);
  redraw();
}

void GLWindow::contextualSelectAtomsOutsideSurface() {
  Q_ASSERT(scene);
  scene->selectAtomsSeparatedBySurface(false);
  redraw();
}

void GLWindow::contextualGenerateAllExternalFragments() {
  Q_ASSERT(scene);
  scene->generateAllExternalFragments();
}

void GLWindow::contextualGenerateInternalFragment() {
  Q_ASSERT(scene);
  scene->generateInternalFragment();
}

void GLWindow::contextualGenerateExternalFragment() {
  Q_ASSERT(scene);
  scene->generateExternalFragment();
}

void GLWindow::contextualHideSurface() {
  Q_ASSERT(scene);

  // TODO
  // emit surfaceHideRequest(scene->selectedSurfaceIndex());
}

void GLWindow::contextualDeleteSurface() {
  Q_ASSERT(scene);

  // TODO
  // emit surfaceDeleteRequest(scene->selectedSurfaceIndex());
}

void GLWindow::contextualShowSurfaceCaps() { showSurfaceCaps(true); }

void GLWindow::contextualHideSurfaceCaps() { showSurfaceCaps(false); }

void GLWindow::showSurfaceCaps(bool show) {
  Q_ASSERT(scene);

  // TODO
  /*
  Surface *surface = scene->selectedSurface();
  surface->setCapsVisible(show);
  redraw();
  */
}

void GLWindow::contextualCompletePickedAtom() {
  Q_ASSERT(scene);

  int atomIndex = scene->selectedAtom().index;
  scene->completeFragmentContainingAtom(atomIndex);
  redraw();
}

void GLWindow::contextualCompleteSelectedBond() {
  Q_ASSERT(scene);

  int firstBondAtomIndex = scene->selectedBond().a.index;
  scene->completeFragmentContainingAtom(firstBondAtomIndex);
  redraw();
}

void GLWindow::contextualEditNonePropertyColor() {
  // TODO fetch none color
  QColor noneColor = Qt::white;
  QColor color = QColorDialog::getColor(noneColor);
  if (color.isValid()) {
    // TODO set none color
    // scene->currentSurface()->setNonePropertyColor(color);
    redraw();
  }
}

void GLWindow::contextualResetNonePropertyColor() {
  // TODO update none color
  // scene->currentSurface()->updateNoneProperty();
  redraw();
}

void GLWindow::contextualEditElement() {
  Q_ASSERT(scene);

  if (_elementEditor == 0) {
    _elementEditor = new ElementEditor();
    connect(_elementEditor, &ElementEditor::elementChanged, this,
            &GLWindow::forcedRedraw);
  }
  QString pickedElementSymbol =
      ElementData::elementFromAtomicNumber(scene->selectedAtom().atomicNumber)
          ->symbol();
  _elementEditor->updateElementComboBox(scene->uniqueElementSymbols(),
                                        pickedElementSymbol);
  _elementEditor->show();
}

void GLWindow::forcedRedraw() {
  Q_ASSERT(scene);
  scene->setNeedsUpdate();
  redraw();
}

void GLWindow::contextualDeleteFragmentWithAtom() {
  Q_ASSERT(scene);

  scene->deleteFragmentContainingAtomIndex(scene->selectedAtom().index);
  redraw();
}

void GLWindow::contextualDeleteFragmentWithBond() {
  Q_ASSERT(scene);

  qDebug() << "Delete fragment containing" << scene->selectedBond().a.index;
  scene->deleteFragmentContainingAtomIndex(scene->selectedBond().a.index);
  redraw();
}

void GLWindow::showMeasurementContextMenu(const QPoint &pos) {
  if (contextMenu != nullptr) {
    delete contextMenu;
  }
  QMenu *contextMenu = new QMenu;
  // Should this allow the user to select "Selection Mode"??
  contextMenu->addAction(tr("Measurement Mode"));
  contextMenu->exec(this->mapToGlobal(pos));
}

void GLWindow::showGeneralContextMenu(const QPoint &pos) {

  if (contextMenu != nullptr) {
    delete contextMenu;
  }
  QMenu *contextMenu = new QMenu;

  addGeneralActionsToContextMenu(contextMenu);

  if (!contextMenu->isEmpty()) {
    contextMenu->exec(this->mapToGlobal(pos));
  }
}

void GLWindow::handleAtomLabelOptionsChanged(AtomLabelOptions options) {
  if(!scene) return;
  auto currentOptions = scene->atomLabelOptions();
  if(options != currentOptions) {
    emit atomLabelOptionsChanged(options);
  }
}

void GLWindow::updateAtomLabelContextMenu(QMenu *contextMenu) {
  if(!scene) return;
  auto current = scene->atomLabelOptions();

  if (current.showAtoms) {
    contextMenu->addAction("Hide Atom Labels", this, [current, this]() {
      auto opts = current;
      opts.showAtoms = false;
      handleAtomLabelOptionsChanged(opts);
    });
  }
  else {
    contextMenu->addAction("Show Atom Labels", this, [current, this]() {
      auto opts = current;
      opts.showAtoms = true;
      handleAtomLabelOptionsChanged(opts);
    });
  }

  if (current.showFragment) {
    contextMenu->addAction("Hide Fragment Labels", this, [current, this]() {
      auto opts = current;
      opts.showFragment = false;
      handleAtomLabelOptionsChanged(opts);
    });
  }
  else {
    contextMenu->addAction("Show Fragment Labels", this, [current, this]() {
      auto opts = current;
      opts.showFragment = true;
      handleAtomLabelOptionsChanged(opts);
    });
  }
}

void GLWindow::addGeneralActionsToContextMenu(QMenu *contextMenu) {
  if (scene) { // create crystal dependent context menu options

    contextMenu->addAction(tr("Reset Origin"), this,
                           &GLWindow::contextualResetOrigin);
    contextMenu->addAction(tr("Reset Structure"), this,
                           &GLWindow::contextualResetCrystal);

    contextMenu->addSeparator();

    if (scene->hasAllAtomsSelected()) {
      contextMenu->addAction(tr("Deselect All Atoms"), this,
                             &GLWindow::contextualDeselectAll);
    } else {
      contextMenu->addAction(tr("Select All Atoms"), this,
                             &GLWindow::contextualSelectAll);
    }

#ifdef ENABLE_ATOM_SUPPRESSION
    if (crystal->hasSuppressedAtoms()) {
      contextMenu->addAction(tr("Select Suppressed Atoms"), this,
                             &GLWindow::contextualSelectSuppressedAtoms);
    }
#endif

    contextMenu->addSeparator();

    if (scene->showCells()) {
      contextMenu->addAction(tr("Hide Unit Cell Axes"), this,
                             &GLWindow::contextualHideUnitCellBox);
    } else {
      contextMenu->addAction(tr("Show Unit Cell Axes"), this,
                             &GLWindow::contextualShowUnitCellBox);
    }

    updateAtomLabelContextMenu(contextMenu);

    if (scene->hasHydrogens()) {
      if (scene->showHydrogenAtoms()) {
        contextMenu->addAction(tr("Hide Hydrogen Atoms"), this,
                               &GLWindow::contextualHideHydrogens);
      } else {
        contextMenu->addAction(tr("Show Hydrogen Atoms"), this,
                               &GLWindow::contextualShowHydrogens);
      }
    }

#ifdef ENABLE_ATOM_SUPPRESSION
    if (crystal->hasSuppressedAtoms()) {
      if (crystal->suppressedAtomsAreVisible()) {
        contextMenu->addAction(tr("Hide Suppressed Atoms"), this,
                               &GLWindow::contextualHideSuppressedAtoms);
      } else {
        contextMenu->addAction(tr("Show Suppressed Atoms"), this,
                               &GLWindow::contextualShowSuppressedAtoms);
      }
    }
#endif

    if (scene->hasIncompleteFragments()) {
      contextMenu->addSeparator();
      contextMenu->addAction(tr("Complete All Fragments"), this,
                             &GLWindow::contextualCompleteAllFragments);
      contextMenu->addAction(tr("Remove Incomplete Fragments"), this,
                             &GLWindow::contextualRemoveIncompleteFragments);
    }

    if (scene->hasSelectedAtoms() || scene->hasSuppressedAtoms()) {
      contextMenu->addSeparator();
    }

#ifdef ENABLE_ATOM_SUPPRESSION
    if (crystal->hasSelectedAtoms()) {
      contextMenu->addAction(tr("Suppress Selected Atoms"), this,
                             &GLWindow::contextualSuppressSelectedAtoms);
      if (crystal->hasSuppressedAtoms()) {
        contextMenu->addAction(tr("Unsuppress Selected Atoms"), this,
                               &GLWindow::contextualUnsuppressSelectedAtoms);
      }
    }
#endif
    if (scene->numberOfSelectedAtoms() > 1) {
      contextMenu->addAction(tr("Bond Selected Atoms"), this,
                             &GLWindow::contextualBondSelectedAtoms);
      contextMenu->addAction(tr("Unbond Selected Atoms"), this,
                             &GLWindow::contextualUnbondSelectedAtoms);
    }

#ifdef ENABLE_ATOM_SUPPRESSION
    if (crystal->hasSuppressedAtoms()) {
      contextMenu->addAction(tr("Unsuppress All Atoms"), this,
                             &GLWindow::contextualUnsuppressAllAtoms);
    }
#endif

    if (scene->hasSelectedAtoms() || scene->hasAtomsWithCustomColor()) {
      contextMenu->addSeparator();
    }
    if (scene->hasSelectedAtoms()) {
      contextMenu->addAction(tr("Remove Selected Atoms"), [this]() {
        emitContextualAtomFilter(AtomFlag::Selected, true);
      });
      contextMenu->addAction(tr("Show only selected Atoms"), [this]() {
        emitContextualAtomFilter(AtomFlag::Selected, false);
      });
      contextMenu->addAction(tr("Set Color of Selected Atoms"),
                             [this]() { contextualColorSelection(false); });
      contextMenu->addAction(tr("Set Color of Selected Fragments"),
                             [this]() { contextualColorSelection(true); });
    }

    if (scene->hasAtomsWithCustomColor()) {
      contextMenu->addAction(tr("Reset All Atom Colors"), this,
                             &GLWindow::contextualResetCustomAtomColors);
    }
    addColorBySubmenu(contextMenu);

    // TODO handle surface case
    /*
    if (scene->hasSurface()) {
      contextMenu->addSeparator();

      if (scene->hasVisibleSurfaces()) {
        contextMenu->addAction(tr("Hide All Surfaces"), this,
                               &GLWindow::contextualHideAllSurfaces);
      }

      if (scene->hasHiddenSurfaces()) {
        contextMenu->addAction(tr("Show All Surfaces"), this,
                               &GLWindow::contextualShowAllSurfaces);
      }
    }
    */
  }

  // Add general actions that don't depend on having a crystal here
}

void GLWindow::addColorBySubmenu(QMenu *menu) {
  QMenu *colorByMenu = menu->addMenu(tr("Color Atoms By..."));
  colorByMenu->addAction(tr("Element"), [this]() {
    updateAtomColoring(ChemicalStructure::AtomColoring::Element);
  });
  colorByMenu->addAction(tr("Fragment"), [this]() {
    updateAtomColoring(ChemicalStructure::AtomColoring::Fragment);
  });
}

void GLWindow::updateAtomColoring(ChemicalStructure::AtomColoring coloring) {
  if (!scene)
    return;
  auto *structure = scene->chemicalStructure();
  if (!structure)
    return;
  structure->setAtomColoring(coloring);
  redraw();
}

void GLWindow::getNewBackgroundColor() {
  QColor color = QColorDialog::getColor(_backgroundColor);
  if (color.isValid()) {
    updateBackgroundColor(color);
  }
}

void GLWindow::updateBackgroundColor(QColor color) {
  if (scene) {
    scene->setBackgroundColor(color);
  }
  setBackgroundColor(color);
  redraw();
}

void GLWindow::updateSurfacesForFingerprintWindow() {
  if (scene) {
    scene->handleSurfacesNeedUpdate();
    redraw();
  }
}

void GLWindow::screenGammaChanged() {
  if (scene) {
    scene->screenGammaChanged();
  }
  redraw();
}

void GLWindow::materialChanged() {
  if (scene) {
    scene->materialChanged();
  }
  redraw();
}

void GLWindow::lightSettingsChanged() {
  if (scene) {
    scene->lightSettingsChanged();
  }
  redraw();
}

void GLWindow::textSettingsChanged() {
  if (scene) {
    scene->textSettingsChanged();
  }
  redraw();
}

void GLWindow::setBackgroundColor(QColor color) {
  _backgroundColor = color;
  if (scene)
    scene->setBackgroundColor(color);
  updateDepthFading();
  makeCurrent();
  glClearColor(color.redF(), color.greenF(), color.blueF(), color.alphaF());
  glClearDepth(0);
  doneCurrent();
}

void GLWindow::updateDepthTest(bool enabled) {
  makeCurrent();
  if (enabled) {
    glEnable(GL_DEPTH_TEST);
  } else {
    glDisable(GL_DEPTH_TEST);
  }
  doneCurrent();
  redraw();
}

void GLWindow::contextualHideHydrogens() { showHydrogens(false); }

void GLWindow::contextualShowHydrogens() { showHydrogens(true); }

void GLWindow::contextualHideSuppressedAtoms() { showSuppressedAtoms(false); }

void GLWindow::contextualShowSuppressedAtoms() { showSuppressedAtoms(true); }

void GLWindow::showHydrogens(bool show) {
  scene->setShowHydrogenAtoms(show);
  redraw();
}

void GLWindow::showSuppressedAtoms(bool show) {
  scene->setShowSuppressedAtoms(show);
  redraw();
}

void GLWindow::contextualResetCrystal() {
  Q_ASSERT(scene);
  emit resetCurrentCrystal();
}

void GLWindow::contextualResetOrigin() { recenterScene(); }

void GLWindow::contextualSelectAll() {
  Q_ASSERT(scene);
  scene->setSelectStatusForAllAtoms(true);
  redraw();
}

void GLWindow::contextualDeselectAll() {
  Q_ASSERT(scene);
  scene->setSelectStatusForAllAtoms(false);
  redraw();
}

void GLWindow::contextualSelectSuppressedAtoms() {
  Q_ASSERT(scene);
  scene->setSelectStatusForSuppressedAtoms(true);
  redraw();
}

void GLWindow::contextualHideUnitCellBox() {
  Q_ASSERT(scene);
  scene->setShowCells(false);
  redraw();
}

void GLWindow::contextualShowUnitCellBox() {
  Q_ASSERT(scene);
  scene->setShowCells(true);
  redraw();
}

void GLWindow::contextualCompleteAllFragments() {
  Q_ASSERT(scene);
  scene->completeAllFragments();
  redraw();
}

void GLWindow::contextualRemoveIncompleteFragments() {
  Q_ASSERT(scene);
  scene->deleteIncompleteFragments();
  redraw();
}

void GLWindow::contextualToggleAtomicLabels() {
  Q_ASSERT(scene);
  scene->toggleShowAtomLabels();
  redraw();
}

void GLWindow::contextualHideAllSurfaces() {
  qDebug() << "contextualHideAllSurfaces";
  /*
  Q_ASSERT(scene);
  scene->setSurfaceVisibilities(false);
  redraw();
  */
}

void GLWindow::contextualShowAllSurfaces() {
  qDebug() << "contextualShowAllSurfaces";
  /*
  Q_ASSERT(scene);
  scene->setSurfaceVisibilities(true);
  redraw();
  */
}

void GLWindow::contextualSuppressSelectedAtoms() {
  Q_ASSERT(scene);
  scene->suppressSelectedAtoms();
  redraw();
}

void GLWindow::contextualUnsuppressSelectedAtoms() {
  Q_ASSERT(scene);
  scene->unsuppressSelectedAtoms();
  redraw();
}

void GLWindow::contextualUnsuppressAllAtoms() {
  Q_ASSERT(scene);
  scene->unsuppressAllAtoms();
  redraw();
}

void GLWindow::contextualBondSelectedAtoms() {
  Q_ASSERT(scene);
  scene->bondSelectedAtoms();
  redraw();
}

void GLWindow::contextualUnbondSelectedAtoms() {
  Q_ASSERT(scene);
  scene->unbondSelectedAtoms();
  redraw();
}

void GLWindow::contextualColorSelection(bool fragments) {
  Q_ASSERT(scene);
  QColor color = QColorDialog::getColor(Qt::red, 0);
  if (color.isValid()) {
    scene->colorSelectedAtoms(color, fragments);
    redraw();
  }
}

void GLWindow::contextualResetCustomAtomColors() {
  Q_ASSERT(scene);
  scene->resetAllAtomColors();
  redraw();
}

QColor GLWindow::pickObjectAt(QPoint pos) {
  if (!scene) {
    return QColor(1.0f, 1.0f, 1.0f,
                  1.0f); // Nothing to select if we haven't got a crystal
  }
  m_pickingImage = renderToImage(1, true);

  const bool needDevicePixelRatio{false};
  int factor = 1;
  if (needDevicePixelRatio) {
    qDebug() << "Device pixel ratio: " << devicePixelRatio();
    factor = devicePixelRatio();
  }
  auto color = QColor(m_pickingImage.pixel(pos.x() * factor, pos.y() * factor));
  return color;
}

void GLWindow::mouseReleaseEvent(QMouseEvent *event) {
  if (!scene) {
    return;
  }

  if (event->button() == Qt::LeftButton) {
    _leftMouseButtonHeld = false;
    setCursor(mouseModeCursor[mouseMode]);

    // Clear selection when clicking on background
    if (mouseModeAllowsSelection[mouseMode] && !_hadHits && !_mouseMoved &&
        m_selectionMode == SelectionMode::Pick) {
      scene->setSelectStatusForAllAtoms(false);
    }

    _hadHits = false;
    _mouseMoved = false;
    redraw();
  }
  if (event->button() == Qt::RightButton) {
    _rightMouseButtonHeld = false;
  }
}

void GLWindow::mouseMoveEvent(QMouseEvent *event) {
  if (!scene) {
    return;
  }

  if (_leftMouseButtonHeld && _hadHits) {
    return;
  }

  hideObjectInformation();

  QPoint mousePosition = event->pos();
  float winWidth = width();
  float winHeight = height();
  QPoint delta = mousePosition - savedMousePosition;
  emit mouseDrag(delta);
  switch (mouseMode) {
  case translate: {
    // TODO: Come up with a better way to convert from screen coords to model
    // coords for the translation.
    float dx = 15 * delta.x() / winHeight;
    float dy = 15 * delta.y() / winWidth;
    auto T = scene->orientation().transformationMatrix();
    occ::Vec3 upVector(T(0, 0), T(0, 1), T(0, 2));
    occ::Vec3 rightVector(T(1, 0), T(1, 1), T(1, 2));
    scene->translateOrigin(-dx * upVector + dy * rightVector);
    savedMousePosition = mousePosition;
    break;
  }
  case rotate: {

    float xRot = 0.0;
    float yRot = 0.0;
    float zRot = 0.0;

    if (_leftMouseButtonHeld &&
        event->modifiers().testFlag(Qt::ShiftModifier)) { // rotate about z-axis
      zRot = fmod(360 * delta.y() / winHeight, 360.0);
    } else { // rotate about x and y axes
      xRot = fmod(360 * delta.y() / winHeight, 360.0);
      yRot = fmod(360 * delta.x() / winWidth, 360.0);
    }
    auto T = scene->orientation().transformationMatrix();
    QVector3D upVector(T(0, 0), T(0, 1), T(0, 2));
    QVector3D rightVector(T(1, 0), T(1, 1), T(1, 2));
    QVector3D lookVector(T(2, 0), T(2, 1), T(2, 2));
    T.rotate(xRot, upVector);
    T.rotate(yRot, rightVector);
    T.rotate(zRot, lookVector);
    scene->setTransformationMatrix(T);
    emit transformationMatrixChanged();
    savedMousePosition = mousePosition;
  } break;
  case zoom:
    GLfloat scale = scene->scale() * (1.0 - 0.1 * delta.y() / winHeight);
    updateScale(scale);
    break;
  }

  _mouseMoved = true;
  redraw();
}

void GLWindow::wheelEvent(QWheelEvent *event) {
  if (!scene) {
    return;
  }

  GLfloat scale =
      qMax(0.0, scene->scale() * (1.0 + event->angleDelta().y() / 1200.0));
  updateScale(scale);
  redraw();
}

void GLWindow::setRotationValues(GLfloat xRot, GLfloat yRot, GLfloat zRot,
                                 bool doEmit) {
  if (!scene) {
    return;
  }

  // adjust rotations to lie between 0 and 360.
  while (xRot >= ROT_MAX_VALUE) {
    xRot -= ROT_RANGE;
  }
  while (yRot >= ROT_MAX_VALUE) {
    yRot -= ROT_RANGE;
  }
  while (zRot >= ROT_MAX_VALUE) {
    zRot -= ROT_RANGE;
  }
  while (xRot < ROT_MIN_VALUE) {
    xRot += ROT_RANGE;
  }
  while (yRot < ROT_MIN_VALUE) {
    yRot += ROT_RANGE;
  }
  while (zRot < ROT_MIN_VALUE) {
    zRot += ROT_RANGE;
  }

  scene->orientation().setXRotation(xRot);
  scene->orientation().setYRotation(yRot);
  scene->orientation().setZRotation(zRot);
  if (doEmit) {
    emit transformationMatrixChanged();
  }
}

void GLWindow::updateScale(GLfloat scale, bool doEmit) {
  if (!scene) {
    return;
  }

  if (scale < SCALE_THRESHOLD) {
    scale = SCALE_THRESHOLD; // Prevent scaling to zero
  }
  scene->orientation().setScale(scale);
  if (doEmit) {
    emit scaleChanged(scale);
  }
}

void GLWindow::setCurrentCrystal(Project *project) {
  scene = project->currentScene();

  if (scene) {
    scene->screenGammaChanged();
    scene->materialChanged();
    getViewAngleAndScaleFromScene();
    QColor color = scene->backgroundColor();
    setBackgroundColor(color);
    // emit backgroundColorChanged(color);
  }
  redraw();
}

void GLWindow::redraw() { update(); }

void GLWindow::getViewAngleAndScaleFromScene() {
  Q_ASSERT(scene);

  auto euler = scene->orientation().eulerAngles();
  setRotationValues(euler.x, euler.y, euler.z);
  if (scene->scale() >= SCALE_THRESHOLD) { // is the saved scale valid?
    updateScale(scene->scale());
  } else {
    GLfloat scale = scaleEstimateFromCrystalRadius(scene->radius());
    updateScale(scale);
  }
}

void GLWindow::resetViewAndRedraw() {
  showMessage("<b>Resetting view<b>");
  getViewAngleAndScaleFromScene();
  redraw();
}

void GLWindow::updateDepthFading() {
  if (scene) {
    scene->depthFogSettingsChanged();
    redraw();
  }
}

float GLWindow::scaleEstimateFromCrystalRadius(float radius) {
  float s;
  if (radius > RADIUS_THRESHOLD) {
    s = 1.0 / sqrt(radius);
  } else {
    s = DEFAULT_SCALE;
  }
  return s;
}

void GLWindow::rotateAboutX(int xRot) {
  if (!scene) {
    return;
  }
  applyRotationToTMatrix(xRot - scene->orientation().xRotation(), 0.0, 0.0);
  setRotationValues(
      xRot, scene->orientation().yRotation(), scene->orientation().zRotation(),
      false); // Preserve y and z rotations and pass along new x rotation
  redraw();
}

void GLWindow::rotateAboutY(int yRot) {
  if (!scene) {
    return;
  }

  applyRotationToTMatrix(0.0, yRot - scene->orientation().yRotation(), 0.0);
  setRotationValues(
      scene->orientation().xRotation(), yRot, scene->orientation().zRotation(),
      false); // Preserve x and z rotations and pass along new y rotation
  redraw();
}

void GLWindow::rotateAboutZ(int zRot) {
  if (!scene) {
    return;
  }

  applyRotationToTMatrix(0.0, 0.0, zRot - scene->orientation().zRotation());
  setRotationValues(
      scene->orientation().xRotation(), scene->orientation().yRotation(), zRot,
      false); // Preserve x and y rotations and pass along new z rotation
  redraw();
}

void GLWindow::rescale(float newScale) {
  updateScale(newScale, false);
  redraw();
}

inline std::string axis_string(float a, float b, float c) {
  if (a == 1.0f && b == 0.0f && c == 0.0f) {
    return "a-axis";
  } else if (a == 0.0f && b == 1.0f && c == 0.0f) {
    return "b-axis";
  } else if (a == 0.0f && b == 0.0f && c == 1.0f) {
    return "c-axis";
  } else {
    return fmt::format("({:.3f}, {:.3f}, {:.3f})", a, b, c);
  }
}

void GLWindow::viewMillerDirection(float x, float y, float z) {
  if (!scene)
    return;
  occ::Vec3 direction =
      scene->convertToCartesian(occ::Vec3(x, y, z)).normalized();
  viewDownVector(direction);
  std::string view_string = axis_string(x, y, z);
  showMessage(QString::fromStdString(
      fmt::format("<b>View down:<b><br/>{}", view_string)));
  emit transformationMatrixChanged();
}

void GLWindow::viewDownVector(const occ::Vec3 &v) {
  if (!scene)
    return;
  auto t = scene->orientation().transformationMatrix();
  auto qv = QVector3D(v(0), v(1), v(2));
  cx::graphics::viewDownVector(qv, t);
  scene->setTransformationMatrix(t);
  redraw();
}

void GLWindow::applyRotationAboutVectorToTMatrix(float theta, float n1,
                                                 float n2, float n3) {
  if (!scene)
    return;
  auto t = scene->orientation().transformationMatrix();
  QVector3D v(n1, n2, n3);
  v.normalize();
  QQuaternion q = QQuaternion::fromAxisAndAngle(v, theta * 180 / PI);
  t.rotate(q);
  scene->setTransformationMatrix(t);
}

void GLWindow::applyRotationToTMatrix(GLfloat xRot, GLfloat yRot,
                                      GLfloat zRot) {
  if (!scene) {
    return;
  }
  QQuaternion q1 = QQuaternion::fromEulerAngles(xRot, yRot, zRot);
  scene->orientation().rotate(q1);
}

void GLWindow::applyTranslationToTMatrix(GLfloat dx, GLfloat dy) {
  if (!scene) {
    return;
  }
  auto t = scene->orientation().transformationMatrix();
  t.translate(dx, dy);
  scene->setTransformationMatrix(t);
}

void GLWindow::recenterScene() {
  if (!scene)
    return;
  scene->resetOrigin();
  getViewAngleAndScaleFromScene();
  showMessage("<b>Re-centered display</b>");
  redraw();
}

void GLWindow::saveOrientation() {
  Q_ASSERT(scene);

  bool ok;
  QString name = QInputDialog::getText(
      this, tr("Save Orientation"), tr("Input name for saved orientation:"),
      QLineEdit::Normal, "Orientation Name", &ok);
  if (ok && !name.isEmpty()) {
    scene->saveOrientation(name);
  }
}

void GLWindow::surfacePropertyChanged() { scene->setNeedsUpdate(); }

void GLWindow::switchToOrientation() {
  Q_ASSERT(scene);

  bool ok;
  QStringList items = QStringList(scene->listOfSavedOrientationNames());
  QString item = QInputDialog::getItem(this, tr("Switch to Saved Orientation"),
                                       tr("Select saved orientation:"), items,
                                       0, false, &ok);

  if (ok && !item.isEmpty()) {
    scene->resetOrientationToSavedOrientation(item);
    getViewAngleAndScaleFromScene();
    redraw();
  }
}

void GLWindow::messageLogged(const QOpenGLDebugMessage &msg) {
  QString error = "source=";
  switch (msg.source()) {
  case QOpenGLDebugMessage::APISource:
    error += "API";
    break;
  case QOpenGLDebugMessage::WindowSystemSource:
    error += "WindowSystem";
    break;
  case QOpenGLDebugMessage::ShaderCompilerSource:
    error += "ShaderCompiler";
    break;
  case QOpenGLDebugMessage::ThirdPartySource:
    error += "ThirdParty";
    break;
  case QOpenGLDebugMessage::ApplicationSource:
    error += "Application";
    break;
  case QOpenGLDebugMessage::OtherSource:
    error += "Other";
    break;
  case QOpenGLDebugMessage::InvalidSource:
    error += "Invalid";
    break;
  case QOpenGLDebugMessage::AnySource:
    error += "Any";
    break;
  }

  error += ", type=";

  switch (msg.type()) {
  case QOpenGLDebugMessage::ErrorType:
    error += "Error";
    break;
  case QOpenGLDebugMessage::DeprecatedBehaviorType:
    error += "DeprecatedBehavior";
    break;
  case QOpenGLDebugMessage::UndefinedBehaviorType:
    error += "UndefinedBehavior";
    break;
  case QOpenGLDebugMessage::PortabilityType:
    error += "Portability";
    break;
  case QOpenGLDebugMessage::PerformanceType:
    error += "Performance";
    break;
  case QOpenGLDebugMessage::OtherType:
    error += "Other";
    break;
  case QOpenGLDebugMessage::MarkerType:
    error += "Marker";
    break;
  case QOpenGLDebugMessage::GroupPushType:
    error += "GroupPush";
    break;
  case QOpenGLDebugMessage::GroupPopType:
    error += "GroupPop";
    break;
  case QOpenGLDebugMessage::AnyType:
    error += "Any";
    break;
  case QOpenGLDebugMessage::InvalidType:
    error += "Invalid";
    break;
  }

  error += "msg=\n" + msg.message();

  switch (msg.severity()) {
  case QOpenGLDebugMessage::NotificationSeverity:
    qDebug() << "NOTIFICATION: " << error;
    break;
  case QOpenGLDebugMessage::HighSeverity:
    qDebug() << "HIGH: " << error;
    break;
  case QOpenGLDebugMessage::MediumSeverity:
    qDebug() << "MEDIUM: " << error;
    break;
  case QOpenGLDebugMessage::LowSeverity:
    qDebug() << "LOW: " << error;
    break;
  case QOpenGLDebugMessage::AnySeverity:
    qDebug() << "ANY: " << error;
    break;
  case QOpenGLDebugMessage::InvalidSeverity:
    qDebug() << "INVALID: " << error;
    break;
  }
}

void GLWindow::emitContextualAtomFilter(AtomFlag flag, bool state) {
  emit contextualFilterAtoms(flag, state);
}
