#pragma once

#include <QDialog>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QGroupBox>

class CrystalStructure;

namespace Ui {
class CrystalCutDialog;
}

struct SlabCutOptions {
    int h{1}, k{0}, l{0};           // Miller indices
    double offset{0.0};             // Cut offset along normal (d-spacing units)
    double thickness{10.0};         // Slab thickness in Angstroms
    bool preserveMolecules{true};   // Keep whole molecules vs cutting atoms
    QString termination{"auto"};    // Surface termination identifier
};

class CrystalCutDialog : public QDialog {
    Q_OBJECT

public:
    explicit CrystalCutDialog(QWidget *parent = nullptr);
    ~CrystalCutDialog();

    // Set initial Miller indices from the plane
    void setMillerIndices(int h, int k, int l);
    void setInitialOffset(double offset);
    void setCrystalStructure(CrystalStructure *structure);

    // Get the final slab cut options
    SlabCutOptions getSlabOptions() const;

signals:
    void slabCutRequested(const SlabCutOptions &options);

private slots:
    void onMillerIndicesChanged();
    void onOffsetChanged();
    void onThicknessChanged();
    void onTerminationChanged();
    void onPreserveMoleculesChanged();
    void onSuggestedOffsetClicked();
    void onCreateSlabClicked();
    void onCancelClicked();

private:
    void setupUI();
    void updateSuggestedOffsets();
    void updateThicknessInfo();
    void connectSignals();

    Ui::CrystalCutDialog *ui;
    
    // Current slab options
    SlabCutOptions _options;
    
    // Crystal structure for generating suggestions
    CrystalStructure *_crystalStructure{nullptr};
};