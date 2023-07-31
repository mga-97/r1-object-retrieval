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

#include "goAndFindIt.h"


YARP_LOG_COMPONENT(GO_AND_FIND_IT, "r1_obr.goAndFindIt")


/****************************************************************/
GoAndFindIt::GoAndFindIt() : m_period(1.0) 
{
    m_input_port_name       = "/goAndFindIt/input:i";
}


/****************************************************************/
bool GoAndFindIt::configure(yarp::os::ResourceFinder &rf)
{
    //Generic config
    if(rf.check("period"))
        m_period = rf.find("period").asFloat32();

    //Open ports
    if(rf.check("input_port"))
        m_input_port_name = rf.find("input_port").asString();
    m_input_port.useCallback(*this);
    if(!m_input_port.open(m_input_port_name))
        yCError(GO_AND_FIND_IT) << "Cannot open port" << m_input_port_name; 
    else
        yCInfo(GO_AND_FIND_IT) << "opened port" << m_input_port_name;

    //Initialize Thread
    m_thread = new GoAndFindItThread(rf);
    bool threadOk = m_thread->start();
    if (!threadOk){
        return false;
    }

    return true;
}


/****************************************************************/
bool GoAndFindIt::close()
{
    if (!m_input_port.isClosed())
        m_input_port.close();    
    
    m_thread->stop();
    delete m_thread;

    return true;
}

/****************************************************************/
double GoAndFindIt::getPeriod()
{
    return m_period;
}

/****************************************************************/
bool GoAndFindIt::updateModule()
{
    return true;
}



/****************************************************************/
void GoAndFindIt::onRead(Bottle& b) 
{
    
    yCInfo(GO_AND_FIND_IT,"Received:  %s",b.toString().c_str());

    if(b.size() == 2)           //expected "<object> <location>"
    {
        string what = b.get(0).asString();
        string where = b.get(1).asString();

        m_thread->setWhatWhere(what,where);

    }
    else if(b.size() == 1)      //expected "<object>" or "stop"
    {
        string what = b.get(0).asString();

        if (what == "stop")
        {
            m_thread->stopSearch();            
        }
        else if (what == "reset")
        {
            m_thread->resetSearch();            
        }
        else
        {
        // ////////////////////////////// TO ADD: ask PAVIS if it sees "what"   //////////////////////////////

           m_thread->setWhat(what);
        }
         
    }
    else
    {
        yCError(GO_AND_FIND_IT,"Error: wrong bottle format received");
        return;
    }
    
}

