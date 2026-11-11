/**
 * @file        no_allocation_test.cpp
 * @brief       Checks that adding measurements and running the filter update does not allocate on the heap after a warm-up.
 */

#include <gtest/gtest.h>

#include <malloc_capture/gtest.hpp>

#include "tsif/filter.h"

namespace {
using namespace tsif;

class MeasAcc : public ElementVector<Element<Vec3, 0>> {
 public:
  MeasAcc() : MeasAcc(Vec3::Zero()) {}
  MeasAcc(const Vec3& acc) : ElementVector<Element<Vec3, 0>>(acc) {}
  const Vec3& GetAcc() const { return Get<0>(); }
};

// Position integrates velocity: a residual that is always available, as in the legged odometry filter.
template <int OUT_POS, int STA_POS, int STA_VEL>
using ResidualPosBase = Residual<ElementVector<Element<Vec3, OUT_POS>>, ElementVector<Element<Vec3, STA_POS>, Element<Vec3, STA_VEL>>,
                                 ElementVector<Element<Vec3, STA_POS>>, MeasEmpty>;

template <int OUT_POS, int STA_POS, int STA_VEL>
class ResidualPos : public ResidualPosBase<OUT_POS, STA_POS, STA_VEL> {
 public:
  typedef ResidualPosBase<OUT_POS, STA_POS, STA_VEL> Base;
  using Base::dt_;
  typedef typename Base::Output Output;
  typedef typename Base::Previous Previous;
  typedef typename Base::Current Current;
  ResidualPos() : ResidualPosBase<OUT_POS, STA_POS, STA_VEL>(true, true, true) {}
  int EvalRes(typename Output::Ref out, const typename Previous::CRef pre, const typename Current::CRef cur) {
    out.template Get<OUT_POS>() = cur.template Get<STA_POS>() - pre.template Get<STA_POS>() - dt_ * pre.template Get<STA_VEL>();
    return 0;
  }
  int JacPre(MatRefX J, const typename Previous::CRef pre, const typename Current::CRef /*cur*/) {
    J.block<3, 3>(Output::Start(OUT_POS), pre.Start(STA_POS)) = -Mat3::Identity();
    J.block<3, 3>(Output::Start(OUT_POS), pre.Start(STA_VEL)) = -Mat3::Identity() * dt_;
    return 0;
  }
  int JacCur(MatRefX J, const typename Previous::CRef /*pre*/, const typename Current::CRef /*cur*/) {
    J.block<3, 3>(Output::Start(OUT_POS), Current::Start(STA_POS)) = Mat3::Identity();
    return 0;
  }
};

// Velocity is updated by the measured acceleration, which arrives at its own times.
template <int OUT_VEL, int STA_VEL>
using ResidualVelBase =
    Residual<ElementVector<Element<Vec3, OUT_VEL>>, ElementVector<Element<Vec3, STA_VEL>>, ElementVector<Element<Vec3, STA_VEL>>, MeasAcc>;

template <int OUT_VEL, int STA_VEL, bool MERGEABLE = false>
class ResidualVel : public ResidualVelBase<OUT_VEL, STA_VEL> {
 public:
  typedef ResidualVelBase<OUT_VEL, STA_VEL> Base;
  using Base::dt_;
  using Base::meas_;
  typedef typename Base::Output Output;
  typedef typename Base::Previous Previous;
  typedef typename Base::Current Current;
  ResidualVel() : ResidualVelBase<OUT_VEL, STA_VEL>(true, MERGEABLE, true) {}
  int EvalRes(typename Output::Ref out, const typename Previous::CRef pre, const typename Current::CRef cur) {
    out.template Get<OUT_VEL>() = cur.template Get<STA_VEL>() - pre.template Get<STA_VEL>() - dt_ * meas_->GetAcc();
    return 0;
  }
  int JacPre(MatRefX J, const typename Previous::CRef pre, const typename Current::CRef /*cur*/) {
    J.block<3, 3>(Output::Start(OUT_VEL), pre.Start(STA_VEL)) = -Mat3::Identity();
    return 0;
  }
  int JacCur(MatRefX J, const typename Previous::CRef /*pre*/, const typename Current::CRef /*cur*/) {
    J.block<3, 3>(Output::Start(OUT_VEL), Current::Start(STA_VEL)) = Mat3::Identity();
    return 0;
  }
};

using FilterA = Filter<ResidualPos<0, 0, 1>, ResidualVel<0, 1>>;
// A second, mergeable acceleration stamped between the update times, as the IMU in the legged odometry filter.
using FilterMerging = Filter<ResidualPos<0, 0, 1>, ResidualVel<0, 1>, ResidualVel<0, 1, true>>;
}  // namespace

TEST(NoAllocationTest, updateAfterWarmup) {  // NOLINT
  constexpr int kSteps = 1100;
  const TimePoint start = Clock::now();
  // The measurements are created before the measured section: creating one is not part of the update.
  std::vector<std::shared_ptr<const MeasAcc>> measurements;
  measurements.reserve(kSteps);
  for (int i = 0; i < kSteps; ++i) {
    measurements.push_back(std::make_shared<const MeasAcc>(Vec3(0.1 * std::sin(0.01 * i), 0.0, 9.8)));
  }
  FilterA filter;
  int step = 0;
  const auto cycle = [&] {
    filter.template AddMeas<1>(start + fromSec(0.02 * step), measurements[step]);
    filter.Update();
    ++step;
  };
  EXPECT_NO_ALLOCATIONS_AFTER_WARMUP(100, 1000, cycle);
  EXPECT_EQ(step, 1100);
}

// A second, mergeable timeline stamped between the update times is split and merged on every update, as the IMU
// timelines of the legged odometry filter.
TEST(NoAllocationTest, splitAndMergeAfterWarmup) {  // NOLINT
  constexpr int kSteps = 1100;
  const TimePoint start = Clock::now();
  std::vector<std::shared_ptr<const MeasAcc>> measurements;
  measurements.reserve(kSteps);
  for (int i = 0; i < kSteps; ++i) {
    measurements.push_back(std::make_shared<const MeasAcc>(Vec3(0.1 * std::sin(0.01 * i), 0.0, 9.8)));
  }
  FilterMerging filter;
  int step = 0;
  const auto cycle = [&] {
    filter.template AddMeas<1>(start + fromSec(0.02 * step), measurements[step]);
    filter.template AddMeas<2>(start + fromSec(0.02 * step + 0.01), measurements[step]);
    filter.Update();
    ++step;
  };
  EXPECT_NO_ALLOCATIONS_AFTER_WARMUP(100, 1000, cycle);
  EXPECT_EQ(step, 1100);
}
