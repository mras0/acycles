#include "cpu_model_060.h"
#include "instruction.h"
#include "util.h"
#include <ostream>

// TODO: Model constraits
// - Whether the instruction can be dispatched in the sOEP
// - Conlficts
// - When a register is ready for addressing
// - Ifetch: https://eab.abime.net/showthread.php?t=111352&page=2 https://eab.abime.net/showthread.php?t=58743 (also info on branch prediction)
// - Indirect address modes (not supported probably ever)
// - Handle e.g. add.l (a0),(a1) // 2 reads + write
// etc.

// d4=1
// divu.l d4,d0 => 22.8 cycles in loop, 21.9 unrolled

namespace {

bool soep_ea_ok(const ea& e)
{
    switch (e.val() >> ea_m_shift) {
    case ea_m_Dn:
    case ea_m_An:
    case ea_m_A_ind:
    case ea_m_A_ind_post:
    case ea_m_A_ind_pre:
    case ea_m_A_ind_disp16:
    case ea_m_A_ind_index:
        return true;
    case ea_m_Other:
        switch (e.val() & ea_xn_mask) {
        case ea_other_abs_w:
        case ea_other_abs_l:
        case ea_other_imm:
            return true;
            // PC relative addressing/indirect modes
        case ea_other_pc_disp16:
        case ea_other_pc_index:
            return false;
        }
        break;
    }
    std::ostringstream oss;
    oss << "TODO: implement soep_ea_ok for " << e;
    throw std::runtime_error(oss.str());
}

} // unnamed namespace

class cpu_model_060 : public cpu_model {
public:
    explicit cpu_model_060(std::ostream& os, const std::vector<instruction>& instructions)
        : os_ { os }
        , instructions_ { instructions }
    {
    }

    double simulate(int unroll, bool print) override;

private:
    struct reg_change {
        int cycle;
        const instruction* inst;
    };
    struct change_use_stall {
        eareg reg;
        int cycles;
    };

    std::ostream& os_;
    const std::vector<instruction>& instructions_;
    int cycle_;
    int unroll_;
    size_t pos_;
    reg_change last_register_change_[16]; // d0..d7/a0..a7

    bool done() const
    {
        return pos_ == (unroll_ + 1) * instructions_.size();
    }

    const instruction* peek() const
    {
        return !done() ? &instructions_[pos_ % instructions_.size()] : nullptr;
    }

    const instruction& get()
    {
        assert(!done());
        return instructions_[pos_++ % instructions_.size()];
    }

    std::string soep_ok(const instruction& p, const instruction& s) const;
    void update_register_change(const instruction& i);
    change_use_stall check_change_use(const instruction& i) const;
    change_use_stall check_change_use(const ea& e) const;
    change_use_stall calc_stall(const eareg& e, int cycles) const;
};

double cpu_model_060::simulate(int unroll, bool print)
{
    cycle_ = 1;
    pos_ = 0;
    unroll_ = unroll;
    for (auto& c : last_register_change_)
        c.inst = nullptr;

    constexpr size_t print_width = 40;
    while (!done()) {
        const auto& poep_ins = get();
        int stall_cycles = 0;
        if (auto stall = check_change_use(poep_ins); stall.cycles) {
            if (print)
                os_ << "\t; pOEP Change/use stall for " << stall.cycles << " cycles waiting for " << stall.reg << "\n";
            stall_cycles += stall.cycles;
        }

        // TODO: The instruction isn't even fetched! https://eab.abime.net/showthread.php?t=111352&page=2
        if (is_branch(poep_ins.op())) {
            if (print) {
                os_ << "\t; Assuming correctly predicated (taking 0 cycles)\n";
                os_ << "\t" << with_width(poep_ins, print_width) << "\n";
            }
            continue;
        }

        // TODO: Instruction is available
        // TODO: 10.1.1 Dispatch Test 1: sOEP Opword and Required Extension Words Are Valid
        auto soep_ins = peek();
        std::string reason;
        if (soep_ins) {
            reason = soep_ok(poep_ins, *soep_ins);
            if (reason.empty()) {
                // Change/use for address operations
                // Seems to better match actual behavior having this here rather than in soep_ok
                if (auto stall = check_change_use(*soep_ins); stall.cycles) {
                    if (print)
                        os_ << "\t; sOEP Change/use stall for " << stall.cycles << " cycles waiting for " << stall.reg << "\n";
                    assert(stall_cycles == 0); // Only one of the 2 OEP's can be stalling
                    stall_cycles += stall.cycles;
                }
            }
        }

        const int icycles = poep_ins.cylces();
        const int tcycles = icycles + stall_cycles;
        assert(icycles > 0);
        if (print) {
            os_ << "\t; Cycle " << cycle_;
            if (tcycles > 1)
                os_ << "-" << (cycle_ + tcycles - 1);
            os_ << "\n\t" << with_width(poep_ins, print_width) << "; pOEP\n";
        }

        cycle_ += stall_cycles;

        update_register_change(poep_ins);

        // TODO: Multicycle instruction with pOEP-until-last
        if (soep_ins) {
            if (reason.empty()) {
                assert(soep_ins->cylces() == 1);
                if (print)
                    os_ << "\t" << with_width(*soep_ins, print_width) << "; sOEP\n";
                ++pos_;
                update_register_change(*soep_ins);
            } else {
                if (print)
                    os_ << "\t; sOEP idle because " << reason << "\n"; 
            }
        }
        cycle_ += icycles;
    }
    if (print) {
        os_ << "\n\n";
        os_ << (cycle_ - 1) << " cycles";
        if (unroll > 0)
            os_ << " " << (static_cast<double>(cycle_ - 1) / (unroll + 1)) << " per iteration";
        os_ << "\n";
    }
    return static_cast<double>(cycle_ - 1) / (unroll + 1);
}

void cpu_model_060::update_register_change(const instruction& i)
{
    // TODO: (An)+/-(An) can also incur a penalty
    auto r = i.execution_result_reg();
    if (!r)
        return;
    assert(static_cast<int>(*r) < 16);
    auto& rc = last_register_change_[static_cast<int>(*r)];
    rc.cycle = cycle_;
    rc.inst = &i;
}

#define REASON(...) do { std::ostringstream oss; oss << __VA_ARGS__; return oss.str(); } while (0)

std::string cpu_model_060::soep_ok(const instruction& p, const instruction& s) const
{
    // 10.1.2 Dispatch Test 2: Instruction Classification
    if (s.oep_classify() != oep_class::poep_or_soep)
        REASON(s.op() << " is " << s.oep_classify());
    if (p.oep_classify() == oep_class::poep_only)
        REASON(p.op() << " is " << p.oep_classify());

    // 10.1.3 Dispatch Test 3: Allowable Effective Addressing Mode in the sOEP
    for (int i = 0; i < num_ea(s.op()); ++i) {
        const auto& e = s.arg(i);
        if (!soep_ea_ok(e))
            REASON(e << " is not an allowable EA");
    }
    // 10.1.4 Dispatch Test 4: Allowable Operand Data Memory Reference
    if (p.mem_cycles() && s.mem_cycles())
        REASON(s.op() << " also uses memory cycle");
    if (s.mem_cycles() > 1)
        REASON(s.op() << " uses more than one memory cycle");

    auto p_result = p.execution_result_reg();
    //10.1.5 Dispatch Test 5: No Register Conflicts on sOEP.AGU Resources
    //10.1.6 Dispatch Test 6: No Register Conflicts on sOEP.IEE Resources
    if (p_result) {
        auto r = s.need_reg(*p_result);
        if (r) {
            // move.l ...,Rx can be forwarded to sOEP.A/B
            // moveq isn't documented but can as well, also seems like clr.l can, maybe also lea
            const bool is_movel = (p.op() == opcode::move && p.opsize() == 'l') || p.op() == opcode::moveq;
            if (*r != resource::a_b || !is_movel)
                REASON(s << " needs " << *p_result);
        }
    }
    return {};
}
#undef REASON

cpu_model_060::change_use_stall cpu_model_060::check_change_use(const instruction& i) const
{
    const int nea = num_ea(i.op());
    if (nea == 0)
        return {};
    if (auto stall = check_change_use(i.arg(0)); stall.cycles)
        return stall;
    if (nea == 1)
        return {};
    return check_change_use(i.arg(1));
}

cpu_model_060::change_use_stall cpu_model_060::check_change_use(const ea& e) const
{
    switch (e.val() >> ea_m_shift) {
    case ea_m_Dn:
    case ea_m_An:
        return {};
    case ea_m_A_ind:
    case ea_m_A_ind_post:
    case ea_m_A_ind_pre:
    case ea_m_A_ind_disp16:
        return calc_stall(static_cast<eareg>(8 + (e.val() & ea_xn_mask)), 2);
    case ea_m_A_ind_index: {
        const auto bew = get_brief_extension_word(e);
        if (auto stall = calc_stall(bew.base, 2); stall.cycles)
            return stall;
        return calc_stall(bew.index, bew.long_size && (bew.scale == 1 || bew.scale == 4) ? 2 : 3);
    }
    case ea_m_Other:
        switch (e.val() & ea_xn_mask) {
        case ea_other_abs_w:
        case ea_other_abs_l:
        case ea_other_pc_disp16:
        case ea_other_imm:
            return {};
        //case ea_other_pc_index: // TODO
        }
    }
    std::ostringstream oss;
    oss << "TODO: implement check_change_use for " << e;
    throw std::runtime_error(oss.str());
}

cpu_model_060::change_use_stall cpu_model_060::calc_stall(const eareg& r, int cycles) const
{
    // TODO: Check optimization in 10.2.3 [iff cycles == 2]
    const int idx = static_cast<int>(r);
    if (idx >= 16)
        return {};
    const auto& s = last_register_change_[idx];
    if (!s.inst)
        return {};
    const int ago = cycle_ - 1 - s.cycle;
    if (cycles <= ago)
        return {};
    return { r, cycles - ago };
}

std::unique_ptr<cpu_model> make_cpu_model_060(std::ostream& os, const std::vector<instruction>& instructions)
{
    return std::make_unique<cpu_model_060>(os, instructions);
}