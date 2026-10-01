#ifndef TSIF_RANDOM_HPP_
#define TSIF_RANDOM_HPP_

#include "tsif/utils/typedefs.h"

#include <random>

namespace tsif{

/*! \brief Normal Random Number Generator
 *         Per-thread singleton for generating normal random numbers (N(0,1)). Each thread owns an engine
 *         seeded with 0, so filters can be constructed concurrently on several threads (e.g. batch
 *         evaluation of parameter sets by an optimizer) without sharing generator state, and every fresh
 *         thread starts the same sequence. SetSeed only affects the calling thread.
 */
class NormalRandomNumberGenerator{
 public:
  void SetSeed(int s){
    generator_.seed(s);
  }
  double Get(){
    return distribution_(generator_);
  }
  template<int N>
  Vec<N> GetVec(){
    Vec<N> n;
    for(int i=0;i<N;i++){
      n(i) = Get();
    }
    return n;
  }
  static NormalRandomNumberGenerator& Instance(){
    thread_local NormalRandomNumberGenerator instance;
    return instance;
  }
  std::default_random_engine& GetGenerator(){
    return generator_;
  }
 protected:
  std::default_random_engine generator_;
  std::normal_distribution<double> distribution_;
  NormalRandomNumberGenerator(): generator_(0), distribution_(0.0,1.0){
  }
};

} // namespace tsif

#endif /* TSIF_RANDOM_HPP_ */
