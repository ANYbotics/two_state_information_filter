#ifndef TSIF_WINDOW_H_
#define TSIF_WINDOW_H_

#include "tsif/utils/common.h"

namespace tsif{

template<typename State>
struct Window {
  Window() = default;
  ~Window() = default;

  MatX pre_I_;
  State pre_state_;
  TimePoint pre_time_;
  bool is_set_{false};
  Duration size_{fromSec(.0025)};

};

} // namespace tsif

#endif  // TSIF_WINDOW_H_
