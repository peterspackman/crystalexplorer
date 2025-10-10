#include "occelastictensortask.h"
#include "settings.h"
#include <QFileInfo>

OccElasticTensorTask::OccElasticTensorTask(QObject *parent)
    : ExternalProgramTask(parent) {
    setExecutable(settings::readSetting(settings::keys::OCC_EXECUTABLE).toString());
}

void OccElasticTensorTask::setInputJsonFile(const QString &filename) {
    m_inputJsonFile = filename;
}

QString OccElasticTensorTask::outputJsonFilename() const {
    if (m_inputJsonFile.isEmpty()) {
        return QString();
    }

    QFileInfo inputInfo(m_inputJsonFile);
    // occ elastic_fit writes to elastic_tensor.txt by default
    return inputInfo.absolutePath() + "/elastic_tensor.txt";
}

void OccElasticTensorTask::start() {
    if (m_inputJsonFile.isEmpty()) {
        qWarning() << "OccElasticTensorTask: No input JSON file specified";
        return;
    }

    QStringList args;
    args << "elastic_fit";
    args << m_inputJsonFile;

    setArguments(args);

    // Set up file dependencies
    FileDependencyList requirements;
    requirements.append(FileDependency(m_inputJsonFile));
    setRequirements(requirements);

    FileDependencyList outputs;
    QString outputFile = outputJsonFilename();
    outputs.append(FileDependency(outputFile, outputFile));
    setOutputs(outputs);

    ExternalProgramTask::start();
}
