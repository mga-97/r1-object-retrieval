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

#include <math.h>
#include "nextLocPlanner.h"

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

    //Load all the locations in m_all_locations
    if(!m_iNav2D->getLocationsList(m_all_locations)) 
    {
        yCError(NEXT_LOC_PLANNER,"Error getting locations list from map server");
        return false;
    }

    if(!m_all_locations.empty()) 
    {
        //remove elements not belonging to the area defined in .ini file (i.e. whose name starts with the name of the area)
        auto new_end = std::remove_if(m_all_locations.begin(), m_all_locations.end(), [this](std::string& str){return (str.find(m_area) == std::string::npos);} );
        m_all_locations.erase(new_end, m_all_locations.end());

        if(m_all_locations.empty()) 
        {
            yCWarning(NEXT_LOC_PLANNER,"Warning: no locations from map server for the area specified");
        }

        m_locations_unchecked.insert(m_locations_unchecked.end(), m_all_locations.begin(), m_all_locations.end());

    }
    else
    {
        yCWarning(NEXT_LOC_PLANNER,"Warning: no locations from map server");
    }
  
     
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
bool NextLocPlanner::setLocationStatus(const std::string location_name, const std::string& location_status)
{
    std::vector<std::string>::iterator findUnchecked {std::find(m_locations_unchecked.begin(), m_locations_unchecked.end(), location_name)};
    std::vector<std::string>::iterator findChecking {std::find(m_locations_checking.begin(), m_locations_checking.end(), location_name)};
    std::vector<std::string>::iterator findChecked {std::find(m_locations_checked.begin(), m_locations_checked.end(), location_name)};
    
    bool locFound { findUnchecked != m_locations_unchecked.end() || findChecking != m_locations_checking.end() || findChecked != m_locations_checked.end()};
    if (!locFound) 
    {
        yCError(NEXT_LOC_PLANNER,"Error: location specified not found");
        return false;
    }

    if ( !(location_status == "unchecked" || location_status == "Unchecked" || location_status == "UNCHECKED" ||
            location_status == "checking" || location_status == "Checking"  || location_status == "CHECKING"  ||
            location_status == "checked"  || location_status == "Checked"   || location_status == "CHECKED")  ) 
    { 
        yCError(NEXT_LOC_PLANNER,"Error: wrong location status specified. You should use: unchecked, checking or checked.");
        return false;
    }

    if (findUnchecked != m_locations_unchecked.end())   { m_locations_unchecked.erase(findUnchecked);}
    else if(findChecking != m_locations_checking.end()) { m_locations_checking.erase(findChecking);  }
    else if(findChecked != m_locations_checked.end())   { m_locations_checked.erase(findChecked);    }


    if (location_status == "unchecked" || location_status == "Unchecked" || location_status == "UNCHECKED")  
        {m_locations_unchecked.push_back(location_name); }
    else if (location_status == "checking" || location_status == "Checking" || location_status == "CHECKING")  
        {m_locations_checking.push_back(location_name); }
    else if (location_status == "checked" || location_status == "Checked" || location_status == "CHECKED")  
        {m_locations_checked.push_back(location_name); }

    return true;
}


/****************************************************************/
bool NextLocPlanner::respond(const yarp::os::Bottle &cmd, yarp::os::Bottle &reply)
{
    std::lock_guard<std::mutex> m_lock(m_mutex);
    reply.clear();
    std::string cmd_0=cmd.get(0).asString();
    if (cmd.size()==1)
    {
        if (cmd_0=="next")
        {      
            if (m_locations_unchecked.size()>0)
            {
                //reading the first unchecked location
                reply.addString(m_locations_unchecked[0]); 
                //setting that location as "checking"
                setLocationStatus(m_locations_unchecked[0], "checking");
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
            reply.addString("list2 : lists all the locations divided by their status");
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

            if (m_all_locations.size()!=0)
            {
                yarp::os::Bottle& tempList1 = reply.addList();
                std::vector<std::string>::iterator it;
                for(it = m_all_locations.begin(); it != m_all_locations.end(); it++)
                {
                    yarp::os::Bottle& tempList = tempList1.addList();
                    tempList.addString(*it);

                    if (std::find(m_locations_unchecked.begin(), m_locations_unchecked.end(), *it) != m_locations_unchecked.end()) 
                        tempList.addString("Unchecked");
                    if (std::find(m_locations_checking.begin(), m_locations_checking.end(), *it) != m_locations_checking.end()) 
                        tempList.addString("Checking");
                    if (std::find(m_locations_checked.begin(), m_locations_checked.end(), *it) != m_locations_checked.end()) 
                        tempList.addString("Checked");
                }
            }
        }
        else if (cmd_0=="list2")
        {
            reply.addVocab32("many");
            yarp::os::Bottle& tempList = reply.addList();
            
            tempList.addString("Unchecked: ");
            if (m_locations_unchecked.size()!=0)
            {
                std::vector<std::string>::iterator it;
                for(it = m_locations_unchecked.begin(); it != m_locations_unchecked.end(); it++)
                {
                    tempList.addString(*it);
                }
            }
            tempList.addString(" ");
            tempList.addString("Checking: ");
            if (m_locations_checking.size()!=0)
            {
                std::vector<std::string>::iterator it;
                for(it = m_locations_checking.begin(); it != m_locations_checking.end(); it++)
                {
                    tempList.addString(*it);
                }
            }
            tempList.addString(" ");
            tempList.addString("Checked: ");
            if (m_locations_checked.size()!=0)
            {
                std::vector<std::string>::iterator it;
                for(it = m_locations_checked.begin(); it != m_locations_checked.end(); it++)
                {
                    tempList.addString(*it);
                }
            }
        }
        else
        {
            reply.addVocab32(yarp::os::Vocab32::encode("nack"));
            yCWarning(NEXT_LOC_PLANNER,"Error: wrong RPC command.");
        }
    }
    else if (cmd.size()==3)    //expected 'set <location> <status>'  or 'set all <status>'
    {
        if (cmd_0=="set")
        {
            std::string cmd_1=cmd.get(1).asString();
            std::string cmd_2=cmd.get(2).asString();

            bool locFound {std::find(m_locations_unchecked.begin(), m_locations_unchecked.end(), cmd_1) != m_locations_unchecked.end() ||
                           std::find(m_locations_checking.begin(), m_locations_checking.end(), cmd_1) != m_locations_checking.end() ||
                           std::find(m_locations_checked.begin(), m_locations_checked.end(), cmd_1) != m_locations_checked.end()};
            if(locFound) 
            {
                if(!setLocationStatus(cmd_1, cmd_2))
                {
                    reply.addVocab32(yarp::os::Vocab32::encode("nack"));
                }
            }
            else if (cmd_1=="all")
            {
                
                if (cmd_2 == "unchecked" || cmd_2 == "Unchecked" || cmd_2 == "UNCHECKED")   
                {
                    m_locations_unchecked.insert(m_locations_unchecked.end(), m_locations_checked.begin(), m_locations_checked.end());
                    m_locations_unchecked.insert(m_locations_unchecked.end(), m_locations_checking.begin(), m_locations_checking.end());
                    m_locations_checking.clear();
                    m_locations_checked.clear();
                }
                else if (cmd_2 == "checking" || cmd_2 == "Checking" || cmd_2 == "CHECKING")    
                {
                    m_locations_checking.insert(m_locations_checking.end(), m_locations_checked.begin(), m_locations_checked.end());
                    m_locations_checking.insert(m_locations_checking.end(), m_locations_unchecked.begin(), m_locations_unchecked.end());
                    m_locations_unchecked.clear();
                    m_locations_checked.clear();
                }
                else if (cmd_2 == "checked" || cmd_2 == "Checked" || cmd_2 == "CHECKED")    
                {
                    m_locations_checked.insert(m_locations_checked.end(), m_locations_checking.begin(), m_locations_checking.end());
                    m_locations_checked.insert(m_locations_checked.end(), m_locations_unchecked.begin(), m_locations_unchecked.end());
                    m_locations_unchecked.clear();
                    m_locations_checking.clear();
                }
                else 
                { 
                    yCError(NEXT_LOC_PLANNER,"Error: wrong location status specified. You should use: unchecked, checking or checked.");
                    reply.addVocab32(yarp::os::Vocab32::encode("nack"));
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
    
    if (reply.size()==0)
        reply.addVocab32(yarp::os::Vocab32::encode("ack")); 

    return true;
}


/****************************************************************/
bool NextLocPlanner::getCurrentCheckingLocation(std::string& location_name)
{
    if (m_locations_checking.size()==0)
    {
        location_name = "<noLocation>";
    }
    else if (m_locations_checking.size()==1)
    {
        location_name = m_locations_checking[0];
    }
    else
    {
        yCWarning(NEXT_LOC_PLANNER,"Warning: more than one location set as Checking");
        location_name = m_locations_checking.back();
    }
    return true;
}


/****************************************************************/
bool NextLocPlanner::getUncheckedLocations(std::vector<std::string>& location_list)
{
    if (m_locations_unchecked.size()==0)
    {
        location_list.push_back("<noLocation>");
    }
    else
    {
        location_list = m_locations_unchecked;
    }
    return true;
}


/****************************************************************/
bool NextLocPlanner::getCheckedLocations(std::vector<std::string>& location_list)
{
    if (m_locations_checked.size()==0)
    {
        location_list.push_back("<noLocation>");
    }
    else
    {
        location_list = m_locations_checked;
    }
    return true;
}


/****************************************************************/
double NextLocPlanner::distRobotLocation(const std::string& location_name)
{
    yarp::dev::Nav2D::Map2DLocation robotLoc;
    yarp::dev::Nav2D::Map2DLocation loc;
    m_iNav2D->getCurrentPosition(robotLoc);
    m_iNav2D->getLocation(location_name, loc);

    return sqrt(pow((robotLoc.x - loc.x), 2) + pow((robotLoc.y - loc.y), 2));
}


/****************************************************************/
bool NextLocPlanner::updateModule()
{   
    
    std::lock_guard<std::mutex> m_lock(m_mutex);

    // --- Sorting m_locations_unchecked by its distance from the robot  --- //
    std::vector<double> m_unchecked_dist;
    for(size_t i=0; i<m_locations_unchecked.size(); ++i)
    {
        m_unchecked_dist.push_back(distRobotLocation(m_locations_unchecked[i]));
    }

    // Zip the vectors together
    std::vector<std::pair<std::string,double>> zipped;
    zip(m_locations_unchecked, m_unchecked_dist, zipped);

    // Sort the vector of pairs
    std::sort(std::begin(zipped), std::end(zipped), 
        [&](const auto& a, const auto& b)
        {
            return a.second < b.second;
        });

    // Write the sorted pairs back to the original vectors
    unzip(zipped, m_locations_unchecked, m_unchecked_dist);
    
    return true;
}
