#ifndef _PERCEPTRON_
#define _PERCEPTRON_

#include <cstdlib>
#include <vector>

#include "base/types.hh"
#include "cpu/pred/bpred_unit.hh"
#include "cpu/pred/sat_counter.hh"
#include "debug/PErceptron.hh"
#include "params/Perceptron.hh"

class Perceptron : public BPredUnit
{
public:

  Perceptron(const PerceptronParams *params);
  bool lookup(ThreadID tid, Addr branch_addr, void * &bp_history);
  void uncondBranch(ThreadID tid, Addr pc, void * &bp_history);
  void btbUpdate(ThreadID tid, Addr branch_addr, void * &bp_history);
  void update(ThreadID tid, Addr branch_addr,
              bool taken, void *bp_history,bool squashed);
  void squash(ThreadID tid, void *bp_history);
  unsigned getGHR(ThreadID tid, void *bp_history) const;

private:

  unsigned globalPredictorSize;
  std::vector<unsigned> globalHistory;
  unsigned globalHistoryBits;
  unsigned globalHistoryMask;
  unsigned historyRegisterMask;
  unsigned numberOfPerceptrons;
  unsigned theta;
  std::vector<std::vector<unsigned>> weights;

  void updateGlobalHistTaken(ThreadID tid);
  void updateGlobalHistNotTaken(ThreadID tid);
  struct BPHistory {
        unsigned globalHistory;
        bool globalPredTaken;
        bool globalUsed;
  };



};

#endif
