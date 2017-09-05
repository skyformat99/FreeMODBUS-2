/*
 * FreeModbus Libary: MCF5235 Demo Application
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
 * File: $Id: demo.c,v 1.2 2006/06/15 15:38:55 wolti Exp $
 */

#include "mcf5xxx.h"
#include "mcf523x.h"

#include "mb.h"
#include "mbport.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"

/* ----------------------- Defines ------------------------------------------*/
#define REG_INPUT_START 1000
#define REG_INPUT_NREGS 4

/* ----------------------- Static variables ---------------------------------*/
static uint16_t   usRegInputStart = REG_INPUT_START;
static uint16_t   usRegInputBuf[REG_INPUT_NREGS];


int
main( int argc, char *argv[], char *envp[] )
{
    //xMBPortSerialInit (9600UL, 8, MB_PAR_EVEN);
    //vMBPortSerialEnable (true , false);
    //xMBPortTimersInit( 200 );
    //vMBPortTimersEnable();
    //
    const uint8_t     ucSlaveID[] = { 0xAA, 0xBB, 0xCC };
    eMBErrorCode    eStatus;

    eStatus = eMBInit( MB_RTU, 0x0A, 0, 38400, MB_PAR_EVEN );

    eStatus = eMBSetSlaveID( 0x34, true, ucSlaveID, 3 );

    /* Enable the Modbus Protocol Stack. */
    eStatus = eMBEnable(  );

    for( ;; )
    {
        ( void )eMBPoll(  );

        /* Here we simply count the number of poll cycles. */
        usRegInputBuf[0]++;
    }
    return 0;
}

eMBErrorCode
eMBRegInputCB( uint8_t * pucRegBuffer, uint16_t usAddress, uint16_t usNRegs )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    int             iRegIndex;

    if( ( usAddress >= REG_INPUT_START )
        && ( usAddress + usNRegs <= REG_INPUT_START + REG_INPUT_NREGS ) )
    {
        iRegIndex = ( int )( usAddress - usRegInputStart );
        while( usNRegs > 0 )
        {
            *pucRegBuffer++ =
                ( unsigned char )( usRegInputBuf[iRegIndex] >> 8 );
            *pucRegBuffer++ =
                ( unsigned char )( usRegInputBuf[iRegIndex] & 0xFF );
            iRegIndex++;
            usNRegs--;
        }
    }
    else
    {
        eStatus = MB_ENOREG;
    }

    return eStatus;
}

eMBErrorCode
eMBRegHoldingCB( uint8_t * pucRegBuffer, uint16_t usAddress, uint16_t usNRegs,
                 eMBRegisterMode eMode )
{
    return MB_ENOREG;
}


eMBErrorCode
eMBRegCoilsCB( uint8_t * pucRegBuffer, uint16_t usAddress, uint16_t usNCoils,
               eMBRegisterMode eMode )
{
    return MB_ENOREG;
}

eMBErrorCode
eMBRegDiscreteCB( uint8_t * pucRegBuffer, uint16_t usAddress, uint16_t usNDiscrete )
{
    return MB_ENOREG;
}
