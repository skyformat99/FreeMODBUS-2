/*
 * FreeModbus Libary: Z8Encore Port for Z8F6422
 * Copyright (C) 2007 Tiago Prado Lone <tiago@maxwellbohr.com.br>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * File: $Id: portevent.c,v 1.1 2007/04/24 23:42:43 wolti Exp $
 */

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"

/* ----------------------- Variables ----------------------------------------*/
static eMBEventType eQueuedEvent;
static bool     xEventInQueue;

/* ----------------------- Start implementation -----------------------------*/
bool
xMBPortEventInit( void )
{
    xEventInQueue = false;
    return true;
}

bool
xMBPortEventPost( eMBEventType eEvent )
{
    xEventInQueue = true;
    eQueuedEvent = eEvent;
    return true;
}

bool
xMBPortEventGet( eMBEventType * eEvent )
{
    bool            xEventHappened = false;

    if( xEventInQueue )
    {
        *eEvent = eQueuedEvent;
        xEventInQueue = false;
        xEventHappened = true;
    }
    return xEventHappened;
}
