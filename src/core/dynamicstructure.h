#pragma once
#include "chemicalstructure.h"

class DynamicStructure : public ChemicalStructure {
    Q_OBJECT

public:
    explicit DynamicStructure(QObject *parent = nullptr);

    // Frame management
    int frameCount() const override { return m_frames.size(); }
    void addFrame(ChemicalStructure *frame);
    void removeFrame(int index) override;
    void setCurrentFrameIndex(int index) override;
    int getCurrentFrameIndex() const override { return m_currentFrameIndex; }

    // Access to current frame
    ChemicalStructure* currentFrame();
    const ChemicalStructure* currentFrame() const;

    StructureType structureType() const override;
signals:
    void frameAdded(int index);
    void frameRemoved(int index);
    void currentFrameChanged(int index);

private:
    QList<ChemicalStructure*> m_frames;
    int m_currentFrameIndex{-1};
    void updateFromCurrentFrame();
};
