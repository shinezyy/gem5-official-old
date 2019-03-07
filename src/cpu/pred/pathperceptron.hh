#ifndef _PATHPERCEPTRON_
#define _PATHPERCEPTRON_

#include <cstdlib>
#include <vector>

#include "base/types.hh"
#include "cpu/pred/bpred_unit.hh"
#include "cpu/pred/sat_counter.hh"
#include "debug/PAthPerceptron.hh"
#include "params/PathPerceptron.hh"

class PathPerceptron : public BPredUnit
{
public:

  PathPerceptron(const PathPerceptronParams *params);
  bool lookup(ThreadID tid, Addr branch_addr, void * &bp_history);
  void uncondBranch(ThreadID tid, Addr pc, void * &bp_history);
  void btbUpdate(ThreadID tid, Addr branch_addr, void * &bp_history);
  void update(ThreadID tid, Addr branch_addr, bool taken, void *bp_history,
              bool squashed);
  void squash(ThreadID tid, void *bp_history);
  unsigned getGHR(ThreadID tid, void *bp_history) const;

private:



  unsigned globalPredictorSize;
  std::vector<unsigned> globalHistory;
  std::vector<unsigned> selectiveGlobalHistory;
  std::vector<unsigned> globalRegister;
  std::vector<unsigned> selectiveGlobalRegister;
  std::vector<unsigned> path;
  unsigned globalHistoryBits;
  unsigned globalHistoryMask;
  unsigned historyRegisterMask;
  unsigned numberOfPerceptrons;
  unsigned theta;
  unsigned max_weight;
  unsigned min_weight;
  std::vector<std::vector<unsigned>> weights;
  void updatePath(Addr branch_addr);
  struct BPHistory {
        unsigned globalHistory;
        bool globalPredTaken;
        bool globalUsed;
  };

  unsigned saturatedUpdate (unsigned weight, bool inc);






};

#endif
