#pragma once
#include "pair_energy_results.h"
#include <QString>

class ChemicalStructure;

bool save_pair_energy_json(const PairInteraction *interaction, const QString &filename);
bool save_pair_interactions_json(const PairInteractions *interactions, const QString &filename);
bool save_pair_interactions_for_model_json(const PairInteractions *interactions, const ChemicalStructure *structure, const QString &model, const QString &filename);