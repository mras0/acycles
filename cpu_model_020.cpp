#include "cpu_model_020.h"
#include "instruction.h"
#include "util.h"
#include <sstream>

// TODO: Pipeline simulation

namespace {

struct cycle_counts {
    int best;
    int cache;
    int worst;

    cycle_counts& operator+=(const cycle_counts& r)
    {
        best += r.best;
        cache += r.cache;
        worst += r.worst;
        return *this;
    }
};

cycle_counts operator+(const cycle_counts& l, const cycle_counts& r)
{
    cycle_counts cc = l;
    return cc += r;
}

std::ostream& operator<<(std::ostream& os, const cycle_counts& cc)
{
    return os << cc.best << "/" << cc.cache << "/" << cc.worst;
}

cycle_counts fetch_effective_address_cost(const ea& e, char opsize)
{
    switch (e.val() >> ea_m_shift) {
    case ea_m_Dn:
    case ea_m_An:
        return {};
    case ea_m_A_ind:
        return { 3, 4, 4 };
    case ea_m_A_ind_post:
        return { 4, 4, 4 };
    case ea_m_A_ind_pre:
        return { 3, 5, 5 };
    case ea_m_A_ind_disp16:
        return { 3, 5, 6 };
    case ea_m_A_ind_index:
        return { 4, 7, 8 };
    case ea_m_Other:
        switch (e.val() & ea_xn_mask) {
        case ea_other_abs_w:
            return { 3, 4, 6 };
        case ea_other_abs_l:
            return { 3, 4, 7 };
        // case ea_other_pc_disp16:
        // case ea_other_pc_index:
        case ea_other_imm:
            if (opsize != 'l')
                return { 0, 2, 3 };
            else
                return { 0, 4, 5 };
        }
    default:
        std::ostringstream oss;
        oss << "TODO: fetch_effective_address_cost (020) for " << e;
        throw std::runtime_error { oss.str() };
    }
}

cycle_counts fetch_effective_address_cost(const instruction& inst)
{
    cycle_counts cost{};
    for (int i = 0; i < num_ea(inst.op()); ++i) {
        const auto& e = inst.arg(i);
        if (e.val() != ea_immediate || !has_embeeded_immediate(inst))
            cost += fetch_effective_address_cost(e, inst.opsize());
    }
    return cost;
}

cycle_counts fetch_immediate_effective_address_cost(const ea& e, char opsize)
{
    // 68020UM
    // Many two-word instructions (e.g., MULU.L, DIV.L, BFSET, etc.) include the fetch
    // immediate effective address time or the calculate immediate effective address time in the
    // execution time calculation. The timing for immediate data of word length (#<data>.W) is
    // used for these calculations. If the instruction has a source and a destination, the source
    // effective address is used for the table lookup. If the instruction is single operand, the
    // effective address of that operand is used.

    const bool w = opsize != 'l';
    switch (e.val() >> ea_m_shift) {
    case ea_m_Dn:
    case ea_m_An:
        return w ? cycle_counts { 0, 2, 3 } : cycle_counts { 1, 4, 5 };
    case ea_m_A_ind:
        return w ? cycle_counts { 3, 4, 4 } : cycle_counts { 3, 4, 7 };
    case ea_m_A_ind_pre: 
        return w ? cycle_counts { 3, 5, 6 } : cycle_counts { 4, 7, 8 };
    case ea_m_A_ind_post:
        return w ? cycle_counts { 4, 6, 7 } : cycle_counts { 5, 8, 9 };
    case ea_m_A_ind_disp16:
    disp16:
        return w ? cycle_counts { 3, 5, 7 } : cycle_counts { 4, 7, 10 };
    case ea_m_A_ind_index:
    disp_index:
        return w ? cycle_counts { 4, 9, 11 } : cycle_counts { 5, 11, 13 };
    case ea_m_Other:
        switch (e.val() & ea_xn_mask) {
        case ea_other_abs_w:
            return w ? cycle_counts { 3, 5, 7 } : cycle_counts { 4, 7, 10 };
        case ea_other_abs_l:
            return w ? cycle_counts { 3, 6, 10 } : cycle_counts { 4, 8, 12 };
        case ea_other_pc_disp16:
            goto disp16;
        case ea_other_pc_index:
            goto disp_index;
        case ea_other_imm:
            return w ? cycle_counts { 0, 4, 6 } : cycle_counts { 1, 8, 10 };
        }
    }
    std::ostringstream oss;
    oss << "TODO: fetch_immediate_effective_address_cost (020) for " << e << " size " << opsize;
    throw std::runtime_error { oss.str() };
}

cycle_counts move_cost_020(const instruction& i)
{
    assert(i.op() == opcode::move && num_ea(i.op()) == 2);
    const auto src_ea_m = i.arg(0).val() >> ea_m_shift;
    const auto dst_ea_m = i.arg(1).val() >> ea_m_shift;

    switch (src_ea_m) {
    case ea_m_Dn:
    case ea_m_An:
        switch (dst_ea_m) {
        case ea_m_Dn:
            return { 0, 2, 3 };
        case ea_m_An:
            return { 0, 2, 3 };
        case ea_m_A_ind:
            return { 3, 4, 5 };
        case ea_m_A_ind_post:
            return { 4, 4, 5 };
        case ea_m_A_ind_pre:
            return { 3, 5, 6 };
        case ea_m_A_ind_disp16:
            return { 3, 5, 7 };
        case ea_m_A_ind_index:
            return { 4, 7, 9 };
        case ea_m_Other:
            switch (i.arg(1).val() & ea_xn_mask) {
            case ea_other_abs_w:
                return { 3, 4, 7 };
            case ea_other_abs_l:
                return { 5, 6, 9 };
            }
        }
        break;
    case ea_m_A_ind:
        switch (dst_ea_m) {
        case ea_m_Dn:
            return { 3, 6, 7 };
        case ea_m_An:
            return { 3, 6, 7 };
        case ea_m_A_ind:
            return { 6, 7, 9 };
        case ea_m_A_ind_post:
            return { 6, 7, 9 };
        case ea_m_A_ind_pre:
            return { 6, 7, 9 };
        case ea_m_A_ind_disp16:
            return { 6, 7, 11 };
        case ea_m_A_ind_index:
            return { 8, 9, 11 };
            // case ea_m_Other:
        }
        break;
    //case ea_m_A_ind_pre:
    // case ea_m_A_ind_post:
    case ea_m_A_ind_disp16: // Also for disp16(pc)
    disp16:
        switch (dst_ea_m) {
        case ea_m_Dn:
            return { 3, 7, 9 };
        case ea_m_An:
            return { 3, 7, 9 };
        case ea_m_A_ind:
            return { 6, 8, 11 };
        case ea_m_A_ind_post:
            return { 6, 8, 11 };
        case ea_m_A_ind_pre:
            return { 6, 8, 11 };
        case ea_m_A_ind_disp16:
            return { 6, 8, 13 };
        case ea_m_A_ind_index:
            return { 8, 10, 13 };
            // case ea_m_Other:
        }
        break;
    case ea_m_A_ind_index: // also for disp8(pc,Xn)
    disp8:
        switch (dst_ea_m) {
        case ea_m_Dn:
            return { 4, 9, 11 };
        case ea_m_An:
            return { 4, 9, 11 };
        case ea_m_A_ind:
            return { 7, 10, 13 };
        case ea_m_A_ind_post:
            return { 7, 10, 13 };
        case ea_m_A_ind_pre:
            return { 7, 10, 13 };
        case ea_m_A_ind_disp16:
            return { 7, 10, 15 };
        case ea_m_A_ind_index:
            return { 9, 12, 15 };
            // case ea_m_Other:
        }
        break;
    case ea_m_Other:
        switch (i.arg(0).val() & ea_xn_mask) {
        //case ea_other_abs_w:
        //case ea_other_abs_l:
         case ea_other_pc_disp16:
            goto disp16;
        case ea_other_pc_index:
            goto disp8;
        case ea_other_imm: {
            const bool w = i.opsize() != 'l';
            // XXX: Some of these are suspicious...
            switch (dst_ea_m) {
            case ea_m_Dn:
                return { 0, w ? 4 : 6, w ? 3 : 5 };
            case ea_m_An:
                return { 0, w ? 4 : 6, w ? 3 : 5 };
            case ea_m_A_ind:
                return { 3, w ? 6 : 8, w ? 5 : 7 };
            case ea_m_A_ind_post:
                return { 4, w ? 6 : 8, w ? 8 : 7 };
            case ea_m_A_ind_pre:
                return { 3, w ? 7 : 9, w ? 6 : 8 };
            case ea_m_A_ind_disp16:
                return { 3, w ? 7 : 9, w ? 7 : 9 };
            case ea_m_A_ind_index:
                return { 4, w ? 7 : 9, w ? 9 : 11 };
                // case ea_m_Other:
            }
        }
        }
    }


    std::ostringstream oss;
    oss << "TODO: move_cost_020 for " << i;
    throw std::runtime_error { oss.str() };
}

cycle_counts arit_cost_020(const instruction& i)
{
    assert(num_ea(i.op()) == 2);
    
    const auto dst_ea_m = i.arg(1).val() >> ea_m_shift;
    const auto base_cost = dst_ea_m == ea_m_Dn || dst_ea_m == ea_m_An ? cycle_counts { 0, 2, 3 } : cycle_counts { 3, 4, 6 };

    if (i.arg(0).val() == ea_immediate && !has_embeeded_immediate(i)) {
        return base_cost + fetch_immediate_effective_address_cost(i.arg(1), i.opsize());
    }

    return base_cost + fetch_effective_address_cost(i);
}

cycle_counts cost_020(const instruction& i)
{
    const bool is_imm = num_ea(i.op()) && i.arg(0).val() == ea_immediate;

    switch (i.op()) {
    case opcode::move:
        return move_cost_020(i);
    case opcode::moveq:
        return { 0, 2, 3 };
    case opcode::swap:
        return { 1, 4, 4 };
    case opcode::neg:
    case opcode::not_:
    case opcode::tst:
        assert(num_ea(i.op()) == 1);
        if ((i.arg(0).val() >> ea_m_shift) == ea_m_Dn)
            return { 0, 2, 3 };
        return cycle_counts { 3, 4, 6 } + fetch_effective_address_cost(i);
    case opcode::cmp:
        // TODO: CMPI/CMPA have different cost
        if (is_imm || (i.arg(1).val() >> ea_m_shift) == ea_m_An)
            break;
        [[fallthrough]];
    case opcode::add:
    case opcode::addq:
    case opcode::and_:
    case opcode::eor:
    case opcode::or_:
    case opcode::sub:
    case opcode::subq:
        return arit_cost_020(i);
    case opcode::muls:
    case opcode::mulu: {
        if (i.opsize() != 'l')
            return cycle_counts { 25, 27, 28 }  + fetch_effective_address_cost(i);
        else
            return  cycle_counts { 41, 43, 44 } + fetch_immediate_effective_address_cost(i.arg(0), is_imm ? 'l' : 'w');
    case opcode::divu:
        if (i.opsize() != 'l')
            return cycle_counts { 42, 44, 44 } + fetch_effective_address_cost(i);
        else
            return cycle_counts { 76, 78, 79 } + fetch_immediate_effective_address_cost(i.arg(0), is_imm ? 'l' : 'w');
    case opcode::divs:
        if (i.opsize() != 'l')
            return cycle_counts { 54, 56, 57 } + fetch_effective_address_cost(i);
        else
            return cycle_counts { 88, 90, 91 } + fetch_immediate_effective_address_cost(i.arg(0), is_imm ? 'l' : 'w');
    }
    }

    if (is_branch(i.op()) || i.op() == opcode::dbra) {
        // Assume taken (otherwise Bcc.B 1/4/5 Bcc.W 3/6/7 Bcc.L 3/6/9)
        return { 3, 6, 9 };

    }

    if (is_shift_rot(i.op()) && num_ea(i.op()) == 2 && (i.arg(1).val() >> ea_m_shift) == ea_m_Dn) {
        assert(i.arg(0).val() == ea_immediate || (i.arg(0).val() >> ea_m_shift) == ea_m_Dn);
        switch (i.op()) {
        case opcode::lsl:
        case opcode::lsr:
            return i.arg(0).val() == ea_immediate ? cycle_counts { 1, 4, 4 } : cycle_counts { 3, 6, 6 };
        case opcode::asl:
        case opcode::rol:
        case opcode::ror:
            return { 5, 8, 8 };
        case opcode::asr:
            return { 3, 6, 6 };
        }

    }

    std::ostringstream oss;
    oss << "TODO: cost_020 for " << i;
    throw std::runtime_error { oss.str() };
}

} // unnamed namespace

class cpu_model_020 : public cpu_model
{
public:
    explicit cpu_model_020(std::ostream& os, const std::vector<instruction>& instructions)
        : os_ { os }
        , instructions_ { instructions }
    {
    }

    double simulate(int unroll, bool print) override;

public:
    std::ostream& os_;
    const std::vector<instruction>& instructions_;
};

double cpu_model_020::simulate(int unroll, bool print)
{
    constexpr size_t print_width = 40;
    cycle_counts total {};
    for (const auto& inst : instructions_) {
        const auto cost = cost_020(inst);
        if (print) {
            os_ << "\t" << with_width(inst, print_width) << "\t; " << cost;
            if (is_branch(inst.op()) || inst.op() == opcode::dbra)
                os_ << " (assuming taken)";
            os_ << "\n";
        }
        total += cost;
    }
    if (print)
        os_ << "\t" << std::string(print_width, ' ') << "\t; " << total << "\n";
    return total.cache * (unroll + 1);
}


std::unique_ptr<cpu_model> make_cpu_model_020(std::ostream& os, const std::vector<instruction>& instructions)
{
    return std::make_unique<cpu_model_020>(os, instructions);
}
