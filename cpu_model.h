#ifndef CPU_MODEL_H
#define CPU_MODEL_H

class cpu_model {
public:
    virtual ~cpu_model() {}
    virtual double simulate(int unroll, bool print) = 0;
};

#endif
