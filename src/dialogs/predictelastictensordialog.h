#pragma once
#include <QDialog>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QDialogButtonBox>
#include <QLabel>

class PairInteractions;

class PredictElasticTensorDialog : public QDialog {
    Q_OBJECT

public:
    explicit PredictElasticTensorDialog(QWidget *parent = nullptr);

    void setAvailableModels(const QStringList &models);
    QString selectedModel() const;
    double cutoffRadius() const;

private:
    void setupUI();

    QComboBox *m_modelComboBox{nullptr};
    QDoubleSpinBox *m_radiusSpinBox{nullptr};
    QLabel *m_infoLabel{nullptr};
    QDialogButtonBox *m_buttonBox{nullptr};
};
