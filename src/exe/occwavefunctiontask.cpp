#include "occwavefunctiontask.h"
#include <occ/core/element.h>
#include "filedependency.h"
#include "exefileutilities.h"
#include <QJsonDocument>
#include <fmt/core.h>

QString toJson(const wfn::Parameters &params) {
	QJsonObject root;
	QJsonObject topology;


	auto nums = params.structure->atomicNumbersForIndices(params.atoms);
	auto pos = params.structure->atomicPositionsForIndices(params.atoms);

	QJsonArray positions;
	QJsonArray symbols;
	

	// should be in angstroms
	for(int i = 0; i < nums.rows(); i++) {
	    symbols.push_back(QString::fromStdString(occ::core::Element(nums(i)).symbol()));
	    positions.push_back(pos(0, i));
	    positions.push_back(pos(1, i));
	    positions.push_back(pos(2, i));
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

void OccWavefunctionTask::setParameters(const wfn::Parameters &params) {
    m_parameters = params;
}

const wfn::Parameters& OccWavefunctionTask::getParameters() const {
    return m_parameters;
}

void OccWavefunctionTask::start() {
    QString json = toJson(m_parameters);
    emit progressText("Generated JSON input");

    QString name = baseName();
    QString inputName = name + inputSuffix();
    QString outputName = wavefunctionFilename();

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

QString OccWavefunctionTask::wavefunctionFilename() const {
    return baseName() + wavefunctionSuffix();
}
