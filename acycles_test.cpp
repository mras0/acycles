#include "cpu_model_020.h"
#include "cpu_model_060.h"
#include "parser.h"
#include "util.h"
#include <fstream>
#include <iostream>
#include <filesystem>

int main()
{
    std::string last_file;
    try {        
        for (const auto& fn : std::filesystem::recursive_directory_iterator(std::filesystem::path { ".." }  /  "tests")) {
            last_file = fn.path().string();
            std::ifstream in { fn.path() };
            if (!in)
                throw std::runtime_error { "Error opening file" };

            // TODO: Check that cycle counts are correct (and stay correct)
            const auto insts = parser { in }.all();
            auto cpu_020 = make_cpu_model_020(std::cout, insts);
            auto cpu_060 = make_cpu_model_060(std::cout, insts);
            [[maybe_unused]] const auto res_020 = cpu_020->simulate(0, false);
            [[maybe_unused]] const auto res_060 = cpu_060->simulate(0, false);
            std::cout << fn.path().filename() << "\t" << res_020 << "\t" << res_060 << "\n";
        }

    } catch (const std::exception& e) {
        if (!last_file.empty())
            std::cerr << "Error while processing " << last_file << "\n";
        std::cerr << e.what() << "\n";
        return 1;
    }
    return 0;
}
