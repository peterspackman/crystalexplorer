#pragma once

#include "chargemultiplicitypair.h"
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

  void setChargeMultiplicityInfo(const QStringList &,
                                 const std::vector<ChargeMultiplicityPair> &,
                                 bool);
  bool hasChargesAndMultiplicities();
  std::vector<ChargeMultiplicityPair> getChargesAndMultiplicities();
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
                     const std::vector<ChargeMultiplicityPair> &);
  int totalCharge();
  bool chargeIsBalanced();

  Ui::ChargeDialog *ui;

  std::vector<QSpinBox *> _chargeSpinBoxes;
  std::vector<QSpinBox *> _multiplicitySpinBoxes;
  std::vector<QLabel *> _labels;
  std::vector<QHBoxLayout *> _layouts;
};
