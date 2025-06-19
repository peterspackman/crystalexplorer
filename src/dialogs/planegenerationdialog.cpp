#include "planegenerationdialog.h"
#include "ui_planegenerationdialog.h"
#include <QColorDialog>
#include "colormap.h"
#include "crystalstructure.h"
#include "surface_cut_generator.h"

inline void setButtonColor(QAbstractButton *colorButton, QColor color) {
  QString styleSheet = QString("background-color: %1;").arg(color.name());
  colorButton->setStyleSheet(styleSheet);
}

PlaneGenerationDialog::PlaneGenerationDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::PlaneGenerationDialog), 
      m_planesModel(new CrystalPlanesModel(this)),
      m_colorDelegate(new ColorDelegate(this)) {
  
  // Set initial color from colormap
  updateColorFromMap();
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
  connect(ui->createSurfaceCutButton, &QAbstractButton::clicked, this,
          &PlaneGenerationDialog::createSurfaceCutButtonClicked);
  
  // Connect visualization option signals
  connect(ui->infinitePlaneCheckBox, &QCheckBox::toggled, this,
          &PlaneGenerationDialog::visualizationOptionsChanged);
  connect(ui->showGridCheckBox, &QCheckBox::toggled, this,
          &PlaneGenerationDialog::visualizationOptionsChanged);
  connect(ui->showUnitCellIntersectionCheckBox, &QCheckBox::toggled, this,
          &PlaneGenerationDialog::visualizationOptionsChanged);
  connect(ui->gridSpacingSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
          &PlaneGenerationDialog::visualizationOptionsChanged);
  connect(ui->repeatRangeMinSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this,
          &PlaneGenerationDialog::visualizationOptionsChanged);
  connect(ui->repeatRangeMaxSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this,
          &PlaneGenerationDialog::visualizationOptionsChanged);
  
  // Connect Miller index changes to update suggested cuts
  connect(ui->hSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this,
          &PlaneGenerationDialog::updateSuggestedCuts);
  connect(ui->kSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this,
          &PlaneGenerationDialog::updateSuggestedCuts);
  connect(ui->lSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this,
          &PlaneGenerationDialog::updateSuggestedCuts);
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

double PlaneGenerationDialog::surfaceCutDepth() const {
  return ui->surfaceCutDepthSpinBox->value();
}

void PlaneGenerationDialog::removeSelectedPlane() {
  m_planesModel->removePlane(ui->currentPlanes->currentIndex().row());
  // Update color for next plane based on new total count
  updateColorFromMap();
}

void PlaneGenerationDialog::removeAllPlanes() { 
  m_planesModel->clear(); 
  // Update color for next plane based on new total count
  updateColorFromMap();
}

void PlaneGenerationDialog::createSurfaceGeometryButtonClicked() {
  CrystalPlane plane{{h(), k(), l()}, offset(), m_color};
  emit createSurfaceGeometry(plane);
}

void PlaneGenerationDialog::addPlaneFromCurrentSettings() {
  CrystalPlane plane{{h(), k(), l()}, offset(), m_color};
  if (ui->symmetryEquivalentCheckBox->isChecked() && m_crystalStructure &&
      (m_crystalStructure->spaceGroup().symmetry_operations().size() > 0)) {
    occ::Vec3 hklVec =
        occ::Vec3{static_cast<double>(h()), static_cast<double>(k()),
                 static_cast<double>(l())};
    QSet<CrystalPlane> uniquePlanes;
    uniquePlanes.insert(plane);
    for (const auto &symop: m_crystalStructure->spaceGroup().symmetry_operations()) {
      occ::Vec3 candidate = symop.rotation() * hklVec;
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
  
  // Update color for next plane based on new total count
  updateColorFromMap();
}

void PlaneGenerationDialog::loadPlanes(
    const std::vector<CrystalPlane> &planes) {
  m_planesModel->clear();
  m_planesModel->addPlanes(planes);
  // Update color for next plane based on loaded planes count
  updateColorFromMap();
}

std::vector<CrystalPlane> PlaneGenerationDialog::planes() const {
  std::vector<CrystalPlane> result;
  for (int i = 0; i < m_planesModel->rowCount(); i++) {
    result.push_back(m_planesModel->planes[i]);
  }
  return result;
}

void PlaneGenerationDialog::setCrystalStructure(CrystalStructure *crystalStructure) {
  m_crystalStructure = crystalStructure;
  updateSuggestedCuts();
}

PlaneVisualizationOptions PlaneGenerationDialog::getVisualizationOptions() const {
  PlaneVisualizationOptions options;
  options.useInfinitePlanes = ui->infinitePlaneCheckBox->isChecked();
  options.showGrid = ui->showGridCheckBox->isChecked();
  options.showUnitCellIntersection = ui->showUnitCellIntersectionCheckBox->isChecked();
  options.gridSpacing = ui->gridSpacingSpinBox->value();
  
  // Get repeat ranges from UI controls
  int minRange = ui->repeatRangeMinSpinBox->value();
  int maxRange = ui->repeatRangeMaxSpinBox->value();
  options.repeatRangeA = QVector2D(minRange, maxRange);
  options.repeatRangeB = QVector2D(minRange, maxRange);  // Use same range for both directions
  
  return options;
}

void PlaneGenerationDialog::setVisualizationOptions(const PlaneVisualizationOptions &options) {
  ui->infinitePlaneCheckBox->setChecked(options.useInfinitePlanes);
  ui->showGridCheckBox->setChecked(options.showGrid);
  ui->showUnitCellIntersectionCheckBox->setChecked(options.showUnitCellIntersection);
  ui->gridSpacingSpinBox->setValue(options.gridSpacing);
  
  // Set repeat ranges from the options (using the A range for both since UI only has one range)
  ui->repeatRangeMinSpinBox->setValue(static_cast<int>(options.repeatRangeA.x()));
  ui->repeatRangeMaxSpinBox->setValue(static_cast<int>(options.repeatRangeA.y()));
}

void PlaneGenerationDialog::updateColorFromMap() {
  try {
    // Use Hokusai1 colormap to assign colors based on number of existing planes
    ColorMap colorMap("Hokusai1", 0.0, 1.0);  // Normalized range
    int numPlanes = m_planesModel->rowCount();
    
    // Get color from colormap position (cycle through colors every 8 planes)
    double position = static_cast<double>(numPlanes % 8) / 8.0;
    m_color = colorMap(position);
  } catch (...) {
    // Fallback to a simple color cycle if colormap fails
    QStringList fallbackColors = {"#e74c3c", "#3498db", "#2ecc71", "#f39c12", 
                                  "#9b59b6", "#1abc9c", "#e67e22", "#34495e"};
    int numPlanes = m_planesModel->rowCount();
    QString colorName = fallbackColors[numPlanes % fallbackColors.size()];
    m_color = QColor(colorName);
  }
  
  // Update the UI button if it exists
  if (ui && ui->colorButton) {
    setButtonColor(ui->colorButton, m_color);
  }
}

void PlaneGenerationDialog::createSurfaceCutButtonClicked() {
  emit createSurfaceCut(h(), k(), l(), offset(), surfaceCutDepth());
}

QStringList PlaneGenerationDialog::getSuggestedCuts() const {
  QStringList suggestions;
  if (!m_crystalStructure) {
    return suggestions;
  }

  std::vector<double> cuts = cx::crystal::getSuggestedCuts(m_crystalStructure, h(), k(), l());
  
  for (double cut : cuts) {
    suggestions.append(QString::number(cut, 'f', 4));
  }

  return suggestions;
}

void PlaneGenerationDialog::updateSuggestedCuts() {
  if (!ui || !ui->suggestedCutsLabel) {
    return;
  }
  
  QStringList suggestions = getSuggestedCuts();
  if (suggestions.isEmpty()) {
    ui->suggestedCutsLabel->setText("Suggested cuts: (set valid Miller indices and crystal structure)");
  } else {
    QString text = QString("Suggested cuts: %1").arg(suggestions.join(", "));
    ui->suggestedCutsLabel->setText(text);
  }
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
  if(rowCount() <= 0) return;
  beginRemoveRows(QModelIndex(), 0, rowCount() - 1);
  planes.clear();
  endRemoveRows();
}
