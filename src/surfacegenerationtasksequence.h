#pragma once
#include "surfacedescription.h"
#include "tasksequence.h"

class SurfaceGenerationTaskSequence : public TaskSequence {
  Q_OBJECT
public:
  SurfaceGenerationTaskSequence();

private:
  SurfaceType m_surfaceType{SurfaceType::SurfaceType::Hirshfeld};
};
