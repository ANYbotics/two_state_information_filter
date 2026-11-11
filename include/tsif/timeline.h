#ifndef TSIF_TIMELINE_H_
#define TSIF_TIMELINE_H_

#include <boost/container/flat_map.hpp>

#include "element_vector.h"
#include "tsif/utils/common.h"

namespace tsif {

template <typename Measurement>
class Timeline {
 private:
  //! Sorted by time. Kept in one array, so the nodes of a std::map are not allocated on every insertion.
  boost::container::flat_map<TimePoint, std::shared_ptr<const Measurement>> mm_;

 public:
  Timeline(Duration max_wait_time, Duration min_wait_time) : max_wait_time_(max_wait_time), min_wait_time_(min_wait_time) {}
  void Add(TimePoint t, std::shared_ptr<const Measurement> m) {
    TSIF_LOG("Add measurement at " << tsif::Print(t));
    TSIF_LOGWIF(mm_.count(t) > 0, "Entry already exists for measurement!");
    mm_[t] = m;
  }
  bool HasMeas(TimePoint t) { return mm_.count(t) > 0; }
  std::shared_ptr<const Measurement> Get(TimePoint t) { return mm_.at(t); }
  void Clean(TimePoint t) {
    // A single range erase, so the array is shifted once. Keeps the last measurement at or before t.
    auto last = mm_.upper_bound(t);
    if (last - mm_.begin() > 1) {
      mm_.erase(mm_.begin(), std::prev(last));
    }
  }
  void Clear() { mm_.clear(); }
  int CountSmallerOrEqual(TimePoint t) { return static_cast<int>(mm_.upper_bound(t) - mm_.begin()); }
  TimePoint GetLastTime() const {
    if (mm_.empty()) {
      return TimePoint::min();
    } else {
      return mm_.rbegin()->first;
    }
  }
  TimePoint GetFirstTime() const {
    if (mm_.empty()) {
      return TimePoint::max();
    } else {
      return mm_.begin()->first;
    }
  }
  TimePoint GetMaximalUpdateTime(TimePoint current) const { return std::max(current - max_wait_time_, GetLastTime() + min_wait_time_); }
  void GetAllInRange(TimeSet& times, TimePoint start, TimePoint end) const {
    auto it = mm_.upper_bound(start);
    while (it != mm_.end() && it->first <= end) {
      times.insert(it->first);
      ++it;
    }
  }
  template <typename Residual>
  void Split(TimePoint t0, TimePoint t1, TimePoint t2, const Residual& res) {
    assert(t0 <= t1 && t1 <= t2);
    TSIF_LOG("Split measurement at " << tsif::Print(t1));
    // SplitMeasurements overwrites the new entry, so it starts as a copy of the pointer at t2 instead of an allocation.
    Add(t1, mm_.at(t2));
    res.SplitMeasurements(t0, t1, t2, mm_.at(t0), mm_.at(t1), mm_.at(t2));
  }
  template <typename Residual>
  void Merge(TimePoint t0, TimePoint t1, TimePoint t2, const Residual& res) {
    assert(t0 <= t1 && t1 <= t2);
    TSIF_LOG("Merging measurement at " << tsif::Print(t1));
    res.MergeMeasurements(t0, t1, t2, mm_.at(t0), mm_.at(t1), mm_.at(t2));
    mm_.erase(t1);
  }
  template <typename Residual>
  void SplitAndMerge(TimePoint t0, const TimeSet& times, const Residual& res) {
    if (times.size() == 0) {
      return;
    }
    if (res.isSplitable_) {
      for (const auto& t : times) {
        auto it = mm_.lower_bound(t);
        if (it == mm_.end()) {
          TSIF_LOGW("Partial splitting only!");
          break;
        }
        if (it->first == t) {
          continue;  // Measurement already available
        }
        assert(it != mm_.begin());
        TimePoint previous = std::prev(it)->first;
        Split(previous, t, it->first, res);
      }
    }
    if (res.isMergeable_) {
      // Indices, not iterators: Merge erases an element, which shifts the ones after it.
      std::size_t i = 0;
      while (i < mm_.size()) {
        const TimePoint current = mm_.nth(i)->first;
        if (current <= t0 || times.count(current) > 0) {
          ++i;
          continue;  // Ignore the first or if in times
        }
        if (current > *times.rbegin()) {
          break;  // Ignore the last
        }
        if (i + 1 == mm_.size()) {
          TSIF_LOGW("Partial merging only!");
          break;
        }
        assert(i > 0);
        // The element after the merged one moves to index i, so i is not advanced.
        Merge(mm_.nth(i - 1)->first, current, mm_.nth(i + 1)->first, res);
      }
    }
  }

  std::string Print(const TimePoint& start, int start_offset, double resolution) const {
    std::ostringstream out;
    const int width =
        mm_.empty() ? start_offset : start_offset + std::max(0, (int)(std::ceil(toSec(mm_.rbegin()->first - start) / resolution)) + 1);
    std::vector<int> counts(width, 0);
    for (auto it = mm_.begin(); it != mm_.end(); ++it) {
      const int x = start_offset + ceil(toSec(it->first - start) / resolution);
      if (x >= 0) {
        counts.at(x)++;
      }
    }
    for (const auto& c : counts) {
      if (c == 0) {
        out << "-";
      } else {
        out << c;
      }
    };
    return out.str();
  }

  void SetMaxWaitTime(const double& max_wait_time) { max_wait_time_ = fromSec(max_wait_time); }
  void SetMinWaitTime(const double& min_wait_time) { min_wait_time_ = fromSec(min_wait_time); }

 protected:
  Duration max_wait_time_;
  Duration min_wait_time_;
};

template <>
class Timeline<MeasEmpty> {
 public:
  Timeline(Duration max_wait_time, Duration min_wait_time)
      : max_wait_time_(max_wait_time), min_wait_time_(min_wait_time), empty_(std::make_shared<MeasEmpty>()) {}
  void Add(TimePoint /*t*/, std::shared_ptr<const MeasEmpty> /*m*/) { TSIF_LOGW("Unnecessary addition of empty measurement!"); }
  bool HasMeas(TimePoint /*t*/) { return true; }
  std::shared_ptr<const MeasEmpty> Get(TimePoint /*t*/) { return empty_; }
  void Clean(TimePoint /*t*/) {}
  void Clear() {}
  TimePoint GetLastTime() const { return TimePoint::max(); }
  TimePoint GetFirstTime() const { return TimePoint::min(); }
  TimePoint GetMaximalUpdateTime(TimePoint /*current*/) const { return TimePoint::max(); }
  void GetAllInRange(TimeSet& /*times*/, TimePoint /*start*/, TimePoint /*end*/) const {}
  template <typename Residual>
  void SplitAndMerge(TimePoint /*time*/, const TimeSet& /*times*/, const Residual& /*res*/) {}
  std::string Print(const TimePoint& /*start*/, int start_offset, double /*resolution*/) const {
    const int c = start_offset > 4 ? start_offset - 4 : 0;
    std::ostringstream out;
    for (int i = 0; i < c; i++) out << "-";
    out << "MeasEmpty";
    for (int i = 0; i < c; i++) out << "-";
    return out.str();
  }

  void SetMaxWaitTime(const double& max_wait_time) { max_wait_time_ = fromSec(max_wait_time); }
  void SetMinWaitTime(const double& min_wait_time) { min_wait_time_ = fromSec(min_wait_time); }

 protected:
  Duration max_wait_time_;
  Duration min_wait_time_;
  // MeasEmpty carries no data, so Get() returns this instance instead of allocating one per call.
  std::shared_ptr<const MeasEmpty> empty_;
};

}  // namespace tsif

#endif  // TSIF_TIMELINE_H_
