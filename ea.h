#ifndef EA_H_INCLUDED
#define EA_H_INCLUDED

#include <cstdint>
#include <ostream>
#include <cassert>

#define EAREGS(X)\
    X(d0) X(d1) X(d2) X(d3) X(d4) X(d5) X(d6) X(d7)\
    X(a0) X(a1) X(a2) X(a3) X(a4) X(a5) X(a6) X(a7)\
    X(pc)

enum class eareg {
#define X(r) r,
    EAREGS(X)
#undef X
};

constexpr bool is_areg(eareg r)
{
    return (static_cast<int>(r) & 0b11000) == 0b01000;
}

std::ostream& operator<<(std::ostream& os, eareg r);

enum ea_m {
    ea_m_Dn = 0b000, // Dn
    ea_m_An = 0b001, // An
    ea_m_A_ind = 0b010, // (An)
    ea_m_A_ind_post = 0b011, // (An)+
    ea_m_A_ind_pre = 0b100, // -(An)
    ea_m_A_ind_disp16 = 0b101, // (d16, An)
    ea_m_A_ind_index = 0b110, // (d8, An, Xn)
    ea_m_Other = 0b111, // (Other)
};

enum ea_other {
    ea_other_abs_w = 0b000, // (addr.W)
    ea_other_abs_l = 0b001, // (addr.L)
    ea_other_pc_disp16 = 0b010, // (d16, PC)
    ea_other_pc_index = 0b011, // (d8, PC, Xn)
    ea_other_imm = 0b100,
};

constexpr uint8_t ea_m_shift = 3;
constexpr uint8_t ea_xn_mask = 7;

constexpr uint8_t ea_pc_disp16 = ea_m_Other << ea_m_shift | ea_other_pc_disp16;
constexpr uint8_t ea_pc_index = ea_m_Other << ea_m_shift | ea_other_pc_index;
constexpr uint8_t ea_immediate = ea_m_Other << ea_m_shift | ea_other_imm;

bool ea_has_extra(uint8_t val);

class ea {
public:
    ea() = default;
    explicit ea(uint8_t val)
        : val_ { val }
    {
        assert(!ea_has_extra(val));
    }

    explicit ea(uint8_t val, uint32_t extra)
        : val_ { val }
        , extra_ { extra }
    {
        assert(ea_has_extra(val));
    }

    uint8_t val() const
    {
        return val_;
    }

    uint32_t extra() const
    {
        assert(ea_has_extra(val_));
        return extra_;
    }

    bool is_mem() const
    {
        switch (val_ >> ea_m_shift) {
        case ea_m_Dn:
        case ea_m_An:
            return false;
        case ea_m_Other:
            return val_ != ea_immediate;
        }
        return true;
    }

    int num_words(bool is_long = false) const;

private:
    uint8_t val_;
    uint32_t extra_;
};
std::ostream& operator<<(std::ostream& os, const ea& e);

struct brief_extension_word {
    eareg base;
    eareg index;
    bool long_size;
    int scale;
    int displacement; // 8-bit
};

brief_extension_word get_brief_extension_word(const ea& e);

#endif
