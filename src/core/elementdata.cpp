#include <QColor>
#include <QDataStream>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QList>
#include <QListIterator>
#include <QString>
#include <QtDebug>

#include "element.h"
#include "elementdata.h"

QVector<Element *> ElementData::g_elementData = {};
QJsonObject ElementData::m_elementJson = {};
QVector<QColor> ElementData::m_jmolColors = {};
QJsonObject ElementData::m_jmolColorJson = {};

bool ElementData::getData(const QString &filename, bool useJmolColors) {
  Q_ASSERT(g_elementData.size() == 0);

  readData(filename, useJmolColors);

  return (g_elementData.size() > 0);
}

void ElementData::readData(QString filename, bool useJmolColors) {
  m_elementJson = {};
  readJmolColors();
  QFile file(filename);
  if (file.open(QIODevice::ReadOnly)) {
    QTextStream ts(&file);
    QString contents = file.readAll();
    file.close();
    QJsonDocument doc = QJsonDocument::fromJson(contents.toUtf8());
    m_elementJson = doc.object();
    QJsonArray elements = m_elementJson["elements"].toArray();
    int elementIdx = 0;
    for (const auto &obj : elements) {
      g_elementData.append(elementFromJson(obj.toObject()));
      if (useJmolColors)
        g_elementData[elementIdx]->setColor(m_jmolColors[elementIdx]);
      elementIdx++;
    }
  }
}

void ElementData::readJmolColors() {
  m_jmolColorJson = {};
  m_jmolColors.clear();
  QFile file(":/resources/jmol_colours.json");
  if (file.open(QIODevice::ReadOnly)) {
    QTextStream ts(&file);
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    m_jmolColorJson = doc.object();
    for (const auto &obj : m_jmolColorJson["jmol_colours"].toArray()) {
      QJsonObject j = obj.toObject();
      QJsonArray rgb = j["rgb"].toArray();
      int r = rgb[0].toInt();
      int g = rgb[1].toInt();
      int b = rgb[2].toInt();
      m_jmolColors.push_back(QColor(r, g, b));
    }
  }
}

Element *ElementData::elementFromJson(const QJsonObject &j) {

  int r = j["rgb"][0].toInt();
  int g = j["rgb"][1].toInt();
  int b = j["rgb"][2].toInt();
  QColor color = QColor(r, g, b);
  return new Element(j["name"].toString(), j["symbol"].toString(),
                     j["number"].toInt(), j["covalent_radius"].toDouble(),
                     j["vdw_radius"].toDouble(), j["mass"].toDouble(), color);
}

bool ElementData::resetAll(bool useJmolColors) {

  int elementIndex = 0;
  QJsonArray elements = m_elementJson["elements"].toArray();
  for (const auto &obj : elements) {
    Q_ASSERT(elementIndex < g_elementData.size());

    Element *element = elementFromJson(obj.toObject());
    QColor color =
        useJmolColors ? m_jmolColors[elementIndex] : element->color();
    g_elementData[elementIndex]->update(
        element->name(), element->symbol(), element->number(),
        element->covRadius(), element->vdwRadius(), element->mass(), color);
    delete element;
    elementIndex++;
  }
  return (elementIndex == g_elementData.size()); // check we reset all elements
}

Element *ElementData::elementFromSymbol(const QString &symbol) {
  for (const auto &el : std::as_const(g_elementData)) {
    if (el->symbol().toUpper() == symbol.toUpper())
      return el;
  }
  return nullptr;
}

Element *ElementData::elementFromAtomicNumber(int atomicNumber) {
  int i = atomicNumber - 1;
  if ((i > -1) && (i < g_elementData.size())) {
    return g_elementData[i];
  }
  return nullptr;
}

QStringList ElementData::elementSymbols() {
  QStringList symbols;
  for (const auto &el : g_elementData) {
    symbols.push_back(el->symbol());
  }
  return symbols;
}

bool ElementData::resetElement(const QString &symbol) {
  int elementIndex = 0;
  bool found = false;

  for (const auto &obj : m_elementJson["elements"].toArray()) {
    Q_ASSERT(elementIndex < g_elementData.size());

    Element *element = elementFromJson(obj.toObject());
    if (element->symbol().toUpper() == symbol.toUpper()) {
      g_elementData[elementIndex]->update(
          element->name(), element->symbol(), element->number(),
          element->covRadius(), element->vdwRadius(), element->mass(),
          element->color());
      found = true;
      delete element;
      break;
    }
    delete element;
    elementIndex++;
  }
  return found;
}

void ElementData::writeToStream(QDataStream &datastream) {
  datastream << g_elementData.size();
  qDebug() << "Writing " << g_elementData.size() << "elements";
  for (const auto * element : g_elementData) {
    datastream << *element;
  }

  if (datastream.status() != QDataStream::Ok) {
      qDebug() << "Error writing to datastream";
  }
}

void ElementData::readFromStream(QDataStream &ds) {
  ElementData::clear();

  qsizetype numberOfElements{0};
  ds >> numberOfElements;
  qDebug() << "Found " << numberOfElements << "elements in stream";
  for (qsizetype i = 0; i < numberOfElements; ++i) {
    auto *element = new Element("dummy", "dummy", 0, 0.0, 0.0, 0.0, QColor());
    ds >> *element;
    g_elementData.append(element);
  }
}

void ElementData::clear() {
  for (auto &el : g_elementData) {
    delete el;
  }
  g_elementData.clear();
}
