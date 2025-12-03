#pragma once
#include "pair_energy_results.h"
#include <QString>

bool save_pair_energy_json(const PairInteraction *interaction, const QString &filename);
bool save_pair_interactions_json(const PairInteractions *interactions, const QString &filename);