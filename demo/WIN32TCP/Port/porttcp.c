/*
 * FreeModbus Libary: Win32 Port
 * Copyright (C) 2006 Christian Walter <wolti@sil.at>
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * File: $Id: porttcp.c,v 1.2 2006/06/26 19:24:07 wolti Exp $
 */

/*
 * Design Notes:
 *
 * The xMBPortTCPInit function allocates a socket and binds the socket to
 * all available interfaces ( bind with INADDR_ANY ). In addition it
 * creates an array of event objects which is used to check the state of
 * the clients. On event object is used to handle new connections or
 * closed ones. The other objects are used on a per client basis for
 * processing.
 */

#include <stdio.h>
#include "winsock2.h"

#include "port.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"

/* ----------------------- MBAP Header --------------------------------------*/
#define MB_TCP_UID          6
#define MB_TCP_LEN          4
#define MB_TCP_FUNC         7

/* ----------------------- Defines  -----------------------------------------*/
#define MB_TCP_DEFAULT_PORT 502 /* TCP listening port. */
#define MB_TCP_POOL_TIMEOUT 50  /* pool timeout for event waiting. */
#define MB_TCP_READ_TIMEOUT 1000        /* Maximum timeout to wait for packets. */
#define MB_TCP_READ_CYCLE   100 /* Time between checking for new data. */

#define MB_TCP_DEBUG        1   /* Set to 1 for additional debug output. */

#define MB_TCP_BUF_SIZE     ( 256 + 7 ) /* Must hold a complete Modbus TCP frame. */

#define EV_CONNECTION       0
#define EV_CLIENT           1
#define EV_NEVENTS          EV_CLIENT + 1

/* ----------------------- Static variables ---------------------------------*/
SOCKET          xListenSocket;
SOCKET          xClientSocket;
WSAEVENT        xEvents[EV_NEVENTS];

static uint8_t    aucTCPBuf[MB_TCP_BUF_SIZE];
static uint16_t   usTCPBufPos;
static uint16_t   usTCPFrameBytesLeft;

/* ----------------------- External functions -------------------------------*/
TCHAR          *WsaError2String( DWORD dwError );

/* ----------------------- Static functions ---------------------------------*/
bool            prvMBTCPPortAddressToString( SOCKET xSocket, LPTSTR szAddr, uint16_t usBufSize );
LPTSTR          prvMBTCPPortFrameToString( uint8_t * pucFrame, uint16_t usFrameLen );
static bool     prvbMBPortAcceptClient( void );
static void     prvvMBPortReleaseClient( void );
static bool     prvMBTCPGetFrame( void );

/* ----------------------- Begin implementation -----------------------------*/

bool
xMBTCPPortInit( uint16_t usTCPPort )
{
    bool            bOkay = false;
    uint16_t          usPort;
    SOCKADDR_IN     xService;
    WSADATA         wsaData;

    int             i;

    if( WSAStartup( MAKEWORD( 2, 2 ), &wsaData ) != 0 )
    {
        return false;
    }

    if( usTCPPort == 0 )
    {
        usPort = MB_TCP_DEFAULT_PORT;
    }
    else
    {
        usPort = ( uint16_t ) usTCPPort;
    }

    xService.sin_family = AF_INET;
    xService.sin_port = htons( usPort );
    xService.sin_addr.s_addr = INADDR_ANY;

    xClientSocket = INVALID_SOCKET;
    for( i = 0; i < EV_NEVENTS; i++ )
    {
        if( ( xEvents[i] = WSACreateEvent(  ) ) == WSA_INVALID_EVENT )
            break;
    }
    if( i != EV_NEVENTS )
    {
        vMBPortLog( MB_LOG_ERROR, _T( "TCP-POLL" ), _T( "can't create event objects: %s\r\n" ),
                    WsaError2String( WSAGetLastError(  ) ) );
    }
    else if( ( xListenSocket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP ) ) == INVALID_SOCKET )
    {
        vMBPortLog( MB_LOG_ERROR, _T( "TCP-POLL" ), _T( "can't create socket: %s\r\n" ),
                    WsaError2String( WSAGetLastError(  ) ) );
    }
    else if( bind( xListenSocket, ( SOCKADDR * ) & xService, sizeof( xService ) ) == SOCKET_ERROR )
    {
        vMBPortLog( MB_LOG_ERROR, _T( "TCP-POLL" ), _T( "can't bind on socket: %s\r\n" ),
                    WsaError2String( WSAGetLastError(  ) ) );
    }
    else if( listen( xListenSocket, 5 ) == SOCKET_ERROR )
    {
        vMBPortLog( MB_LOG_ERROR, _T( "TCP-POLL" ), _T( "can't listen on socket: %s\r\n" ),
                    WsaError2String( WSAGetLastError(  ) ) );
    }
    else if( WSAEventSelect( xListenSocket, xEvents[EV_CONNECTION], FD_ACCEPT ) == SOCKET_ERROR )
    {
        vMBPortLog( MB_LOG_ERROR, _T( "TCP-POLL" ), _T( "can't enable events on socket: %s\r\n" ),
                    WsaError2String( WSAGetLastError(  ) ) );
    }
    else
    {
        vMBPortLog( MB_LOG_INFO, _T( "TCP-POLL" ), _T( "Modbus TCP server listening on %S:%d\r\n" ),
                    inet_ntoa( xService.sin_addr ), ntohs( xService.sin_port ) );
        bOkay = true;
    }

    /* Perform cleanup on error. */
    if( bOkay != true )
    {
        for( i = 0; i < EV_NEVENTS; i++ )
        {
            if( xEvents[i] != WSA_INVALID_EVENT )
                WSACloseEvent( xEvents[i] );
        }
        if( xListenSocket != SOCKET_ERROR )
            closesocket( xListenSocket );
    }

    return bOkay;
}

void
vMBTCPPortClose(  )
{
    int             i;

    /* Release all event handlers. */
    for( i = 0; i < EV_NEVENTS; i++ )
    {
        if( xEvents[i] != WSA_INVALID_EVENT )
            WSACloseEvent( xEvents[ i ] );
    }
    /* Close all client sockets. */
    if( xClientSocket != SOCKET_ERROR )
    {
        prvvMBPortReleaseClient(  );
    }
    /* Close the listener socket. */
    if( xListenSocket != SOCKET_ERROR )
    {
        closesocket( xListenSocket );
    }
    ( void )WSACleanup(  );
}

void
vMBTCPPortDisable( void )
{
    /* Close all client sockets. */
    if( xClientSocket != SOCKET_ERROR )
    {
        prvvMBPortReleaseClient(  );
    }
}

/*! \ingroup port_win32tcp
 *
 * \brief Pool the listening socket and currently connected Modbus TCP clients
 *   for new events.
 * \internal
 *
 * This function checks if new clients want to connect or if already connected
 * clients are sending requests. If a new client is connected and there are
 * still client slots left (The current implementation supports only one)
 * then the connection is accepted and an event object for the new client
 * socket is activated (See prvbMBPortAcceptClient() ).
 * Events for already existing clients in \c FD_READ and \c FD_CLOSE. In case of
 * an \c FD_CLOSE the client connection is released (See prvvMBPortReleaseClient() ).
 * In case of an \c FD_READ command the existing data is read from the client
 * and if a complete frame has been received the Modbus Stack is notified.
 *
 * \return false in case of an internal I/O error. For example if the internal
 *   event objects are in an invalid state. Note that this does not include any
 *   client errors. In all other cases returns true.
 */
bool
xMBPortTCPPool( void )
{

    bool            bOkay = true;
    DWORD           dwWaitResult;
    WSANETWORKEVENTS xNetworkEvents;
    int             iEventNr;
    int             iRes;

    dwWaitResult = WSAWaitForMultipleEvents( EV_NEVENTS, xEvents, false,
                                             MB_TCP_POOL_TIMEOUT, false );

    /* Do nothing because only the timeout has expired. */
    if( ( dwWaitResult == WAIT_IO_COMPLETION ) || ( dwWaitResult == WSA_WAIT_TIMEOUT ) )
    {
    }
    /* Waiting for events failed. */
    else if( dwWaitResult == WSA_WAIT_FAILED )
    {
        vMBPortLog( MB_LOG_ERROR, _T( "TCP-POLL" ), _T( "can't wait for network events: %s" ),
                    WsaError2String( WSAGetLastError(  ) ) );
        bOkay = false;
    }
    /* A event occured on one of the sockets */
    else
    {
        /* Get the event number for the result of the wait operation. */
        iEventNr = dwWaitResult - WSA_WAIT_EVENT_0;

        /* A client wants to connect. */
        if( iEventNr == EV_CONNECTION )
        {
            if( MB_TCP_DEBUG )
            {
                vMBPortLog( MB_LOG_DEBUG, _T( "TCP-POLL" ), _T( "got EV_CONNECTION event\r\n" ) );
            }

            /* Get additional event information from the socket. In addition the event
             * object is reseted.
             */
            iRes = WSAEnumNetworkEvents( xListenSocket, xEvents[iEventNr], &xNetworkEvents );
            if( iRes == SOCKET_ERROR )
            {
                vMBPortLog( MB_LOG_ERROR, _T( "TCP-POLL" ), _T( "can't get event list: %S\r\n" ),
                            WsaError2String( WSAGetLastError(  ) ) );
            }
            else if( xNetworkEvents.lNetworkEvents & FD_ACCEPT )
            {
                /* A new connection from a client. Accept it. */
                ( void )prvbMBPortAcceptClient(  );
            }
        }

        /* An already connected client has new data or the connection has
         * been closed. */
        else if( iEventNr == EV_CLIENT )
        {
            if( MB_TCP_DEBUG )
            {
                vMBPortLog( MB_LOG_DEBUG, _T( "TCP-POLL" ), _T( "got EV_CLIENT event\r\n" ) );
            }

            iRes = WSAEnumNetworkEvents( xClientSocket, xEvents[iEventNr], &xNetworkEvents );
            if( iRes == SOCKET_ERROR )
            {
                vMBPortLog( MB_LOG_ERROR, _T( "TCP-POLL" ), _T( "can't get event list: %s\r\n" ),
                            WsaError2String( WSAGetLastError(  ) ) );
            }
            else if( xNetworkEvents.lNetworkEvents & FD_READ )
            {
                if( MB_TCP_DEBUG )
                {
                    vMBPortLog( MB_LOG_DEBUG, _T( "TCP-POLL" ), _T( "FD_READ event\r\n" ) );
                }

                /* Process part of the Modbus TCP frame. In case of an I/O error we have to drop
                 * the client connection.
                 */
                if( prvMBTCPGetFrame(  ) != true )
                {
                    prvvMBPortReleaseClient(  );
                }
            }
            else if( xNetworkEvents.lNetworkEvents & FD_CLOSE )
            {
                if( MB_TCP_DEBUG )
                {
                    vMBPortLog( MB_LOG_DEBUG, _T( "TCP-POLL" ), _T( "FD_CLOSE event\r\n" ) );
                }

                prvvMBPortReleaseClient(  );
            }
            else
            {
                vMBPortLog( MB_LOG_WARN, _T( "TCP-POLL" ), _T( "unknown EV_CLIENT event\r\n" ) );
            }
        }
        else
        {
            /* Error - Log a warning. */
        }
    }
    return bOkay;
}

/*!
 * \ingroup port_win32tcp
 * \brief Receives parts of a Modbus TCP frame and if complete notifies
 *    the protocol stack.
 * \internal
 *
 * This function reads a complete Modbus TCP frame from the protocol stack.
 * It starts by reading the header with an initial request size for
 * usTCPFrameBytesLeft = MB_TCP_FUNC. If the header is complete the
 * number of bytes left can be calculated from it (See Length in MBAP header).
 * Further read calls are issued until the frame is complete.
 *
 * \return \c true if part of a Modbus TCP frame could be processed. In case
 *   of a communication error the function returns \c false.
 */
bool
prvMBTCPGetFrame(  )
{
    bool            bOkay = true;
    uint16_t          usLength;
    int             iRes;
    LPTSTR          szFrameAsStr;

    /* Make sure that we can safely process the next read request. If there
     * is an overflow drop the client.
     */
    if( ( usTCPBufPos + usTCPFrameBytesLeft ) >= MB_TCP_BUF_SIZE )
    {
        vMBPortLog( MB_LOG_WARN, _T( "MBTCP-RCV" ),
                    _T( "Detected buffer overrun. Dropping client.\r\n" ) );
        return false;
    }

    iRes = recv( xClientSocket, &aucTCPBuf[usTCPBufPos], usTCPFrameBytesLeft, 0 );
    switch ( iRes )
    {
    case SOCKET_ERROR:
        vMBPortLog( MB_LOG_WARN, _T( "MBTCP-RCV" ), _T( "recv failed: %s\r\n" ),
                    WsaError2String( WSAGetLastError(  ) ) );
        if( WSAGetLastError(  ) != WSAEWOULDBLOCK )
        {
            bOkay = false;
        }
        break;
    case 0:
        bOkay = false;
        break;
    default:
        usTCPBufPos += iRes;
        usTCPFrameBytesLeft -= iRes;
    }

    /* If we have received the MBAP header we can analyze it and calculate
     * the number of bytes left to complete the current request. If complete
     * notify the protocol stack.
     */
    if( usTCPBufPos >= MB_TCP_FUNC )
    {
        /* Length is a byte count of Modbus PDU (function code + data) and the
         * unit identifier. */
        usLength = aucTCPBuf[MB_TCP_LEN] << 8U;
        usLength |= aucTCPBuf[MB_TCP_LEN + 1];

        /* Is the frame already complete. */
        if( usTCPBufPos < ( MB_TCP_UID + usLength ) )
        {
            usTCPFrameBytesLeft = usLength + MB_TCP_UID - usTCPBufPos;
        }
        /* The frame is complete. */
        else if( usTCPBufPos == ( MB_TCP_UID + usLength ) )
        {
            if( MB_TCP_DEBUG )
            {
                szFrameAsStr = prvMBTCPPortFrameToString( aucTCPBuf, usTCPBufPos );
                if( szFrameAsStr != NULL )
                {
                    vMBPortLog( MB_LOG_DEBUG, _T( "MBTCP-RCV" ), _T( "Received: %s\r\n" ),
                                szFrameAsStr );
                    free( szFrameAsStr );
                }
            }
            ( void )xMBPortEventPost( EV_FRAME_RECEIVED );
        }
        /* This can not happend because we always calculate the number of bytes
         * to receive. */
        else
        {
            assert( usTCPBufPos <= ( MB_TCP_UID + usLength ) );
        }
    }
    return bOkay;
}

bool
xMBTCPPortGetRequest( uint8_t ** ppucMBTCPFrame, uint16_t * usTCPLength )
{
    *ppucMBTCPFrame = &aucTCPBuf[0];
    *usTCPLength = usTCPBufPos;

    /* Reset the buffer. */
    usTCPBufPos = 0;
    usTCPFrameBytesLeft = MB_TCP_FUNC;
    return true;
}

bool
xMBTCPPortSendResponse( const uint8_t * pucMBTCPFrame, uint16_t usTCPLength )
{
    bool            bFrameSent = false;
    bool            bAbort = false;
    int             res;
    int             iBytesSent = 0;
    int             iTimeOut = MB_TCP_READ_TIMEOUT;
    LPTSTR          szFrameAsStr;

    if( MB_TCP_DEBUG )
    {
        szFrameAsStr = prvMBTCPPortFrameToString( aucTCPBuf, usTCPLength );
        if( szFrameAsStr != NULL )
        {
            vMBPortLog( MB_LOG_DEBUG, _T( "MBTCP-SND" ), _T( "Snd: %s\r\n" ), szFrameAsStr );
            free( szFrameAsStr );
        }
    }

    do
    {
        res = send( xClientSocket, &pucMBTCPFrame[iBytesSent], usTCPLength - iBytesSent, 0 );
        switch ( res )
        {
        case SOCKET_ERROR:
            if( ( WSAGetLastError(  ) == WSAEWOULDBLOCK ) && ( iTimeOut > 0 ) )
            {
                iTimeOut -= MB_TCP_READ_CYCLE;
                Sleep( MB_TCP_READ_CYCLE );
            }
            else
            {
                bAbort = true;
            }
            break;
        case 0:
            prvvMBPortReleaseClient(  );
            bAbort = true;
            break;
        default:
            iBytesSent += res;
            break;
        }
    }
    while( ( iBytesSent != usTCPLength ) && !bAbort );

    bFrameSent = iBytesSent == usTCPLength ? true : false;

    return bFrameSent;
}

void
prvvMBPortReleaseClient(  )
{
    TCHAR           szIPAddr[32];


    if( prvMBTCPPortAddressToString( xClientSocket, szIPAddr, _countof( szIPAddr ) ) == true )
    {
        vMBPortLog( MB_LOG_INFO, _T( "MBTCP-CMGT" ), _T( "client %s disconnected.\r\n" ),
                    szIPAddr );
    }
    else
    {
        vMBPortLog( MB_LOG_INFO, _T( "MBTCP-CMGT" ), _T( "unknown client disconnected.\r\n" ) );
    }

    /* Disable event notification for this client socket. */
    if( WSAEventSelect( xClientSocket, xEvents[EV_CLIENT], 0 ) == SOCKET_ERROR )
    {
        vMBPortLog( MB_LOG_ERROR, _T( "MBTCP-CMGT" ),
                    _T( "can't disable events for disconnecting client socket: %s\r\n" ),
                    WsaError2String( WSAGetLastError(  ) ) );
    }

    /* Reset event object in case an event was still pending. */
    if( WSAResetEvent( xEvents[EV_CLIENT] ) == SOCKET_ERROR )
    {
        vMBPortLog( MB_LOG_ERROR, _T( "MBTCP-CMGT" ),
                    _T( "can't disable events for disconnecting client socket: %s\r\n" ),
                    WsaError2String( WSAGetLastError(  ) ) );
    }

    /* Disallow the sender side. This tells the other side that we have finished. */
    if( shutdown( xClientSocket, SD_SEND ) == SOCKET_ERROR )
    {
        vMBPortLog( MB_LOG_ERROR, _T( "MBTCP-CMGT" ), _T( "shutdown failed: %s\r\n" ),
                    WsaError2String( WSAGetLastError(  ) ) );
    }

    /* Read any unread data from the socket. Note that this is not the strictly
     * correct way to do it because our sockets are non blocking and therefore
     * some bytes could remain.
     */
    ( void )recv( xClientSocket, &aucTCPBuf[0], MB_TCP_BUF_SIZE, 0 );

    ( void )closesocket( xClientSocket );
    xClientSocket = INVALID_SOCKET;
}

bool
prvbMBPortAcceptClient(  )
{
    SOCKET          xNewSocket;
    bool            bOkay;

    TCHAR           szIPAddr[32];

    /* Check if we can handle a new connection. */

    if( xClientSocket != INVALID_SOCKET )
    {
        vMBPortLog( MB_LOG_ERROR, _T( "MBTCP-CMGT" ),
                    _T( "can't accept new client. all connections in use.\r\n" ) );
        bOkay = false;
    }
    else if( ( xNewSocket = accept( xListenSocket, NULL, NULL ) ) == INVALID_SOCKET )
    {
        bOkay = false;
    }
    /* Register READ events on the socket file descriptor. */
    else if( WSAEventSelect( xNewSocket, xEvents[EV_CLIENT], FD_READ | FD_CLOSE ) == SOCKET_ERROR )
    {
        bOkay = false;
        ( void )closesocket( xNewSocket );
    }
    /* Everything okay - Register the client connection. */
    else
    {
        xClientSocket = xNewSocket;
        usTCPBufPos = 0;
        usTCPFrameBytesLeft = MB_TCP_FUNC;

        if( prvMBTCPPortAddressToString( xClientSocket, szIPAddr, _countof( szIPAddr ) ) == true )
        {
            vMBPortLog( MB_LOG_INFO, _T( "MBTCP-CMGT" ), _T( "accepted new client %s.\r\n" ),
                        szIPAddr );
        }
        else
        {
            vMBPortLog( MB_LOG_INFO, _T( "MBTCP-CMGT" ), _T( "accepted unknown client.\r\n" ) );
        }
        bOkay = true;

    }
    return bOkay;
}
