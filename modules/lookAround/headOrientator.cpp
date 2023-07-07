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

#include "headOrientator.h"

YARP_LOG_COMPONENT(HEAD_ORIENTATOR, "r1_obr.headOrientator")

bool HeadOrientator::configure(yarp::os::ResourceFinder &rf)
{
    std::string robot=rf.check("robot",yarp::os::Value("cer")).asString();

    // ----------- Polydriver config ----------- //
    yarp::os::Property controllerProp;
    if(!rf.check("REMOTE_CONTROL_BOARD"))
    {
        yCWarning(HEAD_ORIENTATOR,"REMOTE_CONTROL_BOARD section missing in ini file. Using default values.");

        // defaults
        controllerProp.clear();
        controllerProp.put("device","remote_controlboard");
        controllerProp.put("local","/handPointing/goHomeHead");
        controllerProp.put("remote","/cer/head");
    }
    else
    {
        yarp::os::Searchable& goHome_config = rf.findGroup("REMOTE_CONTROL_BOARD");
        if(goHome_config.check("device")) {controllerProp.put("device", goHome_config.find("device").asString());}
        if(goHome_config.check("local")) {controllerProp.put("local", goHome_config.find("local").asString());}
        if(goHome_config.check("remote")) {controllerProp.put("remote", goHome_config.find("remote").asString());}
    }
    if (!m_Poly.open(controllerProp))
    {
        yCError(HEAD_ORIENTATOR,"Unable to connect to remote");
        close();
        return false;
    }

    m_Poly.view(m_iremcal);
    if(!m_iremcal)
    {
        yCError(HEAD_ORIENTATOR,"Error opening iRemoteCalibrator interface. Device not available");
        return false;
    }

    if(!m_iposctrl)
    {
        yCError(HEAD_ORIENTATOR,"Error opening iPositionControl interface. Device not available");
        return false;
    }

    if(!m_ictrlmode)
    {
        yCError(HEAD_ORIENTATOR,"Error opening iControlMode interface. Device not available");
        return false;
    }

    //Configure Head Positions maps
    std::map<std::string, std::string>  orientations_default{
        {"pos1", "(0.0 0.0)" },
        {"pos2" ,"(35.0 0.0)"},
        {"pos3" ,"(-35.0 0.0)"},
        {"pos4" ,"(0.0 20.0)"},
        {"pos5" ,"(35.0 20.0)"},
        {"pos6" ,"(-35.0 20.0)"},
        {"pos7" ,"(0.0 -20.0)"},
        {"pos8" ,"(35.0 -20.0)"},
        {"pos9" ,"(-35.0 -20.0)"}};
    
    if(!rf.check("HEAD_POSITIONS"))
    {
        yCWarning(HEAD_ORIENTATOR,"HEAD_POSITIONS section missing in ini file. Using default values.");

        m_orientations = orientations_default;
        for (int i=0; i<9; i++) {m_orientation_status.insert({"pos" + (i+1), HO_UNCHECKED });}
    }
    else
    {
        yarp::os::Searchable& goHome_config = rf.findGroup("HEAD_POSITIONS");
        for (int i=0; i<9; i++) 
        {
            m_orientations.insert({"pos" + (i+1), goHome_config.check("pos" + (i+1)) ? goHome_config.find("pos" + (i+1)).asString() : orientations_default.at("pos" + (i+1))});
            m_orientation_status.insert({"pos" + (i+1), HO_UNCHECKED });
        }
    }
    
    return true;
}

bool HeadOrientator::respond(const yarp::os::Bottle &cmd, yarp::os::Bottle &reply)
{
    yCInfo(HEAD_ORIENTATOR,"Received: %s",cmd.toString().c_str());

    std::string cmd0 = cmd.get(0).asString();

    if(cmd.size() == 1) 
    {
        if (cmd0=="next")
        {
            std::map<std::string, HeadOrientStatus>::iterator it;
            for (it = m_orientation_status.begin(); it != m_orientation_status.end(); it++  ) 
            {
                if (it->second == HO_UNCHECKED)
                {
                    reply.addString(m_orientations.at(it->first));
                    it->second = HO_CHECKING;
                    return;
                }
                else if (it == m_orientation_status.end())
                {
                    home();
                    reply.addString("noOrient");
                }
            }      
        }
        else if (cmd0=="help")
        {
            reply.addVocab32("many");
            reply.addString("next : responds the next unchecked orientation or noOrient");
            reply.addString("set <orientation> <status> : sets the status of a location to unchecked, checking or checked");
            reply.addString("set all <status> : sets the status of all orientations");
            reply.addString("list : lists all the orientation and their status");
            reply.addString("stop : stops the headOrientator module");
            reply.addString("help : gets this list");

        }
        else if (cmd0=="stop")
        {
            close();
        }
        else if (cmd0=="list")
        {
            reply.addVocab32("many");
            std::map<std::string, std::string>::iterator it;
            for(it = m_orientations.begin(); it != m_orientations.end(); it++)
            {
                yarp::os::Bottle& tempList = reply.addList();
                tempList.addString(it->first);
                tempList.addString(it->second);
                HeadOrientStatus tempStatus = m_orientation_status.at(it->first);
                if (tempStatus==HO_UNCHECKED) {tempList.addString("Unchecked");}
                else if (tempStatus==HO_CHECKING) {tempList.addString("Checking");}
                else if (tempStatus==HO_CHECKED) {tempList.addString("Checked");}
            }
        }
        else
        {
            reply.addVocab32(yarp::os::Vocab32::encode("nack"));
            yCWarning(HEAD_ORIENTATOR,"Error: wrong RPC command.");
        }
    }
    else if (cmd.size()==3)    //expected 'set <orientation> <status>' or 'set all <status>'
    {
        if (cmd0=="set")
        {
            std::string cmd1=cmd.get(1).asString();
            std::string cmd2=cmd.get(2).asString();
            if(m_orientation_status.find(cmd1) != m_orientation_status.end()) 
            {
                if (cmd2 == "unchecked" || cmd2 == "Unchecked" || cmd2 == "UNCHECKED")   {m_orientation_status.at(cmd1) = HO_UNCHECKED; }
                else if (cmd2 == "checking" || cmd2 == "Checking" || cmd2 == "CHECKING") {m_orientation_status.at(cmd1) = HO_CHECKING; }
                else if (cmd2 == "checked" || cmd2 == "Checked" || cmd2 == "CHECKED")    {m_orientation_status.at(cmd1) = HO_CHECKED;  }
                else 
                { 
                    reply.addVocab32(yarp::os::Vocab32::encode("nack"));
                    yCWarning(HEAD_ORIENTATOR,"Error: wrong orientation status specified. You should use: unchecked, checking or checked.");
                }
            }
            else if (cmd1=="all")
            {
                if (cmd2 == "unchecked" || cmd2 == "Unchecked" || cmd2 == "UNCHECKED")   { for (auto &p : m_orientation_status ) p.second = HO_UNCHECKED; }  
                else if (cmd2 == "checking" || cmd2 == "Checking" || cmd2 == "CHECKING") { for (auto &p : m_orientation_status ) p.second = HO_CHECKING; }
                else if (cmd2 == "checked" || cmd2 == "Checked" || cmd2 == "CHECKED")    { for (auto &p : m_orientation_status ) p.second = HO_CHECKED; }
                else 
                { 
                    reply.addVocab32(yarp::os::Vocab32::encode("nack"));
                    yCWarning(HEAD_ORIENTATOR,"Error: wrong orientation status specified. You should use: unchecked, checking or checked.");
                }
            }
            else
            {
                reply.addVocab32(yarp::os::Vocab32::encode("nack"));
                yCWarning(HEAD_ORIENTATOR,"Error: specified location name not found.");
            }
        }
        else
        {
            reply.addVocab32(yarp::os::Vocab32::encode("nack"));
            yCWarning(HEAD_ORIENTATOR,"Error: wrong RPC command.");
        }
    }
    else
    {
        reply.addVocab32(yarp::os::Vocab32::encode("nack"));
        yCWarning(HEAD_ORIENTATOR,"Error: input RPC command bottle has a wrong number of elements.");
    }
    
    if (reply.size()==0)
        reply.addVocab32(yarp::os::Vocab32::encode("ack")); 
    
}


void HeadOrientator::home()
{    
    // ------ set head to Position control mode ------ //
    int NUMBER_OF_JOINTS;
    m_iposctrl->getAxes(&NUMBER_OF_JOINTS);
    for (int i_joint=0; i_joint < NUMBER_OF_JOINTS; i_joint++){ m_ictrlmode->setControlMode(i_joint, VOCAB_CM_POSITION); }    
    
    // ------ homing command  ------ //
    bool ok = m_iremcal->homingWholePart();
    if (!ok)  
    {
        bool isCalib = false;
        m_iremcal->isCalibratorDevicePresent(&isCalib);
        if (!isCalib) // providing better feedback to user by verifying if the calibrator device was set or not. 
                      // (SIM_CER_ROBOT does not have a calibrator!)
        { yCError(HEAD_ORIENTATOR) << "Error: No calibrator device was configured to perform this action, please verify that the wrapper config file for the part has the 'Calibrator' keyword in the attach phase"; }
        else
        { yCError(HEAD_ORIENTATOR) << "Error: The remote calibrator reported that something went wrong during the calibration procedure"; }
    }
    
}


bool HeadOrientator::close()
{
    if(m_Poly.isValid())
    {
        m_Poly.close();
    }

    if (!m_rpc_server_port.asPort().isOpen())
        m_rpc_server_port.close();
    
    return true;
}
