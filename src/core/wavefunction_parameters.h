#pragma once
#include <QString>
#include <QList>
#include <QVector3D>
#include <ankerl/unordered_dense.h>

namespace wfn {
    struct AtomList {
	QList<QString> symbols;
	QList<QVector3D> positions;
    };

    struct Parameters {
	QString method{"b3lyp"};
	QString basis{"def2-qzvp"};
	AtomList atoms;
    };

    struct Result {
	QString filename;
	QString stdoutContents;
	ankerl::unordered_dense::map<QString, double> energy;
	bool success{false};
    };
}
