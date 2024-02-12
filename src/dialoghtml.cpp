#include "dialoghtml.h"

QString DialogHtml::italic(QString text) {
  return QString("<i>" + text + "</i>");
}

QString DialogHtml::bold(QString text) {
  return QString("<b>" + text + "</b>");
}

QString DialogHtml::linebreak() { return QString("<br />"); }

QString DialogHtml::line() { return QString("<hr width=\"300\">"); }

QString DialogHtml::font(QString text, QString fontSize, QString color) {
  return QString("<font size=" + fontSize + " color =" + color + ">" + text +
                 "</font>");
}

QString DialogHtml::paragraph(QString text) {
  return QString("<p>" + text + "</p>");
}

QString DialogHtml::website(QString webaddress, QString link) {
  return QString("<a href=\"" + webaddress + "\">" + link + "</a>");
}

QString DialogHtml::email(QString email, QString link, QString subject,
                          QString body) {
  return QString("<a href=\"mailto:" + email + "?subject=" + subject +
                 "&body=" + body + "\">" + link + "</a>");
}