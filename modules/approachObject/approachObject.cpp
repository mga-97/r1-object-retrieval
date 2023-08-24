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

#include "approachObject.h"


YARP_LOG_COMPONENT(APPROACH_OBJECT, "r1_obr.approachObject")


/****************************************************************/
ApproachObject::ApproachObject() : m_period(1.0) 
{
    m_input_port_name   = "/approachObject/input_coords:i";
}


/****************************************************************/
bool ApproachObject::configure(ResourceFinder &rf)
{
    // ------------ Generic config ------------ //
    if(rf.check("period")) {m_period = rf.find("period").asFloat32();}

    if(rf.check("input_port")) {m_input_port_name = rf.find("input_port").asString();}
    

    // --------- Executor initialization --------- //
    m_executor = new ApproachObjectExecutor(rf);
    bool execOk = m_executor->configure();
    if (!execOk){
        return false;
    }

    std::string posName = "/handPointing/clicked_point:i";
    if(rf.check("clicked_point_port")){posName = rf.find("clicked_point_port").asString();}
    m_input_port.useCallback(*this);


    // ------------ Open ports ------------ //
    if(!m_input_port.open(m_input_port_name))
    {
        yCError(APPROACH_OBJECT) << "Cannot open port" << m_input_port_name; 
        return false;
    }
    else
        yCInfo(APPROACH_OBJECT) << "opened port" << m_input_port_name;


    return true;
}


/****************************************************************/
bool ApproachObject::close()
{
    m_executor->externalStop();
    delete m_executor;
    m_executor =NULL;
    
    if (!m_input_port.isClosed())
        m_input_port.close(); 

    return true;
}


/****************************************************************/
double ApproachObject::getPeriod()
{
    return m_period;
}


/****************************************************************/
bool ApproachObject::updateModule()
{
    if (isStopping())
    {
        if (m_executor) m_executor->externalStop();
        return false;
    }
    return true;
}


/****************************************************************/
void ApproachObject::onRead(Bottle& b) 
{
    yCInfo(APPROACH_OBJECT,"Received:  %s",b.toString().c_str());
    if(b.size() == 2)
    {
        m_executor->exec(b);
    }
    else if(b.size() == 1)
    {
        if (b.get(0).asString() == "stop")
            m_executor->externalStop();
    }
}


