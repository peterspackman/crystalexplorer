#include "element.h"
#include <QMap>

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

inline QString capitalizeString(const QString& str)
{
    QString result = str.toLower();
    if (!result.isEmpty())
    {
        result[0] = result[0].toUpper();
    }
    return result;
}

// BR --> Br, RU --> Ru etc
QString Element::capitalizedSymbol() const {
  return (_symbol.size() == 1) ? _symbol
                               : _symbol[0] + QString(_symbol[1]).toLower();
}

QString formulaSum(const std::vector<QString> &symbols, bool richText) {
    const QString numFormat = richText ? "<sub>%1</sub>" : "%1 ";
    QString result = "";

    QMap<QString, int> formula;
    for (const auto &sym: symbols) {
	QString elementSymbol = capitalizeString(sym);

	if (formula.contains(elementSymbol)) {
	    formula[elementSymbol] += 1;
	} else {
	    formula[elementSymbol] = 1;
	}
    }

    if (formula.contains("C")) {
	int n = formula["C"];
	formula.remove("C");
	if (n == 1) {
	    result += "C";
	} else {
	    result += QString("C" + numFormat).arg(n);
	}
    }

    if (formula.contains("H")) {
	int n = formula["H"];
	formula.remove("H");
	if (n == 1) {
	    result += "H";
	} else {
	    result += QString("H" + numFormat).arg(n);
	}
    }

    auto keys = formula.keys();
    std::sort(keys.begin(), keys.end());
    for(const QString &key: keys) {
	int n = formula[key];
	if (n == 1) {
	    result += key;
	} else {
	    result += QString(key + numFormat).arg(n);
	}
    }

    return result.trimmed();
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


