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

#include "getReadyToNav.h"

YARP_LOG_COMPONENT(GET_READY_TO_NAV, "r1_obr.goAndFindIt.getReadyToNav")

GetReadyToNav::GetReadyToNav()
{
    m_right_arm_pos.fromString("-9.0 9.0 -10.0 50.0 0.0 0.0 0.0 0.0");
    m_left_arm_pos.fromString("-9.0 9.0 -10.0 50.0 0.0 0.0 0.0 0.0");
    m_head_pos.fromString("0.0 0.0");
    m_torso_pos.fromString("0.012");
}


bool GetReadyToNav::configure(yarp::os::ResourceFinder &rf)
{
    std::string robot=rf.check("robot",yarp::os::Value("cer")).asString();
    bool okConfig = rf.check("SET_NAVIGATION_POSITION");
    if(!okConfig)
    {
        yCWarning(GET_READY_TO_NAV,"SET_NAVIGATION_POSITION section missing in ini file. Using the default values");
    }
    else
    {
        yarp::os::Searchable& config = rf.findGroup("SET_NAVIGATION_POSITION");
        if(config.check("right_arm_pos")) 
            m_right_arm_pos.fromString(config.find("right_arm_pos").asString());
        if(config.check("left_arm_pos")) 
            m_left_arm_pos.fromString(config.find("left_arm_pos").asString());
        if(config.check("head_pos")) 
            m_head_pos.fromString(config.find("head_pos").asString());
        if(config.check("torso_pos")) 
            m_torso_pos.fromString(config.find("torso_pos").asString());
    }

    
    // ----------- Polydriver config ----------- //
    yarp::os::Property prop;

    prop.put("device","remote_controlboard");
    prop.put("local","/goAndFindIt/getReadyToNavRArm");
    prop.put("remote","/"+robot+"/right_arm");
    if (!m_drivers[0].open(prop))
    {
        yCError(GET_READY_TO_NAV,"Unable to connect to %s",("/"+robot+"/right_arm").c_str());
        close();
        return false;
    }
    prop.clear();
    prop.put("device","remote_controlboard");
    prop.put("local","/goAndFindIt/getReadyToNavLArm");
    prop.put("remote","/"+robot+"/left_arm");
    if (!m_drivers[1].open(prop))
    {
        yCError(GET_READY_TO_NAV,"Unable to connect to %s",("/"+robot+"/left_arm").c_str());
        close();
        return false;
    }
    prop.clear();
    prop.put("device","remote_controlboard");
    prop.put("local","/goAndFindIt/getReadyToNavHead");
    prop.put("remote","/"+robot+"/head");
    if (!m_drivers[2].open(prop))
    {
        yCError(GET_READY_TO_NAV,"Unable to connect to %s",("/"+robot+"/head").c_str());
        close();
        return false;
    }
    prop.clear();
    prop.put("device","remote_controlboard");
    prop.put("local","/goAndFindIt/getReadyToNavTorso");
    prop.put("remote","/"+robot+"/torso");
    if (!m_drivers[3].open(prop))
    {
        yCError(GET_READY_TO_NAV,"Unable to connect to %s",("/"+robot+"/torso").c_str());
        close();
        return false;
    }

    m_drivers[0].view(m_iposctrl[0]);
    m_drivers[1].view(m_iposctrl[1]);
    m_drivers[2].view(m_iposctrl[2]);
    m_drivers[3].view(m_iposctrl[3]);
    if(!m_iposctrl[0] || !m_iposctrl[1] || !m_iposctrl[2] || !m_iposctrl[3])
    {
        yCError(GET_READY_TO_NAV,"Error opening iPositionControl interfaces. Devices not available");
        return false;
    }

    m_drivers[0].view(m_ictrlmode[0]);
    m_drivers[1].view(m_ictrlmode[1]);
    m_drivers[2].view(m_ictrlmode[2]);
    m_drivers[3].view(m_ictrlmode[3]);
    if(!m_ictrlmode[0] || !m_ictrlmode[1] || !m_ictrlmode[2] || !m_ictrlmode[3])
    {
        yCError(GET_READY_TO_NAV,"Error opening iControlMode interfaces. Devices not available");
        return false;
    }

    return true;
}


void GetReadyToNav::setPosCtrlMode(const int part)
{
    int NUMBER_OF_JOINTS;
    m_iposctrl[part]->getAxes(&NUMBER_OF_JOINTS);
    for (int i_joint=0; i_joint < NUMBER_OF_JOINTS; i_joint++){ m_ictrlmode[part]->setControlMode(i_joint, VOCAB_CM_POSITION); } 
}


void GetReadyToNav::navPosition()
{    
    for (int i = 0 ; i<=3 ; i++)
    {
        setPosCtrlMode(i);
    }

    yCInfo(GET_READY_TO_NAV, "Setting navigation position");    
    
    //right arm
    m_iposctrl[0]->positionMove(0, m_right_arm_pos.get(0).asFloat32());
    m_iposctrl[0]->positionMove(1, m_right_arm_pos.get(1).asFloat32());
    m_iposctrl[0]->positionMove(2, m_right_arm_pos.get(2).asFloat32());
    m_iposctrl[0]->positionMove(3, m_right_arm_pos.get(3).asFloat32());
    m_iposctrl[0]->positionMove(4, m_right_arm_pos.get(4).asFloat32());
    m_iposctrl[0]->positionMove(5, m_right_arm_pos.get(5).asFloat32());
    m_iposctrl[0]->positionMove(6, m_right_arm_pos.get(6).asFloat32());
    m_iposctrl[0]->positionMove(7, m_right_arm_pos.get(7).asFloat32());
    
    //left arm
    m_iposctrl[1]->positionMove(0, m_left_arm_pos.get(0).asFloat32());
    m_iposctrl[1]->positionMove(1, m_left_arm_pos.get(1).asFloat32());
    m_iposctrl[1]->positionMove(2, m_left_arm_pos.get(2).asFloat32());
    m_iposctrl[1]->positionMove(3, m_left_arm_pos.get(3).asFloat32());
    m_iposctrl[1]->positionMove(4, m_left_arm_pos.get(4).asFloat32());
    m_iposctrl[1]->positionMove(5, m_left_arm_pos.get(5).asFloat32());
    m_iposctrl[1]->positionMove(6, m_left_arm_pos.get(6).asFloat32());
    m_iposctrl[1]->positionMove(7, m_left_arm_pos.get(7).asFloat32());
    
    //head
    m_iposctrl[2]->positionMove(0, m_head_pos.get(0).asFloat32());
    m_iposctrl[2]->positionMove(1, m_head_pos.get(1).asFloat32());
    
    //torso
    m_iposctrl[3]->positionMove(0, m_torso_pos.get(0).asFloat32());
}


bool GetReadyToNav::areJointsOk()
{
    int mode=0;
    for (int i = 0 ; i<4 ; i++) 
    {
        int NUMBER_OF_JOINTS;
        m_iposctrl[i]->getAxes(&NUMBER_OF_JOINTS);
        for (int i_joint=0; i_joint < NUMBER_OF_JOINTS; i_joint++)
        { 
            m_ictrlmode[i]->getControlMode(i_joint, &mode);
            if (mode == VOCAB_CM_HW_FAULT)
            {
                yError() << "Error: hardware fault detected";
                return false;
            }
            else if (mode == VOCAB_CM_IDLE)
            {
                yError() << "Error: idle joint detected";
                return false;
            }
        }
    }

    return true;
}

void GetReadyToNav::close()
{
    if(m_drivers[0].isValid())
    {
        m_drivers[0].close();
    }

    if(m_drivers[1].isValid())
    {
        m_drivers[1].close();
    }

    if(m_drivers[2].isValid())
    {
        m_drivers[2].close();
    }

    if(m_drivers[3].isValid())
    {
        m_drivers[3].close();
    }
}

