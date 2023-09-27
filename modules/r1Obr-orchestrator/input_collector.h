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
#ifndef INPUT_COLLECTOR_H
#define INPUT_COLLECTOR_H

#include <yarp/os/all.h>

using namespace yarp::os;
using namespace std;

class InputCollector : public PeriodicThread
{
private:
    
    BufferedPort<Bottle> m_voiceCommandPort;
    BufferedPort<Bottle> m_inputCommandPort;
           
    BufferedPort<Bottle> m_outputPort;

    ResourceFinder    &m_rf;

public:
    
    InputCollector(double _period, yarp::os::ResourceFinder &rf);
    ~InputCollector() = default;

    virtual bool threadInit() override;
    virtual void threadRelease() override;
    virtual void run() override;

};

#endif