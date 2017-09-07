/*
 * FreeModbus Libary: Linux Port
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
 * File: $Id: portevent.c,v 1.1 2006/08/01 20:58:49 wolti Exp $
 */

#include "port.h"
#include "portinstance.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"

/* ----------------------- Start implementation -----------------------------*/
bool
xMBPortEventInit( struct xMBInstance * xInstance )
{
    ( ( struct PortInstance* )xInstance )->xEventInQueue = false;
    return true;
}

bool
xMBPortEventPost( struct xMBInstance * xInstance, eMBEventType eEvent )
{
    ( ( struct PortInstance* )xInstance )->xEventInQueue = true;
    ( ( struct PortInstance* )xInstance )->eQueuedEvent = eEvent;
    return true;
}

bool
xMBPortEventGet( struct xMBInstance * xInstance, eMBEventType * eEvent )
{
    bool            xEventHappened = false;

    if( ( ( struct PortInstance* )xInstance )->xEventInQueue )
    {
        *eEvent = ( ( struct PortInstance* )xInstance )->eQueuedEvent;
        ( ( struct PortInstance* )xInstance )->xEventInQueue = false;
        xEventHappened = true;
    }
    else
    {
        /* Poll the serial device. The serial device timeouts if no
         * characters have been received within for t3.5 during an
         * active transmission or if nothing happens within a specified
         * amount of time. Both timeouts are configured from the timer
         * init functions.
         */
        ( void )xMBPortSerialPoll( xInstance );

        /* Check if any of the timers have expired. */
        vMBPortTimerPoll( xInstance );

    }
    return xEventHappened;
}
