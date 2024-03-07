#pragma once
#include "task.h"
#include "filedependency.h"
#include "chemicalstructure.h"
#include <QString>
#include <occ/core/molecule.h>
#include <QtConcurrent>
#include <ankerl/unordered_dense.h>
#include <QProcessEnvironment>

namespace exe {

#include <QFile>
#include <QFileInfo>

bool copyFile(const QString &sourcePath, const QString &targetPath, bool overwrite = false);

QString findProgramInPath(const QString &program);

struct AtomList {
    QList<QString> symbols;
    QList<QVector3D> positions;
};


namespace wfn {
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

struct ExternalProgramParameters {
    QString executable;
    QString workingDirectory{"."};
    QStringList arguments;
    QStringList requirements;
    QStringList outputs;
    QProcessEnvironment environment;
};

class ExternalProgram {
public:
    ExternalProgram(const QString &location);
    virtual QFuture<wfn::Result> wavefunction(const wfn::Parameters&) = 0;
    virtual QFuture<surface::Result> surface(const surface::Parameters&) = 0;
    virtual QFuture<interaction::Result> interaction(const interaction::Parameters&) = 0;
    bool haveValidExecutableLocation();
    inline const QString &resolvedExecutableLocation() const { return m_resolvedExecutableLocation; }

private:
    QString m_resolvedExecutableLocation;
    QString m_executableLocation;
};


ExternalProgramResult runExternalProgramBlocking(const ExternalProgramParameters &);
}



class ProcessCrashException : public QException
{
public:
    void raise() const override { throw *this; }
    ProcessCrashException *clone() const override { return new ProcessCrashException(*this); }
};


class ExternalProgramTask : public Task {
    Q_OBJECT
public:

    explicit ExternalProgramTask(QObject *parent);

    void setExecutable(const QString &exe);
    inline const auto &executable() const { return m_executable; }

    void setArguments(const QStringList& args);
    inline const auto &arguments() const { return m_arguments; }

    void setTimeout(int);
    inline const auto timeout() const { return m_timeout; }

    void setEnvironment(const QProcessEnvironment&);
    inline const auto &environment() const { return m_environment; }

    void setRequirements(const FileDependencyList&);
    inline const auto &requirements() const { return m_requirements; }

    void setOutputs(const FileDependencyList&);
    inline const auto &outputs() const { return m_outputs; }

    inline const auto exitCode() const { return m_exitCode; }

    virtual void start() override;
    virtual void stop() override;

    virtual QString baseName() const;

    void setOverwrite(bool overwrite=true);
    bool overwrite() const;

signals:
    void stopProcess();

private:
    void updateStdoutStderr(QProcess&);
    bool copyRequirements(const QString &path);
    bool copyResults(const QString &path);
    void setupProcessConnectionsPrivate(QProcess&);

    int m_exitCode{-1};
    int m_timeout{0};
    int m_timeIncrement{100};
    QProcessEnvironment m_environment;

    FileDependencyList m_requirements;
    FileDependencyList m_outputs;

    QString m_executable;
    QStringList m_arguments;
};

