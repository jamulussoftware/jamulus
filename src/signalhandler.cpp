/******************************************************************************\
 * Copyright (c) 2020-2023
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

#include "signalhandler.h"

class CSignalHandlerSingleton : public CSignalHandler
{
public:
    inline CSignalHandlerSingleton() : CSignalHandler() {}
};
Q_GLOBAL_STATIC ( CSignalHandlerSingleton, singleton )

CSignalHandler::CSignalHandler() : pSignalBase ( CSignalBase::withSignalHandler ( this ) ) {}

CSignalHandler::~CSignalHandler() = default;

CSignalHandler* CSignalHandler::getSingletonP() { return singleton; }

bool CSignalHandler::emitSignal ( int sigNum )
{
    return QMetaObject::invokeMethod ( singleton, "HandledSignal", Qt::QueuedConnection, Q_ARG ( int, sigNum ) );
}

#ifndef _WIN32
void CSignalHandler::OnSocketNotify ( int socket )
{
    int sigNum;
    // The following read() triggers a Codacy/flawfinder finding:
    // "Check buffer boundaries if used in a loop including recursive loops (CWE-120, CWE-20)"
    // Investigation has shown that this is a false positive as the proper buffer size is enforced.
    if ( ::read ( socket, &sigNum, sizeof ( int ) ) == sizeof ( int ) ) // Flawfinder: ignore
    {
        emitSignal ( sigNum );
    }
}
#endif

// ----------------------------------------------------------

CSignalBase::CSignalBase ( CSignalHandler* pSignalHandler ) : pSignalHandler ( pSignalHandler ) {}

CSignalBase::~CSignalBase() = default;

CSignalBase* CSignalBase::withSignalHandler ( CSignalHandler* pSignalHandler )
{
#ifdef _WIN32
    return new CSignalWin ( pSignalHandler );
#else
    return new CSignalUnix ( pSignalHandler );
#endif
}

#ifdef _WIN32
CSignalWin::CSignalWin ( CSignalHandler* nPSignalHandler ) : CSignalBase ( nPSignalHandler ), lock ( QReadWriteLock::Recursive )
{
    SetConsoleCtrlHandler ( signalHandler, true );
}

CSignalWin::~CSignalWin() { SetConsoleCtrlHandler ( signalHandler, false ); }

QReadWriteLock* CSignalWin::getLock() const { return &lock; }

BOOL WINAPI CSignalWin::signalHandler ( _In_ DWORD )
{
    auto        self = getSelf<CSignalWin>();
    QReadLocker lock ( self->getLock() );
    return self->pSignalHandler->emitSignal ( -1 );
}

#else
int CSignalUnix::socketPair[2];

CSignalUnix::CSignalUnix ( CSignalHandler* nPSignalHandler ) : CSignalBase ( nPSignalHandler )
{
    if ( ::socketpair ( AF_UNIX, SOCK_STREAM, 0, socketPair ) == 0 )
    {
        socketNotifier = new QSocketNotifier ( socketPair[1], QSocketNotifier::Read );

        QObject::connect ( socketNotifier, &QSocketNotifier::activated, nPSignalHandler, &CSignalHandler::OnSocketNotify );

        socketNotifier->setEnabled ( true );

        setSignalHandled ( SIGUSR1, true );
        setSignalHandled ( SIGUSR2, true );
        setSignalHandled ( SIGINT, true );
        setSignalHandled ( SIGTERM, true );
    }
}

CSignalUnix::~CSignalUnix()
{
    setSignalHandled ( SIGUSR1, false );
    setSignalHandled ( SIGUSR2, false );
    setSignalHandled ( SIGINT, false );
    setSignalHandled ( SIGTERM, false );
}

QReadWriteLock* CSignalUnix::getLock() const { return nullptr; }

bool CSignalUnix::setSignalHandled ( int sigNum, bool state )
{
    struct sigaction sa;
    sigemptyset ( &sa.sa_mask );

    if ( state )
    {
        sa.sa_handler = CSignalUnix::signalHandler;
        sa.sa_flags = SA_RESTART;
    }
    else
    {
        sa.sa_handler = SIG_DFL;
        sa.sa_flags = 0;
    }

    return ::sigaction ( sigNum, &sa, nullptr ) == 0;
}

void CSignalUnix::signalHandler ( int sigNum )
{
    const auto res = ::write ( socketPair[0], &sigNum, sizeof ( int ) );
    Q_UNUSED ( res );
}
#endif
