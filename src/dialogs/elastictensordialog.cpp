#include "elastictensordialog.h"
#include <QGridLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QStringList>
#include <QRegularExpression>
#include <QApplication>
#include <QFont>

ElasticTensorDialog::ElasticTensorDialog(QWidget *parent)
    : QDialog(parent), m_elasticTensor(nullptr), m_matrixValid(false) {
    setWindowTitle("Import Elastic Tensor");
    setModal(true);
    resize(600, 500);
    
    setupUI();
    
    // Initialize with zero matrix
    m_currentMatrix = occ::Mat6::Zero();
}

void ElasticTensorDialog::setupUI() {
    auto *mainLayout = new QVBoxLayout(this);
    
    // Name input
    auto *nameLayout = new QHBoxLayout;
    nameLayout->addWidget(new QLabel("Name:"));
    m_nameEdit = new QLineEdit("Imported");
    m_nameEdit->setPlaceholderText("e.g., Experimental, DFT, Literature...");
    nameLayout->addWidget(m_nameEdit);
    mainLayout->addLayout(nameLayout);
    
    // Matrix input section
    auto *matrixGroup = new QGroupBox("Elastic Constants Matrix (GPa)");
    auto *matrixLayout = new QVBoxLayout(matrixGroup);
    
    // Instructions
    m_instructionLabel = new QLabel(
        "Paste a 6×6 Voigt stiffness matrix (C matrix) in GPa.\n"
        "Accepted formats:\n"
        "• Full 6×6 matrix (space/tab separated)\n"
        "• Upper triangular (6 + 5 + 4 + 3 + 2 + 1 = 21 values)");
    m_instructionLabel->setWordWrap(true);
    matrixLayout->addWidget(m_instructionLabel);
    
    // Load from file button
    m_loadFileButton = new QPushButton("Load from File...");
    connect(m_loadFileButton, &QPushButton::clicked, this, &ElasticTensorDialog::loadFromFile);
    matrixLayout->addWidget(m_loadFileButton);
    
    // Text input for matrix
    m_matrixTextEdit = new QTextEdit;
    m_matrixTextEdit->setFont(QFont("Consolas", 10));
    m_matrixTextEdit->setPlaceholderText(
        "Example:\n"
        "C11 C12 C13 C14 C15 C16\n"
        "C12 C22 C23 C24 C25 C26\n"
        "C13 C23 C33 C34 C35 C36\n"
        "C14 C24 C34 C44 C45 C46\n"
        "C15 C25 C35 C45 C55 C56\n"
        "C16 C26 C36 C46 C56 C66");
    connect(m_matrixTextEdit, &QTextEdit::textChanged, this, &ElasticTensorDialog::matrixTextChanged);
    matrixLayout->addWidget(m_matrixTextEdit);
    
    // Status label
    m_statusLabel = new QLabel("Enter matrix data above");
    m_statusLabel->setStyleSheet("color: gray;");
    matrixLayout->addWidget(m_statusLabel);
    
    mainLayout->addWidget(matrixGroup);
    
    // Properties display
    auto *propsGroup = new QGroupBox("Average Properties");
    auto *propsLayout = new QGridLayout(propsGroup);
    
    propsLayout->addWidget(new QLabel("Bulk Modulus:"), 0, 0);
    m_bulkModulusLabel = new QLabel("--");
    propsLayout->addWidget(m_bulkModulusLabel, 0, 1);
    propsLayout->addWidget(new QLabel("GPa"), 0, 2);
    
    propsLayout->addWidget(new QLabel("Shear Modulus:"), 1, 0);
    m_shearModulusLabel = new QLabel("--");
    propsLayout->addWidget(m_shearModulusLabel, 1, 1);
    propsLayout->addWidget(new QLabel("GPa"), 1, 2);
    
    propsLayout->addWidget(new QLabel("Young's Modulus:"), 2, 0);
    m_youngsModulusLabel = new QLabel("--");
    propsLayout->addWidget(m_youngsModulusLabel, 2, 1);
    propsLayout->addWidget(new QLabel("GPa"), 2, 2);
    
    propsLayout->addWidget(new QLabel("Poisson Ratio:"), 3, 0);
    m_poissonRatioLabel = new QLabel("--");
    propsLayout->addWidget(m_poissonRatioLabel, 3, 1);
    
    propsLayout->addWidget(new QLabel("Stability:"), 4, 0);
    m_stabilityLabel = new QLabel("--");
    propsLayout->addWidget(m_stabilityLabel, 4, 1);
    
    mainLayout->addWidget(propsGroup);
    
    // Buttons
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &ElasticTensorDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    mainLayout->addWidget(m_buttonBox);
}

void ElasticTensorDialog::loadFromFile() {
    QString fileName = QFileDialog::getOpenFileName(
        this, 
        "Load Elastic Tensor Matrix", 
        QString(),
        "Text files (*.txt *.dat);;All files (*.*)");
    
    if (fileName.isEmpty()) {
        return;
    }
    
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "Could not open file: " + fileName);
        return;
    }
    
    QTextStream in(&file);
    QString content = in.readAll();
    file.close();
    
    m_matrixTextEdit->setPlainText(content);
    
    // Set name from filename if current name is default
    if (m_nameEdit->text() == "Elastic Tensor") {
        QFileInfo fileInfo(fileName);
        m_nameEdit->setText(fileInfo.baseName());
    }
}

occ::Mat6 ElasticTensorDialog::parseMatrixText(const QString &text, bool &success) const {
    success = false;
    occ::Mat6 matrix = occ::Mat6::Zero();
    
    // Split text into lines and extract numbers
    QStringList lines = text.split('\n', Qt::SkipEmptyParts);
    QList<double> numbers;
    
    // Extract all numbers from the text
    QRegularExpression numberRegex(R"([+-]?(?:\d+\.?\d*|\.\d+)(?:[eE][+-]?\d+)?)");
    for (const QString &line : lines) {
        // Skip comment lines starting with # or %
        if (line.trimmed().startsWith('#') || line.trimmed().startsWith('%')) {
            continue;
        }
        
        auto matches = numberRegex.globalMatch(line);
        while (matches.hasNext()) {
            auto match = matches.next();
            bool ok;
            double value = match.captured().toDouble(&ok);
            if (ok) {
                numbers.append(value);
            }
        }
    }
    
    if (numbers.size() == 36) {
        // Full 6x6 matrix
        for (int i = 0; i < 6; ++i) {
            for (int j = 0; j < 6; ++j) {
                matrix(i, j) = numbers[i * 6 + j];
            }
        }
        success = true;
    } else if (numbers.size() == 21) {
        // Upper triangular matrix (21 elements)
        int idx = 0;
        for (int i = 0; i < 6; ++i) {
            for (int j = i; j < 6; ++j) {
                matrix(i, j) = numbers[idx];
                matrix(j, i) = numbers[idx]; // Make symmetric
                idx++;
            }
        }
        success = true;
    }
    
    return matrix;
}

void ElasticTensorDialog::matrixTextChanged() {
    bool parseSuccess;
    m_currentMatrix = parseMatrixText(m_matrixTextEdit->toPlainText(), parseSuccess);
    m_matrixValid = parseSuccess;
    
    if (parseSuccess) {
        // Check if matrix is physically reasonable
        constexpr double tolerance = 1e-8;
        auto eigenvals = occ::core::ElasticTensor(m_currentMatrix).eigenvalues();
        bool stable = eigenvals.minCoeff() >= tolerance;

        if (stable) {
            m_statusLabel->setText("✓ Valid elastic tensor matrix");
            m_statusLabel->setStyleSheet("color: green;");
            updateAverageProperties(m_currentMatrix);
        } else {
            m_statusLabel->setText("⚠ Matrix parsed but not physically stable (singular or negative eigenvalues)");
            m_statusLabel->setStyleSheet("color: orange;");
            m_matrixValid = false;
        }
    } else {
        m_statusLabel->setText("✗ Invalid matrix format. Expected 36 (6×6) or 21 (upper triangular) numbers.");
        m_statusLabel->setStyleSheet("color: red;");
        
        // Reset property labels
        m_bulkModulusLabel->setText("--");
        m_shearModulusLabel->setText("--");
        m_youngsModulusLabel->setText("--");
        m_poissonRatioLabel->setText("--");
        m_stabilityLabel->setText("--");
    }
    
    m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(m_matrixValid);
}

void ElasticTensorDialog::updateAverageProperties(const occ::Mat6 &matrix) {
    try {
        occ::core::ElasticTensor tensor(matrix);
        
        constexpr double tolerance = 1e-8;
        double bulkModulus = tensor.average_bulk_modulus();
        double shearModulus = tensor.average_shear_modulus();
        double youngsModulus = tensor.average_youngs_modulus();
        double poissonRatio = tensor.average_poisson_ratio();
        bool stable = tensor.eigenvalues().minCoeff() >= tolerance;

        m_bulkModulusLabel->setText(QString::number(bulkModulus, 'f', 2));
        m_shearModulusLabel->setText(QString::number(shearModulus, 'f', 2));
        m_youngsModulusLabel->setText(QString::number(youngsModulus, 'f', 2));
        m_poissonRatioLabel->setText(QString::number(poissonRatio, 'f', 3));
        m_stabilityLabel->setText(stable ? "Stable" : "Unstable");
        m_stabilityLabel->setStyleSheet(stable ? "color: green;" : "color: red;");
    } catch (const std::exception &e) {
        // Reset on error
        m_bulkModulusLabel->setText("Error");
        m_shearModulusLabel->setText("Error");
        m_youngsModulusLabel->setText("Error");
        m_poissonRatioLabel->setText("Error");
        m_stabilityLabel->setText("Error");
    }
}

void ElasticTensorDialog::accept() {
    if (!m_matrixValid) {
        QMessageBox::warning(this, "Invalid Input", "Please enter a valid elastic tensor matrix.");
        return;
    }
    
    QString name = m_nameEdit->text().trimmed();
    if (name.isEmpty()) {
        QMessageBox::warning(this, "Invalid Input", "Please enter a name for the elastic tensor.");
        return;
    }
    
    // Create the elastic tensor results
    m_elasticTensor = new ElasticTensorResults(m_currentMatrix, name);
    
    QDialog::accept();
}

ElasticTensorResults *ElasticTensorDialog::elasticTensorResults() const {
    return m_elasticTensor;
}