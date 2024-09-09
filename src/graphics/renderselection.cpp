#include "renderselection.h"
#include <QDebug>
#include <QRandomGenerator>

namespace cx::graphics {

GraphicsIdentifier::GraphicsIdentifier(SelectionType k, uint32_t v, uint32_t s)
    : kind(static_cast<uint8_t>(k)), value(v), secondary(s) {}

QString GraphicsIdentifier::toString() const {
  switch (selectionType()) {
  case SelectionType::Atom:
    return QString("<Atom id=%1>").arg(value);
  case SelectionType::Bond:
    return QString("<Bond id=%1>").arg(value);
  case SelectionType::Surface:
    return QString("<Bond id=%1 secondary=%2>").arg(value).arg(secondary);
  case SelectionType::Aggregate:
    return "<Aggregate>";
  default:
    return QString("<Unknown: kind=%1 encoded=%2>").arg(kind).arg(encode());
  }
}

uint32_t GraphicsIdentifier::encode() const {
  if (selectionType() == SelectionType::Surface) {
    // For Surface: 3 bits type, 8 bits primary ID, 13 bits secondary ID
    uint32_t base = value & ((1 << 5) - 1);
    uint32_t offset = secondary & ((1 << 16) - 1);
    return (static_cast<uint32_t>(kind) << 21) | (base << 16) << offset;

  } else {
    // For other types: 3 bits type, 21 bits primary ID
    uint32_t base = value & ((1 << 21) - 1);
    return (static_cast<uint32_t>(kind) << 21) | (base);
  }
}

GraphicsIdentifier GraphicsIdentifier::decode(uint32_t encoded) {

  GraphicsIdentifier result;
  result.kind = (encoded >> 21) & 0x7;
  if (result.selectionType() == SelectionType::Surface) {
    result.value = (encoded >> 16) & ((1 << 5) - 1);
    result.secondary = encoded & ((1 << 16) - 1);
  } else {
    result.value = encoded & ((1 << 21) - 1);
    result.secondary = 0;
  }
  return result;
}

QColor GraphicsIdentifier::toColor() const {
  uint32_t encoded = encode();
  return QColor((encoded >> 16) & 0xFF, (encoded >> 8) & 0xFF, encoded & 0xFF);
}

// Convert RGB color back to encoded ID
GraphicsIdentifier GraphicsIdentifier::fromColor(const QColor &color) {
  uint32_t encoded = (static_cast<uint32_t>(color.red()) << 16) |
                     (static_cast<uint32_t>(color.green()) << 8) |
                     static_cast<uint32_t>(color.blue());
  return decode(encoded);
}

RenderSelection::RenderSelection(QObject *parent) : QObject(parent) {
  m_indexMaps.resize(8);
  m_identifierMaps.resize(8);
}

SelectionResult
RenderSelection::getSelectionFromColor(const QColor &color) const {
  auto id = GraphicsIdentifier::fromColor(color);

  SelectionResult result;

  const auto &map = m_indexMaps[id.kind];
  const auto kv = map.find(id.value);
  if (kv != map.end()) {
    result.type = id.selectionType();
    result.index = kv->second;
    result.secondaryIndex = id.secondary;
  }
  return result;
}

QVector3D RenderSelection::getColorFromId(quint32 identifier) const {
  QColor color = GraphicsIdentifier::decode(identifier).toColor();
  QVector3D result(color.redF(), color.greenF(), color.blueF());
  return result;
}

quint32 RenderSelection::add(SelectionType type, int index) {

  if (type != SelectionType::None) {
    auto &id2idx = m_indexMaps[static_cast<uint8_t>(type)];
    auto &idx2id = m_identifierMaps[static_cast<uint8_t>(type)];
    const auto loc = idx2id.find(index);
    if (loc == idx2id.end()) {
      quint32 next_idx = static_cast<quint32>(idx2id.size());
      auto id = GraphicsIdentifier(type, next_idx, 0);
      quint32 result = id.encode();
      id2idx.insert({next_idx, index});
      idx2id.insert({index, next_idx});
      return result;
    } else {
      return loc->second;
    }
  }
  return 0;
}

void RenderSelection::clear() {
  for (auto &map : m_indexMaps) {
    map.clear();
  }
  for (auto &map : m_identifierMaps) {
    map.clear();
  }
}

void RenderSelection::clear(SelectionType type) {
  m_indexMaps[static_cast<uint8_t>(type)].clear();
  m_identifierMaps[static_cast<uint8_t>(type)].clear();
}

} // namespace cx::graphics
