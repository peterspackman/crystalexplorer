#pragma once
#include "element.h"
#include <QJsonObject>
#include <QString>

class ElementData {
public:
  static bool getData(const QString &, bool useJmolColors = false);
  static bool resetAll(bool useJmolColors = false);
  static Element *elementFromSymbol(const QString &);
  static Element *elementFromAtomicNumber(int);
  static QStringList elementSymbols();
  static void writeToStream(QDataStream &);
  static void readFromStream(QDataStream &);
  static bool resetElement(const QString &);
  static int atomicNumberFromElementSymbol(const QString &);

private:
  static void clear();
  static void readData(QString, bool useJmolColors = false);
  static void readJmolColors();
  static Element *elementFromJson(const QJsonObject &);
  static QVector<Element *> g_elementData;
  static QVector<QColor> m_jmolColors;
  static QJsonObject m_elementJson;
  static QJsonObject m_jmolColorJson;
};
