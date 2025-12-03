#pragma once

#include <QString>

class PairInteractions;
class ChemicalStructure;

bool save_elastic_fit_pairs_json(const PairInteractions *interactions,
                                  const ChemicalStructure *structure,
                                  const QString &model,
                                  const QString &filename);
