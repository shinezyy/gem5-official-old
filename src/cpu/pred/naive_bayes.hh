#include <utility>

#ifndef __NBBP__
#define __NBBP__

#include <cstdio>
#include <random>
#include <string>

#include <boost/dynamic_bitset.hpp>

#include "base/types.hh"
#include "cpu/pred/bpred_unit.hh"
#include "cpu/pred/sat_counter.hh"
#include "params/NBBP.hh"

class NBBP: public BPredUnit{
  public:

    NBBP(const NBBPParams *params);
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

        explicit BPHistory(boost::dynamic_bitset<> &ghr,
                           boost::dynamic_bitset<> local_history,
                           uint32_t tableIndex,
                           bool taken, uint64_t id)
                : globalHistory(ghr),
                  localHistory(std::move(local_history)),
                  tableIndex(tableIndex),
                  predTaken(taken),
                  predictionID(id)
        {}

        ~BPHistory() = default;
    };


    struct NBEntry {

        struct NBCounter {
            const int32_t max;
            const int32_t origin;
            int32_t takenOnT;   // P(will take | has taken)
            int32_t takenOnNT;  // P(will take | has not taken)

            explicit NBCounter (int32_t _max)
                    : max(_max),
                      origin(_max/2),
                      takenOnT(origin),
                      takenOnNT(origin)
            {}

            void shrink(bool taken, float shrink_factor);
        };

        bool valid;
        bool probing{false};
        const uint32_t localHistoryLen;
        const uint32_t globalHistoryLen;

        const uint32_t nLocal;
        const uint32_t nGlobal;

        boost::dynamic_bitset<> localHistory;
        const int32_t counterMax;
        const int32_t origin;

        int32_t prioriProb;
        std::vector<NBCounter> localPosteriorProb;
        std::vector<NBCounter> globalPosteriorProb;

        const float shrinkFactor = 0.5;

        explicit NBEntry (const NBBPParams *params);

        void init();

        double predict(boost::dynamic_bitset<> &ghr);

        void fit(BPHistory *bp_history, bool taken);

    private:
        bool neg_contrib(bool taken, int32_t prob);
    };

    std::vector<NBEntry> table;

    uint32_t computeIndex(Addr addr);

    const uint32_t probeIndex = 310;

    uint64_t predictionID{1};

    uint64_t InvalidPredictionID = 0;

};

#endif
