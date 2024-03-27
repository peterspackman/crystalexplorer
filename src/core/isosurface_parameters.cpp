#include "isosurface_parameters.h"
#include <QDebug>

namespace isosurface {

QString kindToString(Kind kind) {
    switch(kind) {
	case Kind::Promolecule:
	    return "promolecule";
	case Kind::Hirshfeld:
	    return "hirshfeld";
	case Kind::Void:
	    return "void";
	case Kind::ESP:
	    return "esp";
	case Kind::ElectronDensity:
	    return "rho";
	case Kind::DeformationDensity:
	    return "def";
	default:
	    return "unknown";
    };
}

Kind stringToKind(const QString &s) {
    qDebug() << "stringToKind called with:" << s;
    if(s == "promolecule" || s == "Promolecule Density") return Kind::Promolecule;
    else if(s == "hirshfeld" || s == "Hirshfeld") return Kind::Hirshfeld;
    else if(s == "void" || s == "Void" || s == "Crystal Voids") return Kind::Void;
    else if(s == "esp") return Kind::ESP;
    else if(s == "rho") return Kind::ElectronDensity;
    else if(s == "def") return Kind::DeformationDensity;
    else return Kind::Unknown;
}

}
