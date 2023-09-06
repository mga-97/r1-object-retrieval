#!/bin/bash

ROS_DISTRO="humble"
IMAGE_NAME="colombraf/r1images:r1ObjectRetrievalSim"

sudo docker build --build-arg ros_distro=$ROS_DISTRO -t $IMAGE_NAME .
