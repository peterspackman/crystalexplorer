#pragma once
#include "externalprogram.h"

class OccElatTask : public ExternalProgramTask {
    Q_OBJECT
public:
    explicit OccElatTask(QObject *parent = nullptr);

    void setCrystalStructureFile(const QString &filename);
    void setEnergyModel(const QString &model); // e.g., "ce-b3lyp", "hf-3c", etc.
    void setRadius(double radius);
    void setThreads(int threads);

    QString outputJsonFilename() const;

    virtual void start() override;

private:
    QString m_crystalFile;
    QString m_energyModel{"ce-b3lyp"};
    double m_radius{15.0};
    int m_threads{1};
};
