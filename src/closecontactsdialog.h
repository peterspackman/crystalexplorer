#pragma once
#include <QDialog>
#include <QStringList>

#include "closecontactcriteriawidget.h"
#include "globals.h"
#include "hbond_criteria.h"
#include "ui_closecontactsdialog.h"

constexpr int HBOND_TAB = 0;
constexpr int CLOSE_CONTACTS_TAB = 1;


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
  void closeContactsSettingsChanged(int, CloseContactCriteria);

private slots:
  void reportHBondColorChange();
  void reportHBondSettingsChanges();
  void useVdwBasedCriteria(bool);

private:
  HBondCriteria currentHBondCriteria();
  void init();
  void initConnections();
  void updateComboBox(QComboBox *, QStringList);
  void setButtonColor(QToolButton *, QColor);

  void updateContactDistanceCriteria(QComboBox *, QComboBox *,
                                     QDoubleSpinBox *);
  QColor getButtonColor(QToolButton *);

  QMap<QString, CloseContactCriteria> m_closeContactSettings;
};
