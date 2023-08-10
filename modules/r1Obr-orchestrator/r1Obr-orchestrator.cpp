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
}

bool Orchestrator::configure(yarp::os::ResourceFinder &rf) 
{   

    if(rf.check("period")){m_period = rf.find("period").asFloat32();}  

    string input_port_name = "/r1Obr-orchestrator/inputs:i";
    string input_collector_port_name = "/r1Obr-orchestrator/inputs:o";
    if (!m_input_port.open(input_port_name))
    {
        yCError(R1OBR_ORCHESTRATOR, "Unable to open input collector port");
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

    // --------- Input Collector initialization --------- //
    m_input_collector = new InputCollector(0.1, rf);
    if (!m_input_collector->start()){
        return false;
    }
    bool ok = Network::connect(input_collector_port_name.c_str(), input_port_name.c_str());
    if (!ok)
    {
        yCError(R1OBR_ORCHESTRATOR,"Could not connect to %s\n", input_collector_port_name.c_str());
        return false;
    }
   
    // --------- Thread initialization --------- //
    m_inner_thread = new OrchestratorThread(rf);
    bool threadOk = m_inner_thread->start();
    if (!threadOk){
        return false;
    }
    m_input_port.useCallback(*m_inner_thread);

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

    m_input_collector->stop();
    delete m_input_collector;
    
    return true;
}


double Orchestrator::getPeriod()
{
    return m_period;
}


bool Orchestrator::updateModule()
{
    if (isStopping())
    {
        if (m_inner_thread) m_inner_thread->stop();
        return false;
    }

    return true;
}

