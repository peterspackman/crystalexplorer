#pragma once
#include "task.h"
#include "filedependency.h"
#include "chemicalstructure.h"
#include <QString>
#include <occ/core/molecule.h>
#include <QtConcurrent>
#include <ankerl/unordered_dense.h>
#include <QProcessEnvironment>
#include <QPromise>

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

    void setDeleteWorkingFiles(bool shouldDelete) { m_deleteWorkingFiles = shouldDelete; }
    inline bool deleteWorkingFiles() const { return m_deleteWorkingFiles; }

    inline const auto exitCode() const { return m_exitCode; }

    virtual void start() override;
    virtual void stop() override;

    virtual QString baseName() const;

    void setOverwrite(bool overwrite=true);
    bool overwrite() const;

    static QString getInputFilePropertyName(QString filename);
    static QString getOutputFilePropertyName(QString filename);

protected:
    virtual void preProcess();
    virtual void postProcess();
    bool runExternalProgram(QPromise<void>& promise);

signals:
    void stopProcess();
    void stdoutChanged();

private:
    void updateStdoutStderr(QProcess&);
    bool copyRequirements(const QString &path);
    bool copyResults(const QString &path);
    bool deleteRequirements();
    void setupProcessConnectionsPrivate(QProcess&);

    int m_exitCode{-1};
    int m_timeout{0};
    int m_timeIncrement{100};
    bool m_deleteWorkingFiles{false};
    QProcessEnvironment m_environment;

    FileDependencyList m_requirements;
    FileDependencyList m_outputs;

    QString m_executable;
    QStringList m_arguments;
};

