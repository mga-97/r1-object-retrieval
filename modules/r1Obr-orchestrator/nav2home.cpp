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

#include "nav2home.h"

YARP_LOG_COMPONENT(NAV_2_HOME, "r1_obr.orchestrator.nav2home")

bool Nav2Home::configure(yarp::os::ResourceFinder &rf)
{
    
    // --------- Navigation2DClient config --------- //
    yarp::os::Property nav2DProp;
    if(!rf.check("NAVIGATION_CLIENT"))
    {
        yCWarning(NAV_2_HOME,"NAVIGATION_CLIENT section missing in ini file. Using the default values");
    }
    yarp::os::Searchable& nav_config = rf.findGroup("NAVIGATION_CLIENT");
    nav2DProp.put("device", nav_config.check("device", Value("navigation2D_nwc_yarp")));
    nav2DProp.put("local", nav_config.check("local", Value("/r1Obr-orchestrator/nav2home/")));
    nav2DProp.put("navigation_server", nav_config.check("navigation_server", Value("/navigation2D_nws_yarp")));
    nav2DProp.put("map_locations_server", nav_config.check("map_locations_server", Value("/map2D_nws_yarp")));
    nav2DProp.put("localization_server", nav_config.check("localization_server", Value("/localization2D_nws_yarp")));
    
    m_nav2DPoly.open(nav2DProp);
    if(!m_nav2DPoly.isValid())
    {
        yCWarning(NAV_2_HOME,"Error opening PolyDriver check parameters. Using the default values");
    }
    m_nav2DPoly.view(m_iNav2D);
    if(!m_iNav2D){
        yCError(NAV_2_HOME,"Error opening INavigation2D interface. Device not available");
        return false;
    }

    return true;
}


void Nav2Home::close()
{
    if(m_nav2DPoly.isValid())
        m_nav2DPoly.close();
}


bool Nav2Home::go()
{
    MapGrid2D  map;
    if(!m_iNav2D->getCurrentNavigationMap(NavigationMapTypeEnum::global_map, map))
    {
        yCError(NAV_2_HOME, "Error retrieving current global map");
        return false;
    }

    Map2DLocation loc(map.getMapName(), 0.0, 0.0, 0.0);
    if(!m_iNav2D->gotoTargetByAbsoluteLocation(loc))
    {
        yCError(NAV_2_HOME, "Error with navigation to home location");
        return false;
    }

    return true;
}


bool Nav2Home::stop()
{
    yCInfo(NAV_2_HOME, "Not going to home location anymore, navigation stopped");

    if(!m_iNav2D->stopNavigation())
    {
        yCError(NAV_2_HOME, "Error stopping navigation to home location");
        return false;
    }

    return true;
}


bool Nav2Home::areYouArrived()
{
    NavigationStatusEnum currentStatus;
    m_iNav2D->getNavigationStatus(currentStatus);

    if (currentStatus == yarp::dev::Nav2D::navigation_status_goal_reached)
        return true;
    else
        return false;

}