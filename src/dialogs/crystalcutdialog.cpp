#include "crystalcutdialog.h"
#include "ui_crystalcutdialog.h"
#include "crystalstructure.h"
#include "surface_cut_generator.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QLabel>
#include <QListWidgetItem>
#include <QMessageBox>

CrystalCutDialog::CrystalCutDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::CrystalCutDialog) {
    ui->setupUi(this);
    setupUI();
    connectSignals();
    updateSuggestedOffsets();
    updateThicknessInfo();
}

CrystalCutDialog::~CrystalCutDialog() {
    delete ui;
}

void CrystalCutDialog::setupUI() {
    setWindowTitle("Generate Crystal Slab");
    setModal(true);
    resize(400, 500);
    
    // Set initial values
    _options.h = 1;
    _options.k = 0;
    _options.l = 0;
    _options.offset = 0.0;
    _options.thickness = 10.0;
    _options.preserveMolecules = true;
    _options.termination = "auto";
    
    // Update UI to match options
    ui->millerHSpinBox->setValue(_options.h);
    ui->millerKSpinBox->setValue(_options.k);
    ui->millerLSpinBox->setValue(_options.l);
    ui->offsetSpinBox->setValue(_options.offset);
    ui->thicknessSpinBox->setValue(_options.thickness);
    ui->preserveMoleculesCheckBox->setChecked(_options.preserveMolecules);
}

void CrystalCutDialog::connectSignals() {
    // Miller indices
    connect(ui->millerHSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &CrystalCutDialog::onMillerIndicesChanged);
    connect(ui->millerKSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &CrystalCutDialog::onMillerIndicesChanged);
    connect(ui->millerLSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &CrystalCutDialog::onMillerIndicesChanged);
    
    // Cut parameters
    connect(ui->offsetSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &CrystalCutDialog::onOffsetChanged);
    connect(ui->thicknessSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &CrystalCutDialog::onThicknessChanged);
    
    // Options
    connect(ui->terminationComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CrystalCutDialog::onTerminationChanged);
    connect(ui->preserveMoleculesCheckBox, &QCheckBox::toggled,
            this, &CrystalCutDialog::onPreserveMoleculesChanged);
    
    // Suggested offsets
    connect(ui->suggestedOffsetsList, &QListWidget::itemClicked,
            this, &CrystalCutDialog::onSuggestedOffsetClicked);
    
    // Dialog buttons
    connect(ui->buttonBox, &QDialogButtonBox::accepted,
            this, &CrystalCutDialog::onCreateSlabClicked);
    connect(ui->buttonBox, &QDialogButtonBox::rejected,
            this, &CrystalCutDialog::onCancelClicked);
}

void CrystalCutDialog::setMillerIndices(int h, int k, int l) {
    _options.h = h;
    _options.k = k;
    _options.l = l;
    
    ui->millerHSpinBox->setValue(h);
    ui->millerKSpinBox->setValue(k);
    ui->millerLSpinBox->setValue(l);
    
    updateSuggestedOffsets();
    updateThicknessInfo();
}

void CrystalCutDialog::setInitialOffset(double offset) {
    _options.offset = offset;
    ui->offsetSpinBox->setValue(offset);
}

void CrystalCutDialog::setCrystalStructure(CrystalStructure *structure) {
    _crystalStructure = structure;
    updateSuggestedOffsets();
    updateThicknessInfo();
}

SlabCutOptions CrystalCutDialog::getSlabOptions() const {
    return _options;
}

void CrystalCutDialog::onMillerIndicesChanged() {
    _options.h = ui->millerHSpinBox->value();
    _options.k = ui->millerKSpinBox->value();
    _options.l = ui->millerLSpinBox->value();
    
    updateSuggestedOffsets();
    updateThicknessInfo();
}

void CrystalCutDialog::onOffsetChanged() {
    _options.offset = ui->offsetSpinBox->value();
}

void CrystalCutDialog::onThicknessChanged() {
    _options.thickness = ui->thicknessSpinBox->value();
    updateThicknessInfo();
}

void CrystalCutDialog::onTerminationChanged() {
    int index = ui->terminationComboBox->currentIndex();
    if (index == 0) {
        _options.termination = "auto";
    } else {
        _options.termination = "custom";
    }
}

void CrystalCutDialog::onPreserveMoleculesChanged() {
    _options.preserveMolecules = ui->preserveMoleculesCheckBox->isChecked();
}

void CrystalCutDialog::onSuggestedOffsetClicked() {
    QListWidgetItem *item = ui->suggestedOffsetsList->currentItem();
    if (!item) return;
    
    // Extract the offset value from the item text
    QString text = item->text();
    bool ok;
    double offset = text.split(" ")[0].toDouble(&ok);
    if (ok) {
        ui->offsetSpinBox->setValue(offset);
        _options.offset = offset;
    }
}

void CrystalCutDialog::onCreateSlabClicked() {
    // Validate inputs
    if (_options.h == 0 && _options.k == 0 && _options.l == 0) {
        QMessageBox::warning(this, "Invalid Miller Indices",
                           "Miller indices cannot all be zero. Please enter valid (h k l) values.");
        return;
    }
    
    if (_options.thickness <= 0) {
        QMessageBox::warning(this, "Invalid Thickness",
                           "Slab thickness must be greater than zero.");
        return;
    }
    
    emit slabCutRequested(_options);
    accept();
}

void CrystalCutDialog::onCancelClicked() {
    reject();
}

void CrystalCutDialog::updateSuggestedOffsets() {
    ui->suggestedOffsetsList->clear();
    
    if (!_crystalStructure) {
        // Add generic suggestions if no crystal structure is available
        QStringList suggestions = {
            "0.00 d (at main plane)",
            "0.25 d (quarter d-spacing)",
            "0.50 d (half d-spacing)", 
            "0.75 d (three-quarter d-spacing)",
            "1.00 d (one d-spacing)"
        };
        
        for (const QString &suggestion : suggestions) {
            ui->suggestedOffsetsList->addItem(suggestion);
        }
        return;
    }
    
    // Get structure-based suggestions using the same function as PlaneGenerationDialog
    std::vector<double> cuts = cx::crystal::getSuggestedCuts(_crystalStructure, _options.h, _options.k, _options.l);
    
    if (cuts.empty()) {
        ui->suggestedOffsetsList->addItem("No suggestions available for this plane");
        return;
    }
    
    // Convert fractional cuts to d-spacing multiples and add to list
    for (double cut : cuts) {
        QString suggestion = QString("%1 d (fractional: %2)")
                            .arg(cut, 0, 'f', 3)
                            .arg(cut, 0, 'f', 4);
        ui->suggestedOffsetsList->addItem(suggestion);
    }
}

void CrystalCutDialog::updateThicknessInfo() {
    double dSpacing = 3.0; // Default estimate in Angstroms
    
    if (_crystalStructure) {
        // Calculate d-spacing using the same method as CrystalPlaneGenerator
        try {
            occ::crystal::UnitCell uc(_crystalStructure->cellVectors());
            occ::Vec3 hklVec(_options.h, _options.k, _options.l);
            if (hklVec.norm() > 0) { // Avoid division by zero
                dSpacing = 1.0 / (uc.reciprocal() * hklVec).norm();
            }
        } catch (const std::exception &e) {
            qWarning() << "Failed to calculate d-spacing:" << e.what();
        }
    }
    
    // Calculate approximate number of d-spacing units
    double numDSpacings = _options.thickness / dSpacing;
    
    QString info = QString("(~%1 d-spacings, d=%2 Å)")
                   .arg(numDSpacings, 0, 'f', 1)
                   .arg(dSpacing, 0, 'f', 3);
    ui->thicknessInfoLabel->setText(info);
}