/*
 * rpcTransportUart.c
 *
 * This module contains the API for the zll SoC Host Interface.
 *
 * Copyright (C) 2013 Texas Instruments Incorporated - http://www.ti.com/
 *
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/*********************************************************************
 * INCLUDES
 */
#include <termios.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>

//#include "rpc.h"

#include "dbgPrint.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */
/*******************************/
/*** Bootloader Commands ***/
/*******************************/
#define SB_FORCE_BOOT               0xF8
#define SB_FORCE_RUN               (SB_FORCE_BOOT ^ 0xFF)

/************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */
uint8_t uartDebugPrintsEnabled = 0;

/*********************************************************************
 * LOCAL VARIABLES
 */
int serialPortFd;

/*********************************************************************
 * API FUNCTIONS
 */

/*********************************************************************
 * @fn      rpcTransportOpen
 *
 * @brief   opens the serial port to the CC253x.
 *
 * @param   devicePath - path to the UART device
 *
 * @return  status
 */
int32_t rpcTransportOpen(char *_devicePath, uint32_t port, uint32_t baudrate)
{
	struct termios tio;
	static char lastUsedDevicePath[255];
	char * devicePath;
	speed_t znp_baudrate;

	(void) port;

	if (_devicePath != NULL)
	{
		if (strlen(_devicePath) > (sizeof(lastUsedDevicePath) - 1))
		{
			dbg_print(PRINT_LEVEL_ERROR,
			        "rpcTransportOpen: %s - device path too long\n",
			        _devicePath);
			return (-1);
		}
		devicePath = _devicePath;
		strcpy(lastUsedDevicePath, _devicePath);
	}
	else
	{
		devicePath = lastUsedDevicePath;
	}

	/* open the device */
	serialPortFd = open(devicePath, O_RDWR | O_NOCTTY);
	if (serialPortFd < 0)
	{
		perror(devicePath);
		dbg_print(PRINT_LEVEL_ERROR, "rpcTransportOpen: %s open failed\n",
		        devicePath);
		return (-1);
	}

	//flush all the unread data
	if(tcflush(serialPortFd, TCIOFLUSH) == -1) {
		dbg_print(PRINT_LEVEL_ERROR, "tcflush(): %s\n", strerror(errno));
		return (-1);
	}

	//get the terminal attributes in termios structure
	if(tcgetattr(serialPortFd, &tio) == -1) {
		dbg_print(PRINT_LEVEL_ERROR, "tcgetattr(): %s\n", strerror(errno));
		return (-1);
	}

	//set the terminal in raw mode
	cfmakeraw(&tio);

	/* c-iflags
	 B115200 : set board rate to 115200
	 CRTSCTS : HW flow control
	 CS8     : 8n1 (8bit,no parity,1 stopbit)
	 CLOCAL  : local connection, no modem contol
	 CREAD   : enable receiving characters*/
	tio.c_cflag &= ~HUPCL;
	tio.c_cflag &= ~CLOCAL;
	tio.c_cflag |= CS8 | CLOCAL | CREAD;
	tio.c_cflag &= ~CRTSCTS; //No flow-control
// #ifndef CC26xx
// 	tio.c_cflag |= CRTSCTS;
// #endif //CC26xx
	/* c-iflags
	 ICRNL   : maps 0xD (CR) to 0x10 (LR), we do not want this.
	 IGNPAR  : ignore bits with parity errors, I guess it is
	 better to ignore an erroneous bit than interpret it incorrectly. */
	tio.c_iflag = IGNPAR & ~ICRNL;
	tio.c_oflag = 0;
	tio.c_lflag = 0;
	//Make it block
	tio.c_cc[VMIN] = 1;

	switch(baudrate) {
	    case 9600:
	      	znp_baudrate = B9600;
	      	break;
	    case 19200:
	      	znp_baudrate = B19200;
	      	break; 
	    case 38400:
	      	znp_baudrate = B38400;
	      	break; 
	    case 57600:
	      	znp_baudrate = B57600;
	      	break; 
	    case 115200:
	      	znp_baudrate = B115200;
	      	break; 
	    default:
	      	dbg_print(PRINT_LEVEL_ERROR, "Unknown baudrate: %d\n", znp_baudrate);
			return (-1);
	      	break;    
  	}

	cfsetispeed(&tio, znp_baudrate);
	cfsetospeed(&tio, znp_baudrate);

	//set the above attributes
	if(tcsetattr(serialPortFd, TCSAFLUSH, &tio) == -1) {
		dbg_print(PRINT_LEVEL_ERROR, "tcsetattr() 1: %s\n", strerror(errno));
		return (-1);
	}

	tio.c_cflag |= CLOCAL;
	if(tcsetattr(serialPortFd, TCSAFLUSH, &tio) == -1) {
		dbg_print(PRINT_LEVEL_ERROR, "tcsetattr() 2: %s\n", strerror(errno));
		return (-1);
	}

	int i = TIOCM_DTR;
	if(ioctl(serialPortFd, TIOCMBIS, &i) == -1) {
		dbg_print(PRINT_LEVEL_ERROR, "ioctl(): %s\n", strerror(errno));
		return (-1);
	}

	//wait for hardware 10 ms - node-6lbr reference
	usleep(10 * 1000);

	if(tcflush(serialPortFd, TCIOFLUSH) == -1) {
		dbg_print(PRINT_LEVEL_ERROR, "tcflush(): %s\n", strerror(errno));
		return (-1);
	}

	tcflush(serialPortFd, TCIFLUSH);
	tcsetattr(serialPortFd, TCSANOW, &tio);

	return serialPortFd;
}

/*********************************************************************
 * @fn      rpcTransportClose
 *
 * @brief   closes the serial port to the CC253x.
 *
 * @param   fd - file descriptor of the UART device
 *
 * @return  status
 */
void rpcTransportClose(void)
{
	tcflush(serialPortFd, TCOFLUSH);
	close(serialPortFd);

	return;
}

/*********************************************************************
 * @fn      rpcTransportWrite
 *
 * @brief   Write to the the serial port to the CC253x.
 *
 * @param   fd - file descriptor of the UART device
 *
 * @return  status
 */
void rpcTransportWrite(uint8_t* buf, uint8_t len)
{
	int remain = len;
	int offset = 0;
#if 1
	dbg_print(PRINT_LEVEL_VERBOSE, "rpcTransportWrite : len = %d\n", len);

	while (remain > 0)
	{
		int sub = (remain >= 8 ? 8 : remain);
		dbg_print(PRINT_LEVEL_VERBOSE,
		        "writing %d bytes (offset = %d, remain = %d)\n", sub, offset,
		        remain);
		write(serialPortFd, buf + offset, sub);

		tcflush(serialPortFd, TCOFLUSH);
		usleep(1000);
		remain -= 8;
		offset += 8;
	}
#else
	write (serialPortFd, buf, len);
	tcflush(serialPortFd, TCOFLUSH);

#endif
	return;
}

/*********************************************************************
 * @fn      rpcTransportRead
 *
 * @brief   Reads from the the serial port to the CC253x.
 *
 * @param   fd - file descriptor of the UART device
 *
 * @return  status
 */
uint8_t rpcTransportRead(uint8_t* buf, uint8_t len)
{
	uint8_t ret = read(serialPortFd, buf, len);
	if (ret > 0)
	{
		dbg_print(PRINT_LEVEL_VERBOSE, "rpcTransportRead: read %d bytes\n",
		        ret);
	}
	return (ret);

}
