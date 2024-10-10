#include "atomflags.h"

void to_json(nlohmann::json& j, const AtomFlags& flags) {
  j = static_cast<int>(flags);
}

void from_json(const nlohmann::json& j, AtomFlags& flags) {
  flags = static_cast<AtomFlags>(j.get<int>());
}
