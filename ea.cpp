#include "ea.h"
#include "util.h"

static_assert(int(eareg::d0) == 0);
static_assert(int(eareg::d7) == 7);
static_assert(int(eareg::a0) == 0 + 8);
static_assert(int(eareg::a7) == 7 + 8);
static_assert(!is_areg(eareg::d0));
static_assert(!is_areg(eareg::d7));
static_assert(is_areg(eareg::a0));
static_assert(is_areg(eareg::a7));
static_assert(!is_areg(eareg::pc));

std::ostream& operator<<(std::ostream& os, eareg r)
{
    switch (r) {
#define X(r)       \
    case eareg::r: \
        return os << #r;
        EAREGS(X)
#undef X
    default:
        return os << "eareg{" << static_cast<int>(r) << "}";
    }
}

bool ea_has_extra(uint8_t val)
{
    switch (val >> ea_m_shift) {
    case ea_m_Dn:
    case ea_m_An:
    case ea_m_A_ind:
    case ea_m_A_ind_post:
    case ea_m_A_ind_pre:
        return false;
    case ea_m_A_ind_disp16:
    case ea_m_A_ind_index:
        return true;
    case ea_m_Other:
        switch (val & ea_xn_mask) {
        case ea_other_abs_w:
        case ea_other_abs_l:
        case ea_other_pc_disp16:
        case ea_other_pc_index:
        case ea_other_imm:
            return true;
        }
    }
    throw std::runtime_error { "TODO: ea_has_extra for val=$" + hexstring(val) };
}

int ea::num_words(bool is_long) const
{
    switch (val_ >> ea_m_shift) {
    case ea_m_Dn:
    case ea_m_An:
    case ea_m_A_ind:
    case ea_m_A_ind_post:
    case ea_m_A_ind_pre:
        return 0;
    case ea_m_A_ind_disp16:
    case ea_m_A_ind_index:
        return 1;
    case ea_m_Other:
        switch (val_ & ea_xn_mask) {
        case ea_other_abs_w:
            return 1;
        case ea_other_abs_l:
            return 2;
        case ea_other_pc_disp16:
            return 1;
        case ea_other_pc_index:
            return 1;
        case ea_other_imm:
            return is_long ? 2 : 1;
        }
    }
    throw std::runtime_error { "TODO: ea::num_words for val=$" + hexstring(val_) };
}

brief_extension_word get_brief_extension_word(const ea& e)
{
    // TODO: ea_other_pc_index
    assert((e.val() >> ea_m_shift) == ea_m_A_ind_index);

    brief_extension_word bew {};
    const uint16_t extw = e.extra();
    bew.displacement = static_cast<int8_t>(extw & 255);
    bew.base = static_cast<eareg>(8 + (e.val() & 7));
    bew.index = static_cast<eareg>(extw >> 12);
    bew.long_size = !!(extw & (1 << 11));
    bew.scale = 1 << ((extw >> 9) & 3);
    return bew;
}

std::ostream& operator<<(std::ostream& os, const ea& e)
{
    switch (e.val() >> ea_m_shift) {
    case ea_m_Dn:
        return os << "d" << (e.val() & ea_xn_mask);
    case ea_m_An:
        return os << "a" << (e.val() & ea_xn_mask);
    case ea_m_A_ind:
        return os << "(a" << (e.val() & ea_xn_mask) << ")";
    case ea_m_A_ind_post:
        return os << "(a" << (e.val() & ea_xn_mask) << ")+";
    case ea_m_A_ind_pre:
        return os << "-(a" << (e.val() & ea_xn_mask) << ")";
    case ea_m_A_ind_disp16:
        return os << static_cast<int16_t>(e.extra() & 0xffff) << "(a" << (e.val() & ea_xn_mask) << ")";
    case ea_m_A_ind_index: {
        const auto bew = get_brief_extension_word(e);
        os << bew.displacement << '(' << bew.base << ',' << bew.index << '.' << (bew.long_size ? 'l' : 'w');
        if (bew.scale > 1)
            os << '*' << bew.scale;
        return os << ')';
    }
    case ea_m_Other:
        switch (e.val() & ea_xn_mask) {
        case ea_other_abs_w:
            return os << "$" << std::hex << static_cast<uint16_t>(e.extra()) << std::dec << ".w";
        case ea_other_abs_l:
            return os << "$" << std::hex << e.extra() << std::dec;
        // case ea_other_pc_disp16:
        // case ea_other_pc_index:
        case ea_other_imm:
            return os << "#" << static_cast<int>(e.extra());
        }
    }

    throw std::runtime_error { "TODO: operator<<(ea) for val=$" + hexstring(e.val()) };
}
