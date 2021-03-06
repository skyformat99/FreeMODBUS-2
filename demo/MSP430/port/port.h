/*
 * FreeModbus Libary: MSP430 Port
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
 * File: $Id: port.h,v 1.2 2006/11/19 03:36:01 wolti Exp $
 */

#ifndef _PORT_H
#define _PORT_H

#include <stdbool.h>

/* ----------------------- Platform includes --------------------------------*/

#include <msp430x16x.h>
#if defined (__GNUC__)
#include <signal.h>
#endif
#undef int8_t

/* ----------------------- Defines ------------------------------------------*/
#define	INLINE

#define ENTER_CRITICAL_SECTION( )   EnterCriticalSection( )
#define EXIT_CRITICAL_SECTION( )    ExitCriticalSection( )
#define assert( expr )

#define SMCLK                       ( 4000000UL )
#define ACLK                        ( 32768UL )

void            EnterCriticalSection( void );
void            ExitCriticalSection( void );


#endif
