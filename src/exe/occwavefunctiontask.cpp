#include "occwavefunctiontask.h"
#include "filedependency.h"
#include "exefileutilities.h"
#include <QJsonDocument>
#include <fmt/core.h>

QString toJson(const wfn::Parameters &params) {
	QJsonObject root;
	QJsonObject topology;
	QJsonArray positions;

	for(const auto &pos: params.atoms.positions) {
	    positions.push_back(pos.x());
	    positions.push_back(pos.y());
	    positions.push_back(pos.z());
	}
	QJsonArray symbols;
	for(const auto &sym: params.atoms.symbols) {
	    symbols.push_back(sym);
	}
	root["schema_name"] = "qcschema_input";
	root["scema_version"] = 1;
	root["return_output"] = true;

	topology["geometry"] = positions;
	topology["symbols"] = symbols;
	root["molecule"] = topology;
	root["driver"] = "energy";

	QJsonObject model;
	model["method"] = params.method;
	model["basis"] = params.basis;
	root["model"] = model;

	return QJsonDocument(root).toJson();
}

OccWavefunctionTask::OccWavefunctionTask(QObject * parent) : ExternalProgramTask(parent) {
    setExecutable(exe::findProgramInPath("occ"));
    qDebug() << "Executable" << executable();
}

void OccWavefunctionTask::setWavefunctionParameters(const wfn::Parameters &params) {
    m_parameters = params;
}

void OccWavefunctionTask::start() {
    QString json = toJson(m_parameters);
    emit progressText("Generated JSON input");

    QString name = baseName();
    QString inputName = name + inputSuffix();
    QString outputName = name + wavefunctionSuffix();

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


QString OccWavefunctionTask::inputSuffix() const {
    return inputSuffixDefault;
}

QString OccWavefunctionTask::wavefunctionSuffix() const {
    return wavefunctionSuffixDefault;
}
