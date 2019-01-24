#ifndef TSIF_WINDOW_H_
#define TSIF_WINDOW_H_

#include "tsif/utils/common.h"

namespace tsif{

template<typename State>
class Window {
 public:

  Window(double size=0.0025):
    size_(fromSec(size)){}
  ~Window() = default;

  using StateInformationPair = std::pair<State, MatX>;
  using Moment = std::pair<TimePoint, StateInformationPair>;
  using StateHistory = std::map<TimePoint, StateInformationPair>;


  void AddMoment(TimePoint t, State state, MatX information){
    if((t-GetEndTime())>fromSec(0.)){
      is_set_ = true;
      stateHistory_.insert(Moment(t,StateInformationPair(state,information)));
      //std::cout << "Added moment (" << stateHistory_.size() << ")" << std::endl;
    }
  }

  TimePoint GetStartTime() const {
    auto t = TimePoint();
  	if(!stateHistory_.empty()){
  		t = stateHistory_.begin()->first;
  	}
  	return t;
  }

  TimePoint GetEndTime() const {
    auto t = TimePoint();
    if(!stateHistory_.empty()){
      t = stateHistory_.rbegin()->first;
    }
    return t;
  }

  void Clean(){
    while(GetSize() > size_) { RemoveFirstMoment(); }
    //std::cout << "Cleaned history (" << stateHistory_.size() << ")" << std::endl;
  }

  bool GetStartMoment(TimePoint& t, State& state, MatRefX information) const {
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
    is_set_=false;
    stateHistory_.clear();
  }

  void SetSize(double size){
    size_ = fromSec(size);
  }

 protected:
  Duration GetSize(){
    Duration size = fromSec(0.);
    if(stateHistory_.size()>1) size=(GetEndTime() - GetStartTime());
    return size;
  }
  void RemoveFirstMoment(){
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
