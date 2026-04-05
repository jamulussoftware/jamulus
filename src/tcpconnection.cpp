/******************************************************************************\
 * Copyright (c) 2024-2026
 *
 * Author(s):
 *  Tony Mountifield
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

#include "protocol.h"
#include "server.h"
#include "channel.h"

CTcpConnection::CTcpConnection ( QTcpSocket* pTcpSocket, const CHostAddress& tcpAddress, CServer* pServer, CChannel* pChannel ) :
    pTcpSocket ( pTcpSocket ),
    tcpAddress ( tcpAddress ),
    pServer ( pServer ),
    pChannel ( pChannel )
{
    vecbyRecBuf.Init ( MAX_SIZE_BYTES_NETW_BUF );
    iPos           = 0;
    iPayloadRemain = 0;

    connect ( pTcpSocket, &QTcpSocket::disconnected, this, &CTcpConnection::OnDisconnected );
    connect ( pTcpSocket, &QTcpSocket::readyRead, this, &CTcpConnection::OnReadyRead );

    if ( pServer )
    {
        connect ( this, &CTcpConnection::ProtocolCLMessageReceived, pServer, &CServer::OnProtocolCLMessageReceived );
    }

    if ( pChannel )
    {
        connect ( this, &CTcpConnection::ProtocolCLMessageReceived, pChannel, &CChannel::OnProtocolCLMessageReceived );
    }
}

void CTcpConnection::OnDisconnected()
{
    qDebug() << "- Jamulus-TCP: disconnected from:" << tcpAddress.toString();
    pTcpSocket->deleteLater();
    deleteLater(); // delete this object in the next event loop
}

void CTcpConnection::OnReadyRead()
{
    long iBytesAvail = pTcpSocket->bytesAvailable();

    qDebug() << "- readyRead(), bytesAvailable() =" << iBytesAvail;

    while ( iBytesAvail > 0 )
    {
        if ( iPos < MESS_HEADER_LENGTH_BYTE )
        {
            // reading message header
            long iNumBytesRead = pTcpSocket->read ( (char*) &vecbyRecBuf[iPos], MESS_HEADER_LENGTH_BYTE - iPos );
            if ( iNumBytesRead == -1 )
            {
                return;
            }

            qDebug() << "-- (hdr) iNumBytesRead =" << iNumBytesRead;

            iPos += iNumBytesRead;
            iBytesAvail -= iNumBytesRead;

            if ( iPos >= MESS_HEADER_LENGTH_BYTE )
            {
                // now have a complete header
                iPayloadRemain = CProtocol::GetBodyLength ( vecbyRecBuf );

                Q_ASSERT ( iPayloadRemain <= MAX_SIZE_BYTES_NETW_BUF - MESS_HEADER_LENGTH_BYTE );

                iPayloadRemain -= iPos - MESS_HEADER_LENGTH_BYTE;
            }
        }
        else
        {
            // reading message body
            long iNumBytesRead = pTcpSocket->read ( (char*) &vecbyRecBuf[iPos], iPayloadRemain );
            if ( iNumBytesRead == -1 )
            {
                return;
            }

            qDebug() << "-- (body) iNumBytesRead =" << iNumBytesRead;

            iPos += iNumBytesRead;
            iPayloadRemain -= iNumBytesRead;
            iBytesAvail -= iNumBytesRead;

            Q_ASSERT ( iPayloadRemain >= 0 );

            if ( iPayloadRemain == 0 )
            {
                // have a complete payload
                qDebug() << "- Jamulus-TCP: received protocol message of length" << iPos;

                // check if this is a protocol message
                int              iRecCounter;
                int              iRecID;
                CVector<uint8_t> vecbyMesBodyData;

                if ( !CProtocol::ParseMessageFrame ( vecbyRecBuf, iPos, vecbyMesBodyData, iRecCounter, iRecID ) )
                {
                    qDebug() << "- Jamulus-TCP: message parsed OK, ID =" << iRecID;

                    // this is a protocol message, check the type of the message
                    if ( CProtocol::IsConnectionLessMessageID ( iRecID ) )
                    {
                        //### TODO: BEGIN ###//
                        // a copy of the vector is used -> avoid malloc in real-time routine
                        emit ProtocolCLMessageReceived ( iRecID, vecbyMesBodyData, tcpAddress, this );
                        //### TODO: END ###//

                        // disconnect if we are a client
                        if ( pChannel )
                        {
                            pTcpSocket->disconnectFromHost();
                        }
                    }
                    else
                    {
                        //### TODO: BEGIN ###//
                        // a copy of the vector is used -> avoid malloc in real-time routine
                        // emit ProtocolMessageReceived ( iRecCounter, iRecID, vecbyMesBodyData, pTcpConnection->tcpAddress, pTcpConnection );
                        //### TODO: END ###//
                    }
                }
                else
                {
                    qDebug() << "- Jamulus-TCP: failed to parse frame";
                }

                iPos = 0; // ready for next message, if any
            }
        }
    }

    qDebug() << "- end of readyRead(), bytesAvailable() =" << pTcpSocket->bytesAvailable();
}

qint64 CTcpConnection::write ( const char* data, qint64 maxSize )
{
    if ( !pTcpSocket )
    {
        return -1;
    }

    return pTcpSocket->write ( data, maxSize );
}

void CTcpConnection::disconnectFromHost()
{
    if ( pTcpSocket )
    {
        pTcpSocket->disconnectFromHost();
    }
}
