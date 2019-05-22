#include <utility>

#ifndef __SNN__
#define __SNN__

#include <cstdio>
#include <random>
#include <string>

#include <boost/dynamic_bitset.hpp>

#include "base/types.hh"
#include "cpu/pred/bpred_unit.hh"
#include "cpu/pred/sat_counter.hh"
#include "params/SNN.hh"

class SNN: public BPredUnit{
  public:

    explicit SNN(const SNNParams *params);
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
        const int32_t shadowPredVal;

        explicit BPHistory(boost::dynamic_bitset<> &ghr,
                           boost::dynamic_bitset<> local_history,
                           uint32_t tableIndex,
                           bool taken, uint64_t id,
                           int32_t val, int32_t shadow_val)
                : globalHistory(ghr),
                  localHistory(std::move(local_history)),
                  tableIndex(tableIndex),
                  predTaken(taken),
                  predictionID(id),
                  predictionValue(val),
                  shadowPredVal(shadow_val)
        {}

        ~BPHistory() = default;
    };


    struct Neuron {
        enum BlockType{
            InvalidBlock = 0,
            convBlock,
            maxBlock,
            NumBlockTypes
        };

        struct SparseSeg {
            int blockType;
            uint32_t ptr;
            SignedSatCounter weight;
            boost::dynamic_bitset<> convKernel;
        };

        bool valid;
        bool probing{false};
        const uint32_t denseGHLen;

        const uint32_t sparseGHSegLen;
        const uint32_t sparseGHNSegs;

        boost::dynamic_bitset<> localHistory;

        std::vector<SignedSatCounter> denseWeights;

        uint32_t activeStart;
        std::vector<SignedSatCounter> activeWeights;
        uint32_t activeTerm;
        uint32_t activeTime;

        std::vector<SparseSeg> sparseSegs;

        std::vector<SignedSatCounter> shadowWeights;

        explicit Neuron (const SNNParams *params);

        int32_t predict(boost::dynamic_bitset<> &ghr);

        int32_t shadowPredict(boost::dynamic_bitset<> &ghr);

        void fit(BPHistory *bp_history, bool taken);

        void shadowFit(BPHistory *bp_history, bool taken);

        int32_t theta;
        int32_t shadowTheta;

        // 1 -> 1; 0 -> -1; bool to signed
        static int b2s(bool);

        void dump() const;
    };

    std::vector<Neuron> table;

    uint32_t computeIndex(Addr addr);

    const uint32_t probeIndex = 7;

    uint64_t predictionID{1};

    uint64_t InvalidPredictionID = 0;

    void dumpParameters() const;

    void tryDump();

    Tick nextDumpTick{0};
};

#endif
