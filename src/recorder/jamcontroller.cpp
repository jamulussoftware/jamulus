/******************************************************************************\
 * Copyright (c) 2020-2023
 *
 * Author(s):
 *  pljones
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

#include "jamcontroller.h"

using namespace recorder;

CJamController::CJamController ( CServer* pNServer ) :
    pServer ( pNServer ),
    bRecorderInitialised ( false ),
    bEnableRecording ( false ),
    strRecordingDir ( "" ),
    pthJamRecorder ( nullptr ),
    pJamRecorder ( nullptr )
{}

void CJamController::RequestNewRecording()
{

    if ( bRecorderInitialised && bEnableRecording )
    {
        emit RestartRecorder();
    }
}

void CJamController::SetEnableRecording ( bool bNewEnableRecording, bool isRunning )
{

    if ( bRecorderInitialised )
    {

        // message only if the state appears to change
        if ( bEnableRecording != bNewEnableRecording )
        {
            qInfo() << qUtf8Printable ( QString ( "Recording state: %1" ).arg ( bNewEnableRecording ? "enabled" : "disabled" ) );
        }

        // note that this block executes regardless of whether
        // what appears to be a change is being applied, to ensure
        // the requested state is the result
        bEnableRecording = bNewEnableRecording;

        if ( !bEnableRecording )
        {
            emit StopRecorder();
        }
        else if ( !isRunning )
        {
            // This dirty hack is for the GUI.  It doesn't care.
            emit StopRecorder();
        }
    }
}

void CJamController::SetRecordingDir ( QString newRecordingDir, int iServerFrameSizeSamples, bool bDisableRecording )
{
    if ( bRecorderInitialised && pthJamRecorder != nullptr )
    {
        // We have a thread and we want to start a new one.
        // We only want one running.
        // This could take time, unfortunately.
        // Hopefully changing recording directory will NOT happen during a long jam...
        emit EndRecorderThread();
        pthJamRecorder->wait();
        delete pthJamRecorder;
        pthJamRecorder = nullptr;
    }

    if ( !newRecordingDir.isEmpty() )
    {
        pJamRecorder         = new recorder::CJamRecorder ( newRecordingDir, iServerFrameSizeSamples );
        strRecorderErrMsg    = pJamRecorder->Init();
        bRecorderInitialised = ( strRecorderErrMsg == QString() );
        bEnableRecording     = bRecorderInitialised && !bDisableRecording;

        qInfo() << qUtf8Printable ( QString ( "Recording state: %1" ).arg ( bEnableRecording ? "enabled" : "disabled" ) );
    }
    else
    {
        // This is the only time this is ever true - UI needs to handle it
        strRecorderErrMsg    = QString();
        bRecorderInitialised = false;
        bEnableRecording     = false;

        qInfo() << "Recording state not initialised";
    }

    if ( bRecorderInitialised )
    {
        strRecordingDir = newRecordingDir;

        pthJamRecorder = new QThread();
        pthJamRecorder->setObjectName ( "JamRecorder" );

        pJamRecorder->moveToThread ( pthJamRecorder );

        // QT signals
        QObject::connect ( pthJamRecorder, &QThread::finished, pJamRecorder, &QObject::deleteLater );

        QObject::connect ( QCoreApplication::instance(),
                           &QCoreApplication::aboutToQuit,
                           pJamRecorder,
                           &CJamRecorder::OnAboutToQuit,
                           Qt::ConnectionType::BlockingQueuedConnection );

        // from the controller to the recorder
        QObject::connect ( this, &CJamController::RestartRecorder, pJamRecorder, &CJamRecorder::OnTriggerSession );

        QObject::connect ( this, &CJamController::StopRecorder, pJamRecorder, &CJamRecorder::OnEnd );

        QObject::connect ( this,
                           &CJamController::EndRecorderThread,
                           pJamRecorder,
                           &CJamRecorder::OnAboutToQuit,
                           Qt::ConnectionType::BlockingQueuedConnection );

        // from the server to the recorder
        QObject::connect ( this, &CJamController::Stopped, pJamRecorder, &CJamRecorder::OnEnd );

        QObject::connect ( this, &CJamController::ClientDisconnected, pJamRecorder, &CJamRecorder::OnDisconnected );

        qRegisterMetaType<CVector<int16_t>> ( "CVector<int16_t>" );
        QObject::connect ( this, &CJamController::AudioFrame, pJamRecorder, &CJamRecorder::OnFrame );

        // from the recorder to the server
        QObject::connect ( pJamRecorder, &CJamRecorder::RecordingSessionStarted, this, &CJamController::RecordingSessionStarted );

        // from the recorder to the controller
        QObject::connect ( pJamRecorder, &CJamRecorder::RecordingFailed, this, &CJamController::OnRecordingFailed );

        pthJamRecorder->start ( QThread::NormalPriority );
    }
    else
    {
        strRecordingDir = "";
    }
}

void CJamController::OnRecordingFailed ( QString error )
{
    if ( !bEnableRecording )
    {
        // Recording has already been stopped, possibly
        // by a previous OnRecordingFailed call from another client/thread.
        return;
    }
    strRecorderErrMsg = error;
    qWarning() << "Could not start recording:" << error;
    // Turn off recording until it is manually re-enabled via UI or signals.
    // This needs to be done from the CServer side to cover all relevant
    // state.
    pServer->SetEnableRecording ( false );
}

ERecorderState CJamController::GetRecorderState()
{
    // return recorder state
    if ( bRecorderInitialised )
    {
        if ( bEnableRecording )
        {
            return RS_RECORDING;
        }
        else
        {
            return RS_NOT_ENABLED;
        }
    }
    else
    {
        return RS_NOT_INITIALISED;
    }
}
