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

#ifndef NEXT_LOC_PLANNER_H
#define NEXT_LOC_PLANNER_H

#include <yarp/os/Log.h>
#include <yarp/os/LogStream.h>
#include <yarp/os/Network.h>
#include <yarp/os/RFModule.h>
#include <yarp/dev/PolyDriver.h>
#include <yarp/os/Time.h>
#include <yarp/os/Port.h>
#include <yarp/dev/INavigation2D.h>


class NextLocPlanner : public yarp::os::RFModule
{

enum LocationStatus {
    LOC_PLAN_UNCHECKED = 0,
    LOC_PLAN_CHECKING = 1,
    LOC_PLAN_CHECKED = 2   
};

private:  
    double                      m_period;
    std::string                 m_area;
    std::vector<std::string>    m_research_locations;
    std::vector<LocationStatus> m_status_locations;

    //Devices
    yarp::dev::PolyDriver            m_nav2DPoly;
    yarp::dev::Nav2D::INavigation2D* m_iNav2D{nullptr};

    //Ports
    yarp::os::RpcServer         m_rpc_server_port;;

public:
    NextLocPlanner();
    ~NextLocPlanner() = default;
    virtual bool configure(yarp::os::ResourceFinder &rf);
    virtual bool close();
    virtual double getPeriod();
    virtual bool updateModule();
    bool respond(const yarp::os::Bottle &cmd, yarp::os::Bottle &reply);
    void setLocationStatus(const std::string& location_name, const LocationStatus& ls);

};

#endif
