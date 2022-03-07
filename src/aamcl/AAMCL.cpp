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

#include "aamcl/AAMCL.h"

#include "ros/ros.h"

namespace aamcl
{

AAMCL::AAMCL()
: nh_()
{
  pub_particles_ = nh_.advertise<geometry_msgs::PoseArray>("poses", 100);
  init();
}

AAMCL::init()
{
  particles_.resize(NUM_PART);

  for (auto & particle : particles_) {
    particle.prob = 1.0 / NUM_PART;
    particle.pose.setOrigin(tf2::Vector3(0.0, 0.0, 0.0));
    particle.pose.setRotation(tf2::Quaternion(0.0, 0.0, 0.0));
  }
}

void
AAMCL::step()
{
  if (pub_particles_.getNumSubscribers() > 0) {
    publish_particles();
  }
}

void
AAMCL::publish_particles()
{
  geometry_msgs::PoseArray msg;

  for (auto & particle : particles_) {
    geometry_msgs::Pose pose_msg;

    const auto translation = particle.pose.getOrigin();
    const auto rotation = particle.pose.getRotation();

    tf2:: pose_msg.position = translation. .x = translation.x();
    pose_msg.position.y = translation.y();
    pose_msg.position.z = translation.z();

    pose_msg.position.x = translation.x();
    pose_msg.position.y = translation.y();
    pose_msg.position.z = translation.z();

    particle.prob = 1.0 / NUM_PART;
    particle.pose.setOrigin(tf2::Vector3(0.0, 0.0, 0.0));
    particle.pose.setRotation(tf2::Quaternion(0.0, 0.0, 0.0));
  }
}

}  // namespace aamcl
