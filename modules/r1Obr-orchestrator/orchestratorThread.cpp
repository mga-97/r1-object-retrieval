﻿/*
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

#include "orchestratorThread.h"


YARP_LOG_COMPONENT(R1OBR_ORCHESTRATOR_THREAD, "r1_obr.orchestrator.orchestratorThread")


/****************************************************************/
OrchestratorThread::OrchestratorThread(yarp::os::ResourceFinder &rf):
    TypedReaderCallback(),
    m_rf(rf),
    m_status(R1_IDLE),
    m_question_count(0),
    m_where_specified(false),
    m_object_found(false),
    m_object_not_found(false)
{
    //Defaults
    m_sensor_network_rpc_port_name  = "/r1Obr-orchestrator/sensor_network:rpc";
    m_nextLoc_rpc_port_name         = "/r1Obr-orchestrator/nextLocPlanner/request:rpc";
    m_goandfindit_rpc_port_name     = "/r1Obr-orchestrator/goAndFindIt/request:rpc";
    m_goandfindit_result_port_name  = "/r1Obr-orchestrator/goAndFindIt/result:i";
    m_positive_outcome_port_name    = "/r1Obr-orchestrator/positive_outcome:o";
    m_negative_outcome_port_name    = "/r1Obr-orchestrator/negative_outcome:o";
    m_faceexpression_rpc_port_name  = "/r1Obr-orchestrator/faceExpression:rpc";
}

/****************************************************************/
bool OrchestratorThread::threadInit()
{
    // --------- ports config --------- //
    if (m_rf.check("sensor_network_rpc_port"))  {m_sensor_network_rpc_port_name = m_rf.find("sensor_network_rpc_port").asString();}
    if (m_rf.check("next_loc_planner_rpc_port")){m_nextLoc_rpc_port_name        = m_rf.find("next_loc_planner_rpc_port").asString();}
    if (m_rf.check("goandfindit_rpc_port"))     {m_goandfindit_rpc_port_name    = m_rf.find("goandfindit_rpc_port").asString();}
    if (m_rf.check("goandfindit_result_port"))  {m_goandfindit_result_port_name = m_rf.find("goandfindit_result_port").asString();}
    if (m_rf.check("faceexpression_rpc_port"))  {m_faceexpression_rpc_port_name = m_rf.find("faceexpression_rpc_port").asString();}
    
    if(!m_sensor_network_rpc_port.open(m_sensor_network_rpc_port_name)){
        yCError(R1OBR_ORCHESTRATOR_THREAD) << "Cannot open Out port with name" << m_sensor_network_rpc_port_name;
        return false;
    }

    if(!m_nextLoc_rpc_port.open(m_nextLoc_rpc_port_name)){
        yCError(R1OBR_ORCHESTRATOR_THREAD) << "Cannot open nextLocPlanner RPC port with name" << m_nextLoc_rpc_port_name;
        return false;
    }

    if(!m_goandfindit_rpc_port.open(m_goandfindit_rpc_port_name)){
        yCError(R1OBR_ORCHESTRATOR_THREAD) << "Cannot open findObject RPC port with name" << m_goandfindit_rpc_port_name;
        return false;
    }

    if(!m_goandfindit_result_port.open(m_goandfindit_result_port_name)){
        yCError(R1OBR_ORCHESTRATOR_THREAD) << "Cannot open nextOrient RPC port with name" << m_goandfindit_result_port_name;
        return false;
    }

    if(!m_faceexpression_rpc_port.open(m_faceexpression_rpc_port_name)){
        yCError(R1OBR_ORCHESTRATOR_THREAD) << "Cannot open faceExpression RPC port with name" << m_faceexpression_rpc_port_name;
        return false;
    }

    // --------- output ports config --------- //
    if(!m_rf.check("OUTPUT_PORT_GROUP"))
    {
        yCWarning(R1OBR_ORCHESTRATOR_THREAD,"OUTPUT_PORT_GROUP section missing in ini file. Using the default values");
    }
    else
    {
        Searchable& outputs = m_rf.findGroup("OUTPUT_PORT_GROUP");
        if(outputs.check("positive_outcome_port")) {m_positive_outcome_port_name = outputs.find("positive_outcome_port").asString();}
        if(outputs.check("negative_outcome_port")) {m_negative_outcome_port_name = outputs.find("negative_outcome_port").asString();}
    }

    if(!m_positive_outcome_port.open(m_positive_outcome_port_name)){
        yCError(R1OBR_ORCHESTRATOR_THREAD) << "Cannot open findObject RPC port with name" << m_positive_outcome_port_name;
        return false;
    }

    if(!m_negative_outcome_port.open(m_negative_outcome_port_name)){
        yCError(R1OBR_ORCHESTRATOR_THREAD) << "Cannot open nextOrient RPC port with name" << m_negative_outcome_port_name;
        return false;
    }

    // --------- Nav2Home config --------- //
    m_nav2home = new Nav2Home();
    if(!m_nav2home->configure(m_rf))
    {
        yCError(R1OBR_ORCHESTRATOR_THREAD,"Nav2Home configuration failed");
        return false;
    }

    return true;
}

/****************************************************************/
void OrchestratorThread::threadRelease()
{
    if (m_sensor_network_rpc_port.asPort().isOpen())
        m_sensor_network_rpc_port.close(); 
        
    if (m_nextLoc_rpc_port.asPort().isOpen())
        m_nextLoc_rpc_port.close(); 
        
    if (m_goandfindit_rpc_port.asPort().isOpen())
        m_goandfindit_rpc_port.close(); 
    
    if(!m_goandfindit_result_port.isClosed())
        m_goandfindit_result_port.close();

    if(!m_positive_outcome_port.isClosed())
        m_positive_outcome_port.close();

    if(!m_negative_outcome_port.isClosed())
        m_negative_outcome_port.close();
        
    if (m_faceexpression_rpc_port.asPort().isOpen())
        m_faceexpression_rpc_port.close(); 
    
    m_nav2home->close();

    yCInfo(R1OBR_ORCHESTRATOR_THREAD, "Orchestrator thread released");

    return;
}

/****************************************************************/
void OrchestratorThread::run()
{
    while (true)
    {
        setEmotion();
        
        if (m_status == R1_ASKING_NETWORK)
        {
            if (!askNetwork())
            {
                forwardRequest(m_request);
                m_status = R1_SEARCHING;
                yCInfo(R1OBR_ORCHESTRATOR_THREAD, "Requested: %s", m_request.toString().c_str());
            }
        }

        else if (m_status == R1_SEARCHING)
        {
            Bottle req{"status"};
            if(forwardRequest(req).get(0).asString() == "idle") 
                m_status = R1_IDLE;

            Bottle* result = m_goandfindit_result_port.read(false); 
            if(result != nullptr)
            {
                m_result = *result;
                if (result->get(0).asString() == "not found")  
                {
                    if(m_where_specified)
                    {
                        string mess = "do you want to continue your search in other locations?";
                        printf("%s", mess.c_str());
                        yCInfo(R1OBR_ORCHESTRATOR_THREAD, "%s", mess.c_str());
                        m_status = R1_WAITING_FOR_ANSWER;
                    }
                    else
                        m_status = R1_OBJECT_NOT_FOUND;
                }
                else     
                {
                    m_status = R1_OBJECT_FOUND;
                    Bottle&  sendOk = m_positive_outcome_port.prepare();
                    sendOk.clear();
                    sendOk = m_result;
                    m_positive_outcome_port.write();
                    yCInfo(R1OBR_ORCHESTRATOR_THREAD, "Object found");
                    m_object_found = true;
                }
            }
        }

        else if (m_status == R1_OBJECT_FOUND)
        {
            
        }

        else if (m_status == R1_OBJECT_NOT_FOUND)
        {
            
            yCInfo(R1OBR_ORCHESTRATOR_THREAD, "Object not found");
            m_object_not_found = true;

            m_nav2home->go();
            bool arrived{false};
            while (!arrived && m_status == R1_OBJECT_NOT_FOUND )
            {
                arrived = m_nav2home->areYouArrived();
                Time::delay(0.5);
            }

            if (m_status == R1_OBJECT_NOT_FOUND) //in case of external stop
            {
                Bottle&  sendKo = m_negative_outcome_port.prepare();
                sendKo.clear();
                sendKo = m_result;
                m_negative_outcome_port.write();

                m_status = R1_IDLE;
            }
        } 

        else if (m_status == R1_IDLE || m_status == R1_WAITING_FOR_ANSWER )
        {
            Time::delay(0.8);
        }

        else if (m_status == R1_STOP)
        {
            break;
        }

        Time::delay(0.2);
    }
}

/****************************************************************/
void OrchestratorThread::onRead(yarp::os::Bottle &b)
{
    yCInfo(R1OBR_ORCHESTRATOR_THREAD,"Received: %s",b.toString().c_str());

    if(b.size() == 0)
    {
        yCError(R1OBR_ORCHESTRATOR_THREAD,"The input request bottle has the wrong number of elements");
    }
    else 
    {
        string cmd {b.get(0).asString()};

        if (m_status == R1_WAITING_FOR_ANSWER)
        {
            answer(cmd);
        }
        else
        {
            if (cmd=="stop" || cmd=="reset") 
            { 
                stopOrReset(cmd);
            }
            else if (cmd=="navpos") 
            {
                Bottle request{cmd};
                forwardRequest(request); 
            }
            else if (cmd=="resume") 
            {
                resume();
            }
            else if (cmd=="search")
            {
                if (m_status == R1_OBJECT_FOUND || m_status == R1_OBJECT_NOT_FOUND )
                    stopOrReset("stop");
                
                search(b);            
            }
            else
            {
                yCError(R1OBR_ORCHESTRATOR_THREAD, "Error: input command bottle not valid");
            }
        }
    }
}

/****************************************************************/
void OrchestratorThread::onStop()
{
    Bottle request{"stop"};
    forwardRequest(request);
    m_status = R1_STOP;
}

/****************************************************************/
Bottle OrchestratorThread::forwardRequest(const Bottle& request)
{
    Bottle _rep_;
    m_goandfindit_rpc_port.write(request,_rep_);

    return _rep_;
}

/****************************************************************/
void OrchestratorThread::search(const Bottle& btl)
{
    if (m_status == R1_OBJECT_NOT_FOUND || m_status == R1_OBJECT_FOUND)
    {
        stopOrReset("stop");
    } 

    m_object_found = false;
    m_object_not_found = false;
    
    if(resizeSearchBottle(btl))
    {
        m_status = R1_ASKING_NETWORK;
    }
    else
    {
        stopOrReset("stop"); 
    }  
        
}

/****************************************************************/
bool OrchestratorThread::resizeSearchBottle(const Bottle& btl)
{
    int sz = btl.size();

    if (sz > 2)
        m_where_specified = true;
    else 
        m_where_specified = false;

    m_request.clear();
    for (int i=0; i < min(sz,3); i++)
    {
        m_request.addString(btl.get(i).asString());

        if(i==2)
        {
            Bottle req,rep;
            req.fromString("find " + btl.get(i).asString());
            m_nextLoc_rpc_port.write(req,rep); //check if location name is valid
            if (rep.get(0).asString() != "ok")
            {
                yCError(R1OBR_ORCHESTRATOR_THREAD,"Location specified is not valid.");
                m_request.clear();
                return false;
            }   
        }
    }
    
    return true;
}

/****************************************************************/
bool OrchestratorThread::askNetwork()
{
    
    // NOT IMPLEMENTED YET

    return false;
}

/****************************************************************/
Bottle OrchestratorThread::stopOrReset(const string& cmd)
{
    Bottle rep, request{cmd};
    yCInfo(R1OBR_ORCHESTRATOR_THREAD, "Stopping");
    rep = forwardRequest(request); 
    
    if (m_status == R1_OBJECT_NOT_FOUND)
    {
        m_nav2home->stop();
    } 
    else if (m_status == R1_OBJECT_FOUND)
    {
        Bottle&  send = m_positive_outcome_port.prepare();
        send.clear();
        send.addString("stop");
        m_positive_outcome_port.write();
    } 

    if (cmd == "reset")
    {
        m_object_found = false;
        m_object_not_found = false;
        Bottle req("navpos"); 
        forwardRequest(req);
    }

    if (rep.size() == 0)
        rep.addVocab32(Vocab32::encode("ack"));

    m_status = R1_IDLE;
    return rep;
}

/****************************************************************/
string OrchestratorThread::resume()
{
    yCInfo(R1OBR_ORCHESTRATOR_THREAD, "Resuming");

    if (m_object_found)
    {
        Bottle req("navpos"); 
        forwardRequest(req);
        
        Bottle&  send = m_positive_outcome_port.prepare();
        send.clear();
        send.addString("resume");
        m_positive_outcome_port.write();
        
        m_status = R1_OBJECT_FOUND;

        return "resume: object found";
    }
    else if (m_object_not_found)
    {
        Bottle req("navpos"); 
        forwardRequest(req);
        
        m_status = R1_OBJECT_NOT_FOUND;
        return "resume: object not found";
    }
    else
    {
        Bottle rep, request{"resume"};
        rep = forwardRequest(request); 

        request.clear();
        request.addString("status");
        if(forwardRequest(request).get(0).asString() != "idle") 
            m_status = R1_SEARCHING;
        
        return "search resumed";
    }

}

/****************************************************************/
bool OrchestratorThread::answer(const string& ans)
{
    if (ans=="yes") 
    {
        resume();
        m_question_count = 0;
    }
    else if (ans=="no") 
    {
        m_status = R1_OBJECT_NOT_FOUND;
        m_question_count = 0;
    }
    else
    {
        if (m_question_count<3)
        {
            yCError(R1OBR_ORCHESTRATOR_THREAD, "Answer not recognized. Say 'yes' or 'no'");
            m_question_count++;
        }
        else
        {
            yCError(R1OBR_ORCHESTRATOR_THREAD, "Answer not recognized. Stopping the search.");
            m_status = R1_IDLE;
        }
        return false;
    }
    return true;
}


/****************************************************************/
string OrchestratorThread::getWhat()
{
    Bottle request{"what"};
    return forwardRequest(request).toString();
}


/****************************************************************/
string OrchestratorThread::getWhere()
{
    Bottle request{"where"};
    return forwardRequest(request).toString();
}


/****************************************************************/
string OrchestratorThread::getStatus()
{
    Bottle request{"status"}, reply;
    m_goandfindit_rpc_port.write(request,reply);
    string goAndFindItStatus = reply.get(0).asString();
    string str;

    switch (m_status)
    {
    case R1_IDLE:
        str = "idle";
        break;
    case R1_ASKING_NETWORK:
        str = "asking_network";
        break;
    case R1_SEARCHING:
        str = "searching - " + goAndFindItStatus;
        break;
    case R1_WAITING_FOR_ANSWER:
        str = "waiting_for_answer";
        break;
    case R1_OBJECT_FOUND:
        str = "object_found";
        break;
    case R1_OBJECT_NOT_FOUND:
        str = "object_not_found";
        break;
    case R1_STOP:
        str = "stop";
        break;
    default:
        str = "ERROR";
        break;
    };

    return str;
}


/****************************************************************/
void OrchestratorThread::info(Bottle& reply)
{
    reply.clear();
    reply.addVocab32("many");

    Bottle& whatList=reply.addList();
    whatList.addString("what"); whatList.addString(":");
    whatList.addString(getWhat());

    Bottle& whereList=reply.addList();
    whereList.addString("where"); whereList.addString(":");
    whereList.addString(getWhere());

    Bottle& statusList=reply.addList();
    statusList.addString("status"); statusList.addString(":");
    statusList.addString(getStatus());
}


/****************************************************************/
void OrchestratorThread::setEmotion()
{
    Bottle request, reply;
    if (m_status == R1_OBJECT_FOUND || m_object_found)
        request.fromString("emotion 1"); //happy
    else if (m_status == R1_OBJECT_NOT_FOUND || m_object_not_found)
        request.fromString("emotion 0"); //sad
    else if (getStatus() == "searching - searching" || m_status == R1_ASKING_NETWORK)
        request.fromString("emotion 2"); //thinking
    else 
        request.fromString("emotion 1"); //happy
    
    m_faceexpression_rpc_port.write(request,reply);
}