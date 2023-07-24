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

#include "goAndFindIt.h"


YARP_LOG_COMPONENT(GO_AND_FIND_IT, "r1_obr.goAndFindIt")


/****************************************************************/
GoAndFindIt::GoAndFindIt() :
    m_period(1.0),  m_max_nav_time(300.0), m_max_search_time(120.0)
    {
        m_input_port_name       = "/goAndFindIt/input:i";
        m_nextLoc_rpc_port_name = "/goAndFindIt/nextLocPlanner:rpc";
        m_lookObject_port_name  = "/goAndFindIt/lookForObject/object:o";
        m_objectFound_port_name = "/goAndFindIt/lookForObject/result:i";
        m_output_port_name      = "/goAndFindIt/output:o";
    }


/****************************************************************/
bool GoAndFindIt::configure(yarp::os::ResourceFinder &rf)
{
    //Generic config
    if(rf.check("period"))
        m_period = rf.find("period").asFloat32();
    if(rf.check("max_nav_time"))
        m_max_nav_time = rf.find("max_nav_time").asFloat32();
    if(rf.check("max_search_time"))
        m_max_search_time = rf.find("max_search_time").asFloat32();
    
    //Open ports
    if(rf.check("input_port"))
        m_input_port_name = rf.find("input_port").asString();
    m_input_port.useCallback(*this);
    if(!m_input_port.open(m_input_port_name))
        yCError(GO_AND_FIND_IT) << "Cannot open port" << m_input_port_name; 
    else
        yCInfo(GO_AND_FIND_IT) << "opened port" << m_input_port_name;


    if(rf.check("nextLoc_rpc_port"))
        m_nextLoc_rpc_port_name = rf.find("nextLoc_rpc_port").asString();
    if(!m_nextLoc_rpc_port.open(m_nextLoc_rpc_port_name))
        yCError(GO_AND_FIND_IT) << "Cannot open port" << m_nextLoc_rpc_port_name;
    else
        yCInfo(GO_AND_FIND_IT) << "opened port" << m_nextLoc_rpc_port_name;


    if(rf.check("lookObject_port"))
        m_lookObject_port_name = rf.find("lookObject_port").asString();
    if(!m_lookObject_port.open(m_lookObject_port_name))
        yCError(GO_AND_FIND_IT) << "Cannot open port" << m_lookObject_port_name;
    else
        yCInfo(GO_AND_FIND_IT) << "opened port" << m_lookObject_port_name;


    if(rf.check("objectFound_port"))
        m_objectFound_port_name = rf.find("objectFound_port").asString();
    if(!m_objectFound_port.open(m_objectFound_port_name))
        yCError(GO_AND_FIND_IT) << "Cannot open port" << m_objectFound_port_name;
    else
        yCInfo(GO_AND_FIND_IT) << "opened port" << m_objectFound_port_name;


    if(rf.check("output_port"))
        m_output_port_name = rf.find("output_port").asString();
    if(!m_output_port.open(m_output_port_name))
        yCError(GO_AND_FIND_IT) << "Cannot open port" << m_output_port_name; 
    else
        yCInfo(GO_AND_FIND_IT) << "opened port" << m_output_port_name;

    //Navigation2DClient config 
    yarp::os::Property nav2DProp;
    //Defaults
    nav2DProp.put("device", "navigation2D_nwc_yarp");
    nav2DProp.put("local", "/goAndFindIt/navClient");
    nav2DProp.put("navigation_server", "/navigation2D_nws_yarp");
    nav2DProp.put("map_locations_server", "/map2D_nws_yarp");
    nav2DProp.put("localization_server", "/localization2D_nws_yarp");
    if(!rf.check("NAVIGATION_CLIENT"))
    {
        yCWarning(GO_AND_FIND_IT,"NAVIGATION_CLIENT section missing in ini file. Using the default values");
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
        yCWarning(GO_AND_FIND_IT,"Error opening PolyDriver check parameters. Using the default values");
    }
    m_nav2DPoly.view(m_iNav2D);
    if(!m_iNav2D){
        yCError(GO_AND_FIND_IT,"Error opening INavigation2D interface. Device not available");
        return false;
    }


    return true;
}


/****************************************************************/
bool GoAndFindIt::close()
{
    if (!m_input_port.isClosed())
        m_input_port.close();    

    if (!m_nextLoc_rpc_port.asPort().isOpen())
        m_nextLoc_rpc_port.close();  

    if (!m_lookObject_port.isClosed())
        m_lookObject_port.close();   

    if (!m_objectFound_port.isClosed())
        m_objectFound_port.close();  
    
    if (!m_output_port.isClosed())
        m_output_port.close();

    return true;
}

/****************************************************************/
double GoAndFindIt::getPeriod()
{
    return m_period;
}

/****************************************************************/
bool GoAndFindIt::updateModule()
{
    return true;
}

/****************************************************************/
bool GoAndFindIt::goThereAndCheck(const string& what, const string& there)
{
    yCDebug(GO_AND_FIND_IT, "entered function");
    
    //check if "there" is a valid location
    Bottle request,reply;
    request.fromString("find " + there);
    yCDebug(GO_AND_FIND_IT) << "request: " << request.toString().c_str() << ". what: " << what << ". there:" << there ;
    if(m_nextLoc_rpc_port.write(request,reply))
    {
        yCDebug(GO_AND_FIND_IT) << "reply: " << reply.toString().c_str() << ". there:" << there ;
        if (reply.get(0).asString() == "ok" && reply.get(1).asString() == "checked")
        {
            yCWarning(GO_AND_FIND_IT,"Location has already been checked");
            return false;
        }
        else if (reply.get(0).asString() != "ok")
        {
            return false;
        }   
    }

    //navigating to "there"
    m_iNav2D->gotoTargetByLocationName(there);
    yCInfo(GO_AND_FIND_IT, "Going to location %s", there.c_str());
    yarp::dev::Nav2D::NavigationStatusEnum currentStatus;
    m_iNav2D->getNavigationStatus(currentStatus);
    double toomuchtime = Time::now() + m_max_nav_time; //five minutes to reach "there"
    while (currentStatus != yarp::dev::Nav2D::navigation_status_goal_reached)
    {
        if (Time::now() > toomuchtime)
        {
            yCError(GO_AND_FIND_IT,"Too much time has passed to navigate to %s.",there.c_str());
            return false;
        }
        Time::delay(1.0);
        m_iNav2D->getNavigationStatus(currentStatus);
        
    }
    yCInfo(GO_AND_FIND_IT, "Arrived at location %s. Starting looking for %s" , there.c_str() , what.c_str() ) ;

    //looking for "what"
    Bottle&  ask = m_lookObject_port.prepare();
    ask.clear();
    ask.addString(what);
    m_lookObject_port.write();
    toomuchtime = Time::now() + m_max_search_time; //two minutes to find "what"
    Bottle* objFound;
    while (true) 
    {
        objFound = m_objectFound_port.read(); // Read from the port.  Wait until data arrives.
        string result = objFound->get(0).asString();
        if (result == "Object Found!")
        {
            return true;    
        }   
        else if (result == "Object not found :(")
        {
            return false;    
        }
        else if (Time::now() > toomuchtime)
        {
            yCError(GO_AND_FIND_IT,"Too much time has passed waiting for %s to be found.",what.c_str());
            return false;
        }
        Time::delay(1.0);
    }

    return false;
}


/****************************************************************/
void GoAndFindIt::onRead(Bottle& b) 
{
    
    yCInfo(GO_AND_FIND_IT,"Received:  %s",b.toString().c_str());

    Bottle request,_rep_;
    request.fromString("set all unchecked");
    m_nextLoc_rpc_port.write(request,_rep_);

    bool result = false;

    if(b.size() == 2)           //expected "<object> <location>"
    {
        string what = b.get(0).asString();
        string where = b.get(1).asString();

        if(!goThereAndCheck(what,where))
        {
            yCWarning(GO_AND_FIND_IT,"Terminating the search");

            // ////////////////////////////// TO ADD: ask if the user wants to continue the search in the other locations  //////////////////////////////

            result = false;
        }
        else
        {
            yCInfo(GO_AND_FIND_IT,"%s found at %s!", what.c_str(), where.c_str());
            result = true;
        }

        Bottle request,_rep_;
        request.fromString("set " + where + " checked");
        m_nextLoc_rpc_port.write(request,_rep_);

    }
    else if(b.size() == 1)      //expected "<object>"
    {
        string what = b.get(0).asString();
        string where;

        // ////////////////////////////// TO ADD: ask PAVIS if it sees "what"   //////////////////////////////
        // ////////////////////////////// and therefore goThereAndCheck         //////////////////////////////

        bool stillSomeLocationsToCheck {true};
        while (stillSomeLocationsToCheck)
        {
            Bottle request,reply;
            request.addString("next");
            if(m_nextLoc_rpc_port.write(request,reply))
            {
                where = reply.get(0).asString(); 
                if (where != "noLocation")
                {
                    if(!goThereAndCheck(what,where))
                    {
                        yCWarning(GO_AND_FIND_IT,"%s not found at %s", what.c_str(), where.c_str());
                    }
                    else
                    {
                        yCInfo(GO_AND_FIND_IT,"%s found at %s!", what.c_str(), where.c_str());
                        result = true;
                        stillSomeLocationsToCheck = false;
                    }

                    Bottle request,_rep_;
                    request.fromString("set " + where + " checked");
                    m_nextLoc_rpc_port.write(request,_rep_);
                }
                else 
                {
                    yCWarning(GO_AND_FIND_IT,"%s not found. Nowhere else to go", what.c_str());
                    result = false;
                    stillSomeLocationsToCheck = false;
                }
            }
            else
            {
                yCError(GO_AND_FIND_IT,"Error trying to contact nextLocPlanner");
                break;
            }
        }
    }
    else
    {
        yCError(GO_AND_FIND_IT,"Error: wrong bottle format received");
        return;
    }
    
    Bottle&  toSend = m_output_port.prepare();
    toSend.clear();
    toSend.addInt8((int)result);
    m_output_port.write();


}

