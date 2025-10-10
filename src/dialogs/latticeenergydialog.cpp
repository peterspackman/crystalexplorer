#include "latticeenergydialog.h"
#include "settings.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QThread>

LatticeEnergyDialog::LatticeEnergyDialog(QWidget *parent)
    : QDialog(parent) {
    setupUI();
}

void LatticeEnergyDialog::setupUI() {
    setWindowTitle("Calculate Lattice Energy");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Form layout for parameters
    QFormLayout *formLayout = new QFormLayout();

    // Model selection
    m_modelComboBox = new QComboBox(this);
    m_modelComboBox->addItem("CE-B3LYP", "ce-b3lyp");
    m_modelComboBox->addItem("CE-HF", "ce-hf");
    m_modelComboBox->addItem("CE-1P", "ce-1p");
    m_modelComboBox->setCurrentIndex(0);
    formLayout->addRow("Energy Model:", m_modelComboBox);

    // Radius
    m_radiusSpinBox = new QDoubleSpinBox(this);
    m_radiusSpinBox->setRange(5.0, 50.0);
    m_radiusSpinBox->setValue(15.0);
    m_radiusSpinBox->setDecimals(1);
    m_radiusSpinBox->setSuffix(" Å");
    formLayout->addRow("Radius:", m_radiusSpinBox);

    // Threads
    m_threadsSpinBox = new QSpinBox(this);
    m_threadsSpinBox->setRange(1, 256);
    int defaultThreads = settings::readSetting(settings::keys::OCC_NTHREADS).toInt();
    if (defaultThreads <= 0) {
        defaultThreads = QThread::idealThreadCount();
    }
    m_threadsSpinBox->setValue(defaultThreads);
    formLayout->addRow("Threads:", m_threadsSpinBox);

    mainLayout->addLayout(formLayout);

    // Button box
    m_buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    mainLayout->addWidget(m_buttonBox);

    setLayout(mainLayout);
}

QString LatticeEnergyDialog::selectedModel() const {
    return m_modelComboBox->currentData().toString();
}

double LatticeEnergyDialog::radius() const {
    return m_radiusSpinBox->value();
}

int LatticeEnergyDialog::threads() const {
    return m_threadsSpinBox->value();
}
