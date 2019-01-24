#ifndef TSIF_WINDOW_H_
#define TSIF_WINDOW_H_

#include "tsif/utils/common.h"

namespace tsif{

template<typename State>
class Window {
 public:

  Window(double size=0.0025): size_(fromSec(size)) {}
  ~Window() = default;

  using StateInformationPair = std::pair<State, MatX>;
  using Moment = std::pair<TimePoint, StateInformationPair>;
  using StateHistory = std::map<TimePoint, StateInformationPair>;

  void AddMoment(TimePoint t, State state, MatX information){
    //std::cout << "AddMoment (" << stateHistory_.size() << ")" << std::endl;
    is_set_ = true;
    stateHistory_.insert(Moment(t,StateInformationPair(state,information)));
    //std::cout << "Added moment (" << stateHistory_.size() << ")" << std::endl;
  }

  TimePoint GetFirstTime() const {
    //std::cout << "GetFirstTime (" << stateHistory_.size() << ")" << std::endl;
    auto t = TimePoint::min();
  	if(!stateHistory_.empty()){
  		t = stateHistory_.begin()->first;
  	}
  	return t;
  }

  TimePoint GetLastTime() const {
    //std::cout << "GetLastTime (" << stateHistory_.size() << ")" << std::endl;
    auto t = TimePoint::max();
    if(!stateHistory_.empty()){
      t = stateHistory_.rbegin()->first;
    }
    return t;
  }

  //TODO remove states before the max delayed meas time to only use each delayed measurement once
  void Clean(TimePoint t = TimePoint::min()){
    //std::cout << "Clean at " << Print(t) <<" (" << stateHistory_.size() << ")" << std::endl;
    while(!stateHistory_.empty() && GetSize()>size_) {
      //std::cout << "Clean.  (t-GetFirstTime())=" << toSec(t-GetFirstTime()) <<" (" << stateHistory_.size() << ")" << std::endl;
      RemoveFirstMoment();
    }
    //std::cout << "Cleaned at " <<" (" << stateHistory_.size() << ")" << std::endl;
    is_set_ = !stateHistory_.empty();
  }

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

  void Reset(){
    //std::cout << "Reset (" << stateHistory_.size() << ")" << std::endl;
    is_set_ = false;
    stateHistory_.clear();
  }

  void SetSize(double size){
    size_ = fromSec(size);
  }

 protected:

  Duration GetSize(){
    //std::cout << "GetSize (" << stateHistory_.size() << ")" << std::endl;
    Duration size = fromSec(0.);
    if(stateHistory_.size()>1) size=(GetLastTime() - GetFirstTime());
    return size;
  }

  void RemoveFirstMoment(){
    //std::cout << "RemoveFirstMoment (" << stateHistory_.size() << ")" << std::endl;
    if(!stateHistory_.empty()){
      stateHistory_.erase(stateHistory_.begin());
    }
  }

  StateHistory stateHistory_;
  bool is_set_{false};
  Duration size_;

};

} // namespace tsif

#endif  // TSIF_WINDOW_H_
