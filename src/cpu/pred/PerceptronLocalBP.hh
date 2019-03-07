#ifndef __PARAMS__PerceptronLocalBP__
#define __PARAMS__PerceptronLocalBP__

class PerceptronLocalBP;

#include <cstddef>

#include "base/types.hh"
#include "params/BranchPredictor.hh"

struct PerceptronLocalBPParams
    : public BranchPredictorParams
{
    PerceptronLocalBP * create();
    size_t localPercepSize;
    size_t localPredictorSize;
};

#endif // __PARAMS__PerceptronLocalBP__
