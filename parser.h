#ifndef PARSER_H_INCLUDED
#define PARSER_H_INCLUDED

#include <istream>
#include <string>
#include <optional>
#include <vector>
#include "ea.h"
#include "instruction.h"

class parser {
public:
    explicit parser(std::istream& in);

    std::optional<instruction> next();
    std::vector<instruction> all();

private:
    std::istream& in_;
    std::string line_;
    size_t line_num_ = 1;
    size_t pos_ = 0;

    std::optional<instruction> do_parse();
    void skip_space();

    [[noreturn]] void error(const std::string& msg);

    std::optional<eareg> parse_reg();
    uint32_t parse_number();
    ea parse_ea();
};

#endif
