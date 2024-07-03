#pragma once
#include <QDialog>
#include <QStringList>

#include "globals.h"
#include "hbond_criteria.h"
#include "ui_closecontactsdialog.h"

const int HBOND_TAB = 0;
const int CLOSE_CONTACTS_TAB = 1;

class CloseContactDialog : public QDialog, public Ui::CloseContactsDialog {
  Q_OBJECT

public:
  CloseContactDialog(QWidget *parent = 0);
  void updateDonorsAndAcceptors(QStringList, QStringList);

public slots:
  void showDialogWithHydrogenBondTab();
  void showDialogWithCloseContactsTab();

signals:
  void hbondColorChanged();
  void hbondCriteriaChanged(HBondCriteria);
  void hbondsToggled(bool);
  void cc1Toggled(bool);
  void cc2Toggled(bool);
  void cc3Toggled(bool);
  void closeContactsSettingsChanged(int, QString, QString, double);
  void closeContactsColorChanged();

private slots:
  void updateCloseContact1(bool);
  void updateCloseContact2(bool);
  void updateCloseContact3(bool);

  void reportCC1SettingsChanges();
  void reportCC2SettingsChanges();
  void reportCC3SettingsChanges();

  void updateContact1DistanceCriteria();
  void updateContact2DistanceCriteria();
  void updateContact3DistanceCriteria();

  void updateCloseContact1Color();
  void updateCloseContact2Color();
  void updateCloseContact3Color();

  void reportHBondColorChange();
  void reportHBondSettingsChanges();

private:
  HBondCriteria currentHBondCriteria();
  void init();
  void initConnections();
  void updateComboBox(QComboBox *, QStringList);
  void setEnabledCloseContact1(bool);
  void setEnabledCloseContact2(bool);
  void setEnabledCloseContact3(bool);
  void setButtonColor(QToolButton *, QColor);
  void updateContactDistanceCriteria(QComboBox *, QComboBox *,
                                     QDoubleSpinBox *);
  double largestVdwRadiusForAllElements();
  QColor getButtonColor(QToolButton *);
  void updateCloseContactColor(QToolButton *, QString);
};
