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


#define _USE_MATH_DEFINES

#include "lookForObjectThread.h"


YARP_LOG_COMPONENT(LOOK_FOR_OBJECT_THREAD, "r1_obr.lookForObject.LookForObjectThread")
bool print{true};


LookForObjectThread::LookForObjectThread(double _period, yarp::os::ResourceFinder &rf):
    PeriodicThread(_period),
    TypedReaderCallback(),
    m_rf(rf)
{
    //Defaults
    m_outPortName = "/lookForObject/out:o";
    m_nextOrientPortName = "/lookForObject/nextHeadOrient:rpc";
    m_findObjectPortName = "/lookForObject/findObject:rpc";
    m_gazeTargetOutPortName = "/lookForObject/gazeControllerTarget:o";
}

bool LookForObjectThread::threadInit()
{
    // --------- Generic config --------- //
    if (m_rf.check("out_port")) {m_outPortName = m_rf.find("out_port").asString();}
    if (m_rf.check("next_head_orient_port_rpc")) {m_nextOrientPortName = m_rf.find("next_head_orient_port_rpc").asString();}
    if (m_rf.check("find_object_port_rpc")) {m_findObjectPortName = m_rf.find("find_object_port_rpc").asString();}
    if (m_rf.check("gaze_target_point_port")) {m_gazeTargetOutPortName = m_rf.find("gaze_target_point_port").asString();}
    
    
    // --------- Navigation2DClient config --------- //
    yarp::os::Property nav2DProp;
    //Defaults
    nav2DProp.put("device", "navigation2D_nwc_yarp");
    nav2DProp.put("local", "/lookForObject/navClient");
    nav2DProp.put("navigation_server", "/navigation2D_nws_yarp");
    nav2DProp.put("map_locations_server", "/map2D_nws_yarp");
    nav2DProp.put("localization_server", "/localization2D_nws_yarp");
    if(!m_rf.check("NAVIGATION_CLIENT"))
    {
        yCWarning(LOOK_FOR_OBJECT_THREAD,"NAVIGATION_CLIENT section missing in ini file. Using the default values");
    }
    else
    {
        yarp::os::Searchable& nav_config = m_rf.findGroup("NAVIGATION_CLIENT");
        if(nav_config.check("device")) {nav2DProp.put("device", nav_config.find("device").asString());}
        if(nav_config.check("local")) {nav2DProp.put("local", nav_config.find("local").asString());}
        if(nav_config.check("navigation_server")) {nav2DProp.put("navigation_server", nav_config.find("navigation_server").asString());}
        if(nav_config.check("map_locations_server")) {nav2DProp.put("map_locations_server", nav_config.find("map_locations_server").asString());}
        if(nav_config.check("localization_server")) {nav2DProp.put("localization_server", nav_config.find("localization_server").asString());}
        }
   
    m_nav2DPoly.open(nav2DProp);
    if(!m_nav2DPoly.isValid())
    {
        yCWarning(LOOK_FOR_OBJECT_THREAD,"Error opening PolyDriver check parameters. Using the default values");
    }
    m_nav2DPoly.view(m_iNav2D);
    if(!m_iNav2D){
        yCError(LOOK_FOR_OBJECT_THREAD,"Error opening INavigation2D interface. Device not available");
        return false;
    }
    
    // --------- open ports --------- //
    if(!m_outPort.open(m_outPortName)){
        yCError(LOOK_FOR_OBJECT_THREAD) << "Cannot open Out port with name" << m_outPortName;
        return false;
    }

    if(!m_nextOrientPort.open(m_nextOrientPortName)){
        yCError(LOOK_FOR_OBJECT_THREAD) << "Cannot open nextOrient RPC port with name" << m_nextOrientPortName;
        return false;
    }

    if(!m_findObjectPort.open(m_findObjectPortName)){
        yCError(LOOK_FOR_OBJECT_THREAD) << "Cannot open nextOrient RPC port with name" << m_findObjectPortName;
        return false;
    }

    if(!m_gazeTargetOutPort.open(m_gazeTargetOutPortName)){
        yCError(LOOK_FOR_OBJECT_THREAD) << "Cannot open nextOrient RPC port with name" << m_gazeTargetOutPortName;
        return false;
    }

    return true;
}

void LookForObjectThread::run()
{
    
}

bool LookForObjectThread::lookAround(std::string& ob)
{
    yarp::os::Bottle request, reply;
    yarp::os::Bottle orientReq, orientReply;

    orientReq.clear(); orientReply.clear();
    orientReq.fromString("set all unchecked");
    m_nextOrientPort.write(orientReq,orientReply);

    int idx {1};
    while (true)
    {
        //retrieve next head orientation
        orientReq.clear(); orientReply.clear();
        orientReq.addString("next");
        if (m_nextOrientPort.write(orientReq,orientReply))
        {                
            if (orientReply.get(0).asString()=="noOrient")
                break;
            
            yCInfo(LOOK_FOR_OBJECT_THREAD) << "Checking head orientation: pos" + (std::string)(idx<10?"0":"") + std::to_string(idx);
            //gaze target output
            yarp::os::Bottle&  toSend1 = m_gazeTargetOutPort.prepare();
            toSend1.clear();
            yarp::os::Bottle& targetTypeList = toSend1.addList();
            targetTypeList.addString("target-type");
            targetTypeList.addString("angular");
            yarp::os::Bottle& targetLocationList = toSend1.addList();
            targetLocationList.addString("target-location");
            yarp::os::Bottle& targetList1 = targetLocationList.addList();
            yarp::os::Bottle* tmpBottle = orientReply.get(0).asList();
            targetList1.addFloat32(tmpBottle->get(0).asFloat32());
            targetList1.addFloat32(tmpBottle->get(1).asFloat32());
            m_gazeTargetOutPort.write(); //sending output command to gaze-controller 
            yarp::os::Time::delay(4.0);  //waiting for the robot tilting its head

            //search for object
            request.clear(); reply.clear();
            request.addString("where");
            request.addString(ob); 
            if (m_findObjectPort.write(request,reply))
            {
                if (reply.get(0).asString()!="not found")
                {
                    return true;
                } 
            }
            else
            {
                yCError(LOOK_FOR_OBJECT_THREAD,"Unable to communicate with findObject");
            }

            orientReq.clear(); orientReply.clear();
            std::string text = "set pos" + (std::string)(idx<10?"0":"") + std::to_string(idx) + " checked";
            orientReq.fromString(text);
            m_nextOrientPort.write(orientReq,orientReply);
            idx++;
        }
        else
        {
            yCError(LOOK_FOR_OBJECT_THREAD,"Unable to communicate with headOrient");
        }  
    }
    
    orientReq.clear(); orientReply.clear();
    orientReq.addString("home");
    m_nextOrientPort.write(orientReq,orientReply);

    return false;
}

void LookForObjectThread::onRead(yarp::os::Bottle &b)
{
    yCInfo(LOOK_FOR_OBJECT_THREAD,"Received: %s",b.toString().c_str());

    if(b.size() == 0)
    {
        yCError(LOOK_FOR_OBJECT_THREAD,"The input bottle has the wrong number of elements");
    }
    else 
    {
        if(b.size() > 1)
        {
            yCWarning(LOOK_FOR_OBJECT_THREAD,"The input bottle has the wrong number of elements. Using only the first element.");
        }

        std::string obj {b.get(0).asString()};
        bool objectFound {false};

        while (!objectFound)
        {
            objectFound = lookAround(obj);
            
            if (!objectFound)
            {
                yarp::os::Bottle request, reply;
                request.clear(); reply.clear();
                request.addString("turn");
                m_nextOrientPort.write(request,reply);
                if (reply.get(0).asString()=="noTurn")
                    break;
                else
                {
                    //turn
                    double theta = reply.get(0).asFloat32();
                    yCInfo(LOOK_FOR_OBJECT_THREAD) << "Turning" << theta << "degrees";
                    yarp::dev::Nav2D::Map2DLocation loc;
                    m_iNav2D->getCurrentPosition(loc);
                    loc.theta += theta; // <===
                    m_iNav2D->gotoTargetByAbsoluteLocation(loc);
                    yarp::dev::Nav2D::NavigationStatusEnum currentStatus;
                    m_iNav2D->getNavigationStatus(currentStatus);
                    while (currentStatus != yarp::dev::Nav2D::navigation_status_goal_reached)
                    {
                        yarp::os::Time::delay(1.0);
                        m_iNav2D->getNavigationStatus(currentStatus);
                    }
                }
            }
        }
        
        yarp::os::Bottle&  toSendOut = m_outPort.prepare();
        toSendOut.clear();
        if (objectFound)
            toSendOut.addString("Object Found!");
        else
            toSendOut.addString("Object not found :(");
    
        m_outPort.write();
    }
}

void LookForObjectThread::threadRelease()
{
    if(!m_outPort.isClosed())
        m_outPort.close();

    if (m_nextOrientPort.asPort().isOpen())
        m_nextOrientPort.close();

    if (m_findObjectPort.asPort().isOpen())
        m_findObjectPort.close(); 
    
    if(!m_gazeTargetOutPort.isClosed()){
        m_gazeTargetOutPort.close();
    }
    
    yCInfo(LOOK_FOR_OBJECT_THREAD, "Thread released");

    return;
}
