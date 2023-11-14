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

#ifndef SENSOR_NETWORK_RECEIVER_CPP
#define SENSOR_NETWORK_RECEIVER_CPP

#include <yarp/os/Network.h>
#include <yarp/os/Log.h>
#include <yarp/os/LogStream.h>
#include <yarp/os/Time.h>
#include <yarp/os/Port.h>
#include <yarp/os/RFModule.h>
#include <yarp/os/RpcClient.h>
#include <yarp/os/BufferedPort.h>
#include <yarp/dev/ILLM.h>
#include <yarp/dev/PolyDriver.h>

YARP_LOG_COMPONENT(SENSOR_NETWORK_RECEIVER, "r1_obr.sensorNetworkReceiver")

using namespace yarp::os;
using namespace yarp::dev;
using namespace std;

class sensorNetworkReceiver : public RFModule
{
private:

    double               m_period;

    //Ports
    RpcServer            m_rpc_server_port;
    string               m_rpc_server_port_name;

    BufferedPort<Bottle> m_input_port;
    string               m_input_port_name;

    BufferedPort<Bottle> m_output_port;
    string               m_output_port_name;

    RpcClient            m_rpc_to_orchestrator_port;
    string               m_rpc_to_orchestrator_port_name;

    //ChatGPT Filter
    bool                 m_llm_active;
    PolyDriver           m_poly;
    Property             m_prop;
    string               m_llm_local;
    string               m_llm_remote;
    ILLM*                m_iLlm = nullptr;

public:

    //Constructor/Distructor
    sensorNetworkReceiver();
    ~sensorNetworkReceiver() = default;

    //Internal methods
    virtual bool configure(ResourceFinder &rf);
    virtual bool close();
    virtual double getPeriod();
    virtual bool updateModule();

    bool respond(const Bottle &cmd, Bottle &reply);
};



sensorNetworkReceiver::sensorNetworkReceiver()
{
    m_period = 0.1;
    m_rpc_server_port_name  = "/sensorNetworkReceiver/rpc";
    m_input_port_name = "/sensorNetworkReceiver/input:i";
    m_output_port_name = "/sensorNetworkReceiver/output:o";
    m_llm_active = false;
    m_llm_local = "/sensorNetworkReceiver/llm/rpc";
    m_llm_remote = "/yarpgpt/rpc";
    m_rpc_to_orchestrator_port_name = "/sensorNetworkReceiver/orchestrator:rpc";
};

bool sensorNetworkReceiver::configure(ResourceFinder &rf)
{
    if(rf.check("period")){m_period = rf.find("period").asFloat32();}   
    
    if(rf.check("rpc_port")) { m_rpc_server_port_name = rf.find("rpc_port").asString();}
    if (!m_rpc_server_port.open(m_rpc_server_port_name))
    {
        yCError(SENSOR_NETWORK_RECEIVER, "Could not open rpc port %s, check network", m_rpc_server_port_name.c_str());
        return false;
    }
    if (!attach(m_rpc_server_port))
    {
        yCError(SENSOR_NETWORK_RECEIVER, "attach() error with rpc port %s", m_rpc_server_port_name.c_str());
        return false;
    }

    if(rf.check("input_port")) { m_input_port_name = rf.find("input_port").asString();}
    if (!m_input_port.open(m_input_port_name))
    {
        yCError(SENSOR_NETWORK_RECEIVER, "Unable to open input port");
        return false;
    }

    if(rf.check("output_port")) {m_output_port_name = rf.find("output_port").asString();}
    if (!m_output_port.open(m_output_port_name))
    {
        yCError(SENSOR_NETWORK_RECEIVER, "Unable to open output port");
        return false;
    }

    
    if(rf.check("llm_active")) { m_llm_active = (rf.find("llm_active").asString() == "true");}
    if (m_llm_active)
    {
        if(rf.check("llm_local")) { m_llm_local = rf.find("llm_local").asString();}
        if(rf.check("llm_remote")) { m_llm_remote = rf.find("llm_remote").asString();}
        m_prop.put("device", "LLM_nwc_yarp");
        m_prop.put("local", m_llm_local);
        m_prop.put("remote", m_llm_remote);
        if (!m_poly.open(m_prop)) {
            yCWarning(SENSOR_NETWORK_RECEIVER) << "Cannot open LLM_nwc_yarp";
            return false;
        }

        if (!m_poly.view(m_iLlm)) {
            yCWarning(SENSOR_NETWORK_RECEIVER) << "Cannot open interface from driver";
            return false;
        }

        if(rf.check("rpc_to_orchestrator")) {m_rpc_to_orchestrator_port_name = rf.find("rpc_to_orchestrator").asString();}
        if (!m_rpc_to_orchestrator_port.open(m_rpc_to_orchestrator_port_name))
        {
            yCError(SENSOR_NETWORK_RECEIVER, "Unable to open RPC port to orchestrator");
            return false;
        }

    }

    return true;
}

bool sensorNetworkReceiver::respond(const Bottle &b, Bottle &reply)
{
    yCInfo(SENSOR_NETWORK_RECEIVER,"RPC Received: %s",b.toString().c_str());

    reply.clear();
    string cmd=b.get(0).asString();

    if (cmd=="help")
    {   
        reply.addString("Everything you will send to this port will be forwarded to the port " + m_output_port_name + ". The reply is forwarded from port " + m_input_port_name);
        return true;
    }
    
    if(m_llm_active)
    {
        string answer;
        m_iLlm->ask(b.toString(), answer);
        Bottle btl; btl.fromString(answer);
        if (btl.get(0).asString()=="say")
        {
            m_rpc_to_orchestrator_port.write(btl);
            reply.addString("Sto parlando");
            return true;
        }
    }
  

    Bottle&  out = m_output_port.prepare();
    out.clear();
    out.addString(b.toString());
    m_output_port.write();

    bool reply_arrived{false};
    while (!reply_arrived) 
    {
        Bottle* reply_ptr = m_input_port.read(false);
        if (reply_ptr)
        {
            reply_arrived = true;
            reply = *reply_ptr;
        }
        else
            Time::delay(0.5); 
                           
    } 
    
    if (reply.size()==0)
        reply.addVocab32(Vocab32::encode("nack")); 
    
    return true;
}


bool sensorNetworkReceiver::updateModule()
{
    return true;
}

double sensorNetworkReceiver::getPeriod()
{
    return m_period;
}

bool sensorNetworkReceiver::close()
{
    if (m_rpc_server_port.asPort().isOpen())
        m_rpc_server_port.close(); 
    
    if (!m_input_port.isClosed())
        m_input_port.close();
    
    if (!m_output_port.isClosed())
        m_output_port.close();

    if (m_rpc_to_orchestrator_port.asPort().isOpen())
        m_rpc_to_orchestrator_port.close(); 
    
    if(m_poly.isValid())
        m_poly.close();

    return true;
}

int main(int argc, char *argv[])
{
    yarp::os::Network yarp;
    if (!yarp.checkNetwork())
    {
        yError("check Yarp network.\n");
        return -1;
    }

    yarp::os::ResourceFinder rf;
    rf.setVerbose(true);
    rf.setDefaultConfigFile("sensorNetworkReceiver.ini");          //overridden by --from parameter
    rf.setDefaultContext("sensorNetworkReceiver");                 //overridden by --context parameter
    rf.configure(argc,argv);
    sensorNetworkReceiver mod;
    
    return mod.runModule(rf);
}


#endif