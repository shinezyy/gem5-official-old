#include "cpu/pred/perceptron.hh"

#include <iostream>

#include "base/bitfield.hh"
#include "base/intmath.hh"
#include "debug/PErceptron.hh"

//same format as other perceptrons
Perceptron::Perceptron(const PerceptronParams *params)
  : BPredUnit(params),
        globalPredictorSize(params->globalPredictorSize),
        globalHistory(params->numThreads, 0),
        globalHistoryBits(ceilLog2(params->globalPredictorSize)),
    numberOfPerceptrons(params->numberOfPerceptrons)
{
  if (!isPowerOf2(globalPredictorSize)) {
        fatal("Invalid global predictor size!\n");
  }
  historyRegisterMask = mask(globalHistoryBits);
//  numberOfPerceptrons = 20;
  theta = 1.93 * globalPredictorSize + 14; //based on paper for threshold
  weights.assign(numberOfPerceptrons,std::vector<unsigned>\
                 (globalPredictorSize + 1, 0));
}

void Perceptron::updateGlobalHistTaken(ThreadID tid)
{
  globalHistory[tid] = (globalHistory[tid] << 1) | 1;
  globalHistory[tid] = globalHistory[tid] & historyRegisterMask;
}

void Perceptron::updateGlobalHistNotTaken(ThreadID tid)
{
  globalHistory[tid] = (globalHistory[tid] << 1);
  globalHistory[tid] = globalHistory[tid] & historyRegisterMask;
}

void Perceptron::btbUpdate(ThreadID tid, Addr branch_addr, void * &bp_history)
{

    globalHistory[tid] &= (historyRegisterMask & ~ULL(1));
}

bool Perceptron::lookup(ThreadID tid, Addr branch_addr, void * &bp_history)
{

  unsigned thread_history = globalHistory[tid];
  int currentPerceptron = (branch_addr ^ thread_history) % numberOfPerceptrons;


  int sum = weights[currentPerceptron][0];
  for (int i = 1; i <= globalPredictorSize; i++) {
        if ((thread_history >> (i - 1)) & 1)
          sum += weights[currentPerceptron][i];
        else sum -= weights[currentPerceptron][i];
  }

  bool taken = (sum >= 0);
  //check to see if sum is greater than or equal to 0(threshold)

  BPHistory *history       = new BPHistory;
  history->globalHistory   = globalHistory[tid];
  history->globalPredTaken = taken;
  bp_history = (void *)history;

  return taken;
}

void Perceptron::uncondBranch(ThreadID tid, Addr pc, void * &bp_history)
{

  BPHistory *history       = new BPHistory;
  history->globalHistory   = globalHistory[tid];
  history->globalPredTaken = true;
  history->globalUsed      = true;
  bp_history = static_cast<void *>(history);
  updateGlobalHistTaken(tid);
}

void
Perceptron::update(ThreadID tid, Addr branch_addr,
                   bool taken,void *bp_history, bool squashed)
{
  assert(bp_history);

  unsigned thread_history = globalHistory[tid];
  int curPerceptron = (branch_addr ^ thread_history) % numberOfPerceptrons;


  int sum = weights[curPerceptron][0];
  for (int i = 1; i <= globalPredictorSize; i++) {
        if ((thread_history >> (i - 1)) & 1)
          sum += weights[curPerceptron][i];
        else sum -= weights[curPerceptron][i];
  }


  if (squashed || (abs(sum) <= theta)) {
        if (taken)
                weights[curPerceptron][0] += 1;
        else
                weights[curPerceptron][0] -= 1;


        for (int i = 1; i < globalPredictorSize; i++) {
          if (((thread_history >> (i - 1)) & 1) == taken)
                weights[curPerceptron][i]    += 1;
          else
                weights[curPerceptron][i] -= 1;
        }
  }
  globalHistory[tid] = (globalHistory[tid] << 1) | taken;
  globalHistory[tid] &= historyRegisterMask;
}

void Perceptron::squash(ThreadID tid, void *bp_history)
{
  BPHistory *history = static_cast<BPHistory *>(bp_history);
  globalHistory[tid] = history->globalHistory;
  delete history;
}

unsigned Perceptron::getGHR(ThreadID tid, void *bp_history) const
{
  return static_cast<BPHistory *>(bp_history)->globalHistory;
}

Perceptron* PerceptronParams::create()
{
  return new Perceptron(this);
}
