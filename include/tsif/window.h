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
};

template<typename State>
class Window {
 public:

  Window() = default;
  ~Window() = default;

  using StateInformationPair = std::pair<State, MatX>;
  using Moment = std::pair<TimePoint, StateInformationPair>;
  using StateHistory = std::map<TimePoint, StateInformationPair>;

  //add a moment to the state history
  void AddMoment(const TimePoint& t, const typename State::CRef state, const MatCRefX information){
    if((t>GetLastTime()) && stateHistory_.insert(Moment(t,StateInformationPair(state,information))).second){
      if(diagnostics_level_>0u) std::cout << "[Window] Added moment at " << Print(t) << " " << PrintWindowCharacteristics() << std::endl;
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
    auto t = TimePoint::min();
    if(!stateHistory_.empty()){
      t = stateHistory_.rbegin()->first;
    }
    return t;
  }

  //remove all except the first moment
  void Clean(){
    if(diagnostics_level_>0u) std::cout << "[Window] State history before cleaning: " << PrintWindowCharacteristics() << std::endl;
    while(stateHistory_.size()>1){
      stateHistory_.erase(++stateHistory_.begin(), stateHistory_.end());
    }
    if(diagnostics_level_>0u) std::cout << "[Window] Cleaned state history to " << PrintWindowCharacteristics() << std::endl;
  }

  //reduce the size of the window to the allowed max size and remove all moments before the given time
  void Shrink(){
    bool oversized = Oversized();
    if(oversized){
      while(oversized) {
        RemoveFirstMoment();
        oversized = Oversized();
      }
      if(diagnostics_level_>0u) std::cout << "[Window] Shrunk state history " << PrintWindowCharacteristics() << std::endl;
    }
  }

  void Cut(const TimePoint& t) {
    // TODO cut the tail end off the state history
  }

  //access the first moment in the state history
  bool GetFirstMoment(TimePoint& t, State& state, MatRefX information) const {
    if(!stateHistory_.empty()){
      const auto startMoment = stateHistory_.begin();
      t = startMoment->first;
      state = (startMoment->second).first;
      information = (startMoment->second).second;
      if(diagnostics_level_>1u) std::cout << "[Window] Accessing first moment at "<< Print(t) << std::endl;
      return true;
    }
    return false;
  }

  //reset the window to an uninitialized state
  void Reset(){
    stateHistory_.clear();
  }

  //configure the window
  void Configure(const WindowConfig& config){
    size_ = fromSec(config.size_);
    diagnostics_level_ = config.diagnostics_level_;
  }

  //print characteristics of the window as [], [FIRSTTIME, SIZE, LASTTTIME] or [FIRSTTIME]
  std::string PrintWindowCharacteristics() const {
    std::ostringstream out;
    out << "[";
    if(!stateHistory_.empty()){
      out << Print(GetFirstTime());
      const auto size = stateHistory_.size();
      if(size>1){
        out << ", " << size << ", " << Print(GetLastTime());
      }
    }
    out << "]";
    return out.str();
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

  //remove the first moment from the state history
  bool RemoveFirstMoment(){
    if(!stateHistory_.empty()){
      stateHistory_.erase(stateHistory_.begin());
      return true;
    }
    return false;
  }

  //collection of times, states, and information
  StateHistory stateHistory_;
  //maximum allowed size of the window
  Duration size_{fromSec(0.0025)};
  //config for how much diagnostic info to print
  // 0: no diagnostics
  // 1: stateHistory manipulation diagnostics
  // 2: access diagnostics
  unsigned int diagnostics_level_{0u};
};

} // namespace tsif

#endif  // TSIF_WINDOW_H_
