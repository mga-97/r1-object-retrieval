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

#include "nextHeadOrient.h"


YARP_LOG_COMPONENT(NEXT_HEAD_ORIENT, "r1_obr.nextHeadOrient")

bool NextHeadOrient::configure(ResourceFinder &rf)
{
    //Generic config
    m_period = rf.check("period")  ? rf.find("period").asFloat32() : 1.0;
    bool useFov = rf.check("useCameraFOV") ? rf.find("useCameraFOV").asString()=="true" : false;
    m_overlap = rf.check("fov_overlap_degrees")  ? rf.find("fov_overlap_degrees").asFloat32() : 5.0;

    //Open RPC Server Port
    string rpcPortName = rf.check("rpcPort") ? rf.find("rpcPort").asString() : "/nextLocPlanner/request/rpc";
    if (!m_rpc_server_port.open(rpcPortName))
    {
        yCError(NEXT_HEAD_ORIENT, "open() error could not open rpc port %s, check network", rpcPortName.c_str());
        return false;
    }
    if (!attach(m_rpc_server_port))
    {
        yCError(NEXT_HEAD_ORIENT, "attach() error with rpc port %s", rpcPortName.c_str());
        return false;
    }

    // --------- RGBDSensor config --------- //
    Property rgbdProp;
    // Prepare default prop object
    rgbdProp.put("device", RGBDClient);
    rgbdProp.put("localImagePort", RGBDLocalImagePort);
    rgbdProp.put("localDepthPort", RGBDLocalDepthPort);
    rgbdProp.put("localRpcPort", RGBDLocalRpcPort);
    rgbdProp.put("remoteImagePort", RGBDRemoteImagePort);
    rgbdProp.put("remoteDepthPort", RGBDRemoteDepthPort);
    rgbdProp.put("remoteRpcPort", RGBDRemoteRpcPort);
    rgbdProp.put("ImageCarrier", RGBDImageCarrier);
    rgbdProp.put("DepthCarrier", RGBDDepthCarrier);
    bool okRgbdRf = rf.check("RGBD_SENSOR_CLIENT");
    if(!okRgbdRf)
    {
        yCWarning(NEXT_HEAD_ORIENT,"RGBD_SENSOR_CLIENT section missing in ini file. Using default values");
    }
    else
    {
        Searchable& rgbd_config = rf.findGroup("RGBD_SENSOR_CLIENT");
        if(rgbd_config.check("device")) {rgbdProp.put("device", rgbd_config.find("device").asString());}
        if(rgbd_config.check("localImagePort")) {rgbdProp.put("localImagePort", rgbd_config.find("localImagePort").asString());}
        if(rgbd_config.check("localDepthPort")) {rgbdProp.put("localDepthPort", rgbd_config.find("localDepthPort").asString());}
        if(rgbd_config.check("localRpcPort")) {rgbdProp.put("localRpcPort", rgbd_config.find("localRpcPort").asString());}
        if(rgbd_config.check("remoteImagePort")) {rgbdProp.put("remoteImagePort", rgbd_config.find("remoteImagePort").asString());}
        if(rgbd_config.check("remoteDepthPort")) {rgbdProp.put("remoteDepthPort", rgbd_config.find("remoteDepthPort").asString());}
        if(rgbd_config.check("remoteRpcPort")) {rgbdProp.put("remoteRpcPort", rgbd_config.find("remoteRpcPort").asString());}
        if(rgbd_config.check("ImageCarrier")) {rgbdProp.put("ImageCarrier", rgbd_config.find("ImageCarrier").asString());}
        if(rgbd_config.check("DepthCarrier")) {rgbdProp.put("DepthCarrier", rgbd_config.find("DepthCarrier").asString());}
    }

    m_rgbdPoly.open(rgbdProp);
    if(!m_rgbdPoly.isValid())
    {
        yCError(NEXT_HEAD_ORIENT,"Error opening PolyDriver check parameters");
        return false;
    }
    m_rgbdPoly.view(m_iRgbd);
    if(!m_iRgbd)
    {
        yCError(NEXT_HEAD_ORIENT,"Error opening iRGBD interface. Device not available");
        return false;
    }
    

    // ----------- Polydriver config ----------- //
    Property controllerProp;
    if(!rf.check("REMOTE_CONTROL_BOARD"))
    {
        yCWarning(NEXT_HEAD_ORIENT,"REMOTE_CONTROL_BOARD section missing in ini file. Using default values.");

        // defaults
        controllerProp.clear();
        controllerProp.put("device","remote_controlboard");
        controllerProp.put("local","/nextHeadOrient/remote_controlboard");
        controllerProp.put("remote","/cer/head");
    }
    else
    {
        Searchable& pos_configs = rf.findGroup("REMOTE_CONTROL_BOARD");
        if(pos_configs.check("device")) {controllerProp.put("device", pos_configs.find("device").asString());}
        if(pos_configs.check("local")) {controllerProp.put("local", pos_configs.find("local").asString());}
        if(pos_configs.check("remote")) {controllerProp.put("remote", pos_configs.find("remote").asString());}
    }

    if (!m_Poly.open(controllerProp))
    {
        yCError(NEXT_HEAD_ORIENT,"Unable to connect to remote");
        close();
        return false;
    }
    
    m_Poly.view(m_iremcal);
    if(!m_iremcal)
    {
        yCError(NEXT_HEAD_ORIENT,"Error opening iRemoteCalibrator interface. Device not available");
        return false;
    }

    m_Poly.view(m_iposctrl);
    if(!m_iposctrl)
    {
        yCError(NEXT_HEAD_ORIENT,"Error opening iPositionControl interface. Device not available");
        return false;
    }

    m_Poly.view(m_ictrlmode);
    if(!m_ictrlmode)
    {
        yCError(NEXT_HEAD_ORIENT,"Error opening iControlMode interface. Device not available");
        return false;
    }

    m_Poly.view(m_ilimctrl);
    if(!m_ilimctrl)
    {
        yCError(NEXT_HEAD_ORIENT,"Error opening IControlLimits interface. Device not available");
        return false;
    }

    // ----------- Configure Head Positions ----------- //
    map<string, pair<double,double>>  orientations_default{
        {"pos01" , {0.0, 0.0}    },
        {"pos02" , {35.0, 0.0}   },
        {"pos03" , {-35.0, 0.0}  },
        {"pos04" , {0.0, 20.0}   },
        {"pos05" , {35.0, 20.0}  },
        {"pos06" , {-35.0, 20.0} },
        {"pos07" , {0.0, -20.0}  },
        {"pos08" , {35.0, -20.0} },
        {"pos09" , {-35.0, -20.0}}   };

    double visual_span;
    
    if (!useFov) 
    {
        if(!rf.check("HEAD_POSITIONS"))
        {
            yCWarning(NEXT_HEAD_ORIENT,"HEAD_POSITIONS section missing in ini file. Using default values.");

            m_orientations = orientations_default;
            for (size_t i=0; i<orientations_default.size(); i++) {m_orientation_status.insert({"pos0" + to_string(i+1), HO_UNCHECKED });}
            visual_span = 70.0;
        }
        else
        {
            Searchable& pos_configs = rf.findGroup("HEAD_POSITIONS");
            double maxDeg {0.0}, minDeg {0.0}; 
            int idx {1};
            while (true) 
            {
                string pos = "pos" + (string)(idx<10?"0":"") + to_string(idx);
                if (!pos_configs.check(pos))
                    break;

                stringstream  ss(pos_configs.find(pos).asString());
                string s; 
                ss >> s; double n1 = stod(s);
                ss >> s; double n2 = stod(s);            

                m_orientations.insert({pos, {n1,n2} });
                m_orientation_status.insert({pos, HO_UNCHECKED });
                idx++;

                maxDeg = n1>maxDeg ? n1 : maxDeg;
                minDeg = n1<minDeg ? n1 : minDeg;
            }
            visual_span = maxDeg-minDeg;
        }
    }
    else 
    {
        double verticalFov{0.0}, horizontalFov{0.0};
        double newVFov{0.0}, newHFov{0.0};
        bool fovGot = m_iRgbd->getRgbFOV(horizontalFov,verticalFov);
        if(!fovGot)
        {
            yCError(NEXT_HEAD_ORIENT,"An error occurred while retrieving the rgb camera FOV");
        }
        else
        {
            //consider the width of the fov to inspect an area of 180 degrees laterally and 90 degrees vertically
            // overlapping the views by at least <m_overlap> degrees
            newHFov = horizontalFov>60 ?  (90.0 - horizontalFov/2) : (horizontalFov-m_overlap) ;
            newVFov = verticalFov>30 ?  (45.0 - verticalFov/2) : (verticalFov-m_overlap) ;
        }
        
        double min_pos_h {0}, min_pos_v {0};
        double max_pos_h {100}, max_pos_v {100};

        bool limGotV = m_ilimctrl->getLimits(0, &min_pos_v, &max_pos_v); //pitch
        bool limGotH = m_ilimctrl->getLimits(1, &min_pos_h, &max_pos_h); //yaw
        if(!limGotV || !limGotH)
        {
            yCError(NEXT_HEAD_ORIENT,"An error occurred while retrieving the head joint limits");
        }

        double up = limGotV ? min(fovGot ? newVFov : 20.0 , max_pos_v) : (fovGot ? newVFov : 20.0);
        double down = limGotV ? max(fovGot ? -newVFov : -20.0 , min_pos_v) : (fovGot ? -newVFov : -20.0);
        double left = limGotH ? min(fovGot ? newHFov : 35.0 , max_pos_h) : (fovGot ? newHFov : 35.0);;
        double right = limGotH ? max(fovGot ? -newHFov : -35.0 , min_pos_h) : (fovGot ? -newHFov : -35.0); 

        m_orientations = {
            {"pos01", {0.0,  0.0}  },
            {"pos02", {left, 0.0}  },   //left
            {"pos03", {right,0.0}  },   //right
            {"pos04", {0.0,  up}   },   //up
            {"pos05", {left, up}   },   //up left
            {"pos06", {right,up}   },   //up right
            {"pos07", {0.0,  down} },   //down
            {"pos08", {left, down} },   //down left
            {"pos09", {right,down} }};  //down right

        for (int i=0; i<9; i++) {m_orientation_status.insert({"pos0" + to_string(i+1), HO_UNCHECKED });}
        
        visual_span = left-right;
    }

    double _v_, horizontalFov{0.0};
    if(!m_iRgbd->getRgbFOV(horizontalFov,_v_))
        yCError(NEXT_HEAD_ORIENT,"An error occurred while retrieving the rgb camera FOV");
    
    m_max_turns = ceil(360.0/(visual_span+horizontalFov));
    m_turn_deg = 360.0/m_max_turns;

    return true;
}

bool NextHeadOrient::respond(const Bottle &cmd, Bottle &reply)
{
    yCInfo(NEXT_HEAD_ORIENT,"Received: %s",cmd.toString().c_str());

    lock_guard<mutex> m_lock(m_mutex);
    reply.clear();
    string cmd0 = cmd.get(0).asString();

    if(cmd.size() == 1) 
    {
        if (cmd0=="next")
        {
            map<string, HeadOrientStatus>::iterator it;
            for (it = m_orientation_status.begin(); it != m_orientation_status.end(); it++  ) 
            {
                if (it->second == HO_UNCHECKED)
                {
                    Bottle& tempList = reply.addList();
                    pair<double,double> tempPair = m_orientations.at(it->first);
                    tempList.addFloat32(tempPair.first);
                    tempList.addFloat32(tempPair.second);
                    it->second = HO_CHECKING;
                    return true;
                }
            }  
            // if return has not been called before, no unchecked orientation has been found
            reply.addString("noOrient");   
            home(); 
        }
        else if (cmd0=="help")
        {
            reply.addVocab32("many");
            reply.addString("next : responds the next unchecked orientation or noOrient");
            reply.addString("set <orientation> <status> : sets the status of a location to unchecked, checking or checked");
            reply.addString("set all <status> : sets the status of all orientations");
            reply.addString("list : lists all the orientation and their status");
            reply.addString("home : let the robot head go back to its home position");
            reply.addString("turn : returns the angle the robot should turn to inspect another area, or 'noTurn'");
            reply.addString("stop : stops the nextHeadOrient module");
            reply.addString("help : gets this list");

        }
        else if (cmd0=="stop")
        {
            close();
        }
        else if (cmd0=="home")
        {
            home();
        }
        else if (cmd0=="list")
        {
            reply.addVocab32("many");
            map<string, pair<double,double>>::iterator it;
            for(it = m_orientations.begin(); it != m_orientations.end(); it++)
            {
                Bottle& tempList = reply.addList();
                tempList.addString(it->first);
                tempList.addFloat32((it->second).first);
                tempList.addFloat32((it->second).second);
                HeadOrientStatus tempStatus = m_orientation_status.at(it->first);
                if (tempStatus==HO_UNCHECKED) {tempList.addString("Unchecked");}
                else if (tempStatus==HO_CHECKING) {tempList.addString("Checking");}
                else if (tempStatus==HO_CHECKED) {tempList.addString("Checked");}
            }
        }
        else if (cmd0=="turn")
        {
            yCDebug(NEXT_HEAD_ORIENT) << "turning for the time" << m_current_turn << "of" << m_max_turns << "degrees:"<<m_turn_deg;
            m_current_turn++; 
            if (m_current_turn<=m_max_turns)
                reply.addFloat32(m_turn_deg);
            else
                reply.addString("noTurn");
            
            yCDebug(NEXT_HEAD_ORIENT) << "reply" << reply.toString().c_str();
        }
        else
        {
            reply.addVocab32(Vocab32::encode("nack"));
            yCWarning(NEXT_HEAD_ORIENT,"Error: wrong RPC command.");
        }
    }
    else if (cmd.size()==3)    //expected 'set <orientation> <status>' or 'set all <status>'
    {
        if (cmd0=="set")
        {
            string cmd1=cmd.get(1).asString();
            string cmd2=cmd.get(2).asString();
            if(m_orientation_status.find(cmd1) != m_orientation_status.end()) 
            {
                if (cmd2 == "unchecked" || cmd2 == "Unchecked" || cmd2 == "UNCHECKED")   {m_orientation_status.at(cmd1) = HO_UNCHECKED; }
                else if (cmd2 == "checking" || cmd2 == "Checking" || cmd2 == "CHECKING") {m_orientation_status.at(cmd1) = HO_CHECKING; }
                else if (cmd2 == "checked" || cmd2 == "Checked" || cmd2 == "CHECKED")    {m_orientation_status.at(cmd1) = HO_CHECKED;  }
                else 
                { 
                    reply.addVocab32(Vocab32::encode("nack"));
                    yCWarning(NEXT_HEAD_ORIENT,"Error: wrong orientation status specified. You should use: unchecked, checking or checked.");
                }
            }
            else if (cmd1=="all")
            {
                if (cmd2 == "unchecked" || cmd2 == "Unchecked" || cmd2 == "UNCHECKED")   { for (auto &p : m_orientation_status ) p.second = HO_UNCHECKED; }  
                else if (cmd2 == "checking" || cmd2 == "Checking" || cmd2 == "CHECKING") { for (auto &p : m_orientation_status ) p.second = HO_CHECKING; }
                else if (cmd2 == "checked" || cmd2 == "Checked" || cmd2 == "CHECKED")    { for (auto &p : m_orientation_status ) p.second = HO_CHECKED; }
                else 
                { 
                    reply.addVocab32(Vocab32::encode("nack"));
                    yCWarning(NEXT_HEAD_ORIENT,"Error: wrong orientation status specified. You should use: unchecked, checking or checked.");
                }
            }
            else
            {
                reply.addVocab32(Vocab32::encode("nack"));
                yCWarning(NEXT_HEAD_ORIENT,"Error: specified location name not found.");
            }
        }
        else
        {
            reply.addVocab32(Vocab32::encode("nack"));
            yCWarning(NEXT_HEAD_ORIENT,"Error: wrong RPC command.");
        }
    }
    else
    {
        reply.addVocab32(Vocab32::encode("nack"));
        yCWarning(NEXT_HEAD_ORIENT,"Error: input RPC command bottle has a wrong number of elements.");
    }
    
    if (reply.size()==0)
        reply.addVocab32(Vocab32::encode("ack")); 
    
    return true;
}


void NextHeadOrient::home()
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
        { yCError(NEXT_HEAD_ORIENT) << "Error: No calibrator device was configured to perform this action, please verify that the wrapper config file for the part has the 'Calibrator' keyword in the attach phase"; }
        else
        { yCError(NEXT_HEAD_ORIENT) << "Error: The remote calibrator reported that something went wrong during the calibration procedure"; }
    }
    
}


double NextHeadOrient::getPeriod()
{
    return m_period;
}

bool NextHeadOrient::updateModule()
{   
    return true;
}

bool NextHeadOrient::close()
{
    if(m_Poly.isValid())
    {
        m_Poly.close();
    }

    if (!m_rpc_server_port.asPort().isOpen())
        m_rpc_server_port.close();
    
    return true;
}
