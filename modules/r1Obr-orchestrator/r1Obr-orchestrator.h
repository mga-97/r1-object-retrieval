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

#ifndef R1OBR_ORCHESTRATOR_H
#define R1OBR_ORCHESTRATOR_H

#include <yarp/os/all.h>

#include "orchestratorThread.h"
#include "inputManager.h"
#include "speechSynthesizer.h"

using namespace yarp::os;
using namespace std;

class Orchestrator : public RFModule
{
private:
    
    //Input Manager
    InputManager*               m_input_manager;

    //Callback thread
    OrchestratorThread*         m_inner_thread;

    //RPC Server
    RpcServer                   m_rpc_server_port;
    string                      m_rpc_server_port_name;
    
    //Other Input port
    BufferedPort<Bottle>        m_input_port;
    string                      m_input_port_name;

    //Status port
    BufferedPort<Bottle>        m_status_port;
    string                      m_status_port_name;

    //Speech Synthesizer
    SpeechSynthesizer*          m_speaker;

    //Others
    double                      m_period;

public:
    Orchestrator();
    virtual bool configure(ResourceFinder &rf);
    virtual bool close();
    virtual double getPeriod();
    virtual bool updateModule();

    bool respond(const Bottle &cmd, Bottle &reply);
};

#endif
