#ifndef INSTRUCTION_H_INCLUDED
#define INSTRUCTION_H_INCLUDED

#include <ostream>
#include <string>
#include <cassert>
#include <optional>
#include "ea.h"

// Name, RMW, #EA, Cycles, Classification
#define OPCODES(X) \
    X(and_ ,true  ,2 ,1 ,poep_or_soep)  \
    X(add  ,true  ,2 ,1 ,poep_or_soep)  \
    X(addq ,true  ,2 ,1 ,poep_or_soep)  \
    X(addx ,true  ,2 ,1 ,poep_only   )  \
    X(asl  ,true  ,2 ,1 ,poep_or_soep)  \
    X(asr  ,true  ,2 ,1 ,poep_or_soep)  \
    X(bra  ,false ,1 ,1 ,poep_only   )  \
    X(bhi  ,false ,1 ,1 ,poep_only   )  \
    X(bls  ,false ,1 ,1 ,poep_only   )  \
    X(bcc  ,false ,1 ,1 ,poep_only   )  \
    X(bcs  ,false ,1 ,1 ,poep_only   )  \
    X(bne  ,false ,1 ,1 ,poep_only   )  \
    X(beq  ,false ,1 ,1 ,poep_only   )  \
    X(bvc  ,false ,1 ,1 ,poep_only   )  \
    X(bvs  ,false ,1 ,1 ,poep_only   )  \
    X(bpl  ,false ,1 ,1 ,poep_only   )  \
    X(bmi  ,false ,1 ,1 ,poep_only   )  \
    X(bge  ,false ,1 ,1 ,poep_only   )  \
    X(blt  ,false ,1 ,1 ,poep_only   )  \
    X(bgt  ,false ,1 ,1 ,poep_only   )  \
    X(ble  ,false ,1 ,1 ,poep_only   )  \
    X(cmp  ,true  ,2 ,1 ,poep_or_soep)  \
    X(dbra ,false ,2 ,1 ,poep_only   )  \
    X(divu ,true  ,2 ,0 ,poep_only   )  \
    X(divs ,true  ,2 ,0 ,poep_only   )  \
    X(eor  ,true  ,2 ,1 ,poep_or_soep)  \
    X(lsl  ,true  ,2 ,1 ,poep_or_soep)  \
    X(lsr  ,true  ,2 ,1 ,poep_or_soep)  \
    X(move ,false ,2 ,1 ,poep_or_soep)  \
    X(moveq,false ,2 ,1, poep_or_soep)  \
    X(not_ ,true  ,1 ,1, poep_or_soep)  \
    X(neg  ,true  ,1 ,1, poep_or_soep)  \
    X(mulu ,true  ,2 ,2 ,poep_only   )  \
    X(muls ,true  ,2 ,2 ,poep_only   )  \
    X(or_  ,true  ,2 ,1 ,poep_or_soep)  \
    X(rol  ,true  ,2 ,1 ,poep_or_soep)  \
    X(ror  ,true  ,2 ,1 ,poep_or_soep)  \
    X(rts  ,false ,0 ,1 ,poep_or_soep)  \
    X(sub  ,true  ,2 ,1 ,poep_or_soep)  \
    X(subq ,true  ,2 ,1 ,poep_or_soep)  \
    X(subx ,true  ,2 ,1 ,poep_only   )  \
    X(swap ,true  ,1 ,1 ,poep_only   )  \
    X(tst  ,false ,1 ,1, poep_or_soep)  \

enum class opcode {
#define X(o, rmw, nea, cycles, classi) o,
    OPCODES(X)
#undef X
};

std::ostream& operator<<(std::ostream& os, opcode);
opcode opcode_from_string(const std::string& str);
int num_ea(opcode op);
bool valid_size(char ch);
bool is_branch(opcode op);
bool is_shift_rot(opcode op);

enum class oep_class {
    poep_or_soep,
    poep_only,
    poep_until_last,
    poep_but_allows_soep,
};
std::ostream& operator<<(std::ostream& os, oep_class);

enum class resource {
    a_b, base, index
};
std::ostream& operator<<(std::ostream& os, resource);

class instruction {
public:
    explicit instruction(opcode op, char sz)
        : op_ { op }
        , size_ { sz }
        , ea_ {}
    {
        assert(valid_size(sz));
        assert(num_ea(op) == 0);
    }
    explicit instruction(opcode op, char sz, const ea& ea1)
        : op_ { op }
        , size_ { sz }
        , ea_ { ea1 }
    {
        assert(valid_size(sz));
        assert(num_ea(op) == 1);
    }

    explicit instruction(opcode op, char sz, const ea& ea1, const ea& ea2)
        : op_ { op }
        , size_ { sz }
        , ea_ { ea1, ea2 }
    {
        assert(valid_size(sz));
        assert(num_ea(op) == 2);
    }


    opcode op() const
    {
        return op_;
    }

    char opsize() const
    {
        return size_;
    }

    const ea& arg(int n) const
    {
        assert(n >= 0 && n < num_ea(op_));
        return ea_[n];
    }

    oep_class oep_classify() const;

    int mem_cycles() const;
    int cylces() const;
    std::optional<eareg> execution_result_reg() const;
    std::optional<resource> need_reg(eareg r) const;

    int num_words() const;

private:
    opcode op_;
    char size_;
    ea ea_[2];
};
std::ostream& operator<<(std::ostream& os, const instruction&);
bool has_embeeded_immediate(const instruction& ins); // If the immediate is embedded in the instruction

#endif
