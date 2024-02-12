#include <QMessageBox>

#include "confirmationbox.h"
#include "dialoghtml.h"

bool ConfirmationBox::getConfirmation(QString msg) {
  QMessageBox msgBox;
  msgBox.setIcon(QMessageBox::Warning);
  msgBox.setTextFormat(Qt::RichText);
  msgBox.setText(msg);
  msgBox.setInformativeText("Do you want to continue?");
  QPushButton *okButton = msgBox.addButton(QMessageBox::Ok);
  msgBox.addButton(QMessageBox::Cancel);
  msgBox.exec();

  return (msgBox.clickedButton() == (QAbstractButton *)okButton);
}

bool ConfirmationBox::confirmSurfaceDeletion(bool deletingParent,
                                             QString surfaceDescription) {
  QString msg =
      DialogHtml::paragraph("You are about to permanently delete the surface:");
  msg +=
      DialogHtml::paragraph(DialogHtml::font(surfaceDescription, "2", "red"));

  if (deletingParent) {
    msg += DialogHtml::paragraph("AND all of its symmetry related surfaces.");
  }
  return ConfirmationBox::getConfirmation(msg);
}

bool ConfirmationBox::confirmCrystalDeletion(bool deletingAllCrystals,
                                             QString crystalDescription) {
  QString msg;

  if (deletingAllCrystals) {
    msg = DialogHtml::paragraph("You are about to permanently delete all "
                                "crystals and all their surfaces.");
  } else {
    msg = DialogHtml::paragraph(
        "You are about to permanently delete the crystal:");
    msg +=
        DialogHtml::paragraph(DialogHtml::font(crystalDescription, "3", "red"));
    msg += DialogHtml::paragraph("and all its surfaces.");
  }

  return ConfirmationBox::getConfirmation(msg);
}
