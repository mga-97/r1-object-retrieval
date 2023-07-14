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

#ifndef HEAD_ORIENTATOR_H
#define HEAD_ORIENTATOR_H

#include <yarp/os/Log.h>
#include <yarp/os/LogStream.h>
#include <yarp/dev/PolyDriver.h>
#include <yarp/dev/IRGBDSensor.h>
#include <yarp/os/Time.h>
#include <yarp/os/Port.h>
#include <yarp/os/RFModule.h>
#include <yarp/dev/ControlBoardInterfaces.h>
#include <map>
#include <vector>
#include <algorithm>

//Defaults RGBD sensor
#define RGBDClient            "RGBDSensorClient"
#define RGBDLocalImagePort    "/headOrientator/clientRgbPort:i"
#define RGBDLocalDepthPort    "/headOrientator/clientDepthPort:i"
#define RGBDLocalRpcPort      "/headOrientator/clientRpcPort"
#define RGBDRemoteImagePort   "/cer/depthCamera/rgbImage:o"
#define RGBDRemoteDepthPort   "/cer/depthCamera/depthImage:o"
#define RGBDRemoteRpcPort     "/cer/depthCamera/rpc:i"
#define RGBDImageCarrier      "mjpeg"
#define RGBDDepthCarrier      "fast_tcp"

class HeadOrientator : public yarp::os::RFModule
{

enum HeadOrientStatus {
    HO_UNCHECKED = 0,
    HO_CHECKING = 1,
    HO_CHECKED = 2   
};

private:
    //Polydriver
    yarp::dev::PolyDriver            m_Poly;
    yarp::dev::IRemoteCalibrator*    m_iremcal;       
    yarp::dev::IControlMode*         m_ictrlmode;     
    yarp::dev::IPositionControl*     m_iposctrl;
    yarp::dev::IControlLimits*       m_ilimctrl;      

    yarp::dev::PolyDriver            m_rgbdPoly;
    yarp::dev::IRGBDSensor*          m_iRgbd{nullptr};
    
    //Ports
    yarp::os::RpcServer              m_rpc_server_port;

    std::map<std::string, std::string>          m_orientations;
    std::map<std::string, HeadOrientStatus>     m_orientation_status;

    double                           m_period;

public:
    //Constructor/Distructor
    HeadOrientator(){}
    ~HeadOrientator(){}

    //Internal methods
    bool configure(yarp::os::ResourceFinder &rf);
    virtual double getPeriod();
    virtual bool updateModule();
    virtual bool  close();

    void home();
    bool respond(const yarp::os::Bottle &cmd, yarp::os::Bottle &reply);
    bool setLocationStatus(const std::string& location_name, const HeadOrientStatus& ls);

};

#endif //HEAD_ORIENTATOR_H
