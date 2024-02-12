#pragma once
#include <QString>

class DialogHtml {
public:
  static QString bold(QString text);
  static QString italic(QString text);
  static QString linebreak();
  static QString line();
  static QString font(QString text, QString fontSize = "3",
                      QString color = "black");
  static QString paragraph(QString text);
  static QString website(QString webaddress, QString link);
  static QString email(QString email, QString link, QString subject,
                       QString body);
};
