#ifndef _MYPERCEPTRON_
#define _MYPERCEPTRON_

#include <cstdlib>
#include <vector>

#include "base/types.hh"
#include "cpu/pred/bpred_unit.hh"
#include "cpu/pred/sat_counter.hh"
#include "params/Myperceptron.hh"

class MyPerceptron : public BPredUnit{
    public:
        MyPerceptron(const MyPerceptronParams *params);
        bool lookup(ThreadID tid, Addr branch_addr, void * &bp_bistory);








#endif
