#include <gtest/gtest.h>

#include <thread>

#include "tsif/residuals/gyroscope_update.h"

namespace {
// Constructs a residual in a fresh thread and returns its default (random) measurement.
tsif::Vec3 defaultMeasurementInFreshThread() {
  tsif::Vec3 gyr{};
  std::thread thread{[&gyr] {
    const tsif::GyroscopeUpdate<0, 0, 1> upd{};
    gyr = upd.meas_->GetGyr();
  }};
  thread.join();
  return gyr;
}
}  // namespace

// A second thread must not continue the random sequence of the first one.
TEST(NormalRandomNumberGeneratorTest, FreshThreadsGetTheSameDefaultMeasurement) {  // NOLINT
  const tsif::Vec3 a{defaultMeasurementInFreshThread()};
  const tsif::Vec3 b{defaultMeasurementInFreshThread()};
  EXPECT_TRUE(a == b) << a.transpose() << " vs " << b.transpose();
}
