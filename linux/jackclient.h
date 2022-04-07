#pragma once
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

#include <QString>
#include <QVector>
#include "util.h"

#include <jack/jack.h>
#include <jack/midiport.h>

//============================================================================
// JackClient class: Jack client API implementation for Jamulus
//============================================================================

class CJackClient;

//================================================
// CJackClientPort class: a basic Jack client port
//================================================
class CJackClientPort
{
protected:
    friend class CJackAudioPort;
    friend class CJackMidiPort;

    CJackClientPort ( jack_client_t* theJackClient, jack_port_t* theJackPort, const QString& thePortName, bool bInput ) :
        jackClient ( theJackClient ),
        jackPort ( theJackPort ),
        portName ( thePortName ),
        connName ( "" ),
        bIsInput ( bInput )
    {}

    void Unregister()
    {
        if ( jackClient && jackPort )
        {
            jack_port_unregister ( jackClient, jackPort );
        }
    }

public:
public:
    jack_client_t* jackClient;
    jack_port_t*   jackPort;
    QString        portName;
    QString        connName;
    bool           bIsInput;

public:
    inline bool isOpen() { return ( jackClient && jackPort ); }
    inline bool isConnected() { return ( connName.size() > 0 ); }

    inline QString GetJackPortName() { return jackPort ? jack_port_name ( jackPort ) : ""; }

public:
    inline size_t GetBufferSize ( const char* portType ) { return jack_port_type_get_buffer_size ( jackClient, portType ); }
    inline void*  GetBuffer ( jack_nframes_t nframes ) const { return jack_port_get_buffer ( jackPort, nframes ); }

public:
    bool           Connect ( const char* connPort );
    bool           Disconnect();
    const QString& GetConnections();
};

//================================================
// CJackAudioPort class:
//================================================
class CJackAudioPort : public CJackClientPort
{
protected:
    friend class CJackClient;
    friend class QVector<CJackAudioPort>;

    CJackAudioPort() : CJackClientPort ( NULL, NULL, "", false ) {}

    CJackAudioPort ( jack_client_t* theJackClient, jack_port_t* theJackPort, const QString& thePortName, bool bInput ) :
        CJackClientPort ( theJackClient, theJackPort, thePortName, bInput )
    {}

public:
    bool getLatencyRange ( jack_latency_range_t& latrange );
    bool setLatencyRange ( jack_latency_range_t& latrange );
};

//================================================
// CJackMidiPort class:
//================================================

class CJackMidiPort : public CJackClientPort
{
protected:
    friend class CJackClient;
    friend class QVector<CJackMidiPort>;

    CJackMidiPort() : CJackClientPort ( NULL, NULL, "", false ) {}

    CJackMidiPort ( jack_client_t* theJackClient, jack_port_t* theJackPort, const QString& thePortName, bool bInput ) :
        CJackClientPort ( theJackClient, theJackPort, thePortName, bInput )
    {}

public:
    inline jack_nframes_t GetEventCount() const { return jack_midi_get_event_count ( jackPort ); }
    inline bool           GetEvent ( jack_midi_event_t* event, void* buffer, uint32_t eventIndex ) const
    {
        return ( jack_midi_event_get ( event, buffer, eventIndex ) == 0 );
    }
};

//================================================
// CJackClient class: The Jack client and it's ports
//================================================

class CJackClient
{
protected:
    friend class CJackClientPort;
    friend class CJackAudioPort;
    friend class CJackMidiPort;

    QString strServerName;
    QString strClientName;
    QString strJackClientName;

    jack_client_t* jackClient;
    jack_options_t openOptions;
    jack_status_t  openStatus;
    bool           bIsActive;

public:
    // Jack ports
    QVector<CJackAudioPort> AudioInput;
    QVector<CJackAudioPort> AudioOutput;
    QVector<CJackMidiPort>  MidiInput;
    QVector<CJackMidiPort>  MidiOutput;

public:
    CJackClient ( QString aClientName );
    virtual ~CJackClient() { Close(); }

public:
    bool Open();
    bool Close();

    inline bool IsOpen() const { return ( jackClient != NULL ); }

    // temporarily for debugging
    jack_client_t* GetJackClient() { return jackClient; }

public:
    bool AddAudioPort ( const QString& portName, bool bInput );
    bool AddMidiPort ( const QString& portName, bool bInput );

public:
    inline bool SetProcessCallBack ( JackProcessCallback cbProcess, void* arg )
    {
        return jackClient ? ( jack_set_process_callback ( jackClient, cbProcess, arg ) == 0 ) : false;
    }

    inline bool SetShutDownCallBack ( JackShutdownCallback cbShutdown, void* arg )
    {
        if ( jackClient )
        {
            jack_on_shutdown ( jackClient, cbShutdown, arg );
            return true;
        }

        return false;
    }

    inline bool SetBuffersizeCallBack ( JackProcessCallback cbBuffersize, void* arg )
    {
        return jackClient ? ( jack_set_buffer_size_callback ( jackClient, cbBuffersize, arg ) == 0 ) : false;
    }

public:
    inline bool SetBufferSize ( jack_nframes_t nFrames ) const { return jackClient ? ( jack_set_buffer_size ( jackClient, nFrames ) == 0 ) : false; }
    inline jack_nframes_t GetBufferSize() const { return jackClient ? jack_get_buffer_size ( jackClient ) : 0; }
    inline jack_nframes_t GetSamplerate() const { return jackClient ? jack_get_sample_rate ( jackClient ) : 0; }

public:
    inline const char** GetJackPorts ( const char* port_name_pattern, const char* type_name_pattern, unsigned long flags ) const
    {
        return jackClient ? jack_get_ports ( jackClient, port_name_pattern, type_name_pattern, flags ) : NULL;
    }

public:
    bool         AutoConnect();
    unsigned int GetAudioInputConnections();
    unsigned int GetAudioOutputConnections();

public:
    bool Activate();
    bool Deactivate(); // Also disconnects all ports !

    inline bool IsActive() const { return bIsActive; }
};
