#include "instruction.h"
#include <stdexcept>
#include <sstream>

std::optional<eareg> reg_or_none(const ea& e)
{
    switch (e.val() >> ea_m_shift) {
    case ea_m_Dn:
        return static_cast<eareg>(e.val() & ea_xn_mask);
    case ea_m_An:
        return static_cast<eareg>(8 + (e.val() & ea_xn_mask));
    default:
        return {};
    }
}

std::ostream& operator<<(std::ostream& os, opcode o)
{
    // Special cases...
    switch (o) {
    case opcode::and_: return os << "and";
    case opcode::or_: return os << "or";
    }
    switch (o) {
#define X(o, rmw, nea, cycles, classi) case opcode::o: return os << #o;
        OPCODES(X)
#undef X
    default:
        return os << "opcode{" << static_cast<int>(o) << "}";
    }
}

opcode opcode_from_string(const std::string& str)
{
#define X(o, rmw, nea, cycles, classi) if (str == #o) return opcode::o;
    OPCODES(X)
#undef X
    // Synonyms/special cases
    if (str == "adda") return opcode::add;
    if (str == "and") return opcode::and_;
    if (str == "or") return opcode::or_;
    if (str == "not") return opcode::not_;
    if (str == "cmpa") return opcode::cmp;
    if (str == "movea") return opcode::move;
    if (str == "dbf") return opcode::dbra;
    throw std::runtime_error("Unknown opcode \"" + str + "\"");
}

bool is_rmw(opcode op)
{
    switch (op) {
#define X(o, rmw, nea, cycles, classi) case opcode::o: return rmw;
        OPCODES(X)
#undef X
    }
    std::ostringstream oss;
    oss << "TODO: Implement is_rmw " << op << "\n";
    throw std::runtime_error { oss.str() };
}

bool valid_size(char ch)
{
    return !ch || ch == 's' || ch == 'b' || ch == 'w' || ch == 'l';
}

int num_ea(opcode op)
{
    switch (op) {
#define X(o, rmw, nea, cycles, classi) case opcode::o: return nea;
        OPCODES(X)
#undef X
    }
    std::ostringstream oss;
    oss << "TODO: Implement num_ea " << op << "\n";
    throw std::runtime_error { oss.str() };
}

bool is_branch(opcode op)
{
    switch (op) {
    case opcode::bra:
    case opcode::bhi:
    case opcode::bls:
    case opcode::bcc:
    case opcode::bcs:
    case opcode::bne:
    case opcode::beq:
    case opcode::bvc:
    case opcode::bvs:
    case opcode::bpl:
    case opcode::bmi:
    case opcode::bge:
    case opcode::blt:
    case opcode::bgt:
    case opcode::ble:
        return true;
    default:
        return false;
    }
}

std::ostream& operator<<(std::ostream& os, oep_class c)
{
    switch (c) {
    case oep_class::poep_or_soep:
        return os << "pOEP | sOEP";
    case oep_class::poep_only:
        return os << "pOEP-only";
    case oep_class::poep_until_last:
        return os << "pOEP-until-last";
    case oep_class::poep_but_allows_soep:
        return os << "pOEP-but-allows-sOEP";
    default:
        return os << "oep_class{" << static_cast<int>(c) << "}";
    }
}

std::ostream& operator<<(std::ostream& os, resource r)
{
    switch (r) {
    case resource::a_b:
        return os << "A/B";
    case resource::base:
        return os << "Base";
    case resource::index:
        return os << "Index";

    default:
        return os << "resource{" << static_cast<int>(r) << "}";
    }
}


std::ostream& operator<<(std::ostream& os, const instruction& i)
{
    os << i.op();
    if (i.opsize())
        os << "." << i.opsize();
    const int nea = num_ea(i.op());
    if (nea)
        os << "\t" << i.arg(0);
    if (nea == 2)
        os << "," << i.arg(1);
    return os;
}

std::optional<resource> need_regX(const ea& e, eareg r)
{
    switch (e.val() >> ea_m_shift) {
    case ea_m_Dn:
    case ea_m_An:
        return static_cast<int>(e.val() & 15) == static_cast<int>(r) ? std::optional(resource::a_b) : std::nullopt;
    case ea_m_A_ind:
    case ea_m_A_ind_post:
    case ea_m_A_ind_pre:
    case ea_m_A_ind_disp16:
        if (!is_areg(r))
            return {};
        return static_cast<int>(e.val() & 7) == static_cast<int>(r) ? std::optional(resource::base) : std::nullopt;
    case ea_m_A_ind_index: {
        const auto bew = get_brief_extension_word(e);
        if (r == bew.base)
            return resource::base;
        if (r == bew.index)
            return resource::index;
        return {};
    }
    case ea_m_Other:
        switch (e.val() & ea_xn_mask) {
        case ea_other_abs_w:
        case ea_other_abs_l:
        case ea_other_pc_disp16:
            return {};
        // case ea_other_pc_index:
        case ea_other_imm:
            return {};
        }
    }
    std::ostringstream oss;
    oss << "TODO: Implement need_regX for " << e << "\n";
    throw std::runtime_error { oss.str() };
}

oep_class instruction::oep_classify() const
{
    // Special cases
    switch (op_) {
    case opcode::move:
        // pOEP-until-last if destination is mem and source is mem/immediate
        return ea_[1].is_mem() && (ea_[0].is_mem() || ea_[0].val() == ea_immediate) ? oep_class::poep_until_last : oep_class::poep_or_soep;
    }
    switch (op_) {
#define X(o, rmw, nea, cycles, classi) case opcode::o: return oep_class::classi;
        OPCODES(X)
#undef X
    }

    std::ostringstream oss;
    oss << "TODO: Implement OEP classificatoin for " << *this << "\n";
    throw std::runtime_error { oss.str() };
}

int instruction::mem_cycles() const
{
    // TODO: complex ea...
    const bool rmw = is_rmw(op_);
    switch (num_ea(op_)) {
    case 0:
        return 0;
    case 1:
        return ea_[0].is_mem() * (rmw ? 2 : 1);
    case 2:
        return ea_[0].is_mem()  + ea_[1].is_mem() * (rmw ? 2 : 1);
    }

    assert(false);
    return 0;
}


int instruction::cylces() const
{
    int base = 0;
    switch (op_) {
#define X(o, rmw, nea, cycles, classi) case opcode::o: base = cycles; break;
        OPCODES(X)
#undef X
    default:
        std::ostringstream oss;
        oss << "TODO: Implement cycles() for " << *this << "\n";
        throw std::runtime_error { oss.str() };
    }

    int mcycles = mem_cycles();
    return mcycles > 1 ? base + mcycles - 1 : base;
}

std::optional<eareg> instruction::execution_result_reg() const
{
    switch (op_) {
    case opcode::cmp:
    //case opcode::btst:
        return {};
    }

    switch (num_ea(op_)) {
    case 0:
        return {};
    case 1:
        return reg_or_none(ea_[0]);
    case 2:
        return reg_or_none(ea_[1]);
    }

    assert(false);
    return {};
}

std::optional<resource> instruction::need_reg(eareg r) const
{
    switch (num_ea(op_)) {
    case 0:
        return {};
    case 1:
        return need_regX(ea_[0], r);
    case 2:
        if (auto r1 = need_regX(ea_[1], r))
            return r1;
        return need_regX(ea_[0], r);
    }

    assert(false);
    return {};
}


int instruction::num_words() const
{
    if (is_branch(op_))
        return size_ == 'w' ? 2 : 1;
    if (op_ == opcode::dbra)
        return 2;

    int nw = 1;
    if (num_ea(op_)) {
        if (ea_[0].val() == ea_immediate) {
            switch (op_) {
            case opcode::asr:
            case opcode::addq:
            case opcode::subq:
            case opcode::moveq:
            case opcode::lsl:
            case opcode::lsr:
            case opcode::rol:
            case opcode::ror:
                goto ea2;
            // TODO: btst (always one word)
            }
        }
        nw += ea_[0].num_words(size_ == 'l');
    }
ea2:
    if (num_ea(op_) > 1)
        nw += ea_[1].num_words(); // second ea can't be immediate
    return nw;
}