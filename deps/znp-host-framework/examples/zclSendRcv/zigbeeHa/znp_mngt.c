/*
 * Copyright (c) 2014, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ======== znp_mngt.c ========
 */

/*********************************************************************
 * INCLUDES
 */
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>

#include "rpc.h"
#include "mtSys.h"
#include "mtZdo.h"
#include "AF.h"
#include "mtAf.h"
#include "mtParser.h"
#include "rpcTransport.h"
#include "dbgPrint.h"
#include "hostConsole.h"
#include "znp_mngt.h"

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

//! \brief init ZDO device state
//!
devStates_t devState = DEV_HOLD;

bool znpHasReset = false;

//! \brief Match Desc rsp variable
//!
afAddrType_t matchDstAddrTbl[10];
uint_least8_t maxMatcheAddrs;
uint_least8_t matchNumAddrs;

//! \brief znp mngt callbacks
//!
zMngt_callbacks_t zMngt_callbacks;

/*********************************************************************
 * LOCAL FUNCTIONS
 */
//! \brief ZDO callbacks
//!
static uint_least8_t mtZdoStateChangeIndCb(uint_least8_t newDevState);
static uint_least8_t mtZdoEndDeviceAnnceIndCb(EndDeviceAnnceIndFormat_t *msg);
static uint_least8_t mtZdoTcEndDeviceAnnceIndCb(TcEndDeviceAnnceIndFormat_t *msg);
static uint_least8_t mtZdoActiveEpRspCb(ActiveEpRspFormat_t *msg);
static uint_least8_t mtZdoSimpleDescRspCb(SimpleDescRspFormat_t *msg);
static uint_least8_t mtZdoMatchDescRsp(MatchDescRspFormat_t *rsp);
static uint_least8_t mtZdoMgmtLeaveRspCb(MgmtLeaveRspFormat_t *msg);
static uint_least8_t mtZdoMgmtLqiRspCb(MgmtLqiRspFormat_t *msg);
//! \brief SYS Callbacks
//!
static uint_least8_t mtSysResetIndCb(ResetIndFormat_t *msg);
static uint_least8_t mtSysOsalNvReadCb(OsalNvReadSrspFormat_t *rsp);

//! \brief helper functions
//!
static uint_least8_t setNVStartNew();
static uint_least8_t setNVStartRestore();
static uint_least8_t setNVChanList(uint_least32_t chanList);
static uint_least8_t setNVPanID(uint_least32_t panId);
static uint_least8_t setNVDevType(uint_least8_t devType);
static uint_least8_t startNetwork(uint_least8_t devType, uint_least32_t chan, uint8_t newNwk, uint16_t panId);
static uint_least8_t znpReset(void);


static uint_least8_t getNVPanID();
static uint_least8_t getNVChanList();

/*********************************************************************
 * CALLBACK FUNCTIONS
 */

//! \brief SYS callbacks
//!
static mtSysCb_t mtSysCb =
{
//mtSysResetInd          //MT_SYS_RESET_IND
        NULL,
        NULL,
        NULL, 
        mtSysResetIndCb,
        NULL,
        mtSysOsalNvReadCb,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL };

static mtZdoCb_t mtZdoCb =
{
NULL,       // MT_ZDO_NWK_ADDR_RSP
        NULL,      // MT_ZDO_IEEE_ADDR_RSP
        NULL,      // MT_ZDO_NODE_DESC_RSP
        NULL,     // MT_ZDO_POWER_DESC_RSP
        mtZdoSimpleDescRspCb,    // MT_ZDO_SIMPLE_DESC_RSP
        mtZdoActiveEpRspCb,      // MT_ZDO_ACTIVE_EP_RSP
        mtZdoMatchDescRsp,     // MT_ZDO_MATCH_DESC_RSP
        NULL,   // MT_ZDO_COMPLEX_DESC_RSP
        NULL,      // MT_ZDO_USER_DESC_RSP
        NULL,     // MT_ZDO_USER_DESC_CONF
        NULL,    // MT_ZDO_SERVER_DISC_RSP
        NULL, // MT_ZDO_END_DEVICE_BIND_RSP
        NULL,          // MT_ZDO_BIND_RSP
        NULL,        // MT_ZDO_UNBIND_RSP
        NULL,   // MT_ZDO_MGMT_NWK_DISC_RSP
        mtZdoMgmtLqiRspCb,       // MT_ZDO_MGMT_LQI_RSP
        NULL,       // MT_ZDO_MGMT_RTG_RSP
        NULL,      // MT_ZDO_MGMT_BIND_RSP
        mtZdoMgmtLeaveRspCb,     // MT_ZDO_MGMT_LEAVE_RSP
        NULL,     // MT_ZDO_MGMT_DIRECT_JOIN_RSP
        NULL,     // MT_ZDO_MGMT_PERMIT_JOIN_RSP
        mtZdoStateChangeIndCb,   // MT_ZDO_STATE_CHANGE_IND
        mtZdoEndDeviceAnnceIndCb,   // MT_ZDO_END_DEVICE_ANNCE_IND
        NULL,        // MT_ZDO_SRC_RTG_IND
        NULL,	 //MT_ZDO_BEACON_NOTIFY_IND
        NULL,			 //MT_ZDO_JOIN_CNF
        NULL,	 //MT_ZDO_NWK_DISCOVERY_CNF
        NULL,                    // MT_ZDO_CONCENTRATOR_IND_CB
        NULL,         // MT_ZDO_LEAVE_IND
        mtZdoTcEndDeviceAnnceIndCb,   // MT_ZDO_TC_END_DEVICE_ANNCE_IND
        NULL,   //MT_ZDO_PERMIT_JOIN_IND
        NULL,   //MT_ZDO_STATUS_ERROR_RSP
        NULL,  //MT_ZDO_MATCH_DESC_RSP_SENT
        NULL,
        NULL };

//! \brief SampleDesc does not contain, define an array
//!  for storing IEEE from devAnnce
//!
typedef struct IeeeMapping
{
    uint8_t ieee[8];
    uint16_t nwkAddr;
} IeeeMapping_t;

#define IEEE_MAX 100
static IeeeMapping_t ieeeMapping[IEEE_MAX] = {{ 0 }};
static uint8_t ieeeMappingIdx = 0;


epInfo_t epInfo;

/********************************************************************
 * START OF SYS CALL BACK FUNCTIONS
 */

static uint_least8_t mtSysResetIndCb(ResetIndFormat_t *msg)
{
    dbg_print(PRINT_LEVEL_INFO, "ZNP Version: %d.%d.%d\n", msg->MajorRel, msg->MinorRel,
            msg->HwRev);
    znpHasReset = true;
    return 0;
}

int gotNVResponse = false;
nvRead_response *nvReadResponse;

static uint_least8_t mtSysOsalNvReadCb(OsalNvReadSrspFormat_t *rsp)
{
    // zWNVItemResponse(&nvReadResponse);
    
    // printf("***************************************************\n");
    // printf("Mt sys osal nv read status- %d len- %d Data- ", rsp->Status, rsp->Len);
    // int i = 0;
    // for(i = 0; i < rsp->Len; i++) {
    //     printf("%d ", rsp->Value[i]);
    // }
    // printf("\n");
    // printf("***************************************************\n");

    nvReadResponse = (nvRead_response*)malloc(sizeof(nvRead_response));
    nvReadResponse->status = rsp->Status;
    nvReadResponse->len = rsp->Len;
    memcpy(nvReadResponse->data, rsp->Value, rsp->Len * sizeof(uint8_t));
    // nvReadResponse->data = rsp->Value;

    gotNVResponse = true;
    // free(nvReadResponse);
    return 0;
}

/********************************************************************
 * START OF ZDO CALL BACK FUNCTIONS
 */

//! \brief Callback function for ZDO State Change Indication
//! \param[in]      zdoState - network state
//! \return         status
static uint_least8_t mtZdoStateChangeIndCb(uint_least8_t newDevState)
{

    switch (newDevState)
    {
    case DEV_HOLD:
        dbg_print(PRINT_LEVEL_INFO,
                "mtZdoStateChangeIndCb: Initialized - not started automatically\n");
        break;
    case DEV_INIT:
        dbg_print(PRINT_LEVEL_INFO,
                "mtZdoStateChangeIndCb: Initialized - not connected to anything\n");
        break;
    case DEV_NWK_DISC:
        dbg_print(PRINT_LEVEL_INFO,
                "mtZdoStateChangeIndCb: Discovering PAN's to join\n");
        dbg_print(PRINT_LEVEL_INFO, "Network Discovering\n");
        break;
    case DEV_NWK_JOINING:
        dbg_print(PRINT_LEVEL_INFO, "mtZdoStateChangeIndCb: Joining a PAN\n");
        dbg_print(PRINT_LEVEL_INFO, "Network Joining\n");
        break;
    case DEV_NWK_REJOIN:
        dbg_print(PRINT_LEVEL_INFO,
                "mtZdoStateChangeIndCb: ReJoining a PAN, only for end devices\n");
        dbg_print(PRINT_LEVEL_INFO, "Network Rejoining\n");
        break;
    case DEV_END_DEVICE_UNAUTH:
        dbg_print(PRINT_LEVEL_INFO, "Network Authenticating\n");
        dbg_print(PRINT_LEVEL_INFO,
                "mtZdoStateChangeIndCb: Joined but not yet authenticated by trust center\n");
        break;
    case DEV_END_DEVICE:
        dbg_print(PRINT_LEVEL_INFO, "Network Joined\n");
        dbg_print(PRINT_LEVEL_INFO,
                "mtZdoStateChangeIndCb: Started as device after authentication\n");
        break;
    case DEV_ROUTER:
        dbg_print(PRINT_LEVEL_INFO, "Network Joined\n");
        dbg_print(PRINT_LEVEL_INFO,
                "mtZdoStateChangeIndCb: Device joined, authenticated and is a router\n");
        break;
    case DEV_COORD_STARTING:
        dbg_print(PRINT_LEVEL_INFO, "Network Starting\n");
        dbg_print(PRINT_LEVEL_INFO,
                "mtZdoStateChangeIndCb: Started as Zigbee Coordinator\n");
        break;
    case DEV_ZB_COORD:
        dbg_print(PRINT_LEVEL_INFO, "Network Started\n");
        dbg_print(PRINT_LEVEL_INFO,
                "mtZdoStateChangeIndCb: Started as Zigbee Coordinator\n");
        break;
    case DEV_NWK_ORPHAN:
        dbg_print(PRINT_LEVEL_INFO, "Network Orphaned\n");
        dbg_print(PRINT_LEVEL_INFO,
                "mtZdoStateChangeIndCb: Device has lost information about its parent\n");
        break;
    default:
        dbg_print(PRINT_LEVEL_INFO, "mtZdoStateChangeIndCb: unknown state");
        break;
    }

    devState = (devStates_t) newDevState;

    return SUCCESS;
}

Node_t nodeList[MAX_NODE_LIST];
uint8_t nodeCount = 0;

void printNodeTopology(uint8_t i) 
{
    // uint8_t i;
    // for (i = 0; i < nodeCount; i++)
    // {
        char *devtype = (nodeList[i].Type == DEVICETYPE_ROUTER ? "ROUTER" : "END DEVICE");
        if (nodeList[i].Type == DEVICETYPE_COORDINATOR)
        {
            devtype = "COORDINATOR";
        }
        dbg_print(PRINT_LEVEL_INFO, "Node Address: 0x%04X   Type: %s\n", nodeList[i].NodeAddr, devtype);
        dbg_print(PRINT_LEVEL_INFO, "Children: %d\n", nodeList[i].ChildCount);
        uint8_t cI;
        for (cI = 0; cI < nodeList[i].ChildCount; cI++)
        {
            uint8_t type = nodeList[i].childs[cI].Type;
            dbg_print(PRINT_LEVEL_INFO, "\tAddress: 0x%04X   Type: %s\n", nodeList[i].childs[cI].ChildAddr, (type == DEVICETYPE_ROUTER ? "ROUTER" : "END DEVICE"));
        }
        dbg_print(PRINT_LEVEL_INFO, "\n");
    // }
}

static uint_least8_t mtZdoMgmtLqiRspCb(MgmtLqiRspFormat_t *msg)
{
    dbg_print(PRINT_LEVEL_VERBOSE, "in lqi response callback\n");
    uint8_t devType = 0;
    uint8_t devRelation = 0;
    uint8_t localNodeCount = nodeCount;
    MgmtLqiReqFormat_t req;

    if (msg->Status == MT_RPC_SUCCESS)
    {
        nodeCount++;
        nodeList[localNodeCount].NodeAddr = msg->SrcAddr;
        nodeList[localNodeCount].Type = (msg->SrcAddr == 0 ? DEVICETYPE_COORDINATOR : DEVICETYPE_ROUTER);
        nodeList[localNodeCount].ChildCount = 0;

        uint32_t i;
        for (i = 0; i < msg->NeighborLqiListCount; i++)
        {
            devType = msg->NeighborLqiList[i].DevTyp_RxOnWhenIdle_Relat & 3;
            devRelation = ((msg->NeighborLqiList[i].DevTyp_RxOnWhenIdle_Relat >> 4) & 7);
            if (devRelation == 1)
            {
                uint8_t cCount = nodeList[localNodeCount].ChildCount;
                nodeList[localNodeCount].childs[cCount].ChildAddr = msg->NeighborLqiList[i].NetworkAddress;
                nodeList[localNodeCount].childs[cCount].Type = devType;
                nodeList[localNodeCount].childs[cCount].Lqi = msg->NeighborLqiList[i].LQI;
                uint8_t j;
                for(j = 0; j < 8; j++)
                    nodeList[localNodeCount].childs[cCount].ExtendedAddress[j] = (uint8_t)(msg->NeighborLqiList[i].ExtendedAddress >> (j * 8)) & 0xFF;
                nodeList[localNodeCount].ChildCount++;
                if (devType == DEVICETYPE_ROUTER)
                {
                    req.DstAddr = msg->NeighborLqiList[i].NetworkAddress;
                    req.StartIndex = 0;
                    zdoMgmtLqiReq(&req);
                }
            }
        }
    }
    else
    {
        dbg_print(PRINT_LEVEL_INFO, "MgmtLqiRsp Status: FAIL 0x%02X\n", msg->Status);
    }

    zWUpdateNetworkTopology(&nodeList[localNodeCount]);

    //printNodeTopology(localNodeCount);
    return msg->Status;
}

void resetNodeTopology()
{
    nodeCount = 0;
    memset(nodeList, 0, sizeof(nodeList));
}

//#define USE_TC_DEV_ANNCE
//Use the trust center device announce as this is a reliable message and not a broadcast
static uint_least8_t mtZdoEndDeviceAnnceIndCb(EndDeviceAnnceIndFormat_t *msg)
{
    zWDeviceJoinedNetwork(msg);
	dbg_print(PRINT_LEVEL_WARNING,"New device joined network.NwkAddr: 0x%04X\n", msg->NwkAddr);
#ifndef USE_TC_DEV_ANNCE
    ActiveEpReqFormat_t actReq;
    actReq.DstAddr = msg->NwkAddr;
    actReq.NwkAddrOfInterest = msg->NwkAddr;
    dbg_print(PRINT_LEVEL_WARNING,"New device joined the Trust Center.NwkAddr: 0x%04X\n", msg->NwkAddr);
    zdoActiveEpReq(&actReq);

    //Store the IEEE addr and nwkAddr for SimpleDesc
    // ieeeMapping[ieeeMappingIdx].nwkAddr = msg->NwkAddr;
    // memcpy(ieeeMapping[ieeeMappingIdx].ieee, (msg->IEEEAddr), 8);
    // ieeeMappingIdx++;
    // if (ieeeMappingIdx == IEEE_MAX)
    // {
    //     ieeeMappingIdx = 0;
    // }
#endif //!USE_TC_DEV_ANNCE
	return 0;
}

static uint_least8_t mtZdoTcEndDeviceAnnceIndCb(TcEndDeviceAnnceIndFormat_t *msg)
{
#ifdef USE_TC_DEV_ANNCE
    ActiveEpReqFormat_t actReq;
    actReq.DstAddr = msg->NwkAddr;
    actReq.NwkAddrOfInterest = msg->NwkAddr;
    dbg_print(PRINT_LEVEL_WARNING,"TC Annce- New device joined the Trust Center.NwkAddr: 0x%04X\n", msg->NwkAddr);
    zdoActiveEpReq(&actReq);

    //Store the IEEE addr and nwkAddr for SimpleDesc
    // ieeeMapping[ieeeMappingIdx].nwkAddr = msg->NwkAddr;
    // memcpy(ieeeMapping[ieeeMappingIdx].ieee, (msg->IEEEAddr), 8);
    // ieeeMappingIdx++;
    // if (ieeeMappingIdx == IEEE_MAX)
    // {
    //     ieeeMappingIdx = 0;
    // }
#endif //USE_TC_DEV_ANNCE
    return 0;
}

static uint_least8_t mtZdoActiveEpRspCb(ActiveEpRspFormat_t *msg)
{

    SimpleDescReqFormat_t simReq;
    if (msg->Status == MT_RPC_SUCCESS)
    {
        simReq.DstAddr = msg->NwkAddr;
        simReq.NwkAddrOfInterest = msg->NwkAddr;
        dbg_print(PRINT_LEVEL_WARNING,"Number of Endpoints: %d\n", msg->ActiveEPCount);
        uint32_t i;
        for (i = 0; i < msg->ActiveEPCount; i++)
        {
            simReq.Endpoint = msg->ActiveEPList[i];
            zdoSimpleDescReq(&simReq);
        }
    } else
    {
        dbg_print(PRINT_LEVEL_WARNING,"ActiveEpRsp Status: FAIL 0x%02X\n", msg->Status);
    }

    return msg->Status;
}

static uint_least8_t mtZdoSimpleDescRspCb(SimpleDescRspFormat_t *msg)
{

    if (msg->Status == MT_RPC_SUCCESS)
    {
        dbg_print(PRINT_LEVEL_WARNING,"\tEndpoint: 0x%02X\n", msg->Endpoint);
        dbg_print(PRINT_LEVEL_WARNING,"\tProfileID: 0x%04X\n", msg->ProfileID);
        dbg_print(PRINT_LEVEL_WARNING,"\tDeviceID: 0x%04X\n", msg->DeviceID);
        dbg_print(PRINT_LEVEL_WARNING,"\tDeviceVersion: 0x%02X\n", msg->DeviceVersion);
        dbg_print(PRINT_LEVEL_WARNING,"\tNumInClusters: %d\n", msg->NumInClusters);
        uint32_t i;
        for (i = 0; i < msg->NumInClusters; i++)
        {
            dbg_print(PRINT_LEVEL_WARNING,"\t\tInClusterList[%d]: 0x%04X\n", i, msg->InClusterList[i]);
        }
        dbg_print(PRINT_LEVEL_WARNING,"\tNumOutClusters: %d\n", msg->NumOutClusters);
        for (i = 0; i < msg->NumOutClusters; i++)
        {
            dbg_print(PRINT_LEVEL_WARNING,"\t\tOutClusterList[%d]: 0x%04X\n", i,
                    msg->OutClusterList[i]);
        }
        dbg_print(PRINT_LEVEL_WARNING,"\n");

        if (zMngt_callbacks.pfnZdoSimpleDescRspCb != NULL)
        {
            uint_least8_t ieeeIdx;

            epInfo.srcAddr = msg->SrcAddr;
            epInfo.deviceID = msg->DeviceID;
            epInfo.profileID = msg->ProfileID;
            epInfo.nwkAddr = msg->NwkAddr;
            epInfo.endpoint = msg->Endpoint;
            epInfo.status = msg->Status;

            epInfo.numInClusters = msg->NumInClusters;
            uint32_t i;
            for (i = 0; i < msg->NumInClusters; i++)
            {
                epInfo.inClusterList[i] = msg->InClusterList[i];
            }
            epInfo.numOutClusters = msg->NumOutClusters;
            for (i = 0; i < msg->NumOutClusters; i++)
            {
                epInfo.outClusterList[i] = msg->OutClusterList[i];
            }

            //retrieve ieee addr
            // for (ieeeIdx = 0; ieeeIdx < IEEE_MAX; ieeeIdx++)
            // {
            //     if (epInfo.nwkAddr == ieeeMapping[ieeeIdx].nwkAddr)
            //     {
            //         memcpy(epInfo.IEEEAddr, ieeeMapping[ieeeIdx].ieee, 8);

            //         for (i = 0; i < 8; i++)
            //         {
            //             dbg_print(PRINT_LEVEL_WARNING,"\t\tIEEEAddr[%d]: 0x%02X\n", i,
            //                     epInfo.IEEEAddr[i]);
            //         }
            //         dbg_print(PRINT_LEVEL_WARNING,"\n");
            //         break;
            //     }
            // }

            //if this is the TL endpoint OR if it is an EP with no cluster then ignore it
            if (!((epInfo.profileID == 0xC05E) && (msg->NumInClusters == 1)
                    && (msg->NumOutClusters == 1))
                    && ((msg->NumInClusters > 0) || (msg->NumOutClusters > 0)))
            {
                zMngt_callbacks.pfnZdoSimpleDescRspCb(&epInfo);
            } else {
                dbg_print(PRINT_LEVEL_INFO, "WARNING: check mtZdoSimpleDescRspCb(), about to drop ZDO reponse from the end-device\n");
                zMngt_callbacks.pfnZdoSimpleDescRspCb(&epInfo);
            }
        }
    } else
    {
        dbg_print(PRINT_LEVEL_INFO, "SimpleDescRsp Status: FAIL 0x%02X\n", msg->Status);
    }

    return msg->Status;
}

static uint_least8_t mtZdoMgmtLeaveRspCb(MgmtLeaveRspFormat_t *msg)
{
    uint_least8_t ieeeIdx;
    //remove device from devvList

    //remove device from ieee addr mapping
    // for (ieeeIdx = 0; ieeeIdx < IEEE_MAX; ieeeIdx++)
    // {
    //     if (msg->SrcAddr == ieeeMapping[ieeeIdx].nwkAddr)
    //     {
    //         ieeeMapping[ieeeIdx].nwkAddr = 0;
    //         memset(ieeeMapping[ieeeIdx].ieee, 0, 8);
    //         break;
    //     }
    // }

    //TODO: forward on the leave to next higher layer

    return 0;
}

//! \brief Callback function for ZDO Match Desc Rsp
//! \param[in]      rsp - match desc response
//! \return         status
static uint_least8_t mtZdoMatchDescRsp(MatchDescRspFormat_t *rsp)
{

    dbg_print(PRINT_LEVEL_WARNING,"mtZdoMatchDescRsp++\n");
    if (rsp->Status == SUCCESS)
    {
        while ((rsp->MatchLength > 0) && (matchNumAddrs < maxMatcheAddrs))
        {
            //Parse from end of list
            rsp->MatchLength--;
            //get the nwk and ep(s) of matching device
            matchDstAddrTbl[matchNumAddrs].addr.shortAddr = rsp->NwkAddr;
            matchDstAddrTbl[matchNumAddrs].addrMode = afAddr16Bit;
            matchDstAddrTbl[matchNumAddrs].endPoint =
                    rsp->MatchList[rsp->MatchLength];

            dbg_print(PRINT_LEVEL_WARNING,"mtZdoMatchDescRsp: %x:%x:%x\n",
                    matchDstAddrTbl[matchNumAddrs].addr.shortAddr,
                    rsp->MatchList[rsp->MatchLength],
                    matchDstAddrTbl[matchNumAddrs].addrMode);
            matchNumAddrs++;
        }
    }

    return SUCCESS;
}

void* zMngt_getNVItem(uint16_t id) {
    uint_least8_t status;
    OsalNvReadFormat_t nvRead;

    // dbg_print(PRINT_LEVEL_INFO, "\n");
    dbg_print(PRINT_LEVEL_INFO, "NV Read NV Item id sending... %d\n", id);

    nvRead.Id = id;
    nvRead.Offset = 0;
    gotNVResponse = false;
    status = sysOsalNvRead(&nvRead);

    // printf("GOT STATUS %d\n", status);
    if(!gotNVResponse || status) {
        nvReadResponse = (nvRead_response*)malloc(sizeof(nvRead_response));
        nvReadResponse->status = 2; //Error
        nvReadResponse->len = 0;
    }
    //status = sysNvWrite(ZCD_NV_PANID, 0, pbuf, 2);
    // dbg_print(PRINT_LEVEL_INFO, "\n");
    dbg_print(PRINT_LEVEL_INFO, "NV Read NV Item sent...[%d]\n", status);

    return (void*)nvReadResponse;
}

uint8_t zMngt_setNVItem(uint16_t id, uint8_t len, uint8_t *value) {
    uint_least8_t status;
    OsalNvWriteFormat_t nvWrite;
    // setting dev type
    nvWrite.Id = id;
    nvWrite.Offset = 0;
    nvWrite.Len = len;
    int i = 0;
    for(i = 0; i < len; i++) {
        nvWrite.Value[i] = value[i];
    }
    status = sysOsalNvWrite(&nvWrite);
    // dbg_print(PRINT_LEVEL_INFO, "\n");
    dbg_print(PRINT_LEVEL_INFO, "NV Write Device Type cmd sent... [%d]\n", status);

    return status;
}

static uint_least8_t getNVPanID()
{
    uint_least8_t status;
    OsalNvReadFormat_t nvRead;

    // dbg_print(PRINT_LEVEL_INFO, "\n");
    dbg_print(PRINT_LEVEL_INFO, "NV Read PAN ID cmd sending...\n");

    nvRead.Id = ZCD_NV_PANID;
    nvRead.Offset = 0;
    status = sysOsalNvRead(&nvRead);
    //status = sysNvWrite(ZCD_NV_PANID, 0, pbuf, 2);
    // dbg_print(PRINT_LEVEL_INFO, "\n");
    dbg_print(PRINT_LEVEL_INFO, "NV Read PAN ID cmd sent...[%d]\n", status);

    return status;
}

static uint_least8_t getNVChanList()
{
    uint_least8_t status;
    OsalNvReadFormat_t nvRead;

    // dbg_print(PRINT_LEVEL_INFO, "\n");
    dbg_print(PRINT_LEVEL_INFO, "NV Read Channel List cmd sending...\n");

    nvRead.Id = ZCD_NV_CHANLIST;
    nvRead.Offset = 0;
    status = sysOsalNvRead(&nvRead);
    //status = sysNvWrite(ZCD_NV_PANID, 0, pbuf, 2);
    // dbg_print(PRINT_LEVEL_INFO, "\n");
    dbg_print(PRINT_LEVEL_INFO, "NV Read Channel List cmd sent...[%d]\n", status);

    return status;
}
//! helper functions for building and sending the NV messages
//!

static uint_least8_t setNVStartNew(void)
{
    uint_least8_t status;
    OsalNvWriteFormat_t nvWrite;
    // sending startup option

    nvWrite.Id = ZCD_NV_STARTUP_OPTION;
    nvWrite.Offset = 0;
    nvWrite.Len = 1;
    nvWrite.Value[0] = ZCD_STARTOPT_CLEAR_STATE | ZCD_STARTOPT_CLEAR_CONFIG;
    status = sysOsalNvWrite(&nvWrite);
    // dbg_print(PRINT_LEVEL_INFO, "\n");

    dbg_print(PRINT_LEVEL_INFO, "NV Write Startup Option cmd sent[%d]...\n",
            status);

    return status;
}

static uint_least8_t setNVStartRestore(void)
{
    uint_least8_t status;
    OsalNvWriteFormat_t nvWrite;
    // sending startup option

    nvWrite.Id = ZCD_NV_STARTUP_OPTION;
    nvWrite.Offset = 0;
    nvWrite.Len = 1;
    nvWrite.Value[0] = 0;
    status = sysOsalNvWrite(&nvWrite);
    // dbg_print(PRINT_LEVEL_INFO, "\n");

    dbg_print(PRINT_LEVEL_INFO, "NV Write Startup Option cmd sent[%d]...\n",
            status);

    return status;
}

static uint_least8_t setNVDevType(uint_least8_t devType)
{
    uint_least8_t status;
    OsalNvWriteFormat_t nvWrite;
    //uint_least8_t pbuf[1];
    // setting dev type
    nvWrite.Id = ZCD_NV_LOGICAL_TYPE;
    nvWrite.Offset = 0;
    nvWrite.Len = 1;
    nvWrite.Value[0] = devType;
    status = sysOsalNvWrite(&nvWrite);
    //pbuf[0] = devType;
    //status = sysNvWrite(ZCD_NV_LOGICAL_TYPE, 0, pbuf, 1);
    // dbg_print(PRINT_LEVEL_INFO, "\n");
    dbg_print(PRINT_LEVEL_INFO, "NV Write Device Type cmd sent... [%d]\n",
            status);

    return status;
}

static uint_least8_t setNVPanID(uint_least32_t panId)
{
    uint_least8_t status;
    OsalNvWriteFormat_t nvWrite;

    // dbg_print(PRINT_LEVEL_INFO, "\n");
    dbg_print(PRINT_LEVEL_INFO, "NV Write PAN ID cmd sending...\n");

    nvWrite.Id = ZCD_NV_PANID;
    nvWrite.Offset = 0;
    nvWrite.Len = 2;
    nvWrite.Value[0] = LO_UINT16(panId);
    nvWrite.Value[1] = HI_UINT16(panId);
    status = sysOsalNvWrite(&nvWrite);
    //status = sysNvWrite(ZCD_NV_PANID, 0, pbuf, 2);
    // dbg_print(PRINT_LEVEL_INFO, "\n");
    dbg_print(PRINT_LEVEL_INFO, "NV Write PAN ID cmd sent...[%d]\n", status);

    return status;
}

static uint_least8_t setNVChanList(uint_least32_t chanList)
{
    OsalNvWriteFormat_t nvWrite;
    uint_least8_t status;
    // setting chanList
    nvWrite.Id = ZCD_NV_CHANLIST;
    nvWrite.Offset = 0;
    nvWrite.Len = 4;
    nvWrite.Value[0] = BREAK_UINT32(chanList, 0);
    nvWrite.Value[1] = BREAK_UINT32(chanList, 1);
    nvWrite.Value[2] = BREAK_UINT32(chanList, 2);
    nvWrite.Value[3] = BREAK_UINT32(chanList, 3);
    status = sysOsalNvWrite(&nvWrite);
    //status = sysNvWrite(ZCD_NV_CHANLIST, 0, pbuf, 4);
    // dbg_print(PRINT_LEVEL_INFO, "\n");
    dbg_print(PRINT_LEVEL_INFO, "NV Write Channel List cmd sent...[%d]\n",
            status);

    return status;
}

static uint_least8_t startNetwork(uint_least8_t devType, uint_least32_t chan, uint8_t newNwk, uint16_t panId)
{
    uint_least8_t status;
    int32_t rpcStatus = 0;

    if(newNwk) {

        dbg_print(PRINT_LEVEL_INFO, "CLEARING NETWORK AGAIN...\n");
        status = zMngt_clearNetwork(true);
        if (status != MT_RPC_SUCCESS)
        {
            dbg_print(PRINT_LEVEL_WARNING, "setNVStartNew failed\n");
            return 0;
        }

        dbg_print(PRINT_LEVEL_INFO, "DEVICE TYPE: %d\n", devType);
        status = setNVDevType(devType);

        if (status != MT_RPC_SUCCESS)
        {
            dbg_print(PRINT_LEVEL_WARNING, "setNVDevType failed\n");
            return 0;
        }
        //Select random PAN ID for Coord and join any PAN for RTR/ED
        dbg_print(PRINT_LEVEL_INFO, "PAN ID: %d\n", panId);
        status = setNVPanID(panId);
        if (status != MT_RPC_SUCCESS)
        {
            dbg_print(PRINT_LEVEL_WARNING, "setNVPanID failed\n");
            return 0;
        }

        // status = getNVPanID();
        // if (status != MT_RPC_SUCCESS)
        // {
        //     dbg_print(PRINT_LEVEL_WARNING, "getNVPanID failed\n");
        //     return 0;
        // } else {
        //     dbg_print(PRINT_LEVEL_INFO, "getNVPanID successful\n");
        // }

        //Sett channel 11-25
        dbg_print(PRINT_LEVEL_INFO, "CHANNEL: %d\n", chan);
        status = setNVChanList(1 << chan);
        if (status != MT_RPC_SUCCESS)
        {
            dbg_print(PRINT_LEVEL_WARNING, "setNVPanID failed\n");
            return 0;
        }

        // status = getNVChanList();
        // if (status != MT_RPC_SUCCESS)
        // {
        //     dbg_print(PRINT_LEVEL_WARNING, "getNVChanList failed\n");
        //     return 0;
        // } else {
        //     dbg_print(PRINT_LEVEL_INFO, "getNVChanList successful\n");
        // }
    }

    status = zdoInit();
    if (status == NEW_NETWORK)
    {
        dbg_print(PRINT_LEVEL_WARNING, "zdoInit NEW_NETWORK\n");
        status = MT_RPC_SUCCESS;
    } else if (status == RESTORED_NETWORK)
    {
        dbg_print(PRINT_LEVEL_WARNING, "zdoInit RESTORE_NETWORK\n");
        status = MT_RPC_SUCCESS;
    } else
    {
        dbg_print(PRINT_LEVEL_WARNING, "zdoInit failed\n");
        status = MT_RPC_ERR_SUBSYSTEM;
    }

    dbg_print(PRINT_LEVEL_WARNING,"process zdoStatechange callbacks\n");

    //flush AREQ ZDO State Change messages
	while (rpcStatus != -1)
	{
		rpcStatus = rpcWaitMqClientMsg(5000);

		if (((devType == DEVICETYPE_COORDINATOR) && (devState == DEV_ZB_COORD))
		        || ((devType == DEVICETYPE_ROUTER) && (devState == DEV_ROUTER))
		        || ((devType == DEVICETYPE_ENDDEVICE)
		                && (devState == DEV_END_DEVICE)))
		{
			break;
		}
	}

/*
    while (rpcStatus != -1)
    {
        rpcStatus = rpcWaitMqClientMsg(1000);
        printf("rpcWaitMqClientMsg returned %x\n", rpcStatus);

        if ( ((devType == DEVICETYPE_COORDINATOR)
                      && (devState == DEV_ZB_COORD)) ||
             ((devType == DEVICETYPE_ROUTER)
            		  && (devState == DEV_ROUTER)) ||
             ((devType == DEVICETYPE_ENDDEVICE)
                      && (devState == DEV_END_DEVICE)) )
        {
        	//Set ZNP to restore the network on next boot
            status = setNVStartRestore();
            if (status != MT_RPC_SUCCESS)
            {
                dbg_print(PRINT_LEVEL_WARNING, "Restore nwk option failed\n");
                printf("Restore nwk option failed %x\n", status);
                return MT_RPC_ERR_SUBSYSTEM;
            }
            break;
        }
    }
*/

    //set startup option back to keep configuration in case of reset
    //zMngt_restoreNetwork();
    
    if (devState < DEV_END_DEVICE)
    {
        //start network failed
        status = MT_RPC_ERR_SUBSYSTEM;
        dbg_print(PRINT_LEVEL_INFO, "devState < DEV_END_DEVICE %x\n", devState);
    }

    return status;
}

static uint_least8_t znpReset(void)
{
    znpHasReset = false;
    //Reset ZNP
    while (znpHasReset == false)
    {
        //Resetting ZNP
        ResetReqFormat_t resReq;
        resReq.Type = 1;
        sysResetReq(&resReq);

        //wait for reset ind
        while (znpHasReset == false)
        {
            //time out after 5*500ms
            uint_least8_t to = 5;
            //flush the Reset Ind
            rpcWaitMqClientMsg(500);
            if (--to == 0)
            {
                dbg_print(PRINT_LEVEL_ERROR,
                        "zMngt_start: Timed out waiting for reset\n");
                return MT_RPC_ERR_SUBSYSTEM;
            }
        }
    }

    return MT_RPC_SUCCESS;
}

/*********************************************************************
 * INTERFACE FUNCTIONS
 */

//! \brief          Initialises the ZNP management module
//! \param          none
//! \return        	status:
//                      SUCCESS
//                      FAILURE
uint_least8_t zMngt_init(void)
{
    //Register Callbacks MT system callbacks
    sysRegisterCallbacks(mtSysCb);
    zdoRegisterCallbacks(mtZdoCb);

    return 0;
}

//! \brief          module management API: register application callbacks
//! \param[in]      zMngtCallbacks: callback table
//! \return        	none
void zMngt_registerCallbacks(zMngt_callbacks_t zMngtCallbacks)
{
    //copy the callback function pointers
    memcpy(&zMngt_callbacks, &zMngtCallbacks, sizeof(zMngt_callbacks_t));
    return;
}

//! \brief          Starts or joins network
//! \param[in]      devType: Deviace type (enum)
//!                     DEVICETYPE_COORDINATOR
//!                     DEVICETYPE_ROUTER
//!                     DEVICETYPE_ENDDEVICE
//! \param[in]      Chan: Channel Mask (bits 11-25 represent channel scanned)
//! \return        	status:
//                      SUCCESS
//                      FAILURE
uint_least8_t zMngt_start(uint_least8_t devType, uint_least32_t chan, uint8_t newNwk, uint16_t panId)
{
    uint_least8_t status = MT_RPC_ERR_SUBSYSTEM;
    int_least8_t rpcStatus = 0;

    //Flush all messages from the que
    while (rpcStatus != -1)
    {
        rpcStatus = rpcWaitMqClientMsg(50);
    }

    status = znpReset();
    if(status == MT_RPC_SUCCESS)
    {
        dbg_print(PRINT_LEVEL_INFO, "ZNP Reset\n");
    } else
    {
        dbg_print(PRINT_LEVEL_INFO, "ZNP Reset error\n");
        return status;
    }

    //init variable
    devState = DEV_HOLD;

    status = startNetwork(devType, chan, newNwk, panId);
    if (status == MT_RPC_SUCCESS)
    {
        dbg_print(PRINT_LEVEL_INFO, "Network up\n");
    } else
    {
        dbg_print(PRINT_LEVEL_INFO, "Network Error\n");
        return status;
    }

    //Configure ZNP to send ZDO messages
    OsalNvWriteFormat_t nvWrite;
    nvWrite.Id = ZCD_NV_ZDO_DIRECT_CB;
    nvWrite.Offset = 0;
    nvWrite.Len = 1;
    nvWrite.Value[0] = 1;
    status = sysOsalNvWrite(&nvWrite);

    return status;
}

//! \brief          Open the network
//
// \param[in]       duration: time to open the network for (0 to close)
//! \return        	status:
//                      SUCCESS
//                      FAILURE
uint_least8_t zMngt_openNetwork(uint_least8_t duration)
{
    MgmtPermitJoinReqFormat_t req;
    uint_least8_t status;

    //open this device (coord)
    req.AddrMode = Addr16Bit;
    req.DstAddr = 0x0000;
    req.Duration = duration;
    req.TCSignificance = 1;

    status = zdoMgmtPermitJoinReq(&req);

    if (status != SUCCESS)
    {
        return status;
    }

    //open network
    req.AddrMode = AddrBroadcast;
    req.DstAddr = NWK_BROADCAST_SHORTADDR_DEVALL;
    req.Duration = duration;
    req.TCSignificance = 1;

    status = zdoMgmtPermitJoinReq(&req);

    return status;
}

//! \brief         Instruction ZNP on the next boot to clear network params
//!                and start a new network
//!
//! \param[in]      reset: True - reset ZNP for nwk clear to take effect
//!                        False - wait for next rest for nwk clear to take effect
//! \return         status:
//                      SUCCESS
//                      FAILURE
uint_least8_t zMngt_clearNetwork(uint_least8_t reset)
{
    uint_least8_t status = SUCCESS;

    status = setNVStartNew();

    if( reset && (status == MT_RPC_SUCCESS) )
    {
        status = znpReset();
        if(status == MT_RPC_SUCCESS)
        {
            dbg_print(PRINT_LEVEL_INFO, "ZNP Reset\n");
        } else
        {
            dbg_print(PRINT_LEVEL_INFO, "ZNP Reset error\n");
        }
    }

    return status;
}

//! \brief         Instruction ZNP on the next boot to restore network params
//!
//! \return         status:
//                      SUCCESS
//                      FAILURE
uint_least8_t zMngt_restoreNetwork(void)
{
    uint_least8_t status = SUCCESS;

    status = setNVStartRestore();

    return status;
}

//! \brief         MT_ZDO_TC_END_DEVICE_ANNCE_IND
//!
//! \return         status:
//                      SUCCESS
//                      FAILURE
uint_least8_t zMngt_endDeviceAnnouncement(EndDeviceAnnceIndFormat_t *msg)
{
    return mtZdoEndDeviceAnnceIndCb(msg);
}

//! \brief          Find device(s) with matching cluster
//
// \param[in]       ClusterId: Cluster to find matching devices for
// \param[in]       dstAddrTbl: Table to add matching devices to
// \param[in]       numMatches: Maximum devices to find
// \param[in]       timeout_ms: Multiple device could respopnd
//! \return        	status:
//                      SUCCESS
//                      FAILURE
uint_least8_t zMngt_findInMatch(uint_least16_t profileId,
        uint_least16_t clusterId, afAddrType_t dstAddrTbl[],
        uint_least8_t maxMatches)
{
    MatchDescReqFormat_t req;

    dbg_print(PRINT_LEVEL_INFO,"zmngt_findInMatch++\n");

    //braodcast to all devices
    req.DstAddr = 0xFFFF;
    req.NwkAddrOfInterest = 0xFFFF;
    req.ProfileID = profileId;
    req.InClusterList[0] = clusterId;
    req.NumInClusters = 1;
    req.NumOutClusters = 0;

    //set match table read for response
    maxMatcheAddrs = maxMatches;
    matchNumAddrs = 0;

    dbg_print(PRINT_LEVEL_INFO,"zdoMatchDescReq++\n");
    zdoMatchDescReq(&req);

    dbg_print(PRINT_LEVEL_INFO,"waiting for match\n");
    //Wait 3s for a match response
    while (matchNumAddrs < maxMatches)
    {
        static uint_least8_t to = 6;
        rpcWaitMqClientMsg(500);
        dbg_print(PRINT_LEVEL_INFO,"zmngt_findInMatch-- to\n");
        if (--to == 0)
        {
            //time out
            matchNumAddrs = 0;
            maxMatcheAddrs = 0;

            dbg_print(PRINT_LEVEL_INFO,"zmngt_findInMatch--: No match found\n");
            return FAILURE;
        }
    }

    memcpy(dstAddrTbl, matchDstAddrTbl, matchNumAddrs * sizeof(afAddrType_t));
    matchNumAddrs = 0;
    maxMatcheAddrs = 0;

    dbg_print(PRINT_LEVEL_INFO,"zmngt_findInMatch--: Match found [%x]:[%x]\n",
            dstAddrTbl[0].endPoint, matchDstAddrTbl[0].endPoint);

    return SUCCESS;
}
