#include <utility>

#ifndef __ZPERCEPTRON__
#define __ZPERCEPTRON__

#include <cstdio>
#include <random>
#include <string>

#include <boost/dynamic_bitset.hpp>

#include "base/types.hh"
#include "cpu/pred/bpred_unit.hh"
#include "cpu/pred/sat_counter.hh"
#include "params/ZPerceptron.hh"

class ZPerceptron: public BPredUnit{
  public:

    explicit ZPerceptron(const ZPerceptronParams *params);
    bool lookup(ThreadID tid, Addr branch_addr, void * &bp_bistory) override;
    void uncondBranch(ThreadID tid, Addr pc, void * &bp_history) override;
    void btbUpdate(ThreadID tid, Addr branch_addr,
            void * &bp_history) override;
    void update(ThreadID tid, Addr branch_addr, bool taken,\
                void *bp_history, bool squashed) override;
    void squash(ThreadID tid, void * bp_history) override;
    unsigned getGHR(ThreadID tid, void * bp_history) const override;

  private:
    void updateGHR(ThreadID tid, bool taken);

    const uint32_t globalHistoryLen;
    const uint32_t tableSize;

    const boost::dynamic_bitset<> emptyLocalHistory;

    std::vector<boost::dynamic_bitset<> > globalHistory;

    uint32_t InvalidTableIndex = static_cast<uint32_t>(~0);

    struct BPHistory {
        boost::dynamic_bitset<> globalHistory;
        boost::dynamic_bitset<> localHistory;
        uint32_t tableIndex;
        bool predTaken;
        const uint64_t predictionID;
        const int32_t predictionValue;

        explicit BPHistory(boost::dynamic_bitset<> &ghr,
                           boost::dynamic_bitset<> local_history,
                           uint32_t tableIndex,
                           bool taken, uint64_t id,
                           int32_t val)
                : globalHistory(ghr),
                  localHistory(std::move(local_history)),
                  tableIndex(tableIndex),
                  predTaken(taken),
                  predictionID(id),
                  predictionValue(val)
        {}

        ~BPHistory() = default;
    };


    struct Neuron {
        bool valid;
        bool probing{false};
        const uint32_t globalHistoryLen;

        boost::dynamic_bitset<> localHistory;

        std::vector<SignedSatCounter> weights;

        explicit Neuron (const ZPerceptronParams *params);

        int32_t predict(boost::dynamic_bitset<> &ghr);

        void fit(BPHistory *bp_history, bool taken);

        int32_t theta;

        // 1 -> 1; 0 -> -1; bool to signed
        static int b2s(bool);

        void dump() const;
    };

    std::vector<Neuron> table;

    uint32_t computeIndex(Addr addr);

    const uint32_t probeIndex = 89;

    uint64_t predictionID{1};

    uint64_t InvalidPredictionID = 0;

    void dumpParameters() const;

    void tryDump();

    Tick nextDumpTick{0};
};

#endif
