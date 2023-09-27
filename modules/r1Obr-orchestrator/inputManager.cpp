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

#include "inputManager.h"

YARP_LOG_COMPONENT(INPUT_MANAGER, "r1_obr.orchestrator.inputManager")


InputManager::InputManager(double _period, yarp::os::ResourceFinder &rf) 
    : PeriodicThread(_period), m_rf(rf) {}


// ****************************************************** //
bool InputManager::threadInit()
{
    // Defaults
    string voiceCommandPortName     = "/r1Obr-orchestrator/voice_command:i";
    string inputCommandPortName     = "/r1Obr-orchestrator/input_command:i";
    string inputManagerRPCPortName  = "/r1Obr-orchestrator/inputManager:rpc";
    string device_chatBot_nwc       = "chatBot_nwc_yarp";
    string local_chatBot_nwc        = "/r1Obr-orchestrator/chatBot";
    string remote_chatBot_nwc       = "/chatBot_nws";
    string language_chatbot         = "auto";

    // ------------------  out ------------------ //  
    if(!m_inputManagerRPCPort.open(inputManagerRPCPortName))
    {
        yCError(INPUT_MANAGER, "Unable to open input manager RPC port");
        return false;
    }

    // ------------------  in  ------------------ //
    if(!m_rf.check("INPUT_MANAGER"))
    {
        yCWarning(INPUT_MANAGER,"INPUT_MANAGER section missing in ini file. Using the default values");
    }
    
    Searchable& inputs_config = m_rf.findGroup("INPUT_MANAGER");
    if(inputs_config.check("voice_command_port")) {voiceCommandPortName = inputs_config.find("voice_command_port").asString();}
    if(inputs_config.check("input_command_port")) {inputCommandPortName = inputs_config.find("input_command_port").asString();}
    

    // Voice Command Port
    if (!m_voiceCommandPort.open(voiceCommandPortName))
    {
        yCError(INPUT_MANAGER, "Unable to open voice command port");
        return false;
    }
    m_voiceCommandPort.useCallback(*this);

    // Other Input Port
    if (!m_inputCommandPort.open(inputCommandPortName))
    {
        yCError(INPUT_MANAGER, "Unable to open input command port");
        return false;
    }

    // iChatBot
    m_chatBot_active = inputs_config.check("chatbot_active") ? (inputs_config.find("chatbot_active").asString() == "true") : false;
    
    if(m_chatBot_active)
    {
        Property chatBotProp;
        chatBotProp.put("device", inputs_config.check("device", Value(device_chatBot_nwc)));
        chatBotProp.put("local",  inputs_config.check("local", Value(local_chatBot_nwc)));
        chatBotProp.put("remote", inputs_config.check("remote", Value(remote_chatBot_nwc)));
        m_PolyCB.open(chatBotProp);
        if(!m_PolyCB.isValid())
        {
            yCWarning(INPUT_MANAGER,"Error opening PolyDriver check parameters. Using the default values");
        }
        m_PolyCB.view(m_iChatBot);
        if(!m_iChatBot){
            yCError(INPUT_MANAGER,"Error opening IChatBot interface. Device not available");
            return false;
        }

        if(inputs_config.check("language")) language_chatbot = inputs_config.find("language").asString();
        m_iChatBot->setLanguage(language_chatbot);
    }

    

    return true;
}

    
// ****************************************************** //
void InputManager::threadRelease()
{    
    if(!m_voiceCommandPort.isClosed())
        m_voiceCommandPort.close();

    if(!m_inputCommandPort.isClosed())
        m_inputCommandPort.close();
    
    if (m_inputManagerRPCPort.asPort().isOpen())
        m_inputManagerRPCPort.close(); 
    
    if(m_PolyCB.isValid())
        m_PolyCB.close();

    yCInfo(INPUT_MANAGER, "Input Manager thread released");
}


// ****************************************************** //
void InputManager::run()
{
    Bottle* inputCommand = m_inputCommandPort.read(false); 
    if(inputCommand  != nullptr)
    {
        writeToRPC(*inputCommand);
    }
}


// ****************************************************** //
void InputManager::onRead(Bottle& b)
{
    string b_str = b.toString();
    yCInfo(INPUT_MANAGER,"Received: %s",b_str.c_str());

    string msg;
    m_iChatBot->interact(b_str, msg);
    yCInfo(INPUT_MANAGER,"ChatBot replied: %s",msg.c_str());

    Bottle msg_btl;
    msg_btl.fromString(msg);
    writeToRPC(msg_btl);
}


// ****************************************************** //
void InputManager::writeToRPC(Bottle& req)
{
    Bottle _rep_;
    m_inputManagerRPCPort.write(req,_rep_);

    yCInfo(INPUT_MANAGER, "Replied from orchestrator: %s", _rep_.toString().c_str() );
}

