#############################
# CMake - ROS Bridge - root/src
#############################

set(ROS_BRIDGE_SOURCES
   rosBridge.cpp
   websocket_session.cpp
   )

add_executable(mohses_ros_bridge ${ROS_BRIDGE_SOURCES})

target_include_directories(mohses_ros_bridge PUBLIC)

target_link_libraries(
   mohses_ros_bridge
   PUBLIC amm_std
   PUBLIC Boost::thread
   PUBLIC tinyxml2
)

install(TARGETS mohses_ros_bridge RUNTIME DESTINATION bin)
install(DIRECTORY ../config DESTINATION bin)
