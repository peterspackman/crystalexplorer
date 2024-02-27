#include "externalprogram.h"
#include <QProcess>


namespace exe {
ExternalProgram::ExternalProgram(const QString &location) : m_executableLocation(location) {}


ExternalProgramResult runExternalProgramBlocking(const QString& exePath, const QStringList& arguments, const QString& workingDirectory) {
    ExternalProgramResult result;
    QProcess process;
    process.setProgram(exePath);
    process.setArguments(arguments);
    process.setWorkingDirectory(workingDirectory);

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
    if (!result.success()) {
	result.errorMessage = "Nonzero exit code";
    }
    result.stderrContents = process.readAllStandardError();
    result.stdoutContents = process.readAllStandardOutput();
    return result;
}


}
