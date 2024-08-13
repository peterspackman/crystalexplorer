#pragma once

#include "chemicalstructure.h"
#include <QObject>
#include <QDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QSpinBox>

const int MAXMINCHARGE = 10;
const bool CONSTRAIN_CHARGES = false;

namespace Ui {
class FragmentStateDialog;
}

class FragmentStateDialog : public QDialog {
  Q_OBJECT
public:
  explicit FragmentStateDialog(QWidget *parent = 0);
  ~FragmentStateDialog();

  void setFragmentInformation(const QStringList &,
                              const std::vector<Fragment::State> &,
                              bool);
  bool hasFragmentStates();
  std::vector<Fragment::State> getFragmentStates();
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
                     const std::vector<Fragment::State> &);
  int totalCharge();
  bool chargeIsBalanced();

  Ui::FragmentStateDialog *ui;

  std::vector<QSpinBox *> _chargeSpinBoxes;
  std::vector<QSpinBox *> _multiplicitySpinBoxes;
  std::vector<QLabel *> _labels;
  std::vector<QHBoxLayout *> _layouts;
};
