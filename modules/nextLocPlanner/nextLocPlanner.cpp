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


#include "nextLocPlanner.h"
#include <algorithm>

YARP_LOG_COMPONENT(NEXT_LOC_PLANNER, "r1_obr.nextLocPlanner")

NextLocPlanner::NextLocPlanner() :
    m_period(1.0),
    m_area("cris_new")
{  
}

/****************************************************************/
bool NextLocPlanner::configure(yarp::os::ResourceFinder &rf) 
{   
    m_period = rf.check("period")  ? rf.find("period").asFloat32() : 1.0;
    m_area   = rf.check("area")    ? rf.find("area").asString()    : "cris_new";

    //Open RPC Server Port
    std::string rpcPortName = rf.check("rpcPort") ? rf.find("rpcPort").asString() : "/nextLocPlanner/request/rpc";
    if (!m_rpc_server_port.open(rpcPortName))
    {
        yCError(NEXT_LOC_PLANNER, "open() error could not open rpc port %s, check network", rpcPortName.c_str());
        return false;
    }
    if (!attach(m_rpc_server_port))
    {
        yCError(NEXT_LOC_PLANNER, "attach() error with rpc port %s", rpcPortName.c_str());
        return false;
    }

    //Navigation2DClient config 
    yarp::os::Property nav2DProp;
        //Defaults
    nav2DProp.put("device", "navigation2D_nwc_yarp");
    nav2DProp.put("local", "/nextLocPlanner/navClient");
    nav2DProp.put("navigation_server", "/navigation2D_nws_yarp");
    nav2DProp.put("map_locations_server", "/map2D_nws_yarp");
    nav2DProp.put("localization_server", "/localization2D_nws_yarp");
    if(!rf.check("NAVIGATION_CLIENT"))
    {
        yCWarning(NEXT_LOC_PLANNER,"NAVIGATION_CLIENT section missing in ini file. Using the default values");
    }
    else
    {
        yarp::os::Searchable& nav_config = rf.findGroup("NAVIGATION_CLIENT");
        if(nav_config.check("device")) {nav2DProp.put("device", nav_config.find("device").asString());}
        if(nav_config.check("local")) {nav2DProp.put("local", nav_config.find("local").asString());}
        if(nav_config.check("navigation_server")) {nav2DProp.put("navigation_server", nav_config.find("navigation_server").asString());}
        if(nav_config.check("map_locations_server")) {nav2DProp.put("map_locations_server", nav_config.find("map_locations_server").asString());}
        if(nav_config.check("localization_server")) {nav2DProp.put("localization_server", nav_config.find("localization_server").asString());}
        }
   
    m_nav2DPoly.open(nav2DProp);
    if(!m_nav2DPoly.isValid())
    {
        yCWarning(NEXT_LOC_PLANNER,"Error opening PolyDriver check parameters. Using the default values");
    }
    m_nav2DPoly.view(m_iNav2D);
    if(!m_iNav2D){
        yCError(NEXT_LOC_PLANNER,"Error opening INavigation2D interface. Device not available");
        return false;
    }

    //Load locally the locations
    if(!m_iNav2D->getLocationsList(m_research_locations)) 
    {
        yCError(NEXT_LOC_PLANNER,"Error getting locations list from map server");
        return false;
    }

    if(m_research_locations.size()>0) //remove elements not belonging to the area defined in .ini file (i.e. whose name starts with the name of the area)
    {
        auto new_end = std::remove_if(m_research_locations.begin(), m_research_locations.end(), [this](std::string& str){return (str.find(m_area) == std::string::npos);} );
        m_research_locations.erase(new_end, m_research_locations.end());
    }
    
    m_status_locations = std::vector<LocationStatus>(m_research_locations.size(), LOC_PLAN_UNCHECKED);
 
    return true;
}

/****************************************************************/
bool NextLocPlanner::close()
{

    if (!m_rpc_server_port.asPort().isOpen())
        m_rpc_server_port.close();

    if(m_nav2DPoly.isValid())
        m_nav2DPoly.close();
       
    return true;
}

/****************************************************************/
double NextLocPlanner::getPeriod()
{
    return m_period;
}


/****************************************************************/
void NextLocPlanner::setLocationStatus(const std::string& location_name, const LocationStatus& ls)
{
    std::vector<std::string>::iterator it = std::find (m_research_locations.begin(), m_research_locations.end(), location_name);
    int idx { (int)distance(it, m_research_locations.begin()) };
    m_status_locations[idx] = ls;

}


/****************************************************************/
bool NextLocPlanner::respond(const yarp::os::Bottle &cmd, yarp::os::Bottle &reply)
{
    reply.clear();
    std::string cmd_0=cmd.get(0).asString();
    if (cmd.size()==1)
    {
        if (cmd_0=="next")
        {
            if (std::find(m_status_locations.begin(), m_status_locations.end(), LOC_PLAN_UNCHECKED) != m_status_locations.end()) // At least one location unchecked
            {
                //Extract first unchecked location index
                std::vector<LocationStatus>::iterator it = std::find_if (m_status_locations.begin(), m_status_locations.end(), [](const LocationStatus& ls){return (ls == LOC_PLAN_UNCHECKED);});
                int locationIdx { (int)distance(it, m_status_locations.begin()) };
                std::string nextLocation = m_research_locations[locationIdx];

                reply.addString(nextLocation);

                //Mark the location status as "Checking"
                *it = LOC_PLAN_CHECKING;
            }
            else
            {
                reply.addString("noLocation");
            }           
        }
        else if (cmd_0=="help")
        {
            reply.addVocab32("many");
            reply.addString("next : responds the next unchecked location or noLocation");
            reply.addString("set <locationName> <status> : sets the status of a location to unchecked, checking or checked");
            reply.addString("set all <status> : sets the status of all locations");
            reply.addString("list : lists all the locations and their status");
            reply.addString("stop : stops the nextLocationPlanner module");
            reply.addString("help : gets this list");

        }
        else if (cmd_0=="stop")
        {
            close();
        }
        else if (cmd_0=="list")
        {
            reply.addVocab32("many");
            for(unsigned int i = 0; i < m_research_locations.size(); i++)
            {
                yarp::os::Bottle& tempList = reply.addList();
                tempList.addString(m_research_locations[i]);
                if (m_status_locations[i]==LOC_PLAN_UNCHECKED) {tempList.addString("Unchecked");}
                else if (m_status_locations[i]==LOC_PLAN_CHECKING) {tempList.addString("Checking");}
                else if (m_status_locations[i]==LOC_PLAN_CHECKED) {tempList.addString("Checked");}
            }
        }
        else
        {
            reply.addVocab32(yarp::os::Vocab32::encode("nack"));
            yCWarning(NEXT_LOC_PLANNER,"Error: wrong RPC command.");
        }
    }
    else if (cmd.size()==3) 
    {
        if (cmd_0=="set")
        {
            std::string cmd_1=cmd.get(1).asString();
            std::string cmd_2=cmd.get(2).asString();
            if(std::find(m_research_locations.begin(), m_research_locations.end(), cmd_1) != m_research_locations.end()) 
            {
                if (cmd_2 == "unchecked" || cmd_2 == "Unchecked" || cmd_2 == "UNCHECKED") {setLocationStatus(cmd_1, LOC_PLAN_UNCHECKED); }
                else if (cmd_2 == "checking" || cmd_2 == "Checking" || cmd_2 == "CHECKING") {setLocationStatus(cmd_1, LOC_PLAN_CHECKING); }
                else if (cmd_2 == "checked" || cmd_2 == "Checked" || cmd_2 == "CHECKED") {setLocationStatus(cmd_1, LOC_PLAN_CHECKED); }
                else 
                { 
                    reply.addVocab32(yarp::os::Vocab32::encode("nack"));
                    yCWarning(NEXT_LOC_PLANNER,"Error: wrong location status specified. You should use: unchecked, checking or checked.");
                }
            }
            else if (cmd_1=="all")
            {
                if (cmd_2 == "unchecked" || cmd_2 == "Unchecked" || cmd_2 == "UNCHECKED") {std::fill(m_status_locations.begin(), m_status_locations.end(), LOC_PLAN_UNCHECKED); }
                else if (cmd_2 == "checking" || cmd_2 == "Checking" || cmd_2 == "CHECKING") {std::fill(m_status_locations.begin(), m_status_locations.end(), LOC_PLAN_CHECKING); }
                else if (cmd_2 == "checked" || cmd_2 == "Checked" || cmd_2 == "CHECKED") {std::fill(m_status_locations.begin(), m_status_locations.end(), LOC_PLAN_CHECKED); }
                else 
                { 
                    reply.addVocab32(yarp::os::Vocab32::encode("nack"));
                    yCWarning(NEXT_LOC_PLANNER,"Error: wrong location status specified. You should use: unchecked, checking or checked.");
                }
            }
            else
            {
                reply.addVocab32(yarp::os::Vocab32::encode("nack"));
                yCWarning(NEXT_LOC_PLANNER,"Error: specified location name not found.");
            }
        }
        else
        {
            reply.addVocab32(yarp::os::Vocab32::encode("nack"));
            yCWarning(NEXT_LOC_PLANNER,"Error: wrong RPC command.");
        }
    }
    else
    {
        reply.addVocab32(yarp::os::Vocab32::encode("nack"));
        yCWarning(NEXT_LOC_PLANNER,"Error: input RPC command bottle has a wrong number of elements.");
    }
    
    return true;
}


/****************************************************************/
bool NextLocPlanner::updateModule()
{   
    
    //  =====>  sort m_reasearch_locations and m_status_locations <====== //

    return true;
}
