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

  QString program() const;
  QString method() const;
  QString basis() const;


  const wfn::Parameters& getParameters() const;

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

  wfn::Parameters m_parameters;

};
