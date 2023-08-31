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

#ifndef R1OBR_ORCHESTRATOR_THREAD_H
#define R1OBR_ORCHESTRATOR_THREAD_H

#include <yarp/os/all.h>
#include "nav2home.h"

using namespace yarp::os;
using namespace std;

class OrchestratorThread : public Thread, public TypedReaderCallback<Bottle>
{
enum R1_status 
{
    R1_IDLE,
    R1_ASKING_NETWORK,
    R1_SEARCHING,
    R1_WAITING_FOR_ANSWER,
    R1_OBJECT_FOUND,
    R1_OBJECT_NOT_FOUND,
    R1_STOP      
};

protected:
    
    // Ports
    string                  m_sensor_network_rpc_port_name;
    RpcClient               m_sensor_network_rpc_port;

    string                  m_nextLoc_rpc_port_name;
    RpcClient               m_nextLoc_rpc_port;

    string                  m_goandfindit_rpc_port_name;
    RpcClient               m_goandfindit_rpc_port;

    string                  m_goandfindit_result_port_name;
    BufferedPort<Bottle>    m_goandfindit_result_port;

    string                  m_positive_outcome_port_name;
    BufferedPort<Bottle>    m_positive_outcome_port;

    string                  m_negative_outcome_port_name;
    BufferedPort<Bottle>    m_negative_outcome_port;

    // Others
    R1_status               m_status;
    Bottle                  m_request;
    Bottle                  m_result;
    int                     m_question_count;
    bool                    m_where_specified;

    Nav2Home*               m_nav2home;

    ResourceFinder&         m_rf;

public:
    //Contructor and distructor
    OrchestratorThread(ResourceFinder &rf);
    ~OrchestratorThread() = default;

    //methods inherited from Thread
    virtual void run() override;
    virtual bool threadInit() override;
    virtual void threadRelease() override;
    virtual void onStop() override;

    //Port inherited from TypedReaderCallback
    using TypedReaderCallback<Bottle>::onRead;
    void onRead(Bottle& b) override;

    Bottle forwardRequest(const Bottle& b);
    void search(const Bottle& btl);
    bool resizeSearchBottle(const Bottle& btl);
    bool askNetwork();
    Bottle stopOrReset(const string& cmd);
    Bottle resume();
    bool answer(const string& ans);
    string getStatus();

};

#endif
