#include "occpairtask.h"
#include "filedependency.h"
#include "exefileutilities.h"


OccPairTask::OccPairTask(QObject * parent) : ExternalProgramTask(parent) {
    setExecutable(exe::findProgramInPath("occ"));
    qDebug() << "Executable" << executable();
}

void OccPairTask::setParameters(const pair_energy::Parameters &params) {
    m_parameters = params;
}

void OccPairTask::start() {
    if(!(m_parameters.wfnA && m_parameters.wfnB)) {
	qWarning() << "Invalid wavefunctions specified";
	return;
    }

    QString name = baseName();
    QString outputName = outputFileName();

    QString nameA = QString("%1_A%2").arg(name).arg(m_parameters.wfnA->fileSuffix());
    QString nameB = QString("%1_B%2").arg(name).arg(m_parameters.wfnB->fileSuffix());

    emit progressText("Writing wavefunction files to disk");

    m_parameters.wfnA->writeToFile(nameA);
    m_parameters.wfnB->writeToFile(nameB);


    QStringList args{"pair", "-a", nameA, "-b", nameB};

    QList<FileDependency> reqs{
	FileDependency(nameA),
	FileDependency(nameB)
    };

    args << QString("--threads=%1").arg(threads());
    qDebug() << "Arguments:" << args;
    setArguments(args);
    setRequirements(reqs);
    setOutputs({FileDependency(outputName, outputName)});
    auto environment = QProcessEnvironment::systemEnvironment();
    environment.insert("OCC_DATA_PATH", "/Users/285699f/git/occ/share");
    setEnvironment(environment);


    emit progressText("Starting OCC process");
    ExternalProgramTask::start();
    qDebug() << "Finish occ task start";
}


int OccPairTask::threads() const {
    return properties().value("threads", 6).toInt();
}

QString OccPairTask::outputFileName() const {
    return properties().value("outputFile", "surface.ply").toString();
}
