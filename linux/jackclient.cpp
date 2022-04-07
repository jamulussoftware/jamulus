/******************************************************************************\
 * Copyright (c) 2022
 *
 * Author(s):
 *  Peter Goderie (pgScorpio)
 *
 ******************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 *
\******************************************************************************/

#include "jackclient.h"

//================================================
// CJackClientPort class: a basic Jack client port
//================================================

bool CJackClientPort::Connect ( const char* connPort )
{
    Disconnect();
    int res;
    // First portname must be the output, second portname must be the input
    if ( bIsInput )
    {
        // I am input, so connPort must be output
        res = jack_connect ( jackClient, connPort, jack_port_name ( jackPort ) /*jack_port_name ( jackPort )*/ );
    }
    else
    {
        // I am output, so connPort must be input
        res = jack_connect ( jackClient, jack_port_name ( jackPort ), connPort );
    }

    bool ok = ( res == 0 ) || ( res == EEXIST );

    if ( ok )
    {
        connName = connPort;
    }

    return ( connName.size() > 0 );
}

bool CJackClientPort::Disconnect()
{
    if ( connName.size() )
    {
        int res;

        // First portname must be the output, second portname must be the input
        if ( bIsInput )
        {
            // I am input, so connName must be output
            res = jack_disconnect ( jackClient, connName.toLocal8Bit().data(), jack_port_name ( jackPort ) );
        }
        else
        {
            // I am output, so connName must be input
            res = jack_disconnect ( jackClient, jack_port_name ( jackPort ), connName.toLocal8Bit().data() );
        }

        bool ok = ( res == 0 );

        // on failure assume we where not connected to connName at all
        connName.clear();

        return ok;
    }

    return ( connName.size() == 0 );
}

const QString& CJackClientPort::GetConnections()
{
    connName.clear();

    const char** connections = jack_port_get_connections ( jackPort );
    if ( connections )
    {
        const char** connlist = connections;
        while ( connlist && *connlist )
        {

            if ( connName.size() )
            {
                connName += "+";
                connName += *connlist;
            }
            else
            {
                connName = *connlist;
            }

            connlist++;
        }

        jack_free ( connections );
    }

    return connName;
}

//================================================
// CJackAudioPort class:
//================================================

bool CJackAudioPort::getLatencyRange ( jack_latency_range_t& latrange )
{
    latrange.min = 0;
    latrange.max = 0;

    if ( jackPort == NULL )
    {
        return false;
    }

    jack_port_get_latency_range ( jackPort, bIsInput ? JackCaptureLatency : JackPlaybackLatency, &latrange );

    return true;
}

bool CJackAudioPort::setLatencyRange ( jack_latency_range_t& latrange )
{
    if ( jackPort == NULL )
    {
        return false;
    }

    jack_port_set_latency_range ( jackPort, bIsInput ? JackCaptureLatency : JackPlaybackLatency, &latrange );

    return true;
}

//================================================
// CJackMidiPort class:
//================================================

//================================================
// CJackClient class: The Jack client and it's ports
//================================================

CJackClient::CJackClient ( QString aClientName ) :
    strServerName ( getenv ( "JACK_DEFAULT_SERVER" ) ),
    strClientName ( aClientName ),
    strJackClientName ( APP_NAME ),
    jackClient ( NULL ),
    openOptions ( JackUseExactName ),
    openStatus ( JackNoSuchClient ),
    bIsActive ( false ),
    AudioInput(),
    AudioOutput(),
    MidiInput(),
    MidiOutput()
{
    if ( strServerName.isEmpty() )
    {
        strServerName = "default";
    }

    qInfo() << qUtf8Printable (
        QString ( "Connecting to JACK \"%1\" instance (use the JACK_DEFAULT_SERVER environment variable to change this)." ).arg ( strServerName ) );

    if ( !strClientName.isEmpty() )
    {
        strJackClientName += " ";
        strJackClientName += strClientName;
    }
}

bool CJackClient::Open()
{
    if ( jackClient == NULL )
    {
        // we shouldn't have any ports !
        MidiInput.clear();
        MidiOutput.clear();
        AudioInput.clear();
        AudioOutput.clear();
        // and we can't be active !
        bIsActive = false;

        jackClient = jack_client_open ( strJackClientName.toLocal8Bit().data(), openOptions, &openStatus );
    }

    return ( ( jackClient != NULL ) && ( openStatus == 0 ) );
}

bool CJackClient::Close()
{
    if ( bIsActive )
    {
        Deactivate();
    }

    // close client
    if ( jackClient != NULL )
    {
        // unregister and delete any ports:
        while ( MidiInput.size() )
        {
            MidiInput.last().Unregister();
            MidiInput.removeLast();
        }

        while ( MidiOutput.size() )
        {
            MidiOutput.last().Unregister();
            MidiOutput.removeLast();
        }

        while ( AudioInput.size() )
        {
            AudioInput.last().Unregister();
            AudioInput.removeLast();
        }

        while ( AudioOutput.size() )
        {
            AudioOutput.last().Unregister();
            AudioOutput.removeLast();
        }

        bool res = ( jack_client_close ( jackClient ) == 0 );

        jackClient = NULL;

        return res;
    }
    else
    {
        // we shouldn't have any ports !
        MidiInput.clear();
        MidiOutput.clear();
        AudioInput.clear();
        AudioOutput.clear();
        // and we can't be active !
        bIsActive = false;
    }

    return true;
}

bool CJackClient::AddAudioPort ( const QString& portName, bool bInput )
{
    const char*  thePortname = portName.toLocal8Bit().data();
    jack_port_t* newPort     = NULL;

    newPort = jack_port_register ( jackClient, thePortname, JACK_DEFAULT_AUDIO_TYPE, bInput ? JackPortIsInput : JackPortIsOutput, 0 );

    if ( newPort )
    {
        const char*    jackName = jack_port_name ( newPort );
        CJackAudioPort newAudioPort ( CJackAudioPort ( jackClient, newPort, portName, bInput ) );

        if ( bInput )
        {
            if ( jackName )
                AudioInput.append ( newAudioPort );
        }
        else
        {
            if ( jackName )
                AudioOutput.append ( newAudioPort );
        }

        return true;
    }

    return false;
}

bool CJackClient::AddMidiPort ( const QString& portName, bool bInput )
{
    jack_port_t* newPort =
        jack_port_register ( jackClient, portName.toLocal8Bit().data(), JACK_DEFAULT_MIDI_TYPE, bInput ? JackPortIsInput : JackPortIsOutput, 0 );

    if ( newPort )
    {
        CJackMidiPort newMidiPort ( jackClient, newPort, portName, bInput );

        if ( bInput )
        {
            MidiInput.append ( newMidiPort );
        }
        else
        {
            MidiOutput.append ( newMidiPort );
        }

        return true;
    }

    return false;
}

bool CJackClient::AutoConnect()
{
    // connect the audio ports, note: you cannot do this before
    // the client is activated, because we cannot allow
    // connections to be made to clients that are not
    // running

    if ( ( jackClient == NULL ) || !bIsActive )
    {
        return false;
    }

    unsigned int numInConnected  = 0;
    unsigned int numOutConnected = 0;
    bool         ok              = true;
    const char** ports;

    // try to connect physical audio input ports
    ports = GetJackPorts ( nullptr, nullptr, JackPortIsPhysical | JackPortIsOutput );
    if ( ports != nullptr )
    {
        int i = 0;
        while ( ports[i] && ( i < AudioInput.size() ) )
        {
            if ( AudioInput[i].Connect ( ports[i] ) )
            {
                numInConnected++;
            }
            else
            {
                ok = false;
            }
            i++;
        }

        jack_free ( ports );
    }

    // try to connect physical audio output ports
    ports = GetJackPorts ( nullptr, nullptr, JackPortIsPhysical | JackPortIsInput );
    if ( ports != nullptr )
    {
        int i = 0;
        while ( ports[i] && ( i < AudioOutput.size() ) )
        {
            if ( AudioOutput[i].Connect ( ports[i] ) )
            {
                numOutConnected++;
            }
            else
            {
                ok = false;
            }
            i++;
        }

        jack_free ( ports );
    }

    return ok && ( numInConnected >= DRV_MIN_IN_CHANNELS ) && ( numOutConnected >= DRV_MIN_OUT_CHANNELS );
}

unsigned int CJackClient::GetAudioInputConnections()
{
    unsigned int count = 0;
    for ( int i = 0; i < AudioInput.size(); i++ )
    {
        if ( !AudioInput[i].GetConnections().isEmpty() )
        {
            count++;
        }
    }

    return count;
}

unsigned int CJackClient::GetAudioOutputConnections()
{
    unsigned int count = 0;
    for ( int i = 0; i < AudioOutput.size(); i++ )
    {
        if ( !AudioOutput[i].GetConnections().isEmpty() )
        {
            count++;
        }
    }

    return count;
}

bool CJackClient::Activate()
{
    if ( !bIsActive )
    {
        bIsActive = jackClient ? ( jack_activate ( jackClient ) == 0 ) : false;
    }

    return bIsActive;
}

bool CJackClient::Deactivate() // Also disconnects ports !
{
    if ( !jackClient )
    {
        bIsActive = false;
    }

    if ( bIsActive && ( jack_deactivate ( jackClient ) == 0 ) )
    {
        bIsActive = false;
    }

    return !bIsActive;
}
