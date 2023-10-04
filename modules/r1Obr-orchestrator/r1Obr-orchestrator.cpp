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
    m_positive_feedback_port_name = "/r1Obr-orchestrator/positive_outcome_feedback:i";
}


/****************************************************************/
bool Orchestrator::configure(ResourceFinder &rf) 
{   

    if(rf.check("period")){m_period = rf.find("period").asFloat32();}  

    
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


    // --------- Open Input command Port --------- //
    if(rf.check("input_port"))
        m_input_port_name = rf.find("input_port").asString();
    if (!m_input_port.open(m_input_port_name))
    {
        yCError(R1OBR_ORCHESTRATOR, "Unable to open input port");
        return false;
    }


    // --------- Open Positive Feedback Port --------- //
    if(rf.check("positive_feedback_port"))
        m_positive_feedback_port_name = rf.find("positive_feedback_port").asString();
    if (!m_positive_feedback_port.open(m_positive_feedback_port_name))
    {
        yCError(R1OBR_ORCHESTRATOR, "Unable to open positive feedback port");
        return false;
    }
    m_positive_feedback_port.useCallback(*this);


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
    
    // --------- SpeechSynthesizer config --------- //
    m_additional_speaker = new SpeechSynthesizer();
    if(!m_additional_speaker->configure(rf, "/second"))
    {
        yCError(R1OBR_ORCHESTRATOR,"SpeechSynthesizer configuration failed");
        return false;
    }

    return true;
}


/****************************************************************/
bool Orchestrator::close()
{
    if (m_rpc_server_port.asPort().isOpen())
        m_rpc_server_port.close();  
        
    if (!m_input_port.isClosed())
        m_input_port.close();
        
    if (!m_positive_feedback_port.isClosed())
        m_positive_feedback_port.close();
    
    m_inner_thread->stop();
    delete m_inner_thread;
    
    return true;
}


/****************************************************************/
double Orchestrator::getPeriod()
{
    return m_period;
}


/****************************************************************/
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


/****************************************************************/
bool Orchestrator::respond(const Bottle &request, Bottle &reply)
{
    yCInfo(R1OBR_ORCHESTRATOR,"RPC Received: %s",request.toString().c_str());
    
    reply.clear();
    string cmd=request.get(0).asString();
    if (request.size()==1)
    {
        if (cmd=="help")
        {
            reply.addVocab32("many");
            reply.addString("help   : gets this list");
            reply.addString("search <what> : starts to search for 'what'");
            reply.addString("search <what> <where>: starts searching for 'what' at location 'where'");
            reply.addString("stop   : stops search");
            reply.addString("reset  : resets search");
            reply.addString("reset_home: resets search and navigates the robot home");
            reply.addString("resume : resumes stopped search");
            reply.addString("status : returns the current status of the search");
            reply.addString("what   : returns the object of the current search");
            reply.addString("where  : returns the location of the current search");
            reply.addString("info   : returns the status, object and location of the current search");
            reply.addString("navpos : sets the robot in navigation position");
            reply.addString("go <location> : navigates the robot to 'location' if it is valid");

        }
        else if (cmd=="status")
        {
            reply.addString(m_inner_thread->getStatus());
        }
        else if (cmd=="stop" || cmd=="reset")
        {
            reply.addString(m_inner_thread->stopOrReset(cmd));
        }
        else if (cmd=="reset_home")
        {
            reply.addString(m_inner_thread->resetHome());
        }
        else if (cmd=="resume")
        {
            reply.addString(m_inner_thread->resume());
        }
        else if (cmd=="what" || cmd=="where")
        {
            reply = m_inner_thread->forwardRequest(request);
        }
        else if (cmd=="info")
        {
            m_inner_thread->info(reply);            
        }
        else if (cmd=="navpos")
        {
            reply = m_inner_thread->forwardRequest(request);
            m_inner_thread->askChatBotToSpeak(OrchestratorThread::navigation_position);
        }
        else
        {
            reply.addVocab32(Vocab32::encode("nack"));
            yCWarning(R1OBR_ORCHESTRATOR,"Error: wrong RPC command. Type 'help'");
            m_inner_thread->askChatBotToSpeak(OrchestratorThread::fallback);
        }
    }
    else if (cmd=="search")
    {
        m_inner_thread->search(request);
        m_inner_thread->setObject(request.get(1).asString());
        reply.addString("searching for '" + request.get(1).asString() + "'");
    }
    else if (cmd=="say")
    {   
        string toSay = request.get(1).asString();
        say(toSay);
        reply.addString("speaking");
    }
    else if (cmd=="go")
    {   
        string location_name = request.get(1).asString();;
        m_inner_thread->go(location_name);
        reply.addString("going to '" + location_name + "'");
    }
    else
    {
        reply.addVocab32(Vocab32::encode("nack"));
        yCWarning(R1OBR_ORCHESTRATOR,"Error: wrong RPC command. Type 'help'");
        m_inner_thread->askChatBotToSpeak(OrchestratorThread::fallback);
    }

    if (reply.size()==0)
        reply.addVocab32(Vocab32::encode("ack")); 

    return true;
}

/****************************************************************/
void Orchestrator::onRead(yarp::os::Bottle &b)
{
    yCInfo(R1OBR_ORCHESTRATOR, "Received confirmation that the object has been found");
    m_inner_thread->objectFound();
}

/****************************************************************/
bool Orchestrator::say(string toSay)
{
    return m_additional_speaker->say(toSay);
}