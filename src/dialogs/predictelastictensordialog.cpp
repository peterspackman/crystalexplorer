#include "predictelastictensordialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QPushButton>

PredictElasticTensorDialog::PredictElasticTensorDialog(QWidget *parent)
    : QDialog(parent) {
    setWindowTitle("Predict Elastic Tensor");
    setupUI();
}

void PredictElasticTensorDialog::setupUI() {
    auto *mainLayout = new QVBoxLayout(this);

    // Info label
    m_infoLabel = new QLabel(
        "Predict the elastic tensor from pairwise interaction energies.\n"
        "This uses the second derivative of lattice energy with respect to strain.",
        this);
    m_infoLabel->setWordWrap(true);
    mainLayout->addWidget(m_infoLabel);

    // Settings group
    auto *settingsGroup = new QGroupBox("Settings", this);
    auto *formLayout = new QFormLayout(settingsGroup);

    // Model selector
    m_modelComboBox = new QComboBox(this);
    m_modelComboBox->setMinimumWidth(200);
    formLayout->addRow("Energy Model:", m_modelComboBox);

    // Cutoff radius
    m_radiusSpinBox = new QDoubleSpinBox(this);
    m_radiusSpinBox->setRange(5.0, 30.0);
    m_radiusSpinBox->setValue(12.0);
    m_radiusSpinBox->setSuffix(" \u00C5"); // Angstrom symbol
    m_radiusSpinBox->setDecimals(1);
    m_radiusSpinBox->setSingleStep(1.0);
    m_radiusSpinBox->setToolTip("Maximum distance for pair interactions to include");
    formLayout->addRow("Cutoff Radius:", m_radiusSpinBox);

    mainLayout->addWidget(settingsGroup);

    // Buttons
    m_buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_buttonBox->button(QDialogButtonBox::Ok)->setText("Predict");
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    mainLayout->addWidget(m_buttonBox);

    setMinimumWidth(350);
}

void PredictElasticTensorDialog::setAvailableModels(const QStringList &models) {
    m_modelComboBox->clear();
    m_modelComboBox->addItems(models);
    m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(!models.isEmpty());
}

QString PredictElasticTensorDialog::selectedModel() const {
    return m_modelComboBox->currentText();
}

double PredictElasticTensorDialog::cutoffRadius() const {
    return m_radiusSpinBox->value();
}
