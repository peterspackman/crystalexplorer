#include "occelattask.h"
#include "settings.h"
#include <QFileInfo>

OccElatTask::OccElatTask(QObject *parent)
    : ExternalProgramTask(parent) {
    setExecutable(settings::readSetting(settings::keys::OCC_EXECUTABLE).toString());
}

void OccElatTask::setCrystalStructureFile(const QString &filename) {
    m_crystalFile = filename;
}

void OccElatTask::setEnergyModel(const QString &model) {
    m_energyModel = model;
}

void OccElatTask::setRadius(double radius) {
    m_radius = radius;
}

void OccElatTask::setThreads(int threads) {
    m_threads = threads;
}

QString OccElatTask::outputJsonFilename() const {
    if (m_crystalFile.isEmpty()) {
        return QString();
    }

    QFileInfo inputInfo(m_crystalFile);
    QString baseName = inputInfo.completeBaseName();

    // occ elat always writes to {basename}_elat_results.json (ignores --json argument)
    return baseName + "_elat_results.json";
}

void OccElatTask::start() {
    if (m_crystalFile.isEmpty()) {
        qWarning() << "OccElatTask: No crystal structure file specified";
        return;
    }

    QStringList args;
    args << "elat";
    args << m_crystalFile;
    args << QString("--model=%1").arg(m_energyModel);
    args << QString("--radius=%1").arg(m_radius);
    args << QString("--threads=%1").arg(m_threads);
    // Note: --json argument is ignored by occ elat, it always writes to {basename}_elat_results.json

    // Debug: print full command line
    QString executable = settings::readSetting(settings::keys::OCC_EXECUTABLE).toString();
    qDebug() << "Running command:" << executable << args.join(" ");

    setArguments(args);

    // Set up file dependencies
    FileDependencyList requirements;
    requirements.append(FileDependency(m_crystalFile));
    setRequirements(requirements);

    FileDependencyList outputs;
    QString outputFile = outputJsonFilename();
    outputs.append(FileDependency(outputFile, outputFile));
    setOutputs(outputs);

    ExternalProgramTask::start();
}
