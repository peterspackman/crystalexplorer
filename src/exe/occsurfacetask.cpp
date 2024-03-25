#include "occsurfacetask.h"
#include "filedependency.h"
#include "exefileutilities.h"


OccSurfaceTask::OccSurfaceTask(QObject * parent) : ExternalProgramTask(parent) {
    setExecutable(exe::findProgramInPath("occ"));
    qDebug() << "Executable" << executable();
}

void OccSurfaceTask::setSurfaceParameters(const isosurface::Parameters &params) {
    m_parameters = params;
}

void OccSurfaceTask::start() {
    emit progressText("Generated JSON input");

    QString name = baseName();
    QString input = inputFileName();
    QString env = environmentFileName();
    QString outputName = outputFileName();


    QStringList args{"isosurface", input};
    if(!env.isEmpty()) args << env;
    args << "-o" << outputName;
    args << QString("--kind=%1").arg(kind());
    args << QString("--separation=%1").arg(separation());
    args << QString("--isovalue=%1").arg(isovalue());
    if(properties().contains("background_density")) {
	args << QString("--background-density=%1").arg(properties().value("background_density").toFloat());
    }
    setArguments(args);
    setRequirements({FileDependency(input)});
    setOutputs({FileDependency(outputName, outputName)});
    auto environment = QProcessEnvironment::systemEnvironment();
    environment.insert("OCC_DATA_PATH", "/Users/285699f/git/occ/share");
    setEnvironment(environment);


    emit progressText("Starting OCC process");
    ExternalProgramTask::start();
    qDebug() << "Finish occ task start";
}

QString OccSurfaceTask::kind() const {
    return isosurface::kindToString(m_parameters.kind);
}


float OccSurfaceTask::separation() const {
    return properties().value("separation", 0.2).toFloat();
}

float OccSurfaceTask::isovalue() const {
    return properties().value("isovalue", 0.002).toFloat();
}

QString OccSurfaceTask::inputFileName() const {
    return properties().value("inputFile", "file.xyz").toString();
}

QString OccSurfaceTask::environmentFileName() const {
    return properties().value("environmentFile", "").toString();
}

QString OccSurfaceTask::outputFileName() const {
    return properties().value("outputFile", "surface.ply").toString();
}

QString OccSurfaceTask::wavefunctionSuffix() const {
    return wavefunctionSuffixDefault;
}
