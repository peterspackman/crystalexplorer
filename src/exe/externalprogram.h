#pragma once
#include "chemicalstructure.h"
#include <QString>
#include <occ/core/molecule.h>
#include <QtConcurrent>
#include <ankerl/unordered_dense.h>

namespace exe {

namespace wfn {
    struct Parameters {
	QString methodName{"HartreeFock"};
	QString basisName{"3-21G"};
	occ::core::Molecule molecule;
    };
    struct Result {
	QString filename;
	QString stdoutContents;
	ankerl::unordered_dense::map<QString, double> energy;
	bool success{false};
    };
}

namespace surface {
    enum class Kind {
	Hirshfeld,
	Promolecule
    };

    struct Parameters {
	Kind kind{Kind::Promolecule};
    };

    struct Result {
	QString filename;
	QString stdoutContents;
	bool success{false};
    };
}

namespace interaction {

    struct Parameters {
	QString model;
	QString wfnA;
	QString wfnB;
    };

    struct Result {
	QString filename;
	ankerl::unordered_dense::map<QString, double> components;
    };
}

struct ExternalProgramResult {
    int exitCode{1};
    QString errorMessage{};
    QString stdoutContents;
    QString stderrContents;
    inline bool success() const { return exitCode == 0; }
};


class ExternalProgram {
public:
    ExternalProgram(const QString &location);
    virtual QPromise<wfn::Result> wavefunction(const wfn::Parameters&) = 0;
    virtual QPromise<surface::Result> surface(const surface::Parameters&) = 0;
    virtual QPromise<interaction::Result> interaction(const interaction::Parameters&) = 0;
private:
    QString m_executableLocation;
};



ExternalProgramResult runExternalProgramBlocking(const QString& exePath, const QStringList& arguments, const QString& workingDirectory);

}
