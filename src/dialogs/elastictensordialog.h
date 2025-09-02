#pragma once
#include "elastic_tensor_results.h"
#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>
#include <QPushButton>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDialogButtonBox>

class ElasticTensorDialog : public QDialog {
    Q_OBJECT

public:
    explicit ElasticTensorDialog(QWidget *parent = nullptr);
    ~ElasticTensorDialog() = default;

    ElasticTensorResults *elasticTensorResults() const;

public slots:
    void accept() override;

private slots:
    void matrixTextChanged();
    void loadFromFile();

private:
    void setupUI();
    occ::Mat6 parseMatrixText(const QString &text, bool &success) const;
    void updateAverageProperties(const occ::Mat6 &matrix);
    
    // UI elements
    QLineEdit *m_nameEdit;
    QTextEdit *m_matrixTextEdit;
    QPushButton *m_loadFileButton;
    QLabel *m_instructionLabel;
    QLabel *m_statusLabel;
    
    // Average properties display
    QLabel *m_bulkModulusLabel;
    QLabel *m_shearModulusLabel;
    QLabel *m_youngsModulusLabel;
    QLabel *m_poissonRatioLabel;
    QLabel *m_stabilityLabel;
    
    
    QDialogButtonBox *m_buttonBox;
    
    ElasticTensorResults *m_elasticTensor;
    occ::Mat6 m_currentMatrix;
    bool m_matrixValid;
};