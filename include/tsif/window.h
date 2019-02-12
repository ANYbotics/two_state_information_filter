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

  // add a moment to the state history
  void AddMoment(const TimePoint& t, const typename State::CRef state, const MatCRefX information){
    if(stateHistory_.insert(Moment(t,StateInformationPair(state,information))).second){
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
    auto t = TimePoint::min();
    if(!stateHistory_.empty()){
      t = stateHistory_.rbegin()->first;
    }
    return t;
  }

  //remove all except the first moment
  void Clean(){
    while(stateHistory_.size()>1){
      RemoveFirstMoment();
    }
    if(diagnostics_level_>1u) std::cout << "[Window] Cleaned state history (" << stateHistory_.size() << ")" << std::endl;
  }

  //reduce the size of the window to the allowed max size and remove all moments before the given time
  void Shrink(){
    while(Oversized()) {
      RemoveFirstMoment();
    }
    if(diagnostics_level_>1u) std::cout << "[Window] Shrunk state history ("<< stateHistory_.size() <<")" << std::endl;
  }

  void Cut(const TimePoint& t) { // TODO add another bool argument for whether or not to cut off the measurement
    // TODO cut the tail end off the state history
    // check if t is earlier or equal to the first moment. if so, do nothing and return
    // get lower bound of t as it
    // erase(begin(), it), i.e. remove everything before t but not t in order to keep the associated meas
  }

  //access the first moment in the state history
  bool GetFirstMoment(TimePoint& t, State& state, MatRefX information) const {
    if(!stateHistory_.empty()){
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
    stateHistory_.clear();
  }

  //configure the window
  void Configure(const WindowConfig& config){
    size_ = fromSec(config.size_);
    diagnostics_level_ = config.diagnostics_level_;
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
  void RemoveFirstMoment(){
    if(!stateHistory_.empty()){
      stateHistory_.erase(stateHistory_.begin());
    }
  }

  //collection of times, states, and information
  StateHistory stateHistory_;
  //maximum allowed size of the window
  Duration size_;
  //config for how much diagnostic info to print
  unsigned int diagnostics_level_{0u};
};

} // namespace tsif

#endif  // TSIF_WINDOW_H_
