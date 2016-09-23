#ifndef GIF_LEGKINEMATICUPDATE_HPP_
#define GIF_LEGKINEMATICUPDATE_HPP_

#include "generalized_information_filter/common.h"
#include "generalized_information_filter/unary-update.h"

namespace GIF {

/*! \brief Leg Kinematic Measurement
 *         ElementVector that can be used to hold leg kinematic measurements. It is composed of an
 *         array of vectors containing the single encoder measurements.
 */
template<int NumLeg, int NumDof>
class KinematicMeasurement : public ElementVector {
 public:
  KinematicMeasurement(const std::array<Vec<NumDof>,NumLeg>& kin
                       = ElementTraits<std::array<Vec<NumDof>,NumLeg>>::Identity())
      : ElementVector(std::shared_ptr<ElementVectorDefinition>(
            new ElementPack<std::array<Vec<NumDof>,NumLeg>>({"kin"}))),
        kin_(ElementVector::GetValue<std::array<Vec<NumDof>,NumLeg>>("kin")) {
    kin_ = kin;
    for(int i=0;i<NumLeg;i++){
      contact_flag_[i] = true;
    }
  }
  std::array<Vec<NumDof>,NumLeg>& kin_;
  std::array<bool,NumLeg> contact_flag_;
};

/*! \brief Leg Kinematic Update
 *         Transforms the kinematic measurements into relative measurement between main body and
 *         foothold. Here the foothold is directly stored in robocentric coordinates and therefore
 *         the residual condenses down to prediction - forward kinematics. The forward kinematics
 *         are expressed with respect to B (body frame) and the state is expressed with respect to
 *         M (Imu).
 */
template<typename KinModel>
class LegKinematicUpdate : public UnaryUpdate<ElementPack<std::array<Vec3,KinModel::kNumLeg>>,
      ElementPack<std::array<Vec3,KinModel::kNumLeg>>, ElementPack<std::array<Vec3,KinModel::kNumLeg>>,
      KinematicMeasurement<KinModel::kNumLeg,KinModel::kNumDof>> {
 public:
  using mtUnaryUpdate = UnaryUpdate<ElementPack<std::array<Vec3,KinModel::kNumLeg>>,
      ElementPack<std::array<Vec3,KinModel::kNumLeg>>, ElementPack<std::array<Vec3,KinModel::kNumLeg>>,
      KinematicMeasurement<KinModel::kNumLeg,KinModel::kNumDof>>;
  using mtUnaryUpdate::meas_;
  enum Elements {KIN};

  LegKinematicUpdate(const std::string& errorName = "BrBL",
                     const std::string& stateName = "MrML",
                     const std::string& noiseName = "BrBL")
      : mtUnaryUpdate({errorName},{stateName},{noiseName}) {
  }

  virtual ~LegKinematicUpdate() {
  }

  void Eval(std::array<Vec3,KinModel::kNumLeg>& BrBL_inn,
            const std::array<Vec3,KinModel::kNumLeg>& MrML_cur,
            const std::array<Vec3,KinModel::kNumLeg>& BrBL_noi) const {
    for(int i=0;i<KinModel::kNumLeg;i++){
      if(GetContactFlagFromMeas(i)){
        const Vec3 BrBL = BrBM_ + qMB_.inverseRotate(MrML_cur[i]);
        BrBL_inn[i] = BrBL
            - model_->forwardKinematicsBaseToFootInBaseFrame(meas_->kin_[i],i) + BrBL_noi[i];
      } else {
        BrBL_inn[i] = BrBL_noi[i];
      }
    }
  }
  void JacCur(MatX& J, const std::array<Vec3,KinModel::kNumLeg>& MrML_cur,
                       const std::array<Vec3,KinModel::kNumLeg>& BrBL_noi) const {
    J.setZero();
    for(int i=0;i<KinModel::kNumLeg;i++){
      if(GetContactFlagFromMeas(i)){
        this->template GetJacBlockCur<KIN, KIN>(J).template block<3,3>(3*i,3*i) =
            RotMat(qMB_.inverted()).matrix();
      }
    }
  }
  void JacNoi(MatX& J, const std::array<Vec3,KinModel::kNumLeg>& MrML_cur,
                       const std::array<Vec3,KinModel::kNumLeg>& BrBL_noi) const {
    J.setZero();
    for(int i=0;i<KinModel::kNumLeg;i++){
      this->template GetJacBlockNoi<KIN, KIN>(J).template block<3,3>(3*i,3*i) = Mat3::Identity();
    }
  }

  void SetModelPtr(const std::shared_ptr<KinModel>& model) {
    model_ = model;
  }

  void SetExtrinsics(Vec3 BrBM, Quat qMB){
    BrBM_  = BrBM;
    qMB_ = qMB;
  }

  bool GetContactFlagFromMeas(int i) const{
    return meas_->contact_flag_[i];
  }

  Vec3 GetMrMLFromMeas(int i) const{
    return qMB_.rotate(Vec3(model_->forwardKinematicsBaseToFootInBaseFrame(meas_->kin_[i],i)
                            - BrBM_));
  }

 protected:
  std::shared_ptr<KinModel> model_;
  Vec3 BrBM_;
  Quat qMB_;
};

}

#endif /* GIF_LEGKINEMATICUPDATE_HPP_ */