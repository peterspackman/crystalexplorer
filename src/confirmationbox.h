#pragma once
#include <QString>

class ConfirmationBox {
public:
  static bool getConfirmation(QString);
  static bool confirmSurfaceDeletion(bool, QString);
  static bool confirmCrystalDeletion(bool,
                                     QString crystalDescription = QString());
};
