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


#include "r1Obr-orchestrator.h"

YARP_LOG_COMPONENT(R1OBR_ORCHESTRATOR, "r1_obr.orchestrator")

Orchestrator::Orchestrator() :
    m_period(1.0)
{  
    m_rpc_server_port_name  = "/r1Obr-orchestrator/rpc";
    m_input_port_name = "/r1Obr-orchestrator/input:i";
    m_status_port_name = "/r1Obr-orchestrator/status:o";
}

bool Orchestrator::configure(ResourceFinder &rf) 
{   

    if(rf.check("period")){m_period = rf.find("period").asFloat32();}  

    if(rf.check("input_port"))
        m_input_port_name = rf.find("input_port").asString();
    if (!m_input_port.open(m_input_port_name))
    {
        yCError(R1OBR_ORCHESTRATOR, "Unable to open input manager port");
        return false;
    }

    // --------- Open RPC Server Port --------- //
    if(rf.check("input_rpc_port"))
        m_rpc_server_port_name = rf.find("input_rpc_port").asString();
    if (!m_rpc_server_port.open(m_rpc_server_port_name))
    {
        yCError(R1OBR_ORCHESTRATOR, "Could not open rpc port %s, check network", m_rpc_server_port_name.c_str());
        return false;
    }
    if (!attach(m_rpc_server_port))
    {
        yCError(R1OBR_ORCHESTRATOR, "attach() error with rpc port %s", m_rpc_server_port_name.c_str());
        return false;
    }

    // --------- Input Manager initialization --------- //
    m_input_manager = new InputManager(0.1, rf);
    if (!m_input_manager->start()){
        return false;
    }
    string input_manager_port_name = "/r1Obr-orchestrator/inputManager:rpc";
    bool ok = Network::connect(input_manager_port_name.c_str(), m_rpc_server_port_name.c_str());
    if (!ok)
    {
        yCError(R1OBR_ORCHESTRATOR,"Could not connect to %s\n", input_manager_port_name.c_str());
        return false;
    }
   
    // --------- Thread initialization --------- //
    m_inner_thread = new OrchestratorThread(rf);
    bool threadOk = m_inner_thread->start();
    if (!threadOk){
        return false;
    }
    m_input_port.useCallback(*m_inner_thread);

    // --------- Status output ----------- //
    if(rf.check("status_port")){m_status_port_name = rf.find("status_port").asString();} 
    if (!m_status_port.open(m_status_port_name))
    {
        yCError(R1OBR_ORCHESTRATOR, "Unable to open output status port");
        return false;
    }

    return true;
}


bool Orchestrator::close()
{
    if (m_rpc_server_port.asPort().isOpen())
        m_rpc_server_port.close();  
        
    if (!m_input_port.isClosed())
        m_input_port.close();
    
    m_inner_thread->stop();
    delete m_inner_thread;

    m_input_manager->stop();
    delete m_input_manager;
    
    return true;
}


double Orchestrator::getPeriod()
{
    return m_period;
}


bool Orchestrator::updateModule()
{
    Bottle&  status = m_status_port.prepare();
    status.clear();
    m_inner_thread->info(status);
    m_status_port.write();
    
    if (isStopping())
    {
        if (m_inner_thread) m_inner_thread->stop();
        return false;
    }

    return true;
}



bool Orchestrator::respond(const Bottle &request, Bottle &reply)
{
    reply.clear();
    string cmd=request.get(0).asString();
    if (request.size()==1 || cmd == "search")
    {
        if (cmd=="help")
        {
            reply.addVocab32("many");
            reply.addString("help   : gets this list");
            reply.addString("status : returns the current status of the search");
            reply.addString("what   : returns the object of the current search");
            reply.addString("where  : returns the location of the current search");
            reply.addString("info   : returns the status, object and location of the current search");
            reply.addString("stop   : stops search");
            reply.addString("reset  : resets search");
            reply.addString("navpos : sets the robot in navigation position");
            reply.addString("");
            reply.addString(" ---> The following commands cannot be sent if the status is 'waiting_for_answer'");
            reply.addString("search <what> : starts to search for 'what'");
            reply.addString("search <what> <where>: starts searching for 'what' at location 'where'");
            reply.addString("resume : resumes stopped search");
        }
        else if (cmd=="status")
        {
            reply.addString(m_inner_thread->getStatus());
        }
        else if (cmd=="stop" || cmd=="reset")
        {
            reply = m_inner_thread->stopOrReset(cmd);
        }
        else if (cmd=="what" || cmd=="where" || cmd=="navpos")
        {
            reply = m_inner_thread->forwardRequest(request);
        }
        else if (cmd=="info")
        {
            m_inner_thread->info(reply);
            
        }
        else if (m_inner_thread->getStatus() == "waiting_for_answer")
        {
            if(m_inner_thread->answer(cmd))
                reply.addVocab32(Vocab32::encode("ack"));
            else
            {
                reply.addVocab32(Vocab32::encode("nack"));
                reply.addString("Answer not recognized. Say 'yes' or 'no'");
            }
        }
        else if (cmd=="search")
        {
            m_inner_thread->search(request);
            m_inner_thread->setObject(request.get(1).asString());
            reply.addString("searching for '" + request.get(1).asString() + "'");
        }
        else if (cmd=="resume")
        {
            reply.addString(m_inner_thread->resume());
        }
        else
        {
            reply.addVocab32(Vocab32::encode("nack"));
            yCWarning(R1OBR_ORCHESTRATOR,"Error: wrong RPC command. Type 'help'");
        }
    }
    else
    {
        reply.addVocab32(Vocab32::encode("nack"));
        yCWarning(R1OBR_ORCHESTRATOR,"Error: wrong RPC command. Type 'help'");
    }

    if (reply.size()==0)
        reply.addVocab32(Vocab32::encode("ack")); 

    return true;
}