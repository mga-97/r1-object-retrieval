/*
 * Copyright (C) 2006-2020 Istituto Italiano di Tecnologia (IIT)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef APPROACH_OBJECT_EXECUTOR_H
#define APPROACH_OBJECT_EXECUTOR_H

#include <yarp/os/all.h>
#include <yarp/dev/PolyDriver.h>
#include <yarp/dev/IFrameTransform.h>
#include <yarp/dev/INavigation2D.h>
#include <yarp/sig/IntrinsicParams.h>
#include <yarp/dev/IRGBDSensor.h> 
#include <yarp/math/Math.h>
#include <cmath>
#include <map>


using namespace std;
using namespace yarp::os;
using namespace yarp::dev;
using namespace yarp::dev::Nav2D;
using namespace yarp::sig;

class ApproachObjectExecutor 
{
private:
    double                  m_safe_distance;
    ResourceFinder&         m_rf;
    bool                    m_ext_stop;

    //Ports
    BufferedPort<Bottle>    m_gaze_target_port;
    RpcClient               m_object_finder_rpc_port;
    BufferedPort<Bottle>    m_object_finder_result_port;
    BufferedPort<Bottle>    m_output_coordinates_port;
    string                  m_gaze_target_port_name;  
    string                  m_object_finder_rpc_port_name;
    string                  m_object_finder_result_port_name;
    string                  m_output_coordinates_port_name;

    //Devices
    PolyDriver              m_tcPoly;
    IFrameTransform*        m_iTc{nullptr}; 
    PolyDriver              m_nav2DPoly;
    INavigation2D*          m_iNav2D{nullptr}; 
    PolyDriver              m_rgbdPoly;
    IRGBDSensor*            m_iRgbd{nullptr}; 

    //Computation related attributes
    string                  m_base_frame_id;
    string                  m_camera_frame_id; 
    Property                m_propIntrinsics;
    IntrinsicParams         m_intrinsics;       
    
public:
    ApproachObjectExecutor(ResourceFinder &rf);
    ~ApproachObjectExecutor() = default;

    bool configure();
    void close();
    void exec(Bottle& b);
    bool lookAgain(string object);
    bool externalStop();    
};

#endif 