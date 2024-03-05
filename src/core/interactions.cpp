#include "interactions.h"

DimerInteractions::DimerInteractions(QObject *parent) : QObject(parent) {}

void DimerInteractions::clear() {
    m_interactions.clear();
}

void DimerInteractions::clearValue(DimerPair pair, const QString &label) {
    auto kv = m_interactions.find(pair);
    if(kv != m_interactions.end()) {
	kv->second.erase(label);
    }
}

void DimerInteractions::clearValues(DimerPair pair) {
    m_interactions.erase(pair);
}

void DimerInteractions::setValue(DimerPair pair, double value, const QString &label) {
    m_interactions[pair][label] = value;
}

void DimerInteractions::setValues(DimerPair pair, Components &&components) {
    auto kv = m_interactions.find(pair);
    if(kv == m_interactions.end()) {
	m_interactions.insert({pair, components});
    }
    else {
	auto &dest = kv->second;
	for(const auto &pair: components) {
	    dest[pair.first] = pair.second;
	}
    }
}
    
double DimerInteractions::valueForDimer(DimerPair pair, const QString &label) const {
    auto kv = m_interactions.find(pair);
    if(kv == m_interactions.end()) {
	return DefaultValue;
    }
    else {
	const auto &components = kv->second;
	auto kv2 = components.find(label);
	if(kv2 != components.end()) return kv2->second;
	else return DefaultValue;
    }
}

bool DimerInteractions::haveValuesForDimer(DimerPair pair) const {
    auto kv = m_interactions.find(pair);
    if(kv == m_interactions.end()) return false;
    return kv->second.size() > 0;
}

const DimerInteractions::Components& DimerInteractions::valuesForDimer(DimerPair pair) const {
    auto kv = m_interactions.find(pair);
    if(kv == m_interactions.end()) {
	throw std::runtime_error("Called valuesForDimer when there are no dimers");;
    }
    return kv->second;
}
