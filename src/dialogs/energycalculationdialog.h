#pragma once
#include "chemicalstructure.h"
#include "molecular_wavefunction.h"
#include "pair_energy_parameters.h"
#include "ui_energycalculationdialog.h"
#include <QDialog>

class EnergyCalculationDialog : public QDialog,
                                public Ui::EnergyCalculationDialog {

  Q_OBJECT

public:
  enum class WavefunctionRequirement {
    None,
    ChooseA,
    ComplementaryA,
    ComplementaryB
  };

  EnergyCalculationDialog(QWidget *);
  void setChemicalStructure(ChemicalStructure *);
  std::vector<wfn::Parameters> requiredWavefunctions();
  FragmentPairs fragmentPairs();

  QString selectedEnergyModel() const;

signals:
  void energyParametersChosen(pair_energy::EnergyModelParameters);
private slots:
  void validate();
  void showEvent(QShowEvent *);

  void handleModelChange();
  bool handleStructureChange();
  void updateWavefunctionComboBox();

private:
  bool methodIsDefined() const;
  void init();
  void initConnections();

  ChemicalStructure *m_structure{nullptr};
  std::vector<MolecularWavefunction *> m_wavefunctions;
  QString m_method{"b3lyp"};
  QString m_basis{"def2-svp"};

  std::vector<wfn::Parameters> m_requiredWavefunctions;
  FragmentPairs m_fragmentPairs;
  std::vector<FragmentDimer> m_fragmentPairsToCalculate;
};
