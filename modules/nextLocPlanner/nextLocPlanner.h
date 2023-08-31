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

using namespace yarp::os;
using namespace yarp::dev;
using namespace yarp::dev::Nav2D;
using namespace std;


class NextLocPlanner : public RFModule
{

private:  
    double            m_period;
    string            m_area;

    //Devices
    PolyDriver        m_nav2DPoly;
    INavigation2D*    m_iNav2D{nullptr};

    //Ports
    RpcServer         m_rpc_server_port;

    //Locations
    vector<string>    m_all_locations;
    vector<string>    m_locations_unchecked;
    vector<string>    m_locations_checking;
    vector<string>    m_locations_checked;
    
    mutex             m_mutex;

public:
    NextLocPlanner();
    ~NextLocPlanner() = default;
    virtual bool configure(ResourceFinder &rf);
    virtual bool close();
    virtual double getPeriod();
    virtual bool updateModule();
    bool respond(const Bottle &cmd, Bottle &reply);
    bool setLocationStatus(const string location_name, const string& location_status);
    bool getCurrentCheckingLocation(string& location_name);
    bool getUncheckedLocations(vector<string>& location_list);
    bool getCheckedLocations(vector<string>& location_list);
    void sortUncheckedLocations();
    bool addLocation(string& loc);
    bool removeLocation(string& loc);

private:
    double distRobotLocation(const string& location_name);

    template <typename A, typename B>
    void zip(const vector<A> &a, const vector<B> &b,  vector<pair<A,B>> &zipped)
    {
        for(size_t i=0; i<a.size(); ++i)
        {
            zipped.push_back(make_pair(a[i], b[i]));
        }
    }

    template <typename A, typename B>
    void unzip(const vector<pair<A, B>> &zipped, vector<A> &a, vector<B> &b)
    {
        for(size_t i=0; i<a.size(); i++)
        {
            a[i] = zipped[i].first;
            b[i] = zipped[i].second;
        }
    }


};

#endif
