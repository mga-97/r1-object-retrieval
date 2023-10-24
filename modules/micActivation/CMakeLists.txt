project(micActivation)

add_executable(${PROJECT_NAME} src/Module.cpp main.cpp)
find_package(YARP REQUIRED COMPONENTS os dev)
target_include_directories(${PROJECT_NAME} PUBLIC include)
target_link_libraries(${PROJECT_NAME} ${YARP_LIBRARIES})
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
