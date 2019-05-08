#include <utility>

#ifndef _MYPERCEPTRON_
#define _MYPERCEPTRON_

#include <cstdio>
#include <random>
#include <string>

#include <boost/dynamic_bitset.hpp>

#include "base/types.hh"
#include "cpu/pred/bpred_unit.hh"
#include "cpu/pred/sat_counter.hh"
#include "params/OGBBP.hh"

class OGBBP: public BPredUnit{
  public:

    OGBBP(const OGBBPParams *params);
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

    std::random_device rd;

    std::mt19937 mt;

    const uint32_t globalHistoryLenLog;
    const uint32_t globalHistoryLen;
    const uint32_t treeHeight;
    const uint32_t localHistoryLen;
    const uint32_t tableSize;
    const uint32_t factorBits;
    const uint32_t nTrees;
    const uint32_t ctrBits;

    const boost::dynamic_bitset<> emptyLocalHistory;

    std::vector<boost::dynamic_bitset<> > globalHistory;

    uint32_t InvalidTableIndex = static_cast<uint32_t>(~0);

    struct BPHistory {
        boost::dynamic_bitset<> globalHistory;
        boost::dynamic_bitset<> localHistory;
        uint32_t tableIndex;
        bool predTaken;

        explicit BPHistory(boost::dynamic_bitset<> &ghr,
                           boost::dynamic_bitset<> local_history,
                           uint32_t tableIndex,
                           bool taken)
                : globalHistory(ghr),
                  localHistory(std::move(local_history)),
                  tableIndex(tableIndex),
                  predTaken(taken)
        {}

        ~BPHistory() = default;
    };

    struct Tree {
        std::vector<uint32_t> treeNodes; // pointer to history bits
        std::vector<SignedSatCounter> leaves;

        Tree(uint32_t treeHeight, uint32_t ctrBits)
                : treeNodes(power(2, treeHeight) - 1),
                  leaves(power(2, treeHeight), SignedSatCounter(ctrBits, 0))
        {}

        void reRand(std::mt19937 &mt, uint32_t l, uint32_t r);
    };


    struct OGBEntry {
        bool valid;
        const uint32_t localHistoryLen;
        const uint32_t globalHistoryLen;
        const uint32_t nTrees;
        const uint32_t nLocal = 2;
        const uint32_t treeHeight;
        const int32_t base;
        const int32_t eta;
        const int32_t zoomFactor;

        SignedSatCounter baseValue;

        boost::dynamic_bitset<> localHistory;
        std::vector<Tree> trees;
        std::vector<SignedSatCounter> sigma;

        OGBEntry (uint32_t localHistoryLen, uint32_t globalHistoryLen,
                  uint32_t nTrees, uint32_t factorBits,
                  uint32_t treeHeight, uint32_t ctrBits)
                : valid(false),
                  localHistoryLen(localHistoryLen),
                  globalHistoryLen(globalHistoryLen),
                  nTrees(nTrees),
                  treeHeight(treeHeight),
                  base(1),
                  eta(6),
                  zoomFactor(static_cast<const int32_t>(
                          (nTrees + 1) * power(2, ctrBits - 1))),

                  baseValue(ctrBits, 0),
                  localHistory(localHistoryLen),
                  trees(nTrees, Tree(treeHeight, ctrBits)),
                  sigma(nTrees, SignedSatCounter(factorBits, 0))
        {}

        void init(std::mt19937 &mt);

        float predict(boost::dynamic_bitset<> &ghr);

        void gradient_descent(boost::dynamic_bitset<> &ghr, bool taken);
    };

    std::vector<OGBEntry> table;

    uint32_t computeIndex(Addr addr);

};

#endif
