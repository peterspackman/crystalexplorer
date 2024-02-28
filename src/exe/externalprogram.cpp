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
	qDebug() << "Task logic";
	promise.setProgressValueAndText(0, "Starting background process");
	QProcess process;

	qDebug() << "Make temporary directory";
	QTemporaryDir tempDir;
	if (!tempDir.isValid()) {
	    m_errorMessage = "Cannot create temporary directory";
	    promise.finish();
	    return;
	}

	qDebug() << "Set process environment";
	process.setProcessEnvironment(m_environment);
	qDebug() << "Set working in tempdir";
	process.setWorkingDirectory(tempDir.path());

	qDebug() << "Setting up connections";
	setupProcessConnectionsPrivate(process);

	qDebug() << "Copy requirements files";
	if(!copyRequirements(tempDir.path())) {
	    m_errorMessage = "Could not copy into temp";
	    promise.finish();
	    return;
	}
	qDebug() << "Call setting progress value";
	promise.setProgressValueAndText(10, "Starting background process");
	qDebug() << "Start process";
	process.start(exe, args);
	qDebug() << "Wating for start";
	process.waitForStarted();

	int timeTaken = 0;

	while (!process.waitForFinished(m_timeIncrement)) {
	    qDebug() << "Wait for finished";
	    timeTaken += m_timeIncrement;
	    if (promise.isCanceled()) {
		qDebug() << "Canceled promise";
		process.kill();
		promise.finish();
		return;
	    }
	    if (m_timeout > 0) {
		if(timeTaken > m_timeout) {
		    qDebug() << "Kill due to timeout";
		    m_errorMessage = "Process timeout";
		    process.kill();
		    promise.finish();
		    return;
		}
	    }
	}
	qDebug() << "Set progress value";
	promise.setProgressValue(99);
	// SUCCESS
	if (m_exitCode == 0) {
	    qDebug() << "copying results";
	    if(!copyResults(tempDir.path())) {
		m_errorMessage = "Could not copy out of temp";
	    }
	}
	promise.setProgressValue(100);
	qDebug() << "m_exitCode" << m_exitCode;
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
