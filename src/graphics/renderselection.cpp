#include "renderselection.h"
#include <QRandomGenerator>
#include <QDebug>
#include <sys/wait.h>

namespace cx::graphics {

enum class IdentifierType : quint32 {
  Atom = 0, // 00
  Bond = 1, // 10
  Mesh = 2, // 01
  None = 3  // 11
};


quint32 encodeId(IdentifierType type, quint32 idOrBaseId = 0, quint32 vertexId = 0) {
    quint32 typeId = static_cast<quint32>(type);
    if (type == IdentifierType::Mesh) {
        // Mesh: 2 bits for type, 5 bits for base ID, and 17 bits for vertex ID
        idOrBaseId &= (1 << 5) - 1; // Keep only 5 bits
        vertexId &= (1 << 17) - 1;  // Keep only 17 bits
        return (typeId << 22) | (idOrBaseId << 17) | vertexId;
    } else {
        // Atom/Bond: 2 bits for type, 22 bits for ID
        idOrBaseId &= (1 << 22) - 1; // Keep only 22 bits
        return (typeId << 22) | idOrBaseId;
    }
}

void decodeId(quint32 encodedId, IdentifierType& type, quint32& idOrBaseId, quint32& vertexId) {
    type = static_cast<IdentifierType>((encodedId >> 22) & 0x3);
    if (type == IdentifierType::Mesh) {
        idOrBaseId = (encodedId >> 17) & ((1 << 5) - 1);
        vertexId = encodedId & ((1 << 17) - 1);
    } else {
        idOrBaseId = encodedId & ((1 << 22) - 1);
        vertexId = 0; // Not applicable for Atom/Bond
    }
}

inline QString id_type_string(IdentifierType t) {
    switch(t) {
	case IdentifierType::Atom:
	    return "<Atom>";
	case IdentifierType::Bond:
	    return "<Bond>";
	case IdentifierType::Mesh:
	    return "<Mesh>";
	default:
	    return "<None>";
    }
}

QColor encodedIdToQColor(quint32 encodedId) {
    int r = (encodedId >> 16) & 0xFF;
    int g = (encodedId >> 8) & 0xFF;
    int b = encodedId & 0xFF;
    return QColor(r, g, b);
}

QColor encodeIdToQColor(IdentifierType type, uint32_t idOrBaseId = 0, uint32_t vertexId = 0) {
    uint32_t encodedId = encodeId(type, idOrBaseId, vertexId);
    return encodedIdToQColor(encodedId);
}

quint32 encodedIdFromQColor(QColor color) {
    return (color.red() << 16) | (color.green() << 8) | color.blue();
}

void decodeQColorToId(QColor color, IdentifierType& type, uint32_t& idOrBaseId, uint32_t& vertexId) {
    uint32_t encodedId = encodedIdFromQColor(color);
    decodeId(encodedId, type, idOrBaseId, vertexId);
}

RenderSelection::RenderSelection(QObject *parent) : QObject(parent) {}


SelectionResult RenderSelection::getSelectionFromColor(const QColor &color) const {
    IdentifierType id_type;
    quint32 baseId, vertexId;
    decodeQColorToId(color, id_type, baseId, vertexId);

    SelectionResult result;
    switch (id_type) {
    case IdentifierType::Atom: {
	
	const auto kv = m_sphereToAtomIndex.find(baseId);
	if (kv == m_sphereToAtomIndex.end())
	    break;
	result.type = SelectionType::Atom;
	result.index = kv->second;
	return result;
    }
    case IdentifierType::Bond: {
	const auto kv = m_cylinderToBondIndex.find(baseId);
	if (kv == m_cylinderToBondIndex.end())
	    break;
	result.type = SelectionType::Bond;
	result.index = kv->second;
	return result;
    }
    case IdentifierType::Mesh: {
	const auto kv = m_meshToSurfaceIndex.find(baseId);
	if (kv == m_meshToSurfaceIndex.end())
	    break;
	result.type = SelectionType::Surface;
	result.index = kv->second;
	result.secondaryIndex = vertexId;
	return result;
    }
    default:
	break;
    }
    return result;
}

quint32 RenderSelection::add(SelectionType type, int index) {

    auto get_or_insert_id = [&](IdentifierType id_type, IdentifierIndexMap &id2idx, IndexIdentifierMap &idx2id) {
	const auto loc = idx2id.find(index);
	if(loc == idx2id.end()) {
	    quint32 next_idx = static_cast<quint32>(idx2id.size());

	    quint32 id = encodeId(id_type, next_idx, 0);
	    id2idx.insert({next_idx, index});
	    idx2id.insert({index, next_idx});
	    return id;
	}
	else {
	    return loc->second;
	}
    };

    switch(type) {
	case SelectionType::None:
	    break;
	case SelectionType::Atom: {
	    return get_or_insert_id(IdentifierType::Atom, m_sphereToAtomIndex, m_atomToSphereIndex);
	    break;
	}
	case SelectionType::Bond: {
	    return get_or_insert_id(IdentifierType::Bond, m_cylinderToBondIndex, m_bondToCylinderIndex);
	    break;
	}
	case SelectionType::Surface: {
	    return get_or_insert_id(IdentifierType::Mesh, m_meshToSurfaceIndex, m_surfaceToMeshIndex);
	    break;
	}
    }
    return 0;
}


QVector3D RenderSelection::getColorFromId(quint32 id) const {
    QColor color = encodedIdToQColor(id);
    return QVector3D(color.redF(), color.greenF(), color.blueF());
}

void RenderSelection::clear() {
    m_sphereToAtomIndex.clear();
    m_atomToSphereIndex.clear();
    m_cylinderToBondIndex.clear();
    m_bondToCylinderIndex.clear();
    m_meshToSurfaceIndex.clear();
    m_surfaceToMeshIndex.clear();
}

void RenderSelection::clear(SelectionType type) {
    switch(type) {
	case SelectionType::Atom: {
	    m_sphereToAtomIndex.clear();
	    m_atomToSphereIndex.clear();
	    break;
	}
	case SelectionType::Bond: {
	    m_cylinderToBondIndex.clear();
	    m_bondToCylinderIndex.clear();
	    break;
	}
	case SelectionType::Surface: {
	    m_meshToSurfaceIndex.clear();
	    m_surfaceToMeshIndex.clear();
	    break;
	}
	default:
	    break;
    }
}

}
