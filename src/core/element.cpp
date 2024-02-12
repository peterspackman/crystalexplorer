#include "element.h"

Element::Element(const QString &name, const QString &symbol, int number,
                 float covRadius, float vdwRadius, float mass,
                 const QColor &color)
    : _name(name), _symbol(symbol), _number(number), _covRadius(covRadius),
      _vdwRadius(vdwRadius), _mass(mass), _color(color) {}

void Element::update(const QString &name, const QString &symbol, int number,
                     float covRadius, float vdwRadius, float mass,
                     const QColor &color) {
  _number = number;
  _name = name;
  _symbol = symbol;
  _covRadius = covRadius;
  _vdwRadius = vdwRadius;
  _mass = mass;
  _color = color;
}

// BR --> Br, RU --> Ru etc
QString Element::capitalizedSymbol() const {
  return (_symbol.size() == 1) ? _symbol
                               : _symbol[0] + QString(_symbol[1]).toLower();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Stream Functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

QDataStream &operator<<(QDataStream &ds, const Element &element) {
  ds << element._name;
  ds << element._symbol;
  ds << element._covRadius;
  ds << element._vdwRadius;
  ds << element._mass;
  ds << element._color;

  return ds;
}

QDataStream &operator>>(QDataStream &ds, Element &element) {
  ds >> element._name;
  ds >> element._symbol;
  ds >> element._covRadius;
  ds >> element._vdwRadius;
  ds >> element._mass;
  ds >> element._color;

  return ds;
}
