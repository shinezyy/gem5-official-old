#ifndef _MYPERCEPTRON_
#define _MYPERCEPTRON_

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

#include "base/types.hh"
#include "cpu/pred/bpred_unit.hh"
#include "cpu/pred/sat_counter.hh"
#include "params/MyPerceptron.hh"

enum hash_type{
    MODULO,
    BITWISE_XOR,
    IPOLY,
    PRIME_MODULO,
    PRIME_DISPLACEMENT
};

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

        int getIndex(hash_type type, Addr branch_addr,\
                uint64_t global_history);

        // Number of perceptrons(or size of PHT)
        unsigned globalPredictorSize;

        // Global histories of all threads
        std::vector<unsigned> globalHistory;

        // Number of bits used to index the perceptrons
        unsigned globalHistoryBits;

        unsigned historyRegisterMask;

        // Size of each perceptron
        unsigned sizeOfPerceptrons;

        // Pseudo-tagging
        unsigned pseudoTaggingBit;

        // Indexing method
        std::string indexMethod;

        // Hash type
        hash_type hType;

        // Threshold
        unsigned theta;

        // Weights of all perceptrons
        std::vector<std::vector<int>> weights;

        // Weights for pseudo-tagging bits
        std::vector<std::vector<int>> pweights;

        // Bits used to store each weight
        unsigned bitsPerWeight;

        // Maximum value of each weight
        unsigned maxWeight;

        // Learning rate
        unsigned lambda;

        // Dynamic threshold counter bits
        unsigned thresholdCounterBits;

        // Threshold counter
        SatCounter TC;

        void updateGlobalHistTaken(ThreadID tid);
        void updateGlobalHistNotTaken(ThreadID tid);



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
