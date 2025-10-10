#pragma once
#include "task.h"
#include "filedependency.h"
#include "chemicalstructure.h"
#include <QString>
#include <occ/core/molecule.h>
#include <ankerl/unordered_dense.h>

// QProcess and related functionality not available in WASM
#ifndef Q_OS_WASM
#include <QProcess>
#include <QTemporaryDir>
#include <QProcessEnvironment>
#endif

#ifdef CX_HAS_CONCURRENT
#include <QtConcurrent>
#endif

class ExternalProgramTask : public Task {
    Q_OBJECT
public:

    explicit ExternalProgramTask(QObject *parent = nullptr);
    ~ExternalProgramTask();

#ifndef Q_OS_WASM
    void setExecutable(const QString &exe);
    void setArguments(const QStringList& args);
    void setTimeout(int);
    void setEnvironment(const QProcessEnvironment&);
    void setRequirements(const FileDependencyList&);
    void setOutputs(const FileDependencyList&);
    void setDeleteWorkingFiles(bool shouldDelete) { m_deleteWorkingFiles = shouldDelete; }

    inline const auto &executable() const { return m_executable; }
    inline const auto &arguments() const { return m_arguments; }
    inline const auto timeout() const { return m_timeout; }
    inline const auto &environment() const { return m_environment; }
    inline const auto &requirements() const { return m_requirements; }
    inline const auto &outputs() const { return m_outputs; }
    inline bool deleteWorkingFiles() const { return m_deleteWorkingFiles; }
    inline const auto exitCode() const { return m_exitCode; }
#else
    // WASM stubs - no-op implementations (external processes not supported)
    void setExecutable(const QString &) {}
    void setArguments(const QStringList&) {}
    void setTimeout(int) {}
    void setRequirements(const FileDependencyList&) {}
    void setOutputs(const FileDependencyList&) {}
    void setDeleteWorkingFiles(bool) {}

    QString executable() const { return QString(); }
    QStringList arguments() const { return QStringList(); }
    int timeout() const { return 0; }
    FileDependencyList requirements() const { return FileDependencyList(); }
    FileDependencyList outputs() const { return FileDependencyList(); }
    bool deleteWorkingFiles() const { return false; }
    int exitCode() const { return -1; }

    // Can't use QProcessEnvironment in WASM, so just skip it
    template<typename T> void setEnvironment(const T&) {}
#endif

    virtual void start() override;
    virtual void stop() override;

    virtual QString baseName() const;
    QString hashedBaseName() const;

    void setOverwrite(bool overwrite=true);
    bool overwrite() const;

    static QString getInputFilePropertyName(QString filename);
    static QString getOutputFilePropertyName(QString filename);

protected:
    virtual void preProcess();
    virtual void postProcess();
    bool runExternalProgram(std::function<void(int, QString)> progress);

signals:
    void stopProcess();
    void stdoutChanged();

private:
#ifndef Q_OS_WASM
    QTemporaryDir *m_tempDir{nullptr};
    void updateStdoutStderr(QProcess&);
    bool copyRequirements(const QString &path);
    bool copyResults(const QString &path);
    bool deleteRequirements();
    void setupProcessConnectionsPrivate(QProcess&);
    void cleanupResources();

    int m_exitCode{-1};
    int m_timeout{0};
    int m_timeIncrement{100};
    bool m_deleteWorkingFiles{false};
    QProcessEnvironment m_environment;

    FileDependencyList m_requirements;
    FileDependencyList m_outputs;

    QString m_executable;
    QStringList m_arguments;
#else
    // WASM stub - external process execution not supported in browsers
    // Future: could be implemented via Web Workers or WASM bindings
#endif
};

