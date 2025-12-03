#include "elastictensorinfodocument.h"
#include "predictelastictensordialog.h"
#include "chemicalstructure.h"
#include "scene.h"
#include "icosphere_mesh.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFont>
#include <occ/core/constants.h>

inline const char *INFO_HORIZONTAL_RULE =
    "--------------------------------------------------------------------------"
    "------------\n";

ElasticTensorInfoDocument::ElasticTensorInfoDocument(QWidget *parent)
    : QWidget(parent) {
    setupUI();
    populateDocument();
}

void ElasticTensorInfoDocument::setupUI() {
    QVBoxLayout *layout = new QVBoxLayout(this);

    // Predict button at the top
    QHBoxLayout *buttonLayout = new QHBoxLayout();

    m_calculateButton = new QPushButton("Predict Elastic Tensor...", this);
    m_calculateButton->setEnabled(false);
    connect(m_calculateButton, &QPushButton::clicked, this, &ElasticTensorInfoDocument::onCalculateButtonClicked);

    buttonLayout->addWidget(m_calculateButton);
    buttonLayout->addStretch();

    layout->addLayout(buttonLayout);

    QFont monoFont("Courier");
    monoFont.setStyleHint(QFont::Monospace);
    monoFont.setFixedPitch(true);

    m_contents = new QTextEdit(this);
    m_contents->document()->setDefaultFont(monoFont);
    layout->addWidget(m_contents);
}

void ElasticTensorInfoDocument::onCalculateButtonClicked() {
    if (!m_scene) return;

    auto *structure = m_scene->chemicalStructure();
    if (!structure) return;

    auto *interactions = structure->pairInteractions();
    if (!interactions || interactions->getCount() == 0) return;

    PredictElasticTensorDialog dialog(this);
    dialog.setAvailableModels(interactions->interactionModels());

    if (dialog.exec() == QDialog::Accepted) {
        QString model = dialog.selectedModel();
        double radius = dialog.cutoffRadius();
        if (!model.isEmpty()) {
            emit calculateElasticTensorRequested(model, radius);
        }
    }
}

void ElasticTensorInfoDocument::updateButtonState() {
    bool hasInteractions = false;
    if (m_scene) {
        auto *structure = m_scene->chemicalStructure();
        if (structure) {
            auto *interactions = structure->pairInteractions();
            if (interactions && interactions->getCount() > 0) {
                hasInteractions = true;
            }
        }
    }

    m_calculateButton->setEnabled(hasInteractions);
}

void ElasticTensorInfoDocument::populateDocument() {
    m_contents->clear();

    if (!m_currentTensor) {
        m_contents->setText("No elastic tensor selected\n\nImport or calculate an elastic tensor to view properties");
        return;
    }

    QTextCursor cursor = m_contents->textCursor();
    cursor.beginEditBlock();
    
    insertTensorMatrices(cursor, m_currentTensor);
    insertAverageProperties(cursor, m_currentTensor);
    insertEigenvalues(cursor, m_currentTensor);
    insertExtremaAndDirections(cursor, m_currentTensor);
    
    cursor.endEditBlock();
    resetCursorToBeginning();
}

void ElasticTensorInfoDocument::resetCursorToBeginning() {
    QTextCursor cursor = m_contents->textCursor();
    cursor.movePosition(QTextCursor::Start);
    m_contents->setTextCursor(cursor);
    m_contents->ensureCursorVisible();
}

void ElasticTensorInfoDocument::updateScene(Scene *scene) {
    m_scene = scene;
    updateButtonState();
    populateDocument();
}

void ElasticTensorInfoDocument::updateElasticTensor(ElasticTensorResults *tensor) {
    m_currentTensor = tensor;
    populateDocument();
}

void ElasticTensorInfoDocument::forceUpdate() {
    populateDocument();
}

void ElasticTensorInfoDocument::insertTensorMatrices(QTextCursor &cursor, ElasticTensorResults *tensor) {
    if (!tensor) return;

    const QString TITLE = QString("Elastic Tensor: %1").arg(tensor->name());
    
    cursor.insertText(INFO_HORIZONTAL_RULE);
    cursor.insertText(TITLE + "\n");
    cursor.insertText(INFO_HORIZONTAL_RULE);
    cursor.insertText("\n");
    
    // Stability status
    bool stable = tensor->isStable();
    cursor.insertText(QString("Status\t\t%1\n\n").arg(stable ? "Stable" : "Unstable"));
    
    // Stiffness matrix (Voigt notation)
    cursor.insertText("Stiffness Matrix (GPa):\n");
    const auto& stiffness = tensor->voigtStiffness();
    
    cursor.insertText("       C11      C12      C13      C14      C15      C16\n");
    cursor.insertText("    --------------------------------------------------------\n");
    
    for (int i = 0; i < 6; ++i) {
        cursor.insertText(QString("C%1%2").arg(i+1).arg(i+1 < 10 ? " " : ""));
        for (int j = 0; j < 6; ++j) {
            cursor.insertText(QString("%1 ").arg(stiffness(i, j), 8, 'f', 1));
        }
        cursor.insertText("\n");
    }
    cursor.insertText("\n");
    
    // Compliance matrix (Voigt notation) 
    cursor.insertText("Compliance Matrix (1/TPa):\n");
    const auto& compliance = tensor->voigtCompliance();
    
    cursor.insertText("        S11       S12       S13       S14       S15       S16\n");
    cursor.insertText("    ----------------------------------------------------------------\n");
    
    for (int i = 0; i < 6; ++i) {
        cursor.insertText(QString("S%1%2").arg(i+1).arg(i+1 < 10 ? " " : ""));
        for (int j = 0; j < 6; ++j) {
            cursor.insertText(QString("%1 ").arg(compliance(i, j) * 1000.0, 9, 'f', 3)); // Convert to 1/TPa
        }
        cursor.insertText("\n");
    }
    cursor.insertText("\n");
}

void ElasticTensorInfoDocument::insertAverageProperties(QTextCursor &cursor, ElasticTensorResults *tensor) {
    if (!tensor) return;

    const QString TITLE = "Average Elastic Properties";
    
    cursor.insertText(INFO_HORIZONTAL_RULE);
    cursor.insertText(TITLE + "\n");
    cursor.insertText(INFO_HORIZONTAL_RULE);
    cursor.insertText("\n");
    
    // Show properties for all averaging schemes
    const char* schemes[] = {"Hill", "Voigt", "Reuss"};
    occ::core::ElasticTensor::AveragingScheme schemeEnums[] = {
        occ::core::ElasticTensor::AveragingScheme::Hill,
        occ::core::ElasticTensor::AveragingScheme::Voigt,
        occ::core::ElasticTensor::AveragingScheme::Reuss
    };
    
    cursor.insertText("Property        Hill    Voigt   Reuss   Units\n");
    cursor.insertText("-------------------------------------------\n");
    
    for (int i = 0; i < 3; ++i) {
        auto scheme = schemeEnums[i];
        
        if (i == 0) {
            cursor.insertText(QString("Bulk Modulus    %1   %2   %3   GPa\n")
                .arg(tensor->averageBulkModulus(scheme), 6, 'f', 1)
                .arg(tensor->averageBulkModulus(schemeEnums[1]), 6, 'f', 1)
                .arg(tensor->averageBulkModulus(schemeEnums[2]), 6, 'f', 1));
            
            cursor.insertText(QString("Shear Modulus   %1   %2   %3   GPa\n")
                .arg(tensor->averageShearModulus(scheme), 6, 'f', 1)
                .arg(tensor->averageShearModulus(schemeEnums[1]), 6, 'f', 1)
                .arg(tensor->averageShearModulus(schemeEnums[2]), 6, 'f', 1));
            
            cursor.insertText(QString("Young's Modulus %1   %2   %3   GPa\n")
                .arg(tensor->averageYoungsModulus(scheme), 6, 'f', 1)
                .arg(tensor->averageYoungsModulus(schemeEnums[1]), 6, 'f', 1)
                .arg(tensor->averageYoungsModulus(schemeEnums[2]), 6, 'f', 1));
            
            cursor.insertText(QString("Poisson Ratio   %1   %2   %3   -\n")
                .arg(tensor->averagePoissonRatio(scheme), 6, 'f', 3)
                .arg(tensor->averagePoissonRatio(schemeEnums[1]), 6, 'f', 3)
                .arg(tensor->averagePoissonRatio(schemeEnums[2]), 6, 'f', 3));
            break;
        }
    }
    cursor.insertText("\n");
}

void ElasticTensorInfoDocument::insertEigenvalues(QTextCursor &cursor, ElasticTensorResults *tensor) {
    if (!tensor) return;

    const QString TITLE = "Eigenvalue Analysis";
    
    cursor.insertText(INFO_HORIZONTAL_RULE);
    cursor.insertText(TITLE + "\n");
    cursor.insertText(INFO_HORIZONTAL_RULE);
    cursor.insertText("\n");
    
    constexpr double tolerance = 1e-8;
    occ::Vec6 eigenvals = tensor->eigenvalues();

    cursor.insertText("Eigenvalue  Value (GPa)  Stability\n");
    cursor.insertText("--------------------------------\n");

    for (int i = 0; i < 6; ++i) {
        double val = eigenvals(i);
        QString stability = (val >= tolerance) ? "Stable  " : "Unstable";
        cursor.insertText(QString("λ%1          %2    %3\n")
            .arg(i+1)
            .arg(val, 9, 'e', 3)
            .arg(stability));
    }
    cursor.insertText("\n");

    bool allPositive = tensor->isStable();
    cursor.insertText(QString("Overall Stability: %1\n").arg(allPositive ? "Stable (all eigenvalues > 0)" : "Unstable (singular or negative eigenvalues)"));
    cursor.insertText("\n");
}

void ElasticTensorInfoDocument::insertExtremaAndDirections(QTextCursor &cursor, ElasticTensorResults *tensor) {
    if (!tensor) return;

    const QString TITLE = "Directional Extrema";
    
    cursor.insertText(INFO_HORIZONTAL_RULE);
    cursor.insertText(TITLE + "\n");
    cursor.insertText(INFO_HORIZONTAL_RULE);
    cursor.insertText("\n");
    
    // Sample directions to find extrema - using icosphere vertices for good coverage
    const int samples = 162; // 3-subdivision icosphere
    double minYoung = std::numeric_limits<double>::max();
    double maxYoung = std::numeric_limits<double>::lowest();
    double minShear = std::numeric_limits<double>::max();
    double maxShear = std::numeric_limits<double>::lowest();
    double minCompress = std::numeric_limits<double>::max();
    double maxCompress = std::numeric_limits<double>::lowest();
    double minPoisson = std::numeric_limits<double>::max();
    double maxPoisson = std::numeric_limits<double>::lowest();
    
    occ::Vec3 minYoungDir, maxYoungDir;
    occ::Vec3 minShearDir, maxShearDir;
    occ::Vec3 minCompressDir, maxCompressDir;
    occ::Vec3 minPoissonDir, maxPoissonDir;
    
    // Create icosphere for sampling
    auto vertices = IcosphereMesh::generateVertices(3);
    
    for (int i = 0; i < vertices.cols() && i < samples; ++i) {
        occ::Vec3 dir = vertices.col(i).normalized();
        
        double young = tensor->youngsModulus(dir);
        if (young < minYoung) { minYoung = young; minYoungDir = dir; }
        if (young > maxYoung) { maxYoung = young; maxYoungDir = dir; }
        
        double compress = tensor->linearCompressibility(dir);
        if (compress < minCompress) { minCompress = compress; minCompressDir = dir; }
        if (compress > maxCompress) { maxCompress = compress; maxCompressDir = dir; }
        
        // Sample shear and Poisson with multiple angles
        for (double angle = 0; angle < occ::constants::pi<double>; angle += occ::constants::pi<double> / 18.0) {
            double shear = tensor->shearModulus(dir, angle);
            if (shear < minShear) { minShear = shear; minShearDir = dir; }
            if (shear > maxShear) { maxShear = shear; maxShearDir = dir; }
            
            double poisson = tensor->poissonRatio(dir, angle);
            if (poisson < minPoisson) { minPoisson = poisson; minPoissonDir = dir; }
            if (poisson > maxPoisson) { maxPoisson = poisson; maxPoissonDir = dir; }
        }
    }
    
    cursor.insertText("Property               Min Value    Max Value    Units\n");
    cursor.insertText("---------------------------------------------------\n");
    cursor.insertText(QString("Young's Modulus        %1      %2      GPa\n")
        .arg(minYoung, 9, 'f', 2).arg(maxYoung, 9, 'f', 2));
    cursor.insertText(QString("Shear Modulus          %1      %2      GPa\n")
        .arg(minShear, 9, 'f', 2).arg(maxShear, 9, 'f', 2));
    cursor.insertText(QString("Linear Compress.       %1      %2      1/TPa\n")
        .arg(minCompress * 1000.0, 9, 'f', 3).arg(maxCompress * 1000.0, 9, 'f', 3));
    cursor.insertText(QString("Poisson Ratio          %1      %2      -\n")
        .arg(minPoisson, 9, 'f', 3).arg(maxPoisson, 9, 'f', 3));
    cursor.insertText("\n");
    
    cursor.insertText("Extreme Directions (Cartesian):\n");
    cursor.insertText("Property               Min Direction           Max Direction\n");
    cursor.insertText("-----------------------------------------------------------\n");
    cursor.insertText(QString("Young's Modulus        [%1,%2,%3]    [%4,%5,%6]\n")
        .arg(minYoungDir.x(), 6, 'f', 3).arg(minYoungDir.y(), 6, 'f', 3).arg(minYoungDir.z(), 6, 'f', 3)
        .arg(maxYoungDir.x(), 6, 'f', 3).arg(maxYoungDir.y(), 6, 'f', 3).arg(maxYoungDir.z(), 6, 'f', 3));
    cursor.insertText(QString("Shear Modulus          [%1,%2,%3]    [%4,%5,%6]\n")
        .arg(minShearDir.x(), 6, 'f', 3).arg(minShearDir.y(), 6, 'f', 3).arg(minShearDir.z(), 6, 'f', 3)
        .arg(maxShearDir.x(), 6, 'f', 3).arg(maxShearDir.y(), 6, 'f', 3).arg(maxShearDir.z(), 6, 'f', 3));
    cursor.insertText(QString("Linear Compress.       [%1,%2,%3]    [%4,%5,%6]\n")
        .arg(minCompressDir.x(), 6, 'f', 3).arg(minCompressDir.y(), 6, 'f', 3).arg(minCompressDir.z(), 6, 'f', 3)
        .arg(maxCompressDir.x(), 6, 'f', 3).arg(maxCompressDir.y(), 6, 'f', 3).arg(maxCompressDir.z(), 6, 'f', 3));
    cursor.insertText(QString("Poisson Ratio          [%1,%2,%3]    [%4,%5,%6]\n")
        .arg(minPoissonDir.x(), 6, 'f', 3).arg(minPoissonDir.y(), 6, 'f', 3).arg(minPoissonDir.z(), 6, 'f', 3)
        .arg(maxPoissonDir.x(), 6, 'f', 3).arg(maxPoissonDir.y(), 6, 'f', 3).arg(maxPoissonDir.z(), 6, 'f', 3));
    cursor.insertText("\n");
}