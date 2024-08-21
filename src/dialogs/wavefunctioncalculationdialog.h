#pragma once
#include <QDialog>

#include "wavefunction_parameters.h"
#include "generic_atom_index.h"
#include "ui_wavefunctioncalculationdialog.h"

class WavefunctionCalculationDialog : public QDialog,
                                      public Ui::WavefunctionCalculationDialog {
  Q_OBJECT

public:

  explicit WavefunctionCalculationDialog(QWidget *parent = 0);

  void setAtomIndices(const std::vector<GenericAtomIndex> &);
  [[nodiscard]] const std::vector<GenericAtomIndex> &atomIndices() const;

  int charge() const;
  void setCharge(int charge);
  bool isXtbMethod() const;

  int multiplicity() const;
  void setMultiplicity(int mult);

  QString method() const;
  QString basis() const;

  QString selectedProgramName() const;
  wfn::Program selectedProgram() const;


  const wfn::Parameters& getParameters() const;

public slots:
  void show();

signals:
  void wavefunctionParametersChosen(wfn::Parameters);

private slots:
  void accept();
  void updateMethodOptions();
  void updateBasisSetOptions();

private:
  static const QString customEntry;
  void init();
  void initPrograms();

  wfn::Parameters m_parameters;

};
