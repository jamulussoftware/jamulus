/******************************************************************************\
 * Copyright (c) 2020-2025
 *
 * Author(s):
 *  Peter L Jones
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
 ******************************************************************************
 *
 * This code contains some ideas derived from QCtrlSignals
 * https://github.com/Skycoder42/QCtrlSignals.git
 * - mostly the singleton and emitSignal code, plus some of the structure
 * - virtually everything else is common knowledge across SourceForge answers
 *
 * BSD 3-Clause License
 *
 * Copyright (c) 2016, Felix Barz
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
\******************************************************************************/

#pragma once

#include <QtCore/QObject>
#include <QtCore/QScopedPointer>
#include <QtCore/QHash>
#include <QtCore/QReadWriteLock>
#include <QtCore/QSet>
#include <QGlobalStatic>

#ifdef _WIN32
#    include <qt_windows.h>
#    include <QtCore/QCoreApplication>
#    include <QtCore/QSemaphore>
#    include <QtCore/QThread>
#    include <QtCore/QDebug>
#else
#    include <QSocketNotifier>
#    include <QObject>
#    include <QCoreApplication>
#    include <csignal>
#    include <signal.h>
#    include <unistd.h>
#    include <sys/socket.h>
#endif

class CSignalBase;

class CSignalHandler : public QObject
{
    Q_OBJECT

    friend class CSignalBase;
    friend class CSignalHandlerSingleton;

public:
    static CSignalHandler* getSingletonP();

    bool emitSignal ( int );

#ifndef _WIN32
public slots:
    void OnSocketNotify ( int socket );
#endif

signals:
    void HandledSignal ( int sigNum );

private:
    QScopedPointer<CSignalBase> pSignalBase;

    explicit CSignalHandler();
    ~CSignalHandler() override;
};

// ----------------------------------------------------------

class CSignalBase
{
    Q_DISABLE_COPY ( CSignalBase )

public:
    static CSignalBase* withSignalHandler ( CSignalHandler* );
    virtual ~CSignalBase();

    virtual QReadWriteLock* getLock() const = 0;

    QSet<int> sHandledSigNums;

protected:
    CSignalBase ( CSignalHandler* );

    CSignalHandler* pSignalHandler;

    template<typename T>
    static T* getSelf()
    {
        return static_cast<T*> ( CSignalHandler::getSingletonP()->pSignalBase.data() );
    }
};

#ifdef _WIN32

class CSignalWin : public CSignalBase
{
public:
    CSignalWin ( CSignalHandler* );
    ~CSignalWin() override;

    virtual QReadWriteLock* getLock() const override;

private:
    mutable QReadWriteLock lock;

    static BOOL WINAPI signalHandler ( _In_ DWORD );
};

#else

class CSignalUnix : public CSignalBase
{
public:
    CSignalUnix ( CSignalHandler* );
    ~CSignalUnix() override;

    virtual QReadWriteLock* getLock() const override;

private:
    QSocketNotifier* socketNotifier = nullptr;
    bool             setSignalHandled ( int sigNum, bool state );

    static int  socketPair[2];
    static void signalHandler ( int sigNum );
};

#endif
