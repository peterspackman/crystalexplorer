#include "aboutcrystalexplorerdialog.h"
#include "globals.h"
#include "version.h"
#include "settings.h"
#include <QDate>

AboutCrystalExplorerDialog::AboutCrystalExplorerDialog(QWidget *parent)
    : QDialog(parent) {
  setupUi(this);

  using namespace cx::globals;

  QString message = messageLabel->text();
  message.replace("%AUTHORS%", authors);
  message.replace("%BUILD_DATE%", CX_BUILD_DATE);
  message.replace(
      "%COPYRIGHT%",
      QString(copyrightNoticeTemplate).arg(QDate::currentDate().year()));
  message.replace("%APP_NAME%", name);
  message.replace("%OCC_URL%", occUrl);
  message.replace("%GIT_URL%", gitUrl);
  message.replace("%VERSION%", CX_VERSION);
  message.replace("%REVISION%", CX_GIT_REVISION);

  messageLabel->setText(message);

  setWindowTitle(settings::APPLICATION_NAME);
}
