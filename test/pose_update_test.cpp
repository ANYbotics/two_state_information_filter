#include <gtest/gtest.h>

#include "tsif/residuals/pose_update.h"

namespace {
using namespace tsif;

// State ids: 0 IrIB, 1 qIB, 2 IrIJ, 3 qIJ, 4 BrBV, 5 qBV
using State = ElementVector<Element<Vec3, 0>, Element<Quat, 1>, Element<Vec3, 2>, Element<Quat, 3>, Element<Vec3, 4>, Element<Quat, 5>>;
using PoseUpd = PoseUpdate<0, 1, 0, 1, 2, 3, 4, 5>;
using AttUpd = AttitudeUpdate<1, 1, 3, 5>;

const Vec3 kPos{1.0, 2.0, 3.0};
const Quat kAtt{Eigen::AngleAxisd(0.3, Vec3::UnitZ())};

State MakeState(const Vec3& IrIB, const Quat& qIB) {
  State s{};
  s.Get<0>() = IrIB;
  s.Get<1>() = qIB;
  s.Get<2>() = Vec3::Zero();
  s.Get<3>() = Quat::Identity();
  s.Get<4>() = Vec3::Zero();
  s.Get<5>() = Quat::Identity();
  return s;
}
}  // namespace

// A state that matches the pose measurement exactly gives a zero residual.
TEST(PoseUpdateTest, ResidualIsZeroForMatchingState) {  // NOLINT
  PoseUpd upd{};
  upd.meas_ = std::make_shared<const MeasPose>(kPos, kAtt);  // what Filter::PreProcessResidual does
  const State s{MakeState(kPos, kAtt)};
  PoseUpd::Output out{};
  upd.EvalRes(out, ElementVector<>(), s);
  EXPECT_LT(out.Get<0>().norm(), 1e-12) << out.Get<0>().transpose();
  EXPECT_LT(out.Get<1>().norm(), 1e-12) << out.Get<1>().transpose();
}

// Setting a new measurement changes the residual for the same state.
TEST(PoseUpdateTest, ResidualDependsOnMeasurement) {  // NOLINT
  PoseUpd upd{};
  const State s{MakeState(kPos, kAtt)};
  PoseUpd::Output outA{};
  PoseUpd::Output outB{};
  upd.meas_ = std::make_shared<const MeasPose>(kPos, kAtt);
  upd.EvalRes(outA, ElementVector<>(), s);
  upd.meas_ = std::make_shared<const MeasPose>(Vec3{10.0, 10.0, 10.0}, Quat::Identity());
  upd.EvalRes(outB, ElementVector<>(), s);
  Vec<PoseUpd::Output::Dim()> diff{};
  outA.Boxminus(outB, diff);
  EXPECT_GT(diff.norm(), 1.0);
}

// The attitude rows of the Jacobian equal those of an AttitudeUpdate given the measured attitude directly.
TEST(PoseUpdateTest, AttitudeJacobianUsesPoseMeasurement) {  // NOLINT
  PoseUpd upd{};
  AttUpd ref{};
  const Quat measAtt{Eigen::AngleAxisd(0.8, Vec3::UnitX())};
  upd.meas_ = std::make_shared<const MeasPose>(kPos, measAtt);
  ref.meas_ = std::make_shared<const MeasAtt>(measAtt);
  State s{};
  s.SetRandom();
  MatX J{MatX::Zero(PoseUpd::Output::Dim(), State::Dim())};
  MatX JRef{MatX::Zero(AttUpd::Output::Dim(), State::Dim())};
  upd.JacCur(J, ElementVector<>(), s);
  ref.JacCur(JRef, ElementVector<>(), s);
  const double maxDiff{(J.block(PoseUpd::Output::Start(1), 0, 3, State::Dim()) - JRef).cwiseAbs().maxCoeff()};
  EXPECT_LT(maxDiff, 1e-9);
}
