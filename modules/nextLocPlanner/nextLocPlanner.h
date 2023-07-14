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
#include <vector>
#include <map>
#include <algorithm>



class NextLocPlanner : public yarp::os::RFModule
{

private:  
    double                           m_period;
    std::string                      m_area;

    //Devices
    yarp::dev::PolyDriver            m_nav2DPoly;
    yarp::dev::Nav2D::INavigation2D* m_iNav2D{nullptr};

    //Ports
    yarp::os::RpcServer              m_rpc_server_port;

    //Locations
    std::vector<std::string>    m_all_locations;
    std::vector<std::string>    m_locations_unchecked;
    std::vector<std::string>    m_locations_checking;
    std::vector<std::string>    m_locations_checked;

public:
    NextLocPlanner();
    ~NextLocPlanner() = default;
    virtual bool configure(yarp::os::ResourceFinder &rf);
    virtual bool close();
    virtual double getPeriod();
    virtual bool updateModule();
    bool respond(const yarp::os::Bottle &cmd, yarp::os::Bottle &reply);
    bool setLocationStatus(const std::string location_name, const std::string& location_status);
    bool getCurrentCheckingLocation(std::string& location_name);
    bool getUncheckedLocations(std::vector<std::string>& location_list);
    bool getCheckedLocations(std::vector<std::string>& location_list);

private:
    double distRobotLocation(const std::string& location_name);


    template <typename A, typename B>
    void zip(
        const std::vector<A> &a, 
        const std::vector<B> &b, 
        std::vector<std::pair<A,B>> &zipped)
    {
        for(size_t i=0; i<a.size(); ++i)
        {
            zipped.push_back(std::make_pair(a[i], b[i]));
        }
    }

    template <typename A, typename B>
    void unzip(
        const std::vector<std::pair<A, B>> &zipped, 
        std::vector<A> &a, 
        std::vector<B> &b)
    {
        for(size_t i=0; i<a.size(); i++)
        {
            a[i] = zipped[i].first;
            b[i] = zipped[i].second;
        }
    }


};

#endif
