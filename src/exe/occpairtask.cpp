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

void OccPairTask::appendTransformArguments(QStringList &args) {
    const auto ta = m_parameters.transformA.matrix();

    for(int i = 0; i < 3; i++) {
	args << QString("--translation-a=%1").arg(ta(3, i));
    }

    for(int i = 0; i < 3; i++) {
	for(int j = 0; j < 3; j++) {
	    args << QString("--rotation-a=%1").arg(ta(i, j));
	}
    }

    const auto tb = m_parameters.transformB.matrix();

    for(int i = 0; i < 3; i++) {
	args << QString("--translation-b=%1").arg(tb(3, i));
    }

    for(int i = 0; i < 3; i++) {
	for(int j = 0; j < 3; j++) {
	    args << QString("--rotation-b=%1").arg(tb(i, j));
	}
    }
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
    args << QString("--model=%1").arg(m_parameters.model);


    appendTransformArguments(args);

    qDebug() << "Arguments:" << args;
    setArguments(args);
    setRequirements(reqs);
    setOutputs({FileDependency(outputName, outputName)});
    auto environment = QProcessEnvironment::systemEnvironment();
    environment.insert("OCC_DATA_PATH", QDir::homePath() + "/git/occ/share");
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
