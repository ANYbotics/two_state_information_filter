#ifndef TSIF_WINDOW_H_
#define TSIF_WINDOW_H_

#include "tsif/utils/common.h"

namespace tsif{

template<typename State>
class Window {
 public:
  Window() = default;
  ~Window() = default;

  using StateInformationPair = std::pair<State, MatX>;
  using Moment = std::pair<TimePoint, StateInformationPair>;
  using StateHistory = std::map<TimePoint, StateInformationPair>;

  //TODO make private
  bool is_set_{false};
  Duration size_{fromSec(.0025)};

  StateHistory stateHistory_;

  void Add(TimePoint t, State state, MatX information){
  	stateHistory_.insert(Moment(t,StateInformationPair(state,information)));
  }

  TimePoint GetStartTime(){
  	TimePoint t;
  	if(!stateHistory_.empty()){
  		t = stateHistory_.begin()->first;
  	}
  	return t;
  }

  void Clean(const TimePoint& t){
  	auto it = stateHistory_.begin();
  	while(it != stateHistory_.end() && (t - it->first)>Duration(0.)){
  	}
  }

  MatX pre_I_;
  State pre_state_;
  TimePoint pre_time_;

};

} // namespace tsif

#endif  // TSIF_WINDOW_H_
