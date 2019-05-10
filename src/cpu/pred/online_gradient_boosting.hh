#include <utility>

#ifndef _MYPERCEPTRON_
#define _MYPERCEPTRON_

#include <cstdio>
#include <random>
#include <string>
#include <unordered_map>

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

    const uint32_t probeIndex = 310;

  private:

    using myClock = std::chrono::high_resolution_clock;

    myClock::time_point beginning;

    void updateGHR(ThreadID tid, bool taken);

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

    uint64_t InvalidPredictionID = 0;

    uint64_t probe_pred_id = 379500;

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

    struct Tree {
        std::vector<uint32_t> treeNodes; // pointer to history bits
        std::vector<SignedSatCounter> leaves;

        Tree(uint32_t treeHeight, uint32_t ctrBits)
                : treeNodes(power(2, treeHeight) - 1),
                  leaves(power(2, treeHeight), SignedSatCounter(ctrBits, 0))
        {}
    };

    struct OGBEntry {
        bool valid;
        const uint32_t localHistoryLen;
        const uint32_t globalHistoryLen;
        const uint32_t nTrees;
        const uint32_t nLocal;
        const uint32_t treeHeight;
        const float eta;
        const int32_t zoomFactor;

        const std::string _name;

        const std::string name() const {return _name;}

        bool probing{false};

        SignedSatCounter baseValue;

        boost::dynamic_bitset<> localHistory;
        std::vector<Tree> trees;
        std::vector<SatCounter> sigma;
        const int32_t sigma_deno;

        OGBEntry (uint32_t _localHistoryLen, uint32_t _globalHistoryLen,
                  uint32_t _nTrees, uint32_t _nLocal, uint32_t _factorBits,
                  uint32_t _treeHeight, uint32_t _ctrBits)
                : valid(false),
                  localHistoryLen(_localHistoryLen),
                  globalHistoryLen(_globalHistoryLen),
                  nTrees(_nTrees),
                  nLocal(_nLocal),
                  treeHeight(_treeHeight),
                  eta(0.07),
                  zoomFactor(static_cast<const int32_t>(
                          (_nTrees + 1) * power(2, (_ctrBits - 1)/2))),

                  _name("OGBEntry"),

                  baseValue(_ctrBits, 0),
                  localHistory(localHistoryLen),
                  trees(_nTrees, Tree(_treeHeight, _ctrBits)),
                  sigma(_nTrees, SatCounter(_factorBits, 0)),
                  sigma_deno(static_cast<const int32_t>(power(2, _factorBits)))
        {}

        void init(myClock::time_point beginning);

        float predict(boost::dynamic_bitset<> &ghr);

        void gradient_descent(BPHistory *history, bool taken);
    };

    std::vector<OGBEntry> table;

    uint32_t computeIndex(Addr addr);

    uint64_t predictionID{1};

    uint64_t misses{0};

    std::map<Addr, uint32_t> misPredictions{};
};

#endif
