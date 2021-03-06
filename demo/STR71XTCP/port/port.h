 /*
  * FreeModbus Libary: MCF5235 Port
  * Copyright (C) 2006 Christian Walter <wolti@sil.at>
  * Parts of crt0.S Copyright (c) 1995, 1996, 1998 Cygnus Support
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
  * File: $Id: port.h,v 1.2 2006/09/04 14:39:20 wolti Exp $
  */

#ifndef _PORT_H
#define _PORT_H

#include <stdbool.h>

/* ----------------------- Platform includes --------------------------------*/
#include "71x_type.h"
#include "lwip/opt.h"
#include "lwip/sys.h"

/* ----------------------- Defines ------------------------------------------*/
#undef INLINE
#define INLINE                  inline

#define assert( x )             LWIP_ASSERT( #x, x );

#ifdef __cplusplus
extern "C" {
#endif
#define MB_TCP_DEBUG            1       /* Debug output in TCP module. */
/* ----------------------- Type definitions ---------------------------------*/
#ifdef MB_TCP_DEBUG
typedef enum
{
    MB_LOG_DEBUG,
    MB_LOG_INFO,
    MB_LOG_WARN,
    MB_LOG_ERROR
} eMBPortLogLevel;
#endif

/* ----------------------- Function prototypes ------------------------------*/
#ifdef MB_TCP_DEBUG
void            vMBPortLog( eMBPortLogLevel eLevel, const char * szModule,
                            const char * szFmt, ... );
void            prvvMBTCPLogFrame( char * pucMsg, uint8_t * pucFrame, uint16_t usFrameLen );
#endif

#ifdef __cplusplus
}
#endif
#endif
