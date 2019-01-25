#ifndef TSIF_WINDOW_H_
#define TSIF_WINDOW_H_

#include "tsif/utils/common.h"

namespace tsif{

template<typename State>
class Window {
 public:

  Window(double size=0.0025, unsigned int diagnostics_level=0u):
      size_(fromSec(size)),
      diagnostics_level_(diagnostics_level) {}
  ~Window() = default;

  using StateInformationPair = std::pair<State, MatX>;
  using Moment = std::pair<TimePoint, StateInformationPair>;
  using StateHistory = std::map<TimePoint, StateInformationPair>;

  // add a moment to the state history
  void AddMoment(TimePoint t, State state, MatX information){
    is_set_ = true;
    stateHistory_.insert(Moment(t,StateInformationPair(state,information)));
    //std::cout << "Added moment (" << stateHistory_.size() << ")" << std::endl;
  }

  //get the first timepoint in the state history
  TimePoint GetFirstTime() const {
    //std::cout << "GetFirstTime (" << stateHistory_.size() << ")" << std::endl;
    auto t = TimePoint::min();
  	if(!stateHistory_.empty()){
  		t = stateHistory_.begin()->first;
  	}
  	return t;
  }

  //get the last timepoint in the state history
  TimePoint GetLastTime() const {
    //std::cout << "GetLastTime (" << stateHistory_.size() << ")" << std::endl;
    auto t = TimePoint::max();
    if(!stateHistory_.empty()){
      t = stateHistory_.rbegin()->first;
    }
    return t;
  }

  //reduce the size of the window to the allowed max size and remove all moments before the given time
  void Clean(TimePoint t = TimePoint::min()){
    while(!stateHistory_.empty() && (Oversized() || DoesCut(t))) {
      //std::cout << "Oversized()= "<< Oversized() << " DoesCut(t)=" << << DoesCut(t) " (" << stateHistory_.size() << ")" << std::endl;
      RemoveFirstMoment();
    }
    //std::cout << "Cleaned at " <<" (" << stateHistory_.size() << ")" << std::endl;
    is_set_ = !stateHistory_.empty();
  }

  //access the first moment in the state history
  bool GetFirstMoment(TimePoint& t, State& state, MatRefX information) const {
    //std::cout << "GetFirstMoment (" << stateHistory_.size() << ")" << std::endl;
    if(is_set_){
      const auto startMoment = stateHistory_.begin();
      t = startMoment->first;
      state = (startMoment->second).first;
      information = (startMoment->second).second;
      return true;
    }
    return false;
  }

  //reset the window to an uninitialized state
  void Reset(){
    //std::cout << "Reset (" << stateHistory_.size() << ")" << std::endl;
    is_set_ = false;
    stateHistory_.clear();
  }

  //setters for config params
  void SetSize(double size){
    size_ = fromSec(size);
  }
  void SetDiagnosticsLevel(unsigned int diagnostics_level){
    diagnostics_level_ = diagnostics_level;
  }

 protected:

  //check if the window is oversized
  bool Oversized(){
    //std::cout << "GetSize (" << stateHistory_.size() << ")" << std::endl;
    if(stateHistory_.size()>1) {
      auto size=(GetLastTime() - GetFirstTime());
      return size>size_;
    }
    return false;
  }

  //check if the given time is within the window
  bool DoesCut(TimePoint t){
    //std::cout << "GetSize (" << stateHistory_.size() << ")" << std::endl;
    auto it = stateHistory_.lower_bound(t);
    if(it!=stateHistory_.end() && it!=stateHistory_.begin()){
      auto size = it->first - stateHistory_.begin()->first;
      //std::cout << "Cut size = " << toSec(size) << ")" << std::endl;
      return size>Duration::min();
    }
    return false;
  }

  //remove the first moment from the state history
  void RemoveFirstMoment(){
    //std::cout << "RemoveFirstMoment (" << stateHistory_.size() << ")" << std::endl;
    if(!stateHistory_.empty()){
      stateHistory_.erase(stateHistory_.begin());
    }
  }

  //collection of times, states, and information
  StateHistory stateHistory_;
  //indicator for a populated window
  bool is_set_{false};
  //maximum allowed size of the window
  Duration size_;
  //config for how much diagnostic info to print
  unsigned int diagnostics_level_;

};

} // namespace tsif

#endif  // TSIF_WINDOW_H_
