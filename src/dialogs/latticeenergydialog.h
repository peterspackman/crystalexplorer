#pragma once
#include <QDialog>
#include <QLabel>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QDialogButtonBox>

class LatticeEnergyDialog : public QDialog {
    Q_OBJECT

public:
    explicit LatticeEnergyDialog(QWidget *parent = nullptr);
    ~LatticeEnergyDialog() = default;

    QString selectedModel() const;
    double radius() const;
    int threads() const;

private:
    void setupUI();

    // UI elements
    QComboBox *m_modelComboBox;
    QDoubleSpinBox *m_radiusSpinBox;
    QSpinBox *m_threadsSpinBox;
    QDialogButtonBox *m_buttonBox;
};
