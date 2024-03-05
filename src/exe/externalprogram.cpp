#include "externalprogram.h"
#include <QProcess>
#include <QFileInfo>
#include <QDebug>


namespace exe {

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
	    m_errorMessage = "Process crashed";
	} else {
	    m_exitCode = exitCode;
	    m_stdout = process.readAllStandardOutput();
	    m_stderr = process.readAllStandardError();
	    
	}
    });

    QObject::connect(&process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
	// Populate m_errorMessage based on the type of error
	switch (error) {
	    case QProcess::FailedToStart:
		m_errorMessage = "Process failed to start";
		break;
	    case QProcess::Crashed:
		m_errorMessage = "Process crashed during execution";
		break;
	    case QProcess::Timedout:
		// timing out is not an error due to the way we're running
		// m_errorMessage = "Process timeout";
		break;
	    case QProcess::ReadError:
		m_errorMessage = "Process read error";
		break;
	    case QProcess::WriteError:
		m_errorMessage = "Process write error";
		break;
	    case QProcess::UnknownError:
	    default:
		m_errorMessage = "Unknown process error";
		break;
	}
    });

    QObject::connect(this, &ExternalProgramTask::stopProcess, &process, &QProcess::terminate);

}

bool ExternalProgramTask::copyRequirements(const QString &path) {
    for(const auto& [input, input_dest]: m_requirements) {
	QString dest = path + QDir::separator() + QFileInfo(input_dest).fileName();
	if (!QFile::copy(input, dest)) {
	    m_errorMessage = QString("Failed to copy input file to temporary directory: %1 -> %2").arg(input).arg(dest);
	    qDebug() << m_errorMessage;
	    return false;
	}
    }
    return true;
}

bool ExternalProgramTask::copyResults(const QString &path) {
    for(const auto& [output, output_dest]: m_outputs) {
	QString tmpOutput = path + QDir::separator() + QFileInfo(output).fileName();
	if (!QFile::copy(tmpOutput, output_dest)) {
	    m_errorMessage = QString("Failed to copy output file from temporary directory. %1 -> %2").arg(output).arg(output_dest);
	    qDebug() << m_errorMessage;
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

	QTemporaryDir tempDir;
	if (!tempDir.isValid()) {
	    m_errorMessage = "Cannot create temporary directory";
	    promise.finish();
	    return;
	}
	promise.setProgressValueAndText(1, "Temporary directory created");

	process.setProcessEnvironment(m_environment);
	process.setWorkingDirectory(tempDir.path());
	promise.setProgressValueAndText(2, "Process environment set");

	setupProcessConnectionsPrivate(process);

	if(!copyRequirements(tempDir.path())) {
	    m_errorMessage = "Could not copy into temp";
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
		m_errorMessage = "Promise was canceled";
		process.kill();
		promise.setProgressValueAndText(100, "Promise canceled");
		promise.finish();
		return;
	    }
	    if (m_timeout > 0) {
		if(timeTaken > m_timeout) {
		    m_errorMessage = "Process timeout";
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
		m_errorMessage = "Could not copy out of temp";
	    }
	}
	promise.setProgressValueAndText(100, "Task complete");
	promise.finish();
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
