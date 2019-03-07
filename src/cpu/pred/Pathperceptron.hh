#ifndef __PARAMS__PathPerceptron__
#define __PARAMS__PathPerceptron__

class PathPerceptron;

#include <cstddef>

#include "base/types.hh"
#include "params/BranchPredictor.hh"

struct PathPerceptronParams
    : public BranchPredictorParams
{
    PathPerceptron * create();
    size_t globalPredictorSize;
    size_t numberOfPerceptrons;
};

#endif // __PARAMS__PathPerceptron__
