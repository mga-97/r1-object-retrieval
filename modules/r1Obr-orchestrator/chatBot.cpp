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

#include "chatBot.h"

YARP_LOG_COMPONENT(CHAT_BOT_ORCHESTRATOR, "r1_obr.orchestrator.chatBot")


// ****************************************************** //
bool ChatBot::configure(ResourceFinder &rf)
{
    // Defaults
    string voiceCommandPortName     = "/r1Obr-orchestrator/voice_command:i";
    string orchestratorRPCPortName  = "/r1Obr-orchestrator/chatBot:rpc";
    string audiorecorderRPCPortName = "/r1Obr-orchestrator/chatBot/microphone:rpc";
    string audioplayerStatusPortName= "/r1Obr-orchestrator/chatBot/audioplayerStatus:i";
    string device_chatBot_nwc       = "chatBot_nwc_yarp";
    string local_chatBot_nwc        = "/r1Obr-orchestrator/chatBot";
    string remote_chatBot_nwc       = "/chatBot_nws";
    string m_language_chatbot       = "it-IT";

    if(!rf.check("CHAT_BOT"))
    {
        yCWarning(CHAT_BOT_ORCHESTRATOR,"CHAT_BOT section missing in ini file. Using the default values");
    }
    
    Searchable& config = rf.findGroup("CHAT_BOT");
     

    // ------------------  out ------------------ //  
    if(config.check("rpc_orchestrator_port")) {orchestratorRPCPortName = config.find("rpc_orchestrator_port").asString();}
    if(!m_orchestratorRPCPort.open(orchestratorRPCPortName))
    {
        yCError(CHAT_BOT_ORCHESTRATOR, "Unable to open Chat Bot RPC port to orchestrator");
        return false;
    }

    if(config.check("rpc_microphone_port")) {audiorecorderRPCPortName = config.find("rpc_microphone_port").asString();}
    if(!m_audiorecorderRPCPort.open(audiorecorderRPCPortName))
    {
        yCError(CHAT_BOT_ORCHESTRATOR, "Unable to open Chat Bot RPC port to audio recorder");
        return false;
    }

    // ------------------  in  ------------------ //
    
    // Voice Command Port
    if(config.check("voice_command_port")) {voiceCommandPortName = config.find("voice_command_port").asString();}
    if (!m_voiceCommandPort.open(voiceCommandPortName))
    {
        yCError(CHAT_BOT_ORCHESTRATOR, "Unable to open voice command port");
        return false;
    }
    m_voiceCommandPort.useCallback(*this);

    // iChatBot
    m_chatBot_active = config.check("chatbot_active") ? !(config.find("chatbot_active").asString() == "false") : true;

    if(m_chatBot_active)
    {
        Property chatBotProp;
        chatBotProp.put("device", config.check("device", Value(device_chatBot_nwc)));
        chatBotProp.put("local",  config.check("local", Value(local_chatBot_nwc)));
        chatBotProp.put("remote", config.check("remote", Value(remote_chatBot_nwc)));
        m_PolyCB.open(chatBotProp);
        if(!m_PolyCB.isValid())
        {
            yCWarning(CHAT_BOT_ORCHESTRATOR,"Error opening PolyDriver check parameters. Using the default values");
        }
        m_PolyCB.view(m_iChatBot);
        if(!m_iChatBot){
            yCError(CHAT_BOT_ORCHESTRATOR,"Error opening IChatBot interface. Device not available");
            return false;
        }
        
        m_iChatBot->resetBot();
        string msgIn,msgOut;
        m_iChatBot->interact(msgIn = "skip_language_set", msgOut);
        yCInfo(CHAT_BOT_ORCHESTRATOR,"ChatBot: wrote: %s . replied: %s",msgIn.c_str(),msgOut.c_str());
        
        if(config.check("language")) 
            m_language_chatbot = config.find("language").asString();
        m_iChatBot->setLanguage(m_language_chatbot);
        

    }


    //audio player status
    if(config.check("audioplayer_input_port")) {audioplayerStatusPortName = config.find("audioplayer_input_port").asString();}
    if(!m_audioPlayPort.open(audioplayerStatusPortName))
    {
        yCError(CHAT_BOT_ORCHESTRATOR, "Unable to open audio player status port");
        return false;
    }


    // --------- SpeechSynthesizer config --------- //
    m_speaker = new SpeechSynthesizer();
    if(!m_speaker->configure(rf, ""))
    {
        yCError(CHAT_BOT_ORCHESTRATOR,"SpeechSynthesizer configuration failed");
        return false;
    }

    return true;
}

    
// ****************************************************** //
void ChatBot::close()
{    
    if(!m_voiceCommandPort.isClosed())
        m_voiceCommandPort.close();

    if (m_orchestratorRPCPort.asPort().isOpen())
        m_orchestratorRPCPort.close(); 

    if (m_audiorecorderRPCPort.asPort().isOpen())
        m_audiorecorderRPCPort.close();
    
    if (!m_audioPlayPort.isClosed())
        m_audioPlayPort.close();
    
    if(m_PolyCB.isValid())
        m_PolyCB.close();

    m_speaker->close();
    delete m_speaker;
}


// ****************************************************** //
void ChatBot::onRead(Bottle& b)
{
    if(!m_chatBot_active)
    {
        yCWarning(CHAT_BOT_ORCHESTRATOR, "Chat Bot not active. Use RPC port to write commands");
        return;
    }

    string str = b.get(0).asString();

    if(str == "")
        return;

    interactWithChatBot(str);
    
}


// ****************************************************** //
void ChatBot::interactWithChatBot(const string& msgIn)
{
    if(m_chatBot_active)
    {
        yCInfo(CHAT_BOT_ORCHESTRATOR,"ChatBot received: %s",msgIn.c_str());
        
        string msgOut;
        m_iChatBot->interact(msgIn, msgOut); //msgOut should be something like "(say <....>) (<other command 1>) (<other command 2>) ..."
        yCInfo(CHAT_BOT_ORCHESTRATOR,"ChatBot replied: %s",msgOut.c_str());

        Bottle msg_btl; msg_btl.fromString(msgOut);

        for (int i=0; i<(int)msg_btl.size(); i++)
        {     
            Bottle* cmd=msg_btl.get(i).asList();
            
            if(cmd->get(0).asString()=="dialogReset")
            {
                m_iChatBot->resetBot();
                m_iChatBot->setLanguage(m_language_chatbot);
                Bottle req{"reset_home"}, rep;
                m_orchestratorRPCPort.write(req,rep);
                yCInfo(CHAT_BOT_ORCHESTRATOR, "Replied from orchestrator: %s", rep.toString().c_str() );
            }
            else if(cmd->get(0).asString()=="say")
            {
                string toSay = cmd->tail().toString();
                yCInfo(CHAT_BOT_ORCHESTRATOR, "Saying: %s", toSay.c_str());

                //close microphone
                Bottle req{"stopRecording_RPC"}, rep;
                m_audiorecorderRPCPort.write(req,rep);

                //speak
                m_speaker->say(toSay);

                //wait until finish speaking
                Time::delay(0.5);
                bool audio_is_playing{true};
                while (audio_is_playing) 
                {
                    Bottle* player_status = m_audioPlayPort.read(false);
                    if (player_status)
                    {
                        audio_is_playing = (unsigned int)player_status->get(1).asInt64() > 0;
                    }
                    Time::delay(0.1);                    
                }

                //re-open microphone
                req.clear(); rep.clear();
                req.addString("startRecording_RPC");
                m_audiorecorderRPCPort.write(req,rep);
            }
            else if(cmd->get(0).asString()=="setLanguage")
            {
                string lang = cmd->get(1).asString();
                m_iChatBot->setLanguage(lang);
                m_speaker->setLanguage(lang);
                m_language_chatbot=lang;
                yCInfo(CHAT_BOT_ORCHESTRATOR, "Language set to: %s", lang.c_str());
            }
            else
            {
                Bottle reply;
                m_orchestratorRPCPort.write(*cmd,reply);
                yCInfo(CHAT_BOT_ORCHESTRATOR, "Replied from orchestrator: %s", reply.toString().c_str() );
            }
        }
    }
    else
        yCWarning(CHAT_BOT_ORCHESTRATOR, "Chat Bot not active. Use RPC port to write commands");
    
}
