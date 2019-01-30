#ifndef TSIF_WINDOW_H_
#define TSIF_WINDOW_H_

#include "tsif/utils/common.h"

namespace tsif{

//configuration for the window
struct WindowConfig {
  WindowConfig() = default;
  ~WindowConfig() = default;
  double size_{0.0025};
  unsigned int diagnostics_level_{0u};
  bool crop_with_measurements_{false};
};

template<typename State>
class Window {
 public:

  Window() = default;
  ~Window() = default;

  using StateInformationPair = std::pair<State, MatX>;
  using Moment = std::pair<TimePoint, StateInformationPair>;
  using StateHistory = std::map<TimePoint, StateInformationPair>;

  // add a moment to the state history
  void AddMoment(TimePoint t, State state, MatX information){
    if(stateHistory_.insert(Moment(t,StateInformationPair(state,information))).second){
      is_set_ = true;
      if(diagnostics_level_>0u) std::cout << "[Window] Added moment ("<< stateHistory_.size() <<") at " << Print(t) << std::endl;
    }
  }

  //get the first timepoint in the state history
  TimePoint GetFirstTime() const {
    auto t = TimePoint::min();
  	if(!stateHistory_.empty()){
  		t = stateHistory_.begin()->first;
  	}
  	return t;
  }

  //get the last timepoint in the state history
  TimePoint GetLastTime() const {
    auto t = TimePoint::max();
    if(!stateHistory_.empty()){
      t = stateHistory_.rbegin()->first;
    }
    return t;
  }

  //remove all except the first moment
  void Clean(){
    while(stateHistory_.size()>1){
      if(diagnostics_level_>1u) std::cout << "[Window] Cleaning state history (" << stateHistory_.size() << ")" << std::endl;
      RemoveFirstMoment();
    }
  }

  //reduce the size of the window to the allowed max size and remove all moments before the given time
  void Shrink(TimePoint t = TimePoint::min()){
    while(!stateHistory_.empty() && (Oversized() || (crop_with_measurements_ && DoesCut(t)) )) {
      if(diagnostics_level_>1u) std::cout << "[Window] Shrinking state history ("<< stateHistory_.size() << ", " << Oversized() << ", " << DoesCut(t) <<") at " << Print(t) << std::endl;
      RemoveFirstMoment();
    }
    is_set_ = !stateHistory_.empty();
  }

  //access the first moment in the state history
  bool GetFirstMoment(TimePoint& t, State& state, MatRefX information) const {
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
    is_set_ = false;
    stateHistory_.clear();
  }

  //configure the window
  void Configure(const WindowConfig& config){
    size_ = fromSec(config.size_);
    diagnostics_level_ = config.diagnostics_level_;
    crop_with_measurements_ = config.crop_with_measurements_;
  }

 protected:

  //check if the window is oversized
  bool Oversized(){
    if(stateHistory_.size()>1) {
      auto size=(GetLastTime() - GetFirstTime());
      return size>size_;
    }
    return false;
  }

  //check if the given time is within the window
  bool DoesCut(TimePoint t){
    auto it = stateHistory_.lower_bound(t);
    if(it!=stateHistory_.end() && it!=stateHistory_.begin()){
      return it->first > stateHistory_.begin()->first;;
    }
    return false;
  }

  //remove the first moment from the state history
  void RemoveFirstMoment(){
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
  unsigned int diagnostics_level_{0u};
  //use measurements to delete states from the window
  bool crop_with_measurements_{false};
};

} // namespace tsif

#endif  // TSIF_WINDOW_H_
