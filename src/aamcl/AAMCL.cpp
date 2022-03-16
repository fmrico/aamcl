// Copyright 2022 Intelligent Robotics Lab
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "tf2/transform_datatypes.h"
#include "tf2_ros/transform_listener.h"
#include "tf2/LinearMath/Transform.h"
#include "geometry_msgs/TransformStamped.h"
#include "tf2_geometry_msgs/tf2_geometry_msgs.h"
#include "tf2/convert.h"
#include "geometry_msgs/Twist.h"

#include "aamcl/AAMCL.h"

#include "ros/ros.h"

namespace aamcl
{

AAMCL::AAMCL()
: nh_(),
  buffer_(),
  listener_(buffer_)
{
  pub_particles_ = nh_.advertise<geometry_msgs::PoseArray>("poses", 100);
  init();
}

void
AAMCL::init()
{
  particles_.resize(NUM_PART);

  for (auto & particle : particles_) {
    particle.prob = 1.0 / NUM_PART;
    particle.pose.setOrigin(tf2::Vector3(0.0, 0.0, 0.0));

    tf2::Quaternion q;
    q.setEuler(0.0, 0.0, 0.0);
    particle.pose.setRotation(q);
  }
}

void
AAMCL::step()
{
  predict();

  publish_particles();
}

void
AAMCL::predict()
{
  geometry_msgs::TransformStamped odom2bf_msg;
  std::string error;
  if (buffer_.canTransform("odom", "base_footprint", ros::Time(0), ros::Duration(0.1), &error))
  {
      odom2bf_msg = buffer_.lookupTransform("odom", "base_footprint", ros::Time(0));

      tf2::Stamped<tf2::Transform> odom2bf;
      tf2::fromMsg(odom2bf_msg, odom2bf);

      if (valid_prev_odom2bf_) {
        tf2::Transform bfprev2bf = odom2prevbf_.inverse() * odom2bf;

        for (auto & particle : particles_) {
        }
      }

      valid_prev_odom2bf_ = true;
      odom2prevbf_ = odom2bf;
  }
}

void
AAMCL::publish_particles()
{
  if (pub_particles_.getNumSubscribers() == 0) {
    return;
  }

  geometry_msgs::PoseArray msg;

  msg.header.frame_id = "map";
  msg.header.stamp = ros::Time::now();

  for (auto & particle : particles_) {
    geometry_msgs::Pose pose_msg;

    const auto translation = particle.pose.getOrigin();
    const auto rotation = particle.pose.getRotation();

    pose_msg.position.x = translation.x();
    pose_msg.position.y = translation.y();
    pose_msg.position.z = translation.z();

    pose_msg.orientation.x = rotation.x();
    pose_msg.orientation.y = rotation.y();
    pose_msg.orientation.z = rotation.z();
    pose_msg.orientation.w = rotation.w();

    msg.poses.push_back(pose_msg);
  }

  pub_particles_.publish(msg);
}

}  // namespace aamcl
