#include "robot_engine/kinematics/reference_arm.hpp"

#include <cmath>
#include <stdexcept>
#include <string>

#include <Eigen/Geometry>
#include <pinocchio/algorithm/frames.hpp>
#include <pinocchio/multibody/joint/joint-revolute.hpp>
#include <pinocchio/multibody/model.hpp>

namespace robot_engine::kinematics {
namespace {

using pinocchio::Frame;
using pinocchio::FrameIndex;
using pinocchio::JointIndex;
using pinocchio::Model;
using pinocchio::SE3;

struct ReferenceModel {
  Model model;
  FrameIndex tool_frame{};
};

[[nodiscard]] SE3 translated(const double x, const double y, const double z) {
  return {Eigen::Matrix3d::Identity(), Eigen::Vector3d{x, y, z}};
}

template <typename JointModel>
[[nodiscard]] JointIndex
add_joint(Model &model, const JointIndex parent, const JointModel &joint,
          const SE3 &placement, const std::string &name) {
  const auto joint_id = model.addJoint(parent, joint, placement, name);
  model.addJointFrame(joint_id);
  return joint_id;
}

[[nodiscard]] ReferenceModel make_reference_model() {
  ReferenceModel reference;
  auto &model = reference.model;
  model.name = "reference_arm_7dof";

  JointIndex parent = 0;
  parent = add_joint(model, parent, pinocchio::JointModelRZ{},
                     translated(0.0, 0.0, 0.34), "joint_1");
  parent = add_joint(model, parent, pinocchio::JointModelRY{},
                     translated(0.0, 0.0, 0.20), "joint_2");
  parent = add_joint(model, parent, pinocchio::JointModelRZ{},
                     translated(0.32, 0.0, 0.0), "joint_3");
  parent = add_joint(model, parent, pinocchio::JointModelRY{},
                     translated(0.30, 0.0, 0.0), "joint_4");
  parent = add_joint(model, parent, pinocchio::JointModelRZ{},
                     translated(0.24, 0.0, 0.0), "joint_5");
  parent = add_joint(model, parent, pinocchio::JointModelRY{},
                     translated(0.18, 0.0, 0.0), "joint_6");
  parent = add_joint(model, parent, pinocchio::JointModelRZ{},
                     translated(0.12, 0.0, 0.0), "joint_7");

  reference.tool_frame = model.addFrame(
      Frame{"tool0", parent, translated(0.10, 0.0, 0.0), pinocchio::OP_FRAME});
  return reference;
}

[[nodiscard]] const ReferenceModel &reference_model() {
  static const ReferenceModel reference = make_reference_model();
  return reference;
}

} // namespace

Pose forward_reference_arm(
    const std::array<double, kReferenceArmJointCount> &joint_positions_rad) {
  for (const auto position : joint_positions_rad) {
    if (!std::isfinite(position)) {
      throw std::invalid_argument("joint positions must be finite");
    }
  }

  const auto &reference = reference_model();
  pinocchio::Data data{reference.model};
  Eigen::VectorXd configuration(reference.model.nq);
  for (std::size_t index = 0; index < joint_positions_rad.size(); ++index) {
    configuration[static_cast<Eigen::Index>(index)] =
        joint_positions_rad[index];
  }

  pinocchio::framesForwardKinematics(reference.model, data, configuration);
  const auto &placement = data.oMf[reference.tool_frame];
  Eigen::Quaterniond orientation{placement.rotation()};
  orientation.normalize();

  return {
      .translation_m = {placement.translation().x(),
                        placement.translation().y(),
                        placement.translation().z()},
      .orientation_xyzw = {orientation.x(), orientation.y(), orientation.z(),
                           orientation.w()},
  };
}

} // namespace robot_engine::kinematics
