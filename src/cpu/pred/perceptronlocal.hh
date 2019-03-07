#ifndef __PERCEPTRON_HH__
#define __PERCEPTRON_HH__

#include "base/logging.hh"
#include "base/types.hh"

/**
 * Private perceptron class
 * Implements a perceptron
 */
class Perceptron
{
  public:
    /**
     * Constructor for the perceptron.
     */
    Perceptron()
                : N(1), theta(15), last_output(0)
    { weights = std::vector<int>(N, 0); }

    /**
     * Constructor for the perceptron.
     * @param units How many input neurons the perceptron will have
     */
    Perceptron(size_t units)
                : N(units), theta(1.93*units + 14), last_output(0)
    { weights = std::vector<int>(N, 0); }

    /**
     * Constructor for the perceptron.
     * @param units How many input neurons the perceptron will have
     * @param initial_w Initial value of each weight
     */
    Perceptron(size_t units, int initial_w)
        : N(units), theta(1.93*units + 14), last_output(0)
    { weights = std::vector<int>(N, initial_w); }

        /**
     * Constructor for the perceptron.
     * @param units How many input neurons the perceptron will have
     * @param initial_w Initial value of each weight
         * @param train_threshold
     */
        Perceptron(size_t units, int initial_w, size_t train_threshold)
                : N(units), theta(train_threshold), last_output(0)
        { weights = std::vector<int>(N, initial_w); }

    /**
     * Sets the number of input neurons
     */
    void setSize(size_t _N) {
      N = _N;
      theta = 1.93*N + 14;
    }

    void reset() { weights = std::vector<int>(N, 0); }

    /**
     * Read the counter's value.
     */
    uint8_t read(size_t globalHistory) {
                last_output = weights[0];
                last_input = globalHistory;
                for (size_t i = 0; i < N; ++i) {
                        if ((last_input >> i) & 1) {
                                last_output += weights[i+1];
                        } else {
                                last_output -= weights[i+1];
                        }
                }
                return last_output;
        }

        /**
         * Iteratively train the perceptron
         */
        void train(bool taken) {
                if (((last_output >= 0) && !taken) ||
                     (abs(last_output)  <= theta))
                {
                        for (size_t i = 0; i < N; ++i) {
                                if (taken) {
                                        weights[i] += ((last_input >> i) & 1);
                                } else {
                                        weights[i] -= ((last_input << i) & 1);
                                }
                        }
                }
        }

    size_t get_N(){
        return this->N;
    }

    size_t get_theta(){
        return this->theta;
    }

  private:
        size_t N; // Size of perceptron
        std::vector<int> weights;
        size_t theta; // Training Parameter
        int last_output; // Most recent perceptron output
        size_t last_input;

};

#endif // __PERCEPTRON_HH__
