#include <iostream>
#include <fstream>
#include "parser.h"
#include "util.h"
#include "cpu_model_020.h"
#include "cpu_model_060.h"

int main(int argc, char* argv[])
{
    try {
        int argp = 1;
        int model = 68060;

        if (argv[argp][0] == '-') {
            model = atoi(argv[argp] + 1);
            if (model < 68000)
                model += 68000;
            switch (model) {
            case 68020:
            case 68060:
                break;
            default:
                throw std::runtime_error { std::string { "Unsupported CPU model " } + (argv[argp] + 1) };
            }
            ++argp;
        }
        if (argp + 1 != argc || !argv[argp][0])
            throw std::runtime_error { "Usage: " + std::string { argv[0] } + " [-68020/-68060] source" };

        std::ifstream in { argv[argp] };
        if (!in)
            throw std::runtime_error {"Could not open " + std::string{argv[argp] } };
        parser p { in };
        const auto insts = p.all();
        int instruction_words = 0;
        for (const auto& i : insts) {
            instruction_words += i.num_words();
            //std::cout << "\t" << with_width(i,30) << "; length " << i.num_words() << " \n";
        }

        if (model == 68060) {
            auto cpu = make_cpu_model_060(std::cout, insts);
            cpu->simulate(1, true);
            double res = cpu->simulate(100, false);
            std::cout << "Instruction words in loop: " << instruction_words << ", " << res << " cycles/iteration"
                      << "\n";
        } else {
            assert(model == 68020);
            auto cpu = make_cpu_model_020(std::cout, insts);
            cpu->simulate(0, true);
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }
    return 0;
}
