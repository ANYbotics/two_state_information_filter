/**
 * @file        timeline_test.cpp
 * @brief       Checks the splitting and merging of measurements in a timeline.
 */

#include <gtest/gtest.h>

#include "tsif/residual.h"
#include "tsif/timeline.h"

namespace {
using namespace tsif;

class MeasAcc : public ElementVector<Element<Vec3, 0>> {
 public:
  MeasAcc() : MeasAcc(Vec3::Zero()) {}
  explicit MeasAcc(const Vec3& acc) : ElementVector<Element<Vec3, 0>>(acc) {}
  const Vec3& GetAcc() const { return Get<0>(); }
};

using ResidualAccBase =
    Residual<ElementVector<Element<Vec3, 0>>, ElementVector<Element<Vec3, 0>>, ElementVector<Element<Vec3, 0>>, MeasAcc>;

// Only the split and merge flags matter here: the timeline never evaluates the residual.
class ResidualAcc : public ResidualAccBase {
 public:
  ResidualAcc() : ResidualAccBase(true, true, true) {}
  int EvalRes(Output::Ref /*out*/, const Previous::CRef /*pre*/, const Current::CRef /*cur*/) { return 0; }
  int JacPre(MatRefX /*J*/, const Previous::CRef /*pre*/, const Current::CRef /*cur*/) { return 0; }
  int JacCur(MatRefX /*J*/, const Previous::CRef /*pre*/, const Current::CRef /*cur*/) { return 0; }
};

class TimelineTest : public ::testing::Test {
 protected:
  TimePoint At(double seconds) const { return start_ + fromSec(seconds); }
  void Add(double seconds, const Vec3& acc) { timeline_.Add(At(seconds), std::make_shared<const MeasAcc>(acc)); }
  Vec3 AccAt(double seconds) { return timeline_.Get(At(seconds))->GetAcc(); }

  const TimePoint start_{Clock::now()};
  Timeline<MeasAcc> timeline_{fromSec(0.1), fromSec(0.0)};
  ResidualAcc residual_;
};

// Merging m1 at t1 into m2 at t2 gives m2 + (t1 - t0) / (t2 - t0) * (m1 - m2).
Vec3 Merged(double t0, double t1, double t2, const Vec3& m1, const Vec3& m2) {
  return m2 + (t1 - t0) / (t2 - t0) * (m1 - m2);
}
}  // namespace

TEST_F(TimelineTest, mergeKeepsTheUpdateTimes) {  // NOLINT
  const std::vector<Vec3> acc{Vec3(0, 0, 0),    Vec3(1, 2, 3),    Vec3(4, 5, 6),   Vec3(7, 8, 9),
                              Vec3(10, 11, 12), Vec3(13, 14, 15), Vec3(16, 17, 18)};
  for (std::size_t i = 0; i < acc.size(); ++i) {
    Add(static_cast<double>(i), acc[i]);
  }
  TimeSet times;
  times.insert(At(2.0));
  times.insert(At(5.0));

  timeline_.SplitAndMerge(At(0.0), times, residual_);

  // 1 is merged into 2. 3 is merged into 4, then 4 into 5: consecutive merges at the same index.
  // 0 is the start and 6 is after the last update time, so both stay.
  for (double t : {1.0, 3.0, 4.0}) {
    EXPECT_FALSE(timeline_.HasMeas(At(t))) << t;
  }
  for (double t : {0.0, 2.0, 5.0, 6.0}) {
    EXPECT_TRUE(timeline_.HasMeas(At(t))) << t;
  }
  EXPECT_TRUE(AccAt(0.0).isApprox(acc[0]));
  EXPECT_TRUE(AccAt(2.0).isApprox(Merged(0, 1, 2, acc[1], acc[2])));
  const Vec3 merged4 = Merged(2, 3, 4, acc[3], acc[4]);
  EXPECT_TRUE(AccAt(5.0).isApprox(Merged(2, 4, 5, merged4, acc[5])));
  EXPECT_TRUE(AccAt(6.0).isApprox(acc[6]));
}

TEST_F(TimelineTest, mergeReusesReleasedMeasurements) {  // NOLINT
  TimeSet times;
  times.insert(At(2.0));
  for (double offset : {0.0, 10.0}) {
    timeline_.Clear();
    Add(0.0, Vec3::Constant(offset));
    Add(1.0, Vec3::Constant(offset + 1.0));
    Add(2.0, Vec3::Constant(offset + 2.0));
    const std::shared_ptr<const MeasAcc> measAt2 = timeline_.Get(At(2.0));

    timeline_.SplitAndMerge(At(0.0), times, residual_);

    // The second round reuses the measurement the first one merged into, which Clear() released.
    EXPECT_TRUE(AccAt(2.0).isApprox(Merged(0, 1, 2, Vec3::Constant(offset + 1.0), Vec3::Constant(offset + 2.0)))) << offset;
    // The measurement that was merged into is replaced, not modified.
    EXPECT_TRUE(measAt2->GetAcc().isApprox(Vec3::Constant(offset + 2.0))) << offset;
  }
}

TEST_F(TimelineTest, splitAddsTheMissingUpdateTimes) {  // NOLINT
  Add(0.0, Vec3(1, 1, 1));
  Add(2.0, Vec3(2, 2, 2));
  Add(4.0, Vec3(3, 3, 3));
  TimeSet times;
  times.insert(At(1.0));
  times.insert(At(3.0));
  times.insert(At(4.0));

  timeline_.SplitAndMerge(At(0.0), times, residual_);

  // A split copies the next measurement. 2 is then merged into 3, which carries the measurement of 4.
  EXPECT_TRUE(AccAt(1.0).isApprox(Vec3(2, 2, 2)));
  EXPECT_FALSE(timeline_.HasMeas(At(2.0)));
  EXPECT_TRUE(AccAt(3.0).isApprox(Merged(1, 2, 3, Vec3(2, 2, 2), Vec3(3, 3, 3))));
  EXPECT_TRUE(AccAt(4.0).isApprox(Vec3(3, 3, 3)));
}

TEST_F(TimelineTest, cleanKeepsTheLastMeasurementUpToTheTime) {  // NOLINT
  for (int i = 0; i < 5; ++i) {
    Add(static_cast<double>(i), Vec3::Constant(i));
  }

  timeline_.Clean(At(2.5));

  EXPECT_FALSE(timeline_.HasMeas(At(0.0)));
  EXPECT_FALSE(timeline_.HasMeas(At(1.0)));
  EXPECT_TRUE(timeline_.HasMeas(At(2.0)));
  EXPECT_EQ(timeline_.GetFirstTime(), At(2.0));
  EXPECT_EQ(timeline_.CountSmallerOrEqual(At(10.0)), 3);

  timeline_.Clean(At(-1.0));
  EXPECT_EQ(timeline_.CountSmallerOrEqual(At(10.0)), 3);
}
