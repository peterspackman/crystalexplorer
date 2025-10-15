#include <QColor>
#include <QDataStream>
#include <QFile>
#include <QList>
#include <QListIterator>
#include <QString>
#include <QtDebug>

#include "element.h"
#include "elementdata.h"

QVector<Element *> ElementData::g_elementData = {};
nlohmann::json ElementData::m_elementJson = {};
QVector<QColor> ElementData::m_jmolColors = {};
nlohmann::json ElementData::m_jmolColorJson = {};

bool ElementData::getData(const QString &filename, bool useJmolColors) {
  Q_ASSERT(g_elementData.size() == 0);
  readData(filename, useJmolColors);
  return (g_elementData.size() > 0);
}

void ElementData::readData(QString filename, bool useJmolColors) {
  m_elementJson = {};
  readJmolColors();
  QFile file(filename);
  if (!file.open(QIODevice::ReadOnly)) {
    qWarning() << "Couldn't open element data file:" << filename;
    return;
  }
  QByteArray data = file.readAll();

  nlohmann::json doc;
  try {
    m_elementJson = nlohmann::json::parse(data.constData());
  } catch (nlohmann::json::parse_error &e) {
    qWarning() << "JSON parse error:" << e.what();
    return;
  }

  const auto elements = m_elementJson.at("elements");
  int elementIdx = 0;
  for (const auto &obj : elements) {
      g_elementData.append(elementFromJson(obj));
      if (useJmolColors)
        g_elementData[elementIdx]->setColor(m_jmolColors[elementIdx]);
      elementIdx++;
  }
}

void ElementData::readJmolColors() {
  m_jmolColorJson = {};
  m_jmolColors.clear();

  QFile file(":/resources/jmol_colours.json");
  if (!file.open(QIODevice::ReadOnly)) {
    qWarning() << "Couldn't open jmol colour data file";
    return;
  }
  QByteArray data = file.readAll();
  try {
    m_jmolColorJson = nlohmann::json::parse(data.constData());
  } catch (nlohmann::json::parse_error &e) {
    qWarning() << "JSON parse error:" << e.what();
    return;
  }
  for (const auto &obj : m_jmolColorJson.at("jmol_colours")) {
    std::array<int, 3> rgb;
    obj.at("rgb").get_to(rgb);
    m_jmolColors.push_back(QColor(rgb[0], rgb[1], rgb[2]));
  }
}

Element *ElementData::elementFromJson(const nlohmann::json &j) {

  std::array<int, 3> rgb;
  j.at("rgb").get_to(rgb);
  QColor color = QColor(rgb[0], rgb[1], rgb[2]);
  return new Element(
      j.at("name").get<QString>(), j.at("symbol").get<QString>(),
      j.at("number").get<int>(), j.at("covalent_radius").get<double>(),
      j.at("vdw_radius").get<double>(), j.at("mass").get<double>(), color);
}

bool ElementData::resetAll(bool useJmolColors) {

  int elementIndex = 0;
  for (const auto &obj : m_elementJson.at("elements")) {
    Q_ASSERT(elementIndex < g_elementData.size());

    Element *element = elementFromJson(obj);
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
  // Special case for deuterium (D) - return hydrogen with mass 2
  if (symbol.toUpper() == "D") {
    static Element *deuterium = nullptr;
    if (!deuterium) {
      Element *hydrogen = elementFromSymbol("H");
      if (hydrogen) {
        deuterium = new Element(hydrogen->name(), "D", hydrogen->number(),
                                hydrogen->covRadius(), hydrogen->vdwRadius(),
                                2.014f, hydrogen->color());
      }
    }
    return deuterium;
  }

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

  for (const auto &obj : m_elementJson.at("elements")) {
    Q_ASSERT(elementIndex < g_elementData.size());

    Element *element = elementFromJson(obj);
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
  for (const auto *element : g_elementData) {
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

int ElementData::atomicNumberFromElementSymbol(const QString &symbol) {
  Element* element = elementFromSymbol(symbol);
  if (element) {
    return element->number();
  }
  return 0;
}
