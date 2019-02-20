#ifndef TSIF_TIMING_HPP_
#define TSIF_TIMING_HPP_

#include <iostream>
#include <chrono>

namespace tsif{

typedef std::chrono::high_resolution_clock Clock;
typedef Clock::time_point TimePoint;
typedef Clock::duration Duration;
typedef std::set<TimePoint> TimePointSet;

inline Duration fromSec(const double sec){
  return std::chrono::duration_cast < Duration
      > (std::chrono::duration<double>(sec));
}
inline double toSec(const Duration& duration){
  return std::chrono::duration_cast<std::chrono::duration<double>>(duration).count();
}
static std::string Print(TimePoint t){
  std::ostringstream out;
  out.precision(15);
  out << ((double)t.time_since_epoch().count()*Duration::period::num)/(Duration::period::den);
  return out.str();
}
inline TimePoint GetCenterTime(const TimePointSet& times){
  const auto first_time = *times.begin();
  const auto last_time = *times.rbegin();
  const auto delta_t = last_time - first_time;
  return (first_time + delta_t/2);
}

inline std::size_t TimePointHash(const TimePoint& t){
  return std::hash<double>{}(((double)t.time_since_epoch().count()*Duration::period::num)/(Duration::period::den));
}
inline std::size_t TimePointHash(const TimePoint& t, std::size_t h){
  std::size_t new_h = std::hash<double>{}(((double)t.time_since_epoch().count()*Duration::period::num)/(Duration::period::den));
  return h ^ (new_h << 1);
}

class Timer{
 public:
  Timer(){
    start_ = Clock::now();
    last_ = start_;
  }
  double GetIncr(){
    TimePoint now = Clock::now();
    double incr = toSec(now-last_);
    last_ = now;
    return incr;
  }
  double GetFull(){
    last_ = Clock::now();
    return toSec(last_-start_);
  }
  TimePoint start_;
  TimePoint last_;
};

} // namespace tsif

#endif /* TSIF_TIMING_HPP_ */
