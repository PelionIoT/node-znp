/**************************************************************************************************
 * Filename:       dataSendRcv.c
 * Description:    This file contains dataSendRcv application.
 *
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
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <inttypes.h>

#include "rpc.h"
#include "mtSys.h"
#include "mtZdo.h"
#include "mtAf.h"
#include "mtParser.h"
#include "rpcTransport.h"
#include "dbgPrint.h"

#include "znp_mngt.h"
#include "zcl_gateway.h"

#include "znp_cfuncs.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * TYPES
 */

/*********************************************************************
 * LOCAL VARIABLE
 */

/***********************************************************************/

/*********************************************************************
 * LOCAL FUNCTIONS
 */

uint8_t zclGwZclGetSatCb(uint8_t sat, uint16_t nwkAddr, uint8_t endpoint,
uint8_t zclTransId);
uint8_t zclGwZclGetHueCb(uint8_t hue, uint16_t nwkAddr, uint8_t endpoint,
uint8_t zclTransId);
uint8_t zclGwZclGetLevelCb(uint8_t level, uint16_t nwkAddr, uint8_t endpoint,
uint8_t zclTransId);
uint8_t zclGwZclGetInfoCb(uint8_t* manu, uint16_t nwkAddr, uint8_t endpoint,
uint8_t zclTransId);
uint8_t zclGwZclGetStateCb(uint8_t state, uint16_t nwkAddr, uint8_t endpoint,
uint8_t zclTransId);
uint8_t zclGwZclGetSetPointCb(int16_t setPoint, uint16_t nwkAddr, uint8_t endpoint,
uint8_t zclTransId);

/*********************************************************************
 * CALLBACK FUNCTIONS
 */

//! \brief pan ID of the bridges network
uint_least16_t panId = 0x1234;

uint8_t zMngtZdoSimpleDescRspCb(epInfo_t *epInfo);
uint8_t zMngtZdoLeaveIndCb(uint16_t nwkAddr);

static zMngt_callbacks_t zMngt_callbacks =
{ zMngtZdoSimpleDescRspCb, // ZCL callback for simple desc response
        zMngtZdoLeaveIndCb, // ZCL callback for simple desc response
        NULL, };

static zclGw_callbacks_t zclGw_callbacks =
{ 
        zclGwZclGetInfoCb, // ZCL response callback for get info
        zclGwZclGetStateCb, // ZCL response callback for get State
        zclGwZclGetLevelCb, // ZCL response callback for get Level
        zclGwZclGetHueCb, // ZCL response callback for get Hue
        zclGwZclGetSatCb, // ZCL response callback for get Sat
        NULL,
        zclGwZclGetSetPointCb };

#define MAX_DEVICES 100
epInfo_t deviceList[MAX_DEVICES] = {{0}};
uint8_t devIdx = 0;
uint8_t devCnt = 0;

/*********************************************************************
 * LOCAL FUNCTIONS
 */

void printHelp(void)
{
    dbg_print(PRINT_LEVEL_INFO, "Commands:\n");
    dbg_print(PRINT_LEVEL_INFO, "'p' - permit devices to join the network\n");
    dbg_print(PRINT_LEVEL_INFO, "'d' - device list\n");
    dbg_print(PRINT_LEVEL_INFO, "'i' - set device (index in device list)\n");
    dbg_print(PRINT_LEVEL_INFO, "'n' - turn on selected device\n");
    dbg_print(PRINT_LEVEL_INFO, "'f' - turn off selected device\n");
    dbg_print(PRINT_LEVEL_INFO, "'o' - get selected device on/off state\n");
    dbg_print(PRINT_LEVEL_INFO, "'s' - set setpoint\n");
    dbg_print(PRINT_LEVEL_INFO, "'g' - get setpoint\n");
    dbg_print(PRINT_LEVEL_INFO, "'m' - get info\n");
    dbg_print(PRINT_LEVEL_INFO, "'h' - this menu\n");
}

void printDevices(void)
{
    uint8_t idx;

    printf("printDevices: cnt=%x:%x\n", &devCnt, devCnt);
    dbg_print(PRINT_LEVEL_INFO, "Idx\tNwkAdd\tEndpoint\tDeviceId\n");
    for(idx = 0; idx < devCnt; idx++)
    {
    	dbg_print(PRINT_LEVEL_INFO, "%d\t", idx);
    	dbg_print(PRINT_LEVEL_INFO, "0x%04X\t", deviceList[idx].nwkAddr);
    	dbg_print(PRINT_LEVEL_INFO, "0x%02X\t", deviceList[idx].endpoint);
    	dbg_print(PRINT_LEVEL_INFO, "0x%04X\n", deviceList[idx].deviceID);
    }
}

uint8_t zMngtZdoSimpleDescRspCb(epInfo_t *epInfo)
{
    //process simple desc here
    dbg_print(PRINT_LEVEL_INFO, "Device joined network:\n");

    if(devCnt < MAX_DEVICES)
    {
    	memcpy(&deviceList[devCnt], epInfo, sizeof(epInfo_t));
       	dbg_print(PRINT_LEVEL_INFO, "%d\t", devCnt);
        dbg_print(PRINT_LEVEL_INFO, "0x%04X\t", deviceList[devCnt].nwkAddr);
        dbg_print(PRINT_LEVEL_INFO, "0x%02X\t", deviceList[devCnt].endpoint);
        dbg_print(PRINT_LEVEL_INFO, "0x%04X\n", deviceList[devCnt].deviceID);
	    devCnt++;
    }
    else
    {
    	dbg_print(PRINT_LEVEL_INFO, "Error: Max Devices in List\n");
    }
    zWZdoSimpleDescRspCb(epInfo);
    //report the device to deviceJS
    return 0;
}

uint8_t zMngtZdoLeaveIndCb(uint16_t nwkAddr)
{
    //TODO: remove device from dl module
    return 0;
}

uint8_t zclGwZclGetInfoCb(uint8_t *manu, uint16_t nwkAddr, uint8_t endpoint,
uint8_t zclTransId)
{
    //process the rsp here
    dbg_print(PRINT_LEVEL_INFO, "Device 0x%04X:0x%02X\n", nwkAddr, endpoint);

    return 0;
}

uint8_t zclGwZclGetStateCb(uint8_t state, uint16_t nwkAddr, uint8_t endpoint,
uint8_t zclTransId)
{
	//process the rsp here
	dbg_print(PRINT_LEVEL_INFO, "Device 0x%04X:0x%02X on/off state: %d\n", nwkAddr, endpoint, state);

    return 0;
}

uint8_t zclGwZclGetLevelCb(uint8_t level, uint16_t nwkAddr, uint8_t endpoint,
uint8_t zclTransId)
{
	//process the rsp here

    return 0;
}

uint8_t zclGwZclGetHueCb(uint8_t hue, uint16_t nwkAddr, uint8_t endpoint,
uint8_t zclTransId)
{
	//process the rsp here

    return 0;
}

uint8_t zclGwZclGetSatCb(uint8_t sat, uint16_t nwkAddr, uint8_t endpoint,
uint8_t zclTransId)
{
	//process the rsp here
    return 0;
}

uint8_t zclGwZclGetSetPointCb(int16_t setPoint, uint16_t nwkAddr, uint8_t endpoint,
uint8_t zclTransId)
{
	//process the rsp here
	dbg_print(PRINT_LEVEL_INFO, "Device 0x%04X:0x%02X on/off set point: %d\n", nwkAddr, endpoint, setPoint);

    return 0;
}

/*********************************************************************
 * NODE FUNCTIONS
 */
uint8_t wZAddDevice(uint8_t duration) 
{
    dbg_print(PRINT_LEVEL_INFO, "Permit join for %ds\n", duration);
    return (uint8_t)zMngt_openNetwork(duration);
}

uint8_t wZCloseRPC(void) 
{
    dbg_print(PRINT_LEVEL_INFO, "Closing the RPC network\n");
    rpcClose();
    return true;
}

uint8_t wZEndDeviceAnnce(EndDeviceAnnceIndFormat_t *msg) {
    return zMngt_endDeviceAnnouncement(msg);
}

void* wZgetNVItem(uint16_t id) {
    return zMngt_getNVItem(id);
}

uint8_t wZsetNVItem(uint16_t id, uint8_t len, uint8_t *value) {
    return zMngt_setNVItem(id, len, value);
}

uint8_t wZSendLqiReq(uint16_t dstAddr) 
{
    uint8_t status = 0;
    MgmtLqiReqFormat_t req;
    req.DstAddr = dstAddr;
    req.StartIndex = 0;
    resetNodeTopology();
    status = zdoMgmtLqiReq(&req);
    return status;   
}
/*********************************************************************



/*********************************************************************
 * INTERFACE FUNCTIONS
 */
uint32_t appInit(void)
{
    int_least32_t status = 0;
    uint_least32_t msgCnt = 0;

    //Flush all messages from the que
    while (status != -1)
    {
        status = rpcWaitMqClientMsg(100);
        if (status != -1)
        {
            msgCnt++;
        }
    }

    dbg_print(PRINT_LEVEL_INFO, "flushed %d message from msg queue\n", msgCnt);
    // printf("flushed %d message from msg queue\n", msgCnt);

    //initialise the ZNP Management module (management plain)
    zMngt_init();

    //Initialise ZCL module (data plain)
    zclGw_Init();

    //Register management callbacks (control plain) with znp management module
    zMngt_registerCallbacks(zMngt_callbacks);

    //Register zcl (data plain) callbacks with zll module
    zclGw_registerCallbacks(zclGw_callbacks);

    return 0;
}

//seperate mesage thread required so we can service the terminal and not block on messages
uint8_t initDone = 0;
int appMsgProcess(void *argument)
{
    if (initDone){
        rpcWaitMqClientMsg(10000);
    }
	return 0;
}

int appProcess(void *argument)
{
	int32_t status;
	char cmd[128];
	int16_t newSetPoint;
    struct timespec req;
    struct timespec rem;


    // MgmtLqiReqFormat_t req;
    // req.DstAddr = 0;
    // req.StartIndex = 0;

    config_options *opts = (config_options*)argument;

	//Flush all messages from the que
	do
	{
		status = rpcWaitMqClientMsg(50);
	} while (status != -1);

	//We do not remember the device list so always start a new network for now
    if(opts->newNwk) {
        dbg_print(PRINT_LEVEL_INFO, "CLEARING NETWORK\n");
        zMngt_clearNetwork(false);
    } else {
        dbg_print(PRINT_LEVEL_INFO, "RESTORING NETWORK\n");
        zMngt_restoreNetwork();
    }
    status = zMngt_start(opts->devType, opts->channelMask, opts->newNwk, opts->panId);

    if (status != MT_RPC_SUCCESS) {
        zWNetworkFailed();
        return -1;
    } else if(status == MT_RPC_SUCCESS) {
        //signal network is up
        zWNetworkReady();
    }

    //Initialise ZCL and Register endpoints
    zclGw_InitZcl();

	initDone = 1;

	while (1)
	{
        //Set sleep time to 10ms
        req.tv_sec = 0;
        req.tv_nsec = 10000000;
        nanosleep(&req, &rem);
	}

	return 0;
}

