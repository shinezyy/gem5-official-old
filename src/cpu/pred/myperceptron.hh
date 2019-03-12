#ifndef _MYPERCEPTRON_
#define _MYPERCEPTRON_

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <vector>

#include "base/types.hh"
#include "cpu/pred/bpred_unit.hh"
#include "cpu/pred/sat_counter.hh"
#include "params/MyPerceptron.hh"

class MyPerceptron : public BPredUnit{
    public:

        MyPerceptron(const MyPerceptronParams *params);
        bool lookup(ThreadID tid, Addr branch_addr, void * &bp_bistory);
        void uncondBranch(ThreadID tid, Addr pc, void * &bp_history);
        void btbUpdate(ThreadID tid, Addr branch_addr, void * &bp_history);
        void update(ThreadID tid, Addr branch_addr, bool taken,
                void *bp_history, bool squashed);
        void squash(ThreadID tid, void * bp_history);
        unsigned getGHR(ThreadID tid, void * bp_history) const;

    private:

        // Number of perceptrons(or size of PHT)
        unsigned globalPredictorSize;

        // Global histories of all threads
        std::vector<unsigned> globalHistory;

        // Number of bits used to index the perceptrons
        unsigned globalHistoryBits;

        unsigned historyRegisterMask;

        // Size of each perceptron
        unsigned sizeOfPerceptrons;

        // Threshold
        unsigned theta;

        // Weights of all perceptrons
        std::vector<std::vector<int>> weights;

        void updateGlobalHistTaken(ThreadID tid);
        void updateGlobalHistNotTaken(ThreadID tid);

        // void print_perceptron(ofstream *file, const vector<int> perceptron);


        struct BPHistory {
            unsigned globalHistory;
            bool globalPredTaken;
            bool globalUsed;
        };

// Below are vectors used for tuning
        std::vector<bool> stat_perceptrons;
        std::vector<Addr> addr_record;
        std::vector<unsigned> history_record;

};

#endif
