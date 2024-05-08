#pragma once

#include "chemicalstructure.h"
#include <QDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QSpinBox>

const int MAXMINCHARGE = 10;
const bool CONSTRAIN_CHARGES = false;

namespace Ui {
class ChargeDialog;
}

class ChargeDialog : public QDialog {
  Q_OBJECT
public:
  explicit ChargeDialog(QWidget *parent = 0);
  ~ChargeDialog();

  void setFragmentInformation(const QStringList &,
                              const std::vector<ChemicalStructure::FragmentState> &,
                              bool);
  bool hasFragmentStates();
  std::vector<ChemicalStructure::FragmentState> getFragmentStates();
  void populate(ChemicalStructure *);

public slots:
  void accept();

private slots:
  void yesRadioButtonToggled(bool);
  void chargeSpinBoxChanged(int);

private:
  void init();
  void initConnections();
  void registerConnectionsForSpinBoxes();
  void cleanupWidgets();
  void createWidgets(const QStringList &,
                     const std::vector<ChemicalStructure::FragmentState> &);
  int totalCharge();
  bool chargeIsBalanced();

  Ui::ChargeDialog *ui;

  std::vector<QSpinBox *> _chargeSpinBoxes;
  std::vector<QSpinBox *> _multiplicitySpinBoxes;
  std::vector<QLabel *> _labels;
  std::vector<QHBoxLayout *> _layouts;
};
