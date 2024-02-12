#include "planegenerationdialog.h"
#include "ui_planegenerationdialog.h"
#include <QColorDialog>

void setButtonColor(QAbstractButton *colorButton, QColor color) {
  QString styleSheet = QString("background-color: %1;").arg(color.name());
  colorButton->setStyleSheet(styleSheet);
}

PlaneGenerationDialog::PlaneGenerationDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::PlaneGenerationDialog), m_color("red"),
      m_planesModel(new CrystalPlanesModel(this)),
      m_colorDelegate(new ColorDelegate(this)) {
  ui->setupUi(this);
  setButtonColor(ui->colorButton, m_color);
  connect(ui->colorButton, &QAbstractButton::clicked, this,
          &PlaneGenerationDialog::onColorButtonClicked);

  ui->currentPlanes->setModel(m_planesModel);
  ui->currentPlanes->setItemDelegateForColumn(4, m_colorDelegate);
  connect(ui->addPlaneButton, &QAbstractButton::clicked, this,
          &PlaneGenerationDialog::addPlaneFromCurrentSettings);
  connect(ui->removeAllPlanesButton, &QAbstractButton::clicked, this,
          &PlaneGenerationDialog::removeAllPlanes);
  connect(ui->removePlaneButton, &QAbstractButton::clicked, this,
          &PlaneGenerationDialog::removeSelectedPlane);
  connect(ui->createSurfaceStructureButton, &QAbstractButton::clicked, this,
          &PlaneGenerationDialog::createSurfaceGeometryButtonClicked);
}

void PlaneGenerationDialog::onColorButtonClicked() {
  QColor color =
      QColorDialog::getColor(m_color, this, "Select color for the plane");

  if (color.isValid()) {
    setButtonColor(ui->colorButton, color);
    m_color = color;
  }
}

int PlaneGenerationDialog::h() const { return ui->hSpinBox->value(); }

int PlaneGenerationDialog::k() const { return ui->kSpinBox->value(); }

int PlaneGenerationDialog::l() const { return ui->lSpinBox->value(); }

double PlaneGenerationDialog::offset() const {
  return ui->offsetDoubleSpinBox->value();
}

void PlaneGenerationDialog::removeSelectedPlane() {
  m_planesModel->removePlane(ui->currentPlanes->currentIndex().row());
}

void PlaneGenerationDialog::removeAllPlanes() { m_planesModel->clear(); }

void PlaneGenerationDialog::createSurfaceGeometryButtonClicked() {
  CrystalPlane plane{{h(), k(), l()}, offset(), m_color};
  emit createSurfaceGeometry(plane);
}

void PlaneGenerationDialog::addPlaneFromCurrentSettings() {
  CrystalPlane plane{{h(), k(), l()}, offset(), m_color};
  if (ui->symmetryEquivalentCheckBox->isChecked() &&
      (m_spaceGroup.numberOfSymops() > 0)) {
    Vector3q hklVec =
        Vector3q{static_cast<double>(h()), static_cast<double>(k()),
                 static_cast<double>(l())};
    QSet<CrystalPlane> uniquePlanes;
    uniquePlanes.insert(plane);
    for (int i = 0; i < m_spaceGroup.numberOfSymops(); i++) {
      Vector3q candidate = m_spaceGroup.rotationMatrixForSymop(i) * hklVec;
      CrystalPlane candidatePlane = plane;
      candidatePlane.hkl = {static_cast<int>(candidate(0)),
                            static_cast<int>(candidate(1)),
                            static_cast<int>(candidate(2))};
      uniquePlanes.insert(candidatePlane);
    }
    std::vector<CrystalPlane> planesToAdd(uniquePlanes.begin(),
                                          uniquePlanes.end());
    m_planesModel->addPlanes(planesToAdd);
  } else {
    m_planesModel->addPlane(plane);
  }
}

void PlaneGenerationDialog::loadPlanes(
    const std::vector<CrystalPlane> &planes) {
  m_planesModel->clear();
  m_planesModel->addPlanes(planes);
}

std::vector<CrystalPlane> PlaneGenerationDialog::planes() const {
  std::vector<CrystalPlane> result;
  for (int i = 0; i < m_planesModel->rowCount(); i++) {
    result.push_back(m_planesModel->planes[i]);
  }
  return result;
}

void PlaneGenerationDialog::setSpaceGroup(const SpaceGroup &sg) {
  m_spaceGroup = sg;
}

PlaneGenerationDialog::~PlaneGenerationDialog() { delete ui; }

int CrystalPlanesModel::rowCount(const QModelIndex &parent) const {
  Q_UNUSED(parent);
  return planes.count();
}

int CrystalPlanesModel::columnCount(const QModelIndex &parent) const {
  Q_UNUSED(parent);
  return 5;
}

QVariant CrystalPlanesModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid())
    return QVariant();

  if (index.row() >= planes.size() || index.row() < 0)
    return QVariant();

  if (role == Qt::DisplayRole) {
    const auto &plane = planes.at(index.row());

    if (index.column() == 0)
      return plane.hkl.h;
    else if (index.column() == 1)
      return plane.hkl.k;
    else if (index.column() == 2)
      return plane.hkl.l;
    else if (index.column() == 3)
      return plane.offset;
    else if (index.column() == 4)
      return plane.color.name();
  }
  return QVariant();
}

QVariant CrystalPlanesModel::headerData(int section,
                                        Qt::Orientation orientation,
                                        int role) const {
  if (role != Qt::DisplayRole)
    return QVariant();

  if (orientation == Qt::Horizontal) {
    switch (section) {
    case 0:
      return tr("h");
    case 1:
      return tr("k");
    case 2:
      return tr("l");
    case 3:
      return tr("offset");
    case 4:
      return tr("color");
    default:
      return QVariant();
    }
  }
  return QVariant();
}

bool CrystalPlanesModel::setData(const QModelIndex &index,
                                 const QVariant &value, int role) {
  if (index.isValid() && role == Qt::EditRole) {
    int row = index.row();

    auto plane = planes.value(row);

    if (index.column() == 0)
      plane.hkl.h = value.toInt();
    else if (index.column() == 1)
      plane.hkl.k = value.toInt();
    else if (index.column() == 2)
      plane.hkl.l = value.toInt();
    else if (index.column() == 3)
      plane.offset = value.toFloat();
    else if (index.column() == 4)
      plane.color = QColor(value.toString());

    planes.replace(row, plane);
    emit dataChanged(index, index, {role});

    return true;
  }
  return false;
}

Qt::ItemFlags CrystalPlanesModel::flags(const QModelIndex &index) const {
  if (!index.isValid())
    return Qt::ItemIsEnabled;

  return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
}

void CrystalPlanesModel::addPlane(const CrystalPlane &plane) {
  QSet<CrystalPlane> uniquePlanes(planes.begin(), planes.end());
  if (!uniquePlanes.contains(plane)) {
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    planes.append(plane);
    endInsertRows();
  }
}

void CrystalPlanesModel::addPlanes(const std::vector<CrystalPlane> &planesVec) {
  QSet<CrystalPlane> uniquePlanes(planes.begin(), planes.end());

  QList<CrystalPlane> planesToAdd;
  for (const CrystalPlane &plane : planesVec) {
    if (!uniquePlanes.contains(plane)) {
      planesToAdd.push_back(plane);
      uniquePlanes.insert(plane);
    }
  }
  if (!planesToAdd.empty()) {
    beginInsertRows(QModelIndex(), rowCount(),
                    rowCount() + planesToAdd.size() - 1);
    planes.append(planesToAdd);
    endInsertRows();
  }
}

void CrystalPlanesModel::removePlane(int row) {
  if (row < 0 || row >= planes.count())
    return;
  beginRemoveRows(QModelIndex(), row, row);
  planes.removeAt(row);
  endRemoveRows();
}

void CrystalPlanesModel::clear() {
  beginRemoveRows(QModelIndex(), 0, rowCount() - 1);
  planes.clear();
  endRemoveRows();
}
