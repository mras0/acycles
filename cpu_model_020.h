#ifndef CPU_MODEL_020_H
#define CPU_MODEL_020_H

#include "cpu_model.h"
#include <vector>
#include <memory>
#include <iosfwd>

class instruction;

std::unique_ptr<cpu_model> make_cpu_model_020(std::ostream& os, const std::vector<instruction>& instructions);

#endif
