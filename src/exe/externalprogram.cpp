#include "externalprogram.h"
#include <QProcess>
#include <QFileInfo>
#include <QDebug>


namespace exe {

bool copyFile(const QString &sourcePath, const QString &targetPath, bool overwrite) {
    // if they're the same file just do nothing.
    if(sourcePath == targetPath) return true;

    QFileInfo targetFileInfo(targetPath);

    // check if it exists
    if (targetFileInfo.exists()) {
        if (!overwrite) return false;
        if (!QFile::remove(targetPath)) return false;
    }

    // just use qfile::copy
    return QFile::copy(sourcePath, targetPath);
}

QString findProgramInPath(const QString &program) {
    QFileInfo file(program);
    if(file.isAbsolute() && file.isExecutable()) {
	return program;
    }
    QStringList paths = QProcessEnvironment::systemEnvironment().value("PATH").split(QDir::listSeparator());
    for(const auto &path: paths) {
	QString candidate = QDir(path).absoluteFilePath(program);
	QFileInfo candidateFile(candidate);
	if (candidateFile.isExecutable() && !candidateFile.isDir()) {
	    return candidateFile.absoluteFilePath();
	}
    }
    return QString();
}


ExternalProgram::ExternalProgram(const QString &location) : m_executableLocation(location) {
    m_resolvedExecutableLocation = findProgramInPath(location);
}


bool ExternalProgram::haveValidExecutableLocation() {
    return !m_resolvedExecutableLocation.isEmpty();
}

ExternalProgramResult runExternalProgramBlocking(const ExternalProgramParameters &params) {
    ExternalProgramResult result;
    result.exitCode = -1;
    QProcess process;
    process.setProgram(params.executable);
    process.setArguments(params.arguments);

    QTemporaryDir tempDir;
    if (!tempDir.isValid()) {
	result.errorMessage = "Cannot create temporary directory";
	return result;
    }

    qDebug() << "Running in: " << tempDir.path();
    process.setWorkingDirectory(tempDir.path());
    process.setProcessEnvironment(params.environment);

    for(const auto& input: params.requirements) {
	QString dest = tempDir.path() + QDir::separator() + QFileInfo(input).fileName();
	if (!QFile::copy(input, dest)) {
	    result.errorMessage = "Failed to copy input file to temporary directory.";
	    return result;
	}
    }

    process.start();
    bool finished = process.waitForFinished(-1); // Wait indefinitely until finished

    if (!finished) {
        result.errorMessage = "Process failed to finish properly";
	result.exitCode = -1;
	return result;
    }

    if (process.exitStatus() == QProcess::CrashExit) {
        result.errorMessage = "Process crashed";
	result.exitCode = -1;
        return result;
    }

    result.exitCode = process.exitCode();
    if (result.success()) {
	for(const auto& output: params.outputs) {
	    QString tmpOutput = tempDir.path() + QDir::separator() + QFileInfo(output).fileName();
	    if (!QFile::copy(tmpOutput, output)) {
		result.errorMessage = QString("Failed to copy output file from temporary directory. %1 %2").arg(tmpOutput).arg(output);
		return result;
	    }
	}
    }
    else {
	result.errorMessage = "Nonzero exit code";
    }
    result.stderrContents = process.readAllStandardError();
    result.stdoutContents = process.readAllStandardOutput();
    return result;
}


}


ExternalProgramTask::ExternalProgramTask(QObject *parent) : Task(parent), m_environment(QProcessEnvironment::systemEnvironment()) {}


void ExternalProgramTask::setExecutable(const QString &exe) {
    m_executable = exe;
}

void ExternalProgramTask::setArguments(const QStringList& args) {
    m_arguments = args;
}

void ExternalProgramTask::setupProcessConnectionsPrivate(QProcess &process) {
    QObject::connect(&process, &QProcess::finished, [this, &process](int exitCode, QProcess::ExitStatus exitStatus) {
	if (exitStatus == QProcess::CrashExit) {
	    setErrorMessage("Process crashed");
	} else {
	    m_exitCode = exitCode;
	    m_stdout = process.readAllStandardOutput();
	    m_stderr = process.readAllStandardError();
	    
	}
    });

    QObject::connect(&process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
	// Populate errorMessage based on the type of error
	switch (error) {
	    case QProcess::FailedToStart:
		setErrorMessage("Process failed to start");
		break;
	    case QProcess::Crashed:
		setErrorMessage("Process crashed during execution");
		break;
	    case QProcess::Timedout:
		// timing out is not an error due to the way we're running
		// m_errorMessage = "Process timeout";
		break;
	    case QProcess::ReadError:
		setErrorMessage("Process read error");
		break;
	    case QProcess::WriteError:
		setErrorMessage("Process write error");
		break;
	    case QProcess::UnknownError:
	    default:
		setErrorMessage("Unknown process error");
		break;
	}
    });

    QObject::connect(this, &ExternalProgramTask::stopProcess, &process, &QProcess::terminate);

}

bool ExternalProgramTask::copyRequirements(const QString &path) {
    bool force = overwrite();
    for(const auto& [input, input_dest]: m_requirements) {
	QString dest = path + QDir::separator() + QFileInfo(input_dest).fileName();
	if (!exe::copyFile(input, dest, force)) {
	    setErrorMessage(QString("Failed to copy input file to temporary directory: %1 -> %2").arg(input).arg(dest));
	    qDebug() << errorMessage();
	    return false;
	}
    }
    return true;
}

bool ExternalProgramTask::copyResults(const QString &path) {
    bool force = overwrite();
    for(const auto& [output, output_dest]: m_outputs) {
	QString tmpOutput = path + QDir::separator() + QFileInfo(output).fileName();
	if (!exe::copyFile(tmpOutput, output_dest, force)) {
	    setErrorMessage(QString("Failed to copy output file from temporary directory. %1 -> %2").arg(output).arg(output_dest));
	    qDebug() << errorMessage();
	    return false;
	}
    }
    return true;
}



void ExternalProgramTask::start() {
    QString exe = m_executable;
    QStringList args = m_arguments;

    auto taskLogic = [this, exe, args](QPromise<void>& promise) {
	QProcess process;
	qDebug() << "In task logic";

	QTemporaryDir tempDir;
	if (!tempDir.isValid()) {
	    setErrorMessage("Cannot create temporary directory");
	    promise.finish();
	    return;
	}
	promise.setProgressValueAndText(1, "Temporary directory created");

	process.setProcessEnvironment(m_environment);
	process.setWorkingDirectory(tempDir.path());
	promise.setProgressValueAndText(2, "Process environment set");

	setupProcessConnectionsPrivate(process);

	if(!copyRequirements(tempDir.path())) {
	    setErrorMessage("Could not copy necessary files into temporary directory");
	    promise.finish();
	    return;
	}
	promise.setProgressValueAndText(3, "Copied files to temporary directory");
	process.start(exe, args);
	promise.setProgressValueAndText(4, "Starting background process");
	process.waitForStarted();
	promise.setProgressValueAndText(5, "Background process started");

	int timeTaken = 0;

	while (!process.waitForFinished(m_timeIncrement)) {
	    timeTaken += m_timeIncrement;
	    promise.setProgressValueAndText(timeTaken/m_timeIncrement, "test");
	    if (promise.isCanceled()) {
		setErrorMessage("Promise was canceled");
		process.kill();
		promise.setProgressValueAndText(100, "Promise canceled");
		promise.finish();
		return;
	    }
	    if (m_timeout > 0) {
		if(timeTaken > m_timeout) {
		    setErrorMessage("Process timeout");
		    promise.setProgressValueAndText(100, "Background process canceled due to timeout");
		    process.kill();
		    promise.finish();
		    return;
		}
	    }
	}
	promise.setProgressValueAndText(99, "Background process complete");
	// SUCCESS
	if (m_exitCode == 0) {
	    if(!copyResults(tempDir.path())) {
		setErrorMessage("Could not results out of temporary directory");
	    }
	}
	else {
	    setErrorMessage(QString("Failed with exit code: %1").arg(m_exitCode));
	    qDebug() << m_stdout;
	    qDebug() << m_stderr;
	}
	promise.setProgressValueAndText(100, "Task complete");
	promise.finish();
	qDebug() << "Finished" << errorMessage();
    };

    Task::run(taskLogic);
}

void ExternalProgramTask::stop() {
    emit stopProcess();
}

void ExternalProgramTask::setEnvironment(const QProcessEnvironment &env) {
    m_environment = env;
}

void ExternalProgramTask::setRequirements(const FileDependencyList &requirements) {
    m_requirements = requirements;
}

void ExternalProgramTask::setOutputs(const FileDependencyList &outputs) {
    m_outputs = outputs;
}


QString ExternalProgramTask::baseName() const {
    const auto &props = properties();
    const auto loc = props.find("basename");
    if(loc != props.end()) {
	return loc->toString();
    }
    return "external_calculation";
}


void ExternalProgramTask::setOverwrite(bool overwrite) {
    setProperty("overwrite", true);
}

bool ExternalProgramTask::overwrite() const {
    return properties().value("overwrite", true).toBool();
}
