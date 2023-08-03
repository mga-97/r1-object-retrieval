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
    m_set_nav_position_file = "/home/user1/r1-object-retrieval/app/goAndFindIt/scripts/set_navigation_position_sim.sh";
}


bool GetReadyToNav::configure(yarp::os::ResourceFinder &rf)
{
    std::string robot=rf.check("robot",yarp::os::Value("cer")).asString();

    if(rf.check("set_navigation_pos_file"))
        m_set_nav_position_file = rf.find("set_navigation_pos_file").asString();

    // ----------- Polydriver config ----------- //
    yarp::os::Property prop;

    prop.put("device","remote_controlboard");
    prop.put("local","/goAndFindIt/getReadyToNavRArm");
    prop.put("remote","/cer/right_arm");
    if (!m_drivers[0].open(prop))
    {
        yCError(GET_READY_TO_NAV,"Unable to connect to %s",("/"+robot+"/right_arm").c_str());
        close();
        return false;
    }
    prop.clear();
    prop.put("device","remote_controlboard");
    prop.put("local","/goAndFindIt/getReadyToNavLArm");
    prop.put("remote","/cer/left_arm");
    if (!m_drivers[1].open(prop))
    {
        yCError(GET_READY_TO_NAV,"Unable to connect to %s",("/"+robot+"/left_arm").c_str());
        close();
        return false;
    }
    prop.clear();
    prop.put("device","remote_controlboard");
    prop.put("local","/goAndFindIt/getReadyToNavHead");
    prop.put("remote","/cer/head");
    if (!m_drivers[2].open(prop))
    {
        yCError(GET_READY_TO_NAV,"Unable to connect to %s",("/"+robot+"/head").c_str());
        close();
        return false;
    }
    prop.clear();
    prop.put("device","remote_controlboard");
    prop.put("local","/goAndFindIt/getReadyToNavTorso");
    prop.put("remote","/cer/torso");
    if (!m_drivers[3].open(prop))
    {
        yCError(GET_READY_TO_NAV,"Unable to connect to %s",("/"+robot+"/torso").c_str());
        close();
        return false;
    }
    
    m_drivers[0].view(m_iremcal[0]);
    m_drivers[1].view(m_iremcal[1]);
    m_drivers[2].view(m_iremcal[2]);
    m_drivers[3].view(m_iremcal[3]);
    if(!m_iremcal[0] || !m_iremcal[1] || !m_iremcal[2] || !m_iremcal[3])
    {
        yCError(GET_READY_TO_NAV,"Error opening iRemoteCalibrator interfaces. Devices not available");
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



void GetReadyToNav::home()
{    
    for (int i = 0 ; i<=3 ; i++)
    {
        homePart(i);
    }
}

void GetReadyToNav::navPosition()
{    
    for (int i = 0 ; i<=3 ; i++)
    {
        setPosCtrlMode(i);
    }

    yCInfo(GET_READY_TO_NAV, "Setting navigation position");
    system(m_set_nav_position_file.c_str());
}

void GetReadyToNav::setPosCtrlMode(const int part)
{
    int NUMBER_OF_JOINTS;
    m_iposctrl[part]->getAxes(&NUMBER_OF_JOINTS);
    for (int i_joint=0; i_joint < NUMBER_OF_JOINTS; i_joint++){ m_ictrlmode[part]->setControlMode(i_joint, VOCAB_CM_POSITION); } 
}

void GetReadyToNav::homePart(const int part)
{     
    // 0 = right arm
    // 1 = left arm
    // 2 = head
    // 3 = torso

    setPosCtrlMode(part);

    bool ok = m_iremcal[part]->homingWholePart();
    if (!ok)  
    {
        bool isCalib = false;
        m_iremcal[part]->isCalibratorDevicePresent(&isCalib);
        if (!isCalib) // (SIM_CER_ROBOT does not have a calibrator!)
            { yCError(GET_READY_TO_NAV) << "Error with part: " << part << ". No calibrator device was configured to perform this action, please verify that the wrapper config file for the part has the 'Calibrator' keyword in the attach phase"; }
        else
            { yCError(GET_READY_TO_NAV) << "Error with part: " << part << ". The remote calibrator reported that something went wrong during the calibration procedure"; }
    }
    
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
