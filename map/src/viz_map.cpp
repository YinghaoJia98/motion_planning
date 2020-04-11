/// \file
/// \brief Draws Each Obstacle in RViz using MarkerArrays
///
/// PARAMETERS:
/// PUBLISHES:
/// SUBSCRIBES:
/// FUNCTIONS:

#include <ros/ros.h>

#include <math.h>
#include <string>
#include <vector>

#include "map/map.hpp"
#include "nuslam/TurtleMap.h"

#include <functional>  // To use std::bind
#include <visualization_msgs/Marker.h>
#include <visualization_msgs/MarkerArray.h>
#include <geometry_msgs/Point.h>

// Used to convert YAML list of lists to std::vector
#include <xmlrpcpp/XmlRpcValue.h> // catkin component


int main(int argc, char** argv)
/// The Main Function ///
{
  ROS_INFO("STARTING NODE: viz_map");

  // Vars
  double frequency = 10.0;
  // Scale by which to transform marker poses (10 cm/cell so we use 10)
  double SCALE = 10.0;
  std::string frame_id = "base_footprint";
  XmlRpc::XmlRpcValue xml_obstacles;

  // store Obstacle(s) here to create Map
  std::vector<map::Obstacle> obstacles_v;

  ros::init(argc, argv, "viz_map_node"); // register the node on ROS
  ros::NodeHandle nh; // get a handle to ROS
  ros::NodeHandle nh_("~"); // get a handle to ROS
  // Parameters
  nh_.getParam("frequency", frequency);
  nh_.getParam("obstacles", xml_obstacles);

  // 'obstacles' is a triple-nested list.
  // 1st level: obstacle (Obstacle), 2nd level: vertices (std::vector), 3rd level: coordinates (Vector2D)

  // std::vector<Obstacle>
  if(xml_obstacles.getType() != XmlRpc::XmlRpcValue::TypeArray)
  {
    ROS_ERROR("There is no list of obstacles");
  } else {
    for(int i = 0; i < xml_obstacles.size(); ++i)
    {
      // Obstacle contains std::vector<Vector2D>
      // create Obstacle with empty vertex vector
      map::Obstacle obs;
      if(xml_obstacles[i].getType() != XmlRpc::XmlRpcValue::TypeArray)
      {
          ROS_ERROR("obstacles[%d] has no vertices", i);
      } else {
          for(int j = 0; j < xml_obstacles[i].size(); ++j)
          {
            // Vector2D contains x,y coords
            if(xml_obstacles[i][j].size() != 2)
            {
               ROS_ERROR("Vertex[%d] of obstacles[%d] is not a pair of coordinates", j, i);
            } else if(
              xml_obstacles[i][j][0].getType() != XmlRpc::XmlRpcValue::TypeDouble or
              xml_obstacles[i][j][1].getType() != XmlRpc::XmlRpcValue::TypeDouble)
            {
              ROS_ERROR("The coordinates of vertex[%d] of obstacles[%d] are not doubles", j, i);
            } else {
              // PASSED ALL THE TESTS: push Vector2D to vertices list in Obstacle object
              rigid2d::Vector2D vertex(xml_obstacles[i][j][0], xml_obstacles[i][j][1]);
              obs.vertices.push_back(vertex);
            }
          }
      }
      // push Obstacle object to vector of Object(s) in Map
      obstacles_v.push_back(obs);
    }
  }

  // Create Map
  map::Map map(obstacles_v);

  // Initialize Marker
  // Init Marker Array Publisher
  ros::Publisher marker_pub = nh.advertise<visualization_msgs::MarkerArray>("viz_map", 1);

  // Init Marker Array
  visualization_msgs::MarkerArray marker_arr;

  // Init Marker
  visualization_msgs::Marker marker;
  marker.header.frame_id = frame_id;
  marker.header.stamp = ros::Time::now();
  // marker.ns = "my_namespace";
  // marker.id = 0;
  marker.type = visualization_msgs::Marker::LINE_STRIP;
  marker.action = visualization_msgs::Marker::ADD;
  marker.pose.position.z = 0;
  marker.pose.orientation.x = 0.0;
  marker.pose.orientation.y = 0.0;
  marker.pose.orientation.z = 0.0;
  marker.pose.orientation.w = 1.0;
  marker.scale.x = 0.05;
  marker.color.r = 0.5f;
  marker.color.g = 0.0f;
  marker.color.b = 0.5f;
  marker.color.a = 1.0;
  marker.lifetime = ros::Duration();

  // Now, loop through obstacles in map to create line strips.
  // Each obstacle = 1 line strip
  for (auto obs_iter = obstacles_v.begin(); obs_iter != obstacles_v.end(); obs_iter++)
  {
    marker.points.clear();
    marker.id = std::distance(obstacles_v.begin(), obs_iter);
    // Use 1st point as marker pose
    marker.pose.position.x = obs_iter->vertices.at(0).x / SCALE;
    marker.pose.position.y = obs_iter->vertices.at(0).y / SCALE;
    for (auto v_iter = obs_iter->vertices.begin(); v_iter != obs_iter->vertices.end(); v_iter++)
    {
      geometry_msgs::Point new_vertex;
      new_vertex.x = v_iter->x / SCALE; // NOTE: SCALE DOWN
      new_vertex.y = v_iter->y / SCALE;
      new_vertex.z = 0.0;
      marker.points.push_back(new_vertex);
    }
    // Before pushing back, connect last vertex to first vertex
    geometry_msgs::Point new_vertex;
    new_vertex.x = obs_iter->vertices.at(0).x / SCALE;
    new_vertex.y = obs_iter->vertices.at(0).y / SCALE;
    new_vertex.z = 0.0;
    marker.points.push_back(new_vertex);
    marker_arr.markers.push_back(marker);
  }

  ros::Rate rate(frequency);

  // Main While
  while (ros::ok())
  {
  	ros::spinOnce();

    // Publish Map
    marker_pub.publish(marker_arr);

    rate.sleep();
  }

  return 0;
}