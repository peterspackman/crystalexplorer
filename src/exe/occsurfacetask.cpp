#include "occsurfacetask.h"
#include "filedependency.h"
#include "exefileutilities.h"


QString toJson(const exe::surface::Parameters &params) {
	QJsonObject root;
	root["kind"] = "promolecule";
	return QJsonDocument(root).toJson();
}

OccSurfaceTask::OccSurfaceTask(QObject * parent) : ExternalProgramTask(parent) {
    setExecutable(exe::findProgramInPath("occ"));
    qDebug() << "Executable" << executable();
}

void OccSurfaceTask::setSurfaceParameters(const exe::surface::Parameters &params) {
    m_parameters = params;
}

void OccSurfaceTask::start() {
    QString json = toJson(m_parameters);
    emit progressText("Generated JSON input");

    QString name = baseName();
    QString inputName = name + inputSuffix();
    QString outputName = name + surfaceSuffix();

    if(!exe::writeTextFile(inputName, json)) {
	emit errorOccurred("Could not write input file");
	return;
    }
    emit progressText("Wrote input file");


    setArguments({"scf", inputName});
    setRequirements({FileDependency(inputName)});
    setOutputs({FileDependency(outputName, outputName)});
    auto environment = QProcessEnvironment::systemEnvironment();
    environment.insert("OCC_BASIS_PATH", "/Users/285699f/git/occ/share");
    setEnvironment(environment);


    emit progressText("Starting OCC process");
    ExternalProgramTask::start();
    qDebug() << "Finish occ task start";
}


QString OccSurfaceTask::inputSuffix() const {
    return inputSuffixDefault;
}

QString OccSurfaceTask::surfaceSuffix() const {
    return surfaceSuffixDefault;
}

QString OccSurfaceTask::wavefunctionSuffix() const {
    return wavefunctionSuffixDefault;
}
