#include "pathperceptron.hh"

//#include <iostream>

#include "base/bitfield.hh"
#include "base/intmath.hh"
#include "debug/PAthPerceptron.hh"

PathPerceptron::PathPerceptron(const PathPerceptronParams *params)
  : BPredUnit(params),
        globalPredictorSize(params->globalPredictorSize),
        globalHistory (params->numThreads, 0),
        selectiveGlobalHistory(params->numThreads, 0),
        globalHistoryBits(ceilLog2(params->globalPredictorSize)),
    numberOfPerceptrons(params->numberOfPerceptrons)
{
  if (!isPowerOf2(globalPredictorSize)) {
        fatal("Invalid global predictor size!\n");
  }


  globalHistoryMask = globalPredictorSize - 1;


  historyRegisterMask = mask(globalHistoryBits);


  if (globalHistoryMask > historyRegisterMask)
        fatal("Global predictor too large for global history bits!\n");


  selectiveGlobalRegister.assign(globalPredictorSize + 1, 0);


  globalRegister.assign(globalPredictorSize + 1, 0);


  //numberOfPerceptrons = 10;


  theta = 2.14 * (globalPredictorSize + 1) + 20.58;

  weights.assign(numberOfPerceptrons,std::vector<unsigned>\
                 (globalPredictorSize + 1, 0));


  max_weight = (1 << (globalHistoryBits - 1)) - 1;
  min_weight = -(max_weight + 1);
}

void
PathPerceptron::btbUpdate(ThreadID tid, Addr branch_addr, void * &bp_history)
{

    globalHistory[tid] &= (historyRegisterMask & ~ULL(1));
}

void PathPerceptron::updatePath(Addr branch_addr)
{
  path.insert(path.begin(), branch_addr);
  if (path.size() > (globalPredictorSize + 1)) path.pop_back();
}

unsigned PathPerceptron::saturatedUpdate (unsigned weight, bool inc) {
  if ( inc && (weight < max_weight)) return weight + 1;
  else if (!inc && (weight > min_weight)) return weight - 1;
  return weight;
}

bool PathPerceptron::lookup(ThreadID tid, Addr branch_addr, void * &bp_history)
{
  updatePath(branch_addr);

  unsigned k_j;

  int curPerceptron = branch_addr % numberOfPerceptrons;
  int y_out         = weights[curPerceptron][0] +
        selectiveGlobalRegister[globalPredictorSize];
  bool result   = (y_out >= 0);

  BPHistory *history = new BPHistory;
  history->globalHistory   = selectiveGlobalHistory[tid];
  history->globalPredTaken = result;
  bp_history = (void *)history;

  std::vector<unsigned> selectiveRegister_new;
  selectiveRegister_new.assign(globalPredictorSize + 1, 0);

  for (int j = 1; j <= globalPredictorSize; j++) {
        k_j = globalPredictorSize - j;
        selectiveRegister_new[k_j + 1] = selectiveGlobalRegister[k_j];
        if (result)
            selectiveRegister_new[k_j + 1] += weights[curPerceptron][j];
        else
            selectiveRegister_new[k_j + 1] -= weights[curPerceptron][j];
  }

  selectiveGlobalRegister    = selectiveRegister_new;
  selectiveGlobalRegister[0] = 0;

  selectiveGlobalHistory[tid] = ((selectiveGlobalHistory[tid] << 1) | result);
  selectiveGlobalHistory[tid] = (selectiveGlobalHistory[tid] &\
                                 historyRegisterMask);
  return result;
}

void PathPerceptron::uncondBranch(ThreadID tid, Addr pc, void * &bp_history)
{

  BPHistory *history = new BPHistory;
  history->globalHistory = selectiveGlobalHistory[tid];
  history->globalPredTaken = true;
  history->globalUsed = true;
  bp_history = static_cast<void *>(history);

  updatePath(pc);
  selectiveGlobalHistory[tid] = ((selectiveGlobalHistory[tid] << 1) | 1);
  selectiveGlobalHistory[tid] &= historyRegisterMask;
}

void PathPerceptron::update(ThreadID tid, Addr branch_addr, bool taken,
                                void *bp_history, bool squashed)
{
  assert(bp_history);
  unsigned k, k_j;
  int curPerceptron = branch_addr % numberOfPerceptrons;
  int y_out         = weights[curPerceptron][0] +
        selectiveGlobalRegister[globalPredictorSize];

  unsigned thread_history = selectiveGlobalHistory[tid];


  std::vector<unsigned> R_prime;
  R_prime.assign(globalPredictorSize + 1, 0);

  for (int j = 1; j <= globalPredictorSize; j++) {
        k_j = globalPredictorSize - j;
        R_prime[k_j + 1] = globalRegister[k_j];

        if (taken) R_prime[k_j + 1] += weights[curPerceptron][j];
        else       R_prime[k_j + 1] -= weights[curPerceptron][j];
  }

  globalRegister    = R_prime;
  globalRegister[0] = 0;


  globalHistory[tid] = ((globalHistory[tid] << 1) | taken);
  globalHistory[tid] &= historyRegisterMask;


  if (squashed || (abs(y_out) <= theta)) {
        if (squashed) {

          selectiveGlobalHistory[tid] = globalHistory[tid];
          selectiveGlobalRegister = globalRegister;
        }

        weights[curPerceptron][0] = saturatedUpdate(
            weights[curPerceptron][0], taken);
        for (int j = 1; j <= globalPredictorSize; j++) {

          k = (path[j % path.size()] % numberOfPerceptrons);
          weights[k][j] = saturatedUpdate(weights[k][j],
              ((thread_history >> j) & 1) == taken);
        }
  }
}

void PathPerceptron::squash(ThreadID tid, void *bp_history)
{
  BPHistory *history = static_cast<BPHistory *>(bp_history);
  selectiveGlobalHistory[tid] = globalHistory[tid];
  selectiveGlobalRegister = globalRegister;
  delete history;
}

unsigned PathPerceptron::getGHR(ThreadID tid, void *bp_history) const
{
  return static_cast<BPHistory *>(bp_history)->globalHistory;
}

PathPerceptron*
PathPerceptronParams::create()
{
  return new PathPerceptron(this);
}
