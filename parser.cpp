#include "parser.h"
#include <sstream>
#include <limits.h>

char lower(char ch)
{
    return ch >= 'A' && ch <= 'Z' ? ch | 0x20 : ch;
}

void remove_comments(std::string& line)
{
    if (line.empty())
        return;
    auto pos = line.find_first_of(";");
    if (pos == std::string::npos)
        pos = line.size();
    while (pos && isspace(line[pos - 1]))
        --pos;
    line.erase(pos);

}

#define PARSER_EXPECT_NOT_EOL() do { if (pos_ == line_.size()) error("Unexpected end of line in " + std::string(__func__) + " line " + std::to_string(__LINE__)); } while (0)
#define PARSER_EXPECT(ch) do { if (pos_ == line_.size() || line_[pos_] != (ch)) error("Expected " + std::string(1, ch) + " in " + std::string(__func__) + " line " + std::to_string(__LINE__)); ++pos_; } while (0)

parser::parser(std::istream& in)
    : in_ { in }
{
}

std::optional<instruction> parser::next()
{
    for (;;) {
        if (!std::getline(in_, line_))
            return {};
        remove_comments(line_);
        pos_ = 0;
        auto res = do_parse();
        ++line_num_;
        if (res)
            return res;
    }
}

std::vector<instruction> parser::all()
{
    std::vector<instruction> res;
    while (auto i = next())
        res.emplace_back(std::move(*i));
    return res;
}

void parser::skip_space()
{
    while (pos_ < line_.size() && isspace(line_[pos_]))
        ++pos_;
}

void parser::error(const std::string& msg)
{
    std::ostringstream oss;
    oss << "Error in line " << line_num_ << ": " << msg << " at position " << pos_ << " in line \"" << line_ << "\"";
    throw std::runtime_error { oss.str() };
}

std::optional<eareg> parser::parse_reg()
{
    if (pos_ + 2 > line_.size())
        return {};
    const char ch = lower(line_[pos_]);
    if ((ch == 'a' || ch == 'd') && line_[pos_ + 1] >= '0' && line_[pos_ + 1] <= '7') {
        int r = line_[pos_ + 1] - '0';
        if (ch == 'a')
            r += 8;
        pos_ += 2;
        return eareg { r };
    }
    if (ch == 'p' && lower(line_[pos_ + 1]) == 'c') {
        pos_ += 2;
        return eareg::pc;
    }
    return {};
}

uint32_t parser::parse_number()
{
    PARSER_EXPECT_NOT_EOL();
    bool neg = false;
    if (line_[pos_] == '-') {
        neg = true;
        ++pos_;
        PARSER_EXPECT_NOT_EOL();
    }

    if (line_[pos_] != '$' && line_[pos_] != '%' && !isdigit(line_[pos_])) {
        // named constant
        ++pos_;
        for (; pos_ < line_.size(); ++pos_) {
            const auto ch = line_[pos_];
            if (!isalnum(ch) && ch != '_')
                break;
        }
        return 0;
    }
    uint32_t num = 0;
    int base = 10;
    if (line_[pos_] == '$') {
        base = 16;
        ++pos_;
        if (line_[pos_] == '-') {
            neg = true;
            ++pos_;
        }
    } else if (line_[pos_] == '%') {
        base = 2;
        ++pos_;
    }
    PARSER_EXPECT_NOT_EOL();
    for (; pos_ < line_.size(); ++pos_) {
        const auto ch = line_[pos_];
        if (ch >= '0' && ch <= '9')
            num = num * base + (ch - '0');
        else if (ch >= 'a' && ch <= 'f')
            num = num * base + (ch - 'a') + 10;
        else if (ch >= 'A' && ch <= 'F')
            num = num * base + (ch - 'A') + 10;
        else
            break;
    }
    if (neg)
        num = -static_cast<int>(num);
    return num;
}


ea parser::parse_ea()
{
    PARSER_EXPECT_NOT_EOL();
    if (line_[pos_] == '#') {
        ++pos_;
        PARSER_EXPECT_NOT_EOL();
        return ea { ea_immediate, parse_number() };
    }

    if (auto reg = parse_reg()) {
        if (*reg == eareg::pc)
            error("Invalid EA (bare PC)");
        return ea { static_cast<uint8_t>(*reg) };
    }
    // TODO: Check for '-'
    std::optional<uint32_t> dispval {};
    if (line_[pos_] != '(') {
        dispval = parse_number();
        // TODO: ABS.w
        if (pos_ == line_.size() || line_[pos_] == ',')
            return ea { static_cast<uint8_t>(ea_m_Other << ea_m_shift | ea_other_abs_l), *dispval };
    }

    PARSER_EXPECT_NOT_EOL();
    if (line_[pos_] == '(') {
        ++pos_;

        PARSER_EXPECT_NOT_EOL();

        auto basereg = parse_reg();
        if (!basereg || !is_areg(*basereg))
            error("Invalid basre register"); // TODO: PC (and different order...)

        PARSER_EXPECT_NOT_EOL();
        if (line_[pos_] == ')') {
            pos_++;
            if (pos_ < line_.size() && line_[pos_] == '+') {
                if (dispval)
                    error("Invalid EA");
                ++pos_;
                return ea { static_cast<uint8_t>((static_cast<uint8_t>(*basereg) & ea_xn_mask) | ea_m_A_ind_post << ea_m_shift) };
            }
            if (dispval && *dispval) {
                int32_t d = *dispval;
                if (d < SHRT_MIN || d > SHRT_MAX)
                    error("Displacmeent out of range");
                return ea { static_cast<uint8_t>((static_cast<uint8_t>(*basereg) & ea_xn_mask) | ea_m_A_ind_disp16 << ea_m_shift), *dispval };
            }
            return ea { static_cast<uint8_t>((static_cast<uint8_t>(*basereg) & ea_xn_mask) | ea_m_A_ind << ea_m_shift) };
        }

        PARSER_EXPECT(',');
        PARSER_EXPECT_NOT_EOL();
        auto dispreg = parse_reg();
        if (!dispreg || *dispreg == eareg::pc)
            error("Invalid/Unsupported displacement");
        bool long_size = false;
        PARSER_EXPECT_NOT_EOL();
        if (line_[pos_] == '.') {
            ++pos_;
            PARSER_EXPECT_NOT_EOL();
            const auto d = lower(line_[pos_]);
            if (d == 'l')
                long_size = true;
            else if (d != 'w')
                error("Invalid size of displacement");
            ++pos_;
        }
        PARSER_EXPECT_NOT_EOL();
        int scale = 0;
        if (line_[pos_] == '*') {
            ++pos_;
            PARSER_EXPECT_NOT_EOL();
            scale = line_[pos_++] - '0';
            if (scale == 1)
                scale = 0;
            else if (scale == 2)
                scale = 1;
            else if (scale == 4)
                scale = 2;
            else if (scale == 8)
                scale = 4;
            else
                error("Invalid scale");            
        }
        PARSER_EXPECT(')');
        uint8_t disp = 0;
        if (dispval) {
            int32_t d = *dispval;
            if (d < -128 || d > 127)
                error("Displacmeent out of range");
            disp = static_cast<uint8_t>(d & 0xff);
        }
        return ea { static_cast<uint8_t>((static_cast<uint8_t>(*basereg) & ea_xn_mask) | ea_m_A_ind_index << ea_m_shift), static_cast<uint16_t>(static_cast<int>(*dispreg) << 12 | long_size << 11 | scale << 9 | disp) };
    }

    error("Unhandled EA in parse_ea");
}

std::optional<instruction> parser::do_parse()
{
    if (line_.empty())
        return {};

    std::string label;

    while (pos_ < line_.size() && !isspace(line_[pos_]) && line_[pos_] != ':') {
        label.push_back(line_[pos_++]);
    }
    if (pos_ && pos_ < line_.size() && line_[pos_] == ':')
        ++pos_;

    skip_space();

    if (pos_ == line_.size())
        return {};

    std::string ins_str;
    char suffix = 0;
    while (pos_ < line_.size() && !isspace(line_[pos_]) && line_[pos_] != '.')
        ins_str.push_back(lower(line_[pos_++]));

    const auto opcode = opcode_from_string(ins_str);

    if (pos_ < line_.size() && line_[pos_] == '.') {
        ++pos_;
        if (pos_ == line_.size())
            error("Invalid suffix");
        suffix = lower(line_[pos_++]);
        if (!suffix || !valid_size(suffix))
            error("Unrecognized suffix");
    }

    //if (!label.empty()) std::cout << "Label  \"" << label << "\"\t";

    skip_space();

    std::optional<ea> ea1 {}, ea2 {};

    if (pos_ < line_.size()) {
        ea1 = parse_ea();
        if (pos_ < line_.size() && line_[pos_] == ',') {
            ++pos_;
            ea2 = parse_ea();
        }
    }
    if (!!ea1 + !!ea2 != num_ea(opcode))
        error("Invalid numeber of operands for " + ins_str + " expected " + std::to_string(num_ea(opcode)));

    if (pos_ < line_.size())
        error("Junk at end of line: \"" + line_.substr(pos_) + "\"");

    if (ea2)
        return instruction { opcode, suffix, *ea1, *ea2 };
    else if (ea1)
        return instruction { opcode, suffix, *ea1 };
    else
        return instruction { opcode, suffix };
}
