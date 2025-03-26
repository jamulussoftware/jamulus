/******************************************************************************\
 * Copyright (c) 2020-2025
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

#pragma once

#include <QObject>

#include "jamrecorder.h"

namespace recorder
{

class CJamController : public QObject
{
    Q_OBJECT
public:
    explicit CJamController ( CServer* pNServer );

    bool           GetRecorderInitialised() { return bRecorderInitialised; }
    QString        GetRecorderErrMsg() { return strRecorderErrMsg; }
    bool           GetRecordingEnabled() { return bEnableRecording; }
    void           RequestNewRecording();
    void           SetEnableRecording ( bool bNewEnableRecording, bool isRunning );
    QString        GetRecordingDir() { return strRecordingDir; }
    void           SetRecordingDir ( QString newRecordingDir, int iServerFrameSizeSamples, bool bDisableRecording );
    ERecorderState GetRecorderState();

private:
    void OnRecordingFailed ( QString error );

    CServer* pServer;

    bool     bRecorderInitialised;
    bool     bEnableRecording;
    QString  strRecordingDir;
    QThread* pthJamRecorder;

    CJamRecorder* pJamRecorder;
    QString       strRecorderErrMsg;

signals:
    void RestartRecorder();
    void StopRecorder();
    void RecordingSessionStarted ( QString sessionDir );
    void EndRecorderThread();
    void Stopped();
    void ClientDisconnected ( int iChID );
    void AudioFrame ( const int              iChID,
                      const QString          stChName,
                      const CHostAddress     RecHostAddr,
                      const int              iNumAudChan,
                      const CVector<int16_t> vecsData );
};

} // namespace recorder

// This must be included AFTER the above definition of class CJamController,
// because server.h defines CServer, which contains an instance of CJamController,
// and therefore needs to know the its size.
#include "../server.h"

Q_DECLARE_METATYPE ( int16_t )
