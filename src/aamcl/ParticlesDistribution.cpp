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

#include <random>
#include <cmath>

#include "visualization_msgs/MarkerArray.h"
#include "sensor_msgs/LaserScan.h"

#include "costmap_2d/costmap_2d.h"
#include "costmap_2d/cost_values.h"

#include "aamcl/ParticlesDistribution.h"

#include "ros/ros.h"

namespace aamcl
{

ParticlesDistribution::ParticlesDistribution()
: nh_(),
  buffer_(),
  listener_(buffer_)
{
  nh_.param<int>("num_particles", NUM_PART, 200);
  pub_particles_ = nh_.advertise<visualization_msgs::MarkerArray>("poses", NUM_PART);
}

void 
ParticlesDistribution::init()
{
  particles_.resize(NUM_PART);

  for (auto & particle : particles_)
  {
    particle.prob = 1.0 / NUM_PART;
    particle.pose.setOrigin(tf2::Vector3(0.0, 0.0, 0.0));
    particle.pose.setRotation(tf2::Quaternion(0.0, 0.0, 0.0, 1.0));
  }
}

void
ParticlesDistribution::init(const tf2::Transform & pose_init)
{
  init();
  nh_.param<double>("noise_x", noise_x_param, 0.1);
  nh_.param<double>("noise_x", noise_y_param, 0.1);
  nh_.param<double>("noise_x", noise_t_param, 0.1);
  std::normal_distribution<double> noise_x(0, noise_x_param);
  std::normal_distribution<double> noise_y(0, noise_y_param);
  std::normal_distribution<double> noise_t(0, noise_t_param);

  for (auto & particle : particles_)
  {
    particle.pose = pose_init;

    tf2::Vector3 pose = particle.pose.getOrigin();
    pose.setX(pose.getX() + noise_x(generator_));
    pose.setY(pose.getY() + noise_y(generator_));

    particle.pose.setOrigin(pose);

    double roll, pitch, yaw;
    tf2::Matrix3x3(particle.pose.getRotation()).getRPY(roll, pitch, yaw);

    double newyaw = yaw + noise_t(generator_);

    tf2::Quaternion q;
    q.setRPY(roll, pitch, newyaw);
    
    particle.pose.setRotation(q);
  }
}

void
ParticlesDistribution::predict(const tf2::Transform & movement)
{
  for (auto & particle : particles_)
  {
    particle.pose =  particle.pose * movement;// * add_noise(movement);
  }
}

tf2::Transform
ParticlesDistribution::add_noise(const tf2::Transform & dm)
{
  tf2::Transform returned_noise;
  nh_.param<double>("translation_noise", translation_noise_param, 0.1);
  nh_.param<double>("rotation_noise", rotation_noise_param, 0.1);
  std::normal_distribution<double> translation_noise_(0.0, translation_noise_param);
  std::normal_distribution<double> rotation_noise_(0.0, rotation_noise_param);

  double noise_tra = translation_noise_(generator_);
  double noise_rot = rotation_noise_(generator_);

  returned_noise.setOrigin(dm.getOrigin()* noise_tra);

  double roll, pitch, yaw;
  tf2::Matrix3x3(dm.getRotation()).getRPY(roll, pitch, yaw);

  double newyaw = yaw + noise_rot;

  tf2::Quaternion q;
  q.setRPY(roll, pitch, newyaw);
  returned_noise.setRotation(q);

  return returned_noise;
}

void
ParticlesDistribution::publish_particles()
{
  if (pub_particles_.getNumSubscribers() == 0)
  {
    return;
  }

  visualization_msgs::MarkerArray msg;

  int counter = 0;
  for (auto & particle : particles_)
  {
    visualization_msgs::Marker pose_msg;

    pose_msg.header.frame_id = "map";
    pose_msg.header.stamp = ros::Time::now();
    pose_msg.id = counter++;
    pose_msg.type = visualization_msgs::Marker::ARROW;
    pose_msg.type = visualization_msgs::Marker::ADD;    

    const auto translation = particle.pose.getOrigin();
    const auto rotation = particle.pose.getRotation();

    pose_msg.pose.position.x = translation.x();
    pose_msg.pose.position.y = translation.y();
    pose_msg.pose.position.z = translation.z();

    pose_msg.pose.orientation.x = rotation.x();
    pose_msg.pose.orientation.y = rotation.y();
    pose_msg.pose.orientation.z = rotation.z();
    pose_msg.pose.orientation.w = rotation.w();

    pose_msg.scale.x = 0.1;
    pose_msg.scale.y = 0.01;
    pose_msg.scale.z = 0.01;

    pose_msg.color.r = (1.0 - particle.prob);
    pose_msg.color.g = particle.prob;
    pose_msg.color.b = 0.0;
    pose_msg.color.a = 1.0;

    msg.markers.push_back(pose_msg);
  }

  pub_particles_.publish(msg);
}

void
ParticlesDistribution::correct_once(const sensor_msgs::LaserScan & scan, const costmap_2d::Costmap2D & costmap)
{
  //std::cerr << "====================================" << std::endl;

  // if (!bf2laser_init_) {
  //   bf2laser_init_ = true;
    std::string error;
    if (buffer_.canTransform(
      scan.header.frame_id, "base_footprint", scan.header.stamp), ros::Duration(0.1), &error)
    {
      //std::cerr << ")))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))" <<  scan.header.frame_id << std::endl;
      auto bf2laser_msg = buffer_.lookupTransform(
        "base_footprint", scan.header.frame_id, scan.header.stamp);
      tf2::fromMsg(bf2laser_msg, bf2laser_);

    } else {
      ROS_WARN("Timeout while waiting TF %s -> base_footprint [%s]",
        scan.header.frame_id.c_str(), error.c_str());
      return;
    }
  // }

  /*for (int i = 0; i < NUM_PART; i++)
  {
    const auto & p = particles_[i];
    double roll, pitch, yaw;

    tf2::Matrix3x3(p.pose.getRotation()).getRPY(roll, pitch, yaw);
    //std::cerr << "-> [" << i <<"] (" << p.pose.getOrigin().x() << ", " << p.pose.getOrigin().y() << 
    //  ", " << yaw << ") => " << p.prob << std::endl;
  }*/

  for (int j = 0; j < scan.ranges.size(); j++) {

    if (std::isinf(scan.ranges[j]) || std::isnan(scan.ranges[j]) || scan.ranges[j] < scan.range_min || scan.ranges[j] > scan.range_max) {
      continue;
    }
    nh_.param<double>("random_read_noise", random_read_noise_param, 0.01);    
    tf2::Transform laser2point = get_random_read_with_noise(scan, j, random_read_noise_param);

    for (int i = 0; i < NUM_PART; i++)
    {
      auto & p = particles_[i];

      auto map2point = p.pose * bf2laser_ * laser2point;

      /*if (i == 0) {
        {
          double roll, pitch, yaw;
          tf2::Matrix3x3(p.pose.getRotation()).getRPY(roll, pitch, yaw);
          std::cerr << "=== p.pose [" << i <<"] (" << p.pose.getOrigin().x() << ", " << p.pose.getOrigin().y() << 
            ", " << yaw << ")" << std::endl;
        }
        {
          double roll, pitch, yaw;
          tf2::Matrix3x3(bf2laser_.getRotation()).getRPY(roll, pitch, yaw);
          std::cerr << "=== bf2laser_ [" << i <<"] (" << bf2laser_.getOrigin().x() << ", " << bf2laser_.getOrigin().y() << 
            ", " << yaw << ")" << std::endl;
        }        
        {
          double roll, pitch, yaw;
          tf2::Matrix3x3(laser2point.getRotation()).getRPY(roll, pitch, yaw);
          std::cerr << "=== laser2point [" << i <<"] (" << laser2point.getOrigin().x() << ", " << laser2point.getOrigin().y() << 
            ", " << yaw << ")" << std::endl;
        }      
        {
          double roll, pitch, yaw;
          tf2::Matrix3x3(map2point.getRotation()).getRPY(roll, pitch, yaw);
          std::cerr << "=== map2point [" << i <<"] (" << map2point.getOrigin().x() << ", " << map2point.getOrigin().y() << 
            ", " << yaw << ")" ;
        }    

      }*/

      unsigned int mx, my;
      if (costmap.worldToMap(map2point.getOrigin().x(), map2point.getOrigin().y(), mx, my)) {
        auto cost = costmap.getCost(mx, my);

        if (cost == costmap_2d::LETHAL_OBSTACLE) {
          // std::cerr << "^"<< std::endl;
          p.prob = std::clamp(p.prob + (1.0 / static_cast<double>(scan.ranges.size())), 0.0, 1.0);
        } else {
          // std::cerr << "v"<< std::endl;
          p.prob = std::clamp(p.prob - (1.0 / static_cast<double>(scan.ranges.size())), 0.0, 1.0);
        }
      }
    }
  }


  /*for (int i = 0; i < NUM_PART; i++)
  {
    const auto & p = particles_[i];
    double roll, pitch, yaw;

    tf2::Matrix3x3(p.pose.getRotation()).getRPY(roll, pitch, yaw);
    std::cerr << "<- [" << i <<"] (" << p.pose.getOrigin().x() << ", " << p.pose.getOrigin().y() << 
      ", " << yaw << ") => " << p.prob << std::endl;
  }*/


}

tf2::Transform 
ParticlesDistribution::get_random_read_with_noise(const sensor_msgs::LaserScan & scan, int index, double noise)
{
  // std::uniform_int_distribution<int> selector(0, scan.ranges.size() - 1);
  std::normal_distribution<double> dist_noise(0.0, noise);
  
  // int index = selector(generator_);

  double dist = scan.ranges[index];
  double dist_with_noise = dist;//  + dist_noise(generator_);

  double angle = scan.angle_min + static_cast<double>(index) * scan.angle_increment;

  tf2::Transform ret;
  
  double x = dist_with_noise * cos(angle);
  double y = dist_with_noise * sin(angle);

  // std::cerr << "******* a[" << angle << "] d["<< dist_with_noise << "] -> (" << x << ", " << y << ")" << std::endl;

  ret.setOrigin({x, y, 0.0});
  ret.setRotation({0.0, 0.0, 0.0, 1.0});

  return ret;
}

void
ParticlesDistribution::reseed()
{
  // Sort particles by prob
  std::sort(particles_.begin(), particles_.end(),
   [](const Particle & a, const Particle & b) -> bool
  { 
    return a.prob > b.prob; 
  });
  nh_.param<double>("loser_percent", percentage_losers, 0.3);
   nh_.param<double>("winner_percent", percentage_winners, 0.1); 

  int number_losers = particles_.size() * percentage_losers;
  int number_no_losers = particles_.size() - number_losers;
  int number_winners = particles_.size() * percentage_winners;

  std::vector<Particle> new_particles(particles_.begin(), particles_.begin() + number_no_losers);
  
  std::normal_distribution<double> selector(0, number_winners);
  nh_.param<double>("noise_x", noise_x_param, 0.1); //estos valores no se si hay que poner los mismos que en el correct, o habría que cambiarlos, estaban cambiados pero los he modificado para igualarlos.
  nh_.param<double>("noise_y", noise_y_param, 0.1);
  nh_.param<double>("noise_x", noise_t_param, 0.05);
  std::normal_distribution<double> noise_x(0, noise_x_param);
  std::normal_distribution<double> noise_y(0, noise_y_param);
  std::normal_distribution<double> noise_t(0, noise_t_param);

  for (int i = 0; i < number_losers; i++)
  {
    int index = std::clamp(static_cast<int>(selector(generator_)), 0, number_winners);

    Particle p;
    p.prob = new_particles.back().prob;

    auto w_pose = particles_[i].pose.getOrigin();

    double nx = noise_x(generator_);
    double ny = noise_y(generator_);

    p.pose.setOrigin({w_pose.x() + nx, w_pose.y() + ny, w_pose.z()});
    
    double roll, pitch, yaw;

    tf2::Matrix3x3(particles_[i].pose.getRotation()).getRPY(roll, pitch, yaw);

    double newyaw = yaw + noise_t(generator_);

    tf2::Quaternion q;
    q.setRPY(roll, pitch, newyaw);
    
    p.pose.setRotation(q);

    new_particles.push_back(p);
  }

  particles_ = new_particles;
  
  /*
  // Normalize the distribution ton sum 1.0
  double sum = 0.0;
  std::for_each(particles_.begin(), particles_.end(),
    [&sum] (const Particle & p)
    {
      sum += p.prob;
    });
  
  double norm_factor = 1.0;
  if (sum > 0.0) {
    norm_factor = 1.0 / sum;
  }

  std::for_each(particles_.begin(), particles_.end(),
    [norm_factor] (Particle & p)
    {
      p.prob = p.prob * norm_factor;
    });
  */
}

}  // namespace aamcl
