#ifndef CPU_MODEL_060_H
#define CPU_MODEL_060_H

#include "cpu_model.h"
#include <vector>
#include <memory>
#include <iosfwd>

class instruction;

std::unique_ptr<cpu_model> make_cpu_model_060(std::ostream& os, const std::vector<instruction>& instructions);

#endif
