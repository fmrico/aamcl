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

#ifndef AAMCL_AAMCL_H
#define AAMCL_AAMCL_H

#include "geometry_msgs/PoseArray.h"

#include "tf2/LinearMath/Transform.h"
#include "ros/ros.h"


namespace aamcl
{

typedef struct {
  tf2::Transform pose;
  double prob;
} Particle;


class AAMCL
{
public:
  AAMCL();

  void init();
  void step();

protected:
  void publish_particles();

private:
  ros::NodeHandle nh_;
  ros::Publisher pub_particles_;

  static const int NUM_PART = 1;

  std::vector<Particle> particles_; 

  
};

}  // namespace aamcl

#endif  // AAMCL_AAMCL_H
