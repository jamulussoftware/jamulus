/******************************************************************************\
 * Copyright (c) 2020
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
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
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
#include <qt_windows.h>
#include <QtCore/QCoreApplication>
#include <QtCore/QSemaphore>
#include <QtCore/QThread>
#include <QtCore/QDebug>
#else
#include <QSocketNotifier>
#include <QObject>
#include <QCoreApplication>
#include <csignal>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
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
    void ShutdownSignal ( int sigNum );

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

    template <typename T>
    static T *getSelf()
    {
        return static_cast<T*>( CSignalHandler::getSingletonP()->pSignalBase.data() );
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

    static BOOL WINAPI signalHandler ( _In_ DWORD sigNum );
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
    bool setSignalHandled ( int sigNum, bool state );

    static int socketPair[2];
    static void signalHandler ( int sigNum );
};

#endif
