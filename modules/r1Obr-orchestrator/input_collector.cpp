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

#include "input_collector.h"

YARP_LOG_COMPONENT(INPUT_COLLECTOR, "r1_obr.orchestrator.input-collector")

InputCollector::InputCollector(double _period, yarp::os::ResourceFinder &rf) 
    : PeriodicThread(_period), m_rf(rf) {}

// ------------------------------------------------------ //
bool InputCollector::threadInit()
{
    // Defaults
    string voiceCommandPortName = "/r1Obr-orchestrator/voice_command:i";
    string inputCommandPortName = "/r1Obr-orchestrator/input_command:i";
    string inputsCollectorPortName = "/r1Obr-orchestrator/inputs:o";

    // out
    if(!m_outputPort.open(inputsCollectorPortName))
    {
        yCError(INPUT_COLLECTOR, "Unable to open inputs collector port");
        return false;
    }

    // in
    if(!m_rf.check("INPUT_PORT_GROUP"))
    {
        yCWarning(INPUT_COLLECTOR,"INPUT_PORT_GROUP section missing in ini file. Using the default values");
    }
    else
    {
        Searchable& inputs = m_rf.findGroup("INPUT_PORT_GROUP");
        if(inputs.check("voice_command_port")) {voiceCommandPortName = inputs.find("voice_command_port").asString();}
        if(inputs.check("input_command_port")) {inputCommandPortName = inputs.find("input_command_port").asString();}
    }

    if (!m_voiceCommandPort.open(voiceCommandPortName))
    {
        yCError(INPUT_COLLECTOR, "Unable to open voice command port");
        return false;
    }

    if (!m_inputCommandPort.open(inputCommandPortName))
    {
        yCError(INPUT_COLLECTOR, "Unable to open input command port");
        return false;
    }

    return true;
}
    
// ------------------------------------------------------ //
void InputCollector::threadRelease()
{    
    if(!m_voiceCommandPort.isClosed())
        m_voiceCommandPort.close();

    if(!m_inputCommandPort.isClosed())
        m_inputCommandPort.close();
    
    if(!m_outputPort.isClosed())
        m_outputPort.close();

    yCInfo(INPUT_COLLECTOR, "Input collector thread released");
}

// ------------------------------------------------------ //
void InputCollector::run()
{
    Bottle* voiceCommand = m_voiceCommandPort.read(false); 
    if(voiceCommand != nullptr)
    {
        // NOT IMPLEMENTED YET
        yCError(INPUT_COLLECTOR, "Voice command not implemented yet");
        return;
    }
        
    Bottle* inputCommand = m_inputCommandPort.read(false); 
    if(inputCommand  != nullptr)
    {
        Bottle&  toMain = m_outputPort.prepare();
        toMain.clear();
        toMain = *inputCommand;
        m_outputPort.write();
    }
}


