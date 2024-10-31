#include "occsurfacetask.h"
#include "exefileutilities.h"
#include "filedependency.h"

OccSurfaceTask::OccSurfaceTask(QObject *parent) : ExternalProgramTask(parent) {
  setExecutable(exe::findProgramInPath("occ"));
  qDebug() << "Executable" << executable();
}

void OccSurfaceTask::setSurfaceParameters(
    const isosurface::Parameters &params) {
  m_parameters = params;
}

void OccSurfaceTask::appendWavefunctionTransformArguments(QStringList &args) {
  const auto t = m_parameters.wfn_transform.matrix();

  for (int i = 0; i < 3; i++) {
    args << QString("--wfn-translation=%1").arg(t(i, 3));
  }

  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      args << QString("--wfn-rotation=%1").arg(t(i, j));
    }
  }
}

void OccSurfaceTask::start() {
  emit progressText("Generated JSON input");
  auto name = baseName();
  auto input = inputFileName();
  auto env = environmentFileName();
  auto outputName = outputFileName();
  auto wfn = wavefunctionFilename();

  QStringList args{"isosurface", input};

  QList<FileDependency> reqs{FileDependency(input)};

  if (!env.isEmpty()) {
    args << env;
    reqs << env;
  }


  args << "-o" << outputName;
  args << QString("--kind=%1").arg(kind());
  args << QString("--separation=%1").arg(separation());
  args << QString("--isovalue=%1").arg(isovalue());
  args << QString("--threads=%1").arg(threads());
  if (properties().contains("background_density")) {
    args << QString("--background-density=%1")
                .arg(properties().value("background_density").toFloat());
  }

  if(!wfn.isEmpty()) {
    args << "-w" << wfn;
    reqs << wfn;
    appendWavefunctionTransformArguments(args);
  }

  for(const auto &prop: m_parameters.additionalProperties) {
    args << "--properties=" + prop;
  }

  qDebug() << "Arguments:" << args;
  setArguments(args);
  setRequirements(reqs);
  setOutputs({FileDependency(outputName, outputName)});

  emit progressText("Starting OCC process");
  ExternalProgramTask::start();
  qDebug() << "Finish occ task start";
}

QString OccSurfaceTask::kind() const {
  return isosurface::kindToString(m_parameters.kind);
}

float OccSurfaceTask::separation() const { return m_parameters.separation; }

int OccSurfaceTask::threads() const {
  return properties().value("threads", 6).toInt();
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

QString OccSurfaceTask::wavefunctionFilename() const {
  return properties().value("wavefunctionFile", "").toString();
}

QString OccSurfaceTask::wavefunctionSuffix() const {
  return wavefunctionSuffixDefault;
}
