#include "occ_external.h"
#include "filedependency.h"
#include <QJsonDocument>

QString toJson(const exe::wfn::Parameters &params) {
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

void OccWavefunctionTask::setWavefunctionParameters(const exe::wfn::Parameters &params) {
    m_parameters = params;
}

void OccWavefunctionTask::start() {
    qDebug() << "Start called";
    QString json = toJson(m_parameters);

    QFile file("input.json");
    file.open(QIODevice::ReadWrite|QIODevice::Text);
    file.write(json.toUtf8());
    file.close();

    setArguments({"scf", "input.json"});
    setRequirements({FileDependency("input.json")});
    setOutputs({FileDependency("input.owf.json", "test.owf.json")});
    auto environment = QProcessEnvironment::systemEnvironment();
    environment.insert("OCC_BASIS_PATH", "/Users/285699f/git/occ");
    setEnvironment(environment);

    ExternalProgramTask::start();
    qDebug() << "Finish occ task start";
}
