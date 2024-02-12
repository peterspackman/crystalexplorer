#include "renderselection.h"
#include <QRandomGenerator>
#include <QDebug>

namespace cx::graphics {

constexpr quint32 NO_MASK{0b11111111001111111111111111111111};

enum class IdentifierType : quint32 {
  Atom =
      0b00000000100000000000000000000000, // 1 at the second byte i.e. R channel
  Bond = 0b00000000010000000000000000000000,
  Mesh = 0b00000000000000000000000000000000
};

inline quint32
generate_random_id(SelectionType selectionType = SelectionType::Atom) {
    IdentifierType id_type = IdentifierType::Atom;
    switch(selectionType) {
	case SelectionType::Bond:
	    id_type = IdentifierType::Bond;
	    break;
	case SelectionType::Surface:
	    id_type = IdentifierType::Mesh;
	    break;
	default:
	    break;
    }

  quint32 id = (QRandomGenerator::global()->generate() | 0xFF000000) & NO_MASK;
  return id | static_cast<quint32>(id_type);
}

inline quint32 getBaseMeshIdentifier(quint32 id) {
  return id & 0xFFFF0000; // set right 16 bits to 0;
}
inline QString id_type_string(IdentifierType t) {
    switch(t) {
	case IdentifierType::Atom:
	    return "<Atom>";
	case IdentifierType::Bond:
	    return "<Bond>";
	case IdentifierType::Mesh:
	    return "<Mesh>";
    }
}
IdentifierType id_type_from_id(quint32 id) {
  // mesh must go first as it is 11 to atom's 01 and bond's 10
  if (id & static_cast<quint32>(IdentifierType::Bond))
    return IdentifierType::Bond;
  if (id & static_cast<quint32>(IdentifierType::Atom))
    return IdentifierType::Atom;
  return IdentifierType::Mesh;
}

inline QVector3D id_to_color(quint32 id) {
  QColor color(static_cast<QRgb>(id));
  return {color.redF(), color.greenF(), color.blueF()};
}

inline quint32 color_to_id(const QColor &color) {
  return static_cast<quint32>(color.rgba());
}

RenderSelection::RenderSelection(QObject *parent) : QObject(parent) {}


SelectionResult RenderSelection::getSelectionFromColor(const QColor &color) const {
  quint32 id = color_to_id(color);
  IdentifierType id_type = id_type_from_id(id);

  SelectionResult result;
  switch (id_type) {
  case IdentifierType::Atom: {
    const auto kv = m_sphereToAtomIndex.find(id);
    if (kv == m_sphereToAtomIndex.end())
      break;
    result.type = SelectionType::Atom;
    result.index = kv->second;
    qDebug() << "Atom" << result.index;
    return result;
  }
  case IdentifierType::Bond: {
    const auto kv = m_cylinderToBondIndex.find(id);
    if (kv == m_cylinderToBondIndex.end())
      break;
    result.type = SelectionType::Bond;
    result.index = kv->second;
    qDebug() << "Bond" << result.index;
    return result;
  }
  case IdentifierType::Mesh: {
    qDebug() << "Called getSelectionFromColor with id " << id << "type: " << id_type_string(id_type);
    quint32 baseIdx = getBaseMeshIdentifier(id);
    const auto kv = m_meshToSurfaceIndex.find(baseIdx);
    if (kv == m_meshToSurfaceIndex.end())
      break;
    result.type = SelectionType::Surface;
    result.index = kv->second;
    result.secondaryIndex = id - baseIdx;
    qDebug() << "Mesh" << result.secondaryIndex;
    return result;
  }
  default:
    break;
  }
  return result;
}


quint32 RenderSelection::add(SelectionType type, int index) {
    quint32 id = generate_random_id(type);
    switch(type) {
	case SelectionType::None:
	    break;
	case SelectionType::Atom:
	    m_sphereToAtomIndex.insert({id, index});
	    break;
	case SelectionType::Bond:
	    m_cylinderToBondIndex.insert({id, index});
	    break;
	case SelectionType::Surface:
	    qDebug() << "Add surface" << id << index;
	    m_meshToSurfaceIndex.insert({id, index});
	    break;
    }
    return id;
}


QVector3D RenderSelection::getColorFromId(quint32 id) const {
    return id_to_color(id);
}

void RenderSelection::clear() {
    m_sphereToAtomIndex.clear();
    m_cylinderToBondIndex.clear();
    m_meshToSurfaceIndex.clear();
}

void RenderSelection::clear(SelectionType type) {
    switch(type) {
	case SelectionType::Atom:
	    m_sphereToAtomIndex.clear();
	case SelectionType::Bond:
	    m_cylinderToBondIndex.clear();
	case SelectionType::Surface:
	    m_meshToSurfaceIndex.clear();
	default:
	    break;
    }
}

}
