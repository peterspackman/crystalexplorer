#pragma once
#include <QDialog>

#include "atomid.h"
#include "jobparameters.h"
#include "wavefunction_parameters.h"
#include "generic_atom_index.h"
#include "ui_wavefunctioncalculationdialog.h"

const QString GAUSSIAN_TAB_TOOLTIP =
    "This tab is not available because the Gaussian program could not be found";
const ExternalProgram DEFAULT_WAVEFUNCTION_SOURCE = ExternalProgram::Gaussian;

class WavefunctionCalculationDialog : public QDialog,
                                      public Ui::WavefunctionCalculationDialog {
  Q_OBJECT

public:

  explicit WavefunctionCalculationDialog(QWidget *parent = 0);

  void setAtomIndices(const std::vector<GenericAtomIndex> &);
  [[nodiscard]] const std::vector<GenericAtomIndex> &atomIndices() const;

  int charge() const;
  void setCharge(int charge);

  int multiplicity() const;
  void setMultiplicity(int mult);

  QString program() const;
  QString method() const;
  QString basis() const;


public slots:
  void show();

signals:
  void wavefunctionParametersChosen(wfn::Parameters);

private slots:
  void accept();

private:
  static const QString customEntry;
  void init();
  void initPrograms();
  void initMethod();
  void initBasis();

  std::vector<GenericAtomIndex> m_atomIndices;
};
