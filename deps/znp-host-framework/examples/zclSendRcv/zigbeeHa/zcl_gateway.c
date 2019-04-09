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
 *  ======== zcl_gateway.c ========
 */

//*****************************************************************************
// Includes
//*****************************************************************************
#include <stdlib.h>
#include <string.h>

#include "hostConsole.h"

#include "zcl_port.h"
#include "zcl.h"
#include "zcl_general.h"
#include "zcl_lighting.h"
#include "zcl_hvac.h"
#include "zcl_ha.h"

#include "zcl_gateway.h"

#include "mtAf.h"
#include "dbgPrint.h"

#include "znp_cfuncs.h"

//*****************************************************************************
// Constants
//*****************************************************************************

#define ZSW_MAX_ATTRIBUTES    15

#define ZSW_DEVICE_VERSION    0
#define ZSW_HWVERSION         0
#define ZSW_ZCLVERSION        0

#define ZCL_HA_PROFILE_ID     0x0104

#define LIGHT_OFF             0x00
#define LIGHT_ON              0x01

#define ZSW_MAX_INCLUSTERS       2
#define ZSW_MAX_OUTCLUSTERS      1

//*****************************************************************************
// LOCAL VARIABLE
//*****************************************************************************

//! \brief Application's input and output clusters
//!
static uint_least16_t inputClusters[ZSW_MAX_INCLUSTERS] =
{
ZCL_CLUSTER_ID_GEN_BASIC, ZCL_CLUSTER_ID_GEN_IDENTIFY };
static uint_least16_t outputClusters[ZSW_MAX_OUTCLUSTERS] =
{
ZCL_CLUSTER_ID_GEN_ON_OFF };

//! \brief Attribute variables
//!
const uint_least8_t zswHWRevision = ZSW_HWVERSION;
const uint_least8_t zswZCLVersion = ZSW_ZCLVERSION;
const uint_least8_t zswManufacturerName[] =
{ 16, 'T', 'e', 'x', 'a', 's', 'I', 'n', 's', 't', 'r', 'u', 'm', 'e', 'n', 't',
        's' };
const uint_least8_t zswModelId[] =
{ 16, 'T', 'I', '0', '0', '0', '1', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
        ' ' };
const uint_least8_t zswDateCode[] =
{ 16, '2', '0', '0', '6', '0', '8', '3', '1', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
        ' ' };
const uint_least8_t zswPowerSource = POWER_SOURCE_MAINS_1_PHASE;

uint_least8_t zswLocationDescription[17] =
{ 16, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
        ' ' };
uint_least8_t zswPhysicalEnvironment = 0;
uint_least8_t zswDeviceEnable = DEVICE_ENABLED;

//! \brief On/Off cluster variables
//!
uint_least8_t zswOnOff = LIGHT_OFF;
uint_least8_t zswOnOffSwitchType = ON_OFF_SWITCH_TYPE_TOGGLE;
uint_least8_t zswOnOffSwitchActions = ON_OFF_SWITCH_ACTIONS_2; // Toggle -> Toggle

//! \brief Identify Cluster variables
//!
uint_least16_t zswIdentifyTime = 0;

//! \brief SimpleDesc for ZCL
//!
endPointDesc_t zswEpDesc =
{ 0 };
SimpleDescriptionFormat_t afSimpleDesc =
{ 0 };

//! \brief ZCL Callbacks
//!
zclGw_callbacks_t zclGw_callbacks;

//! \brief the flag used for waiting for a zcl response
//!
static bool waitZclGetRspFlag = TRUE;

//! \brief used to store zcl transaction sequence number
//! of expected response
//!
static uint8 waitZclGetRspTransId = 0;

//! \brief used to store zcl transaction sequence number
//!
static uint_least8_t zgwTransID = 0;

//*****************************************************************************
// Local Function Prototypes
//*****************************************************************************

//! \brief Register the Endpoint with ZCL
//!
static void regEndpoints(void);

//! \brief ZCL General Profile Callback table
//!
static zclGeneral_AppCallbacks_t cmdCallbacks =
{
        NULL,                                   // Basic Cluster Reset command
        NULL,                                   // Identify command
#ifdef ZCL_EZMODE
        NULL,                                 // Identify EZ-Mode Invoke command
        NULL,// Identify Update Commission State command
#endif
        NULL,                                 // Identify Trigger Effect command
        NULL,                                 // Identify Query Response command
        NULL,                                   // On/Off cluster commands
        NULL,                 // On/Off cluster enhanced command Off with Effect
        NULL,     // On/Off cluster enhanced command On with Recall Global Scene
        NULL,               // On/Off cluster enhanced command On with Timed Off
#ifdef ZCL_LEVEL_CTRL
        NULL,                             // Level Control Move to Level command
        NULL,                                   // Level Control Move command
        NULL,                                   // Level Control Step command
        NULL,                                   // Level Control Stop command
#endif
#ifdef ZCL_GROUPS
        NULL,                                   // Group Response commands
#endif
#ifdef ZCL_SCENES
        NULL,                                   // Scene Store Request command
        NULL,// Scene Recall Request command
        NULL,// Scene Response command
#endif
#ifdef ZCL_ALARMS
        NULL,                                   // Alarm (Response) commands
#endif
#ifdef SE_UK_EXT
        NULL,                                   // Get Event Log command
        NULL,// Publish Event Log command
#endif
        NULL,                                   // RSSI Location command
        NULL                                  // RSSI Location Response command
        };

//! \brief Attribute table register with ZCL. ZCL will handle attr read/writes based on this table.
//!
const zclAttrRec_t zswAttrs[ZSW_MAX_ATTRIBUTES] =
{
// *** General Basic Cluster Attributes ***
        { ZCL_CLUSTER_ID_GEN_BASIC, // Cluster IDs - defined in the foundation (ie. zcl.h)
                {  // Attribute record
                ATTRID_BASIC_HW_VERSION, // Attribute ID - Found in Cluster Library header (ie. zcl_general.h)
                        ZCL_DATATYPE_UINT8,        // Data Type - found in zcl.h
                        ACCESS_CONTROL_READ, // Variable access control - found in zcl.h
                        (void *) &zswHWRevision // Pointer to attribute variable
                } },
        { ZCL_CLUSTER_ID_GEN_BASIC,
        { // Attribute record
                ATTRID_BASIC_ZCL_VERSION, ZCL_DATATYPE_UINT8,
                ACCESS_CONTROL_READ, (void *) &zswZCLVersion } },
        {
        ZCL_CLUSTER_ID_GEN_BASIC,
        { // Attribute record
                ATTRID_BASIC_MANUFACTURER_NAME, ZCL_DATATYPE_CHAR_STR,
                ACCESS_CONTROL_READ, (void *) zswManufacturerName } },
        {
        ZCL_CLUSTER_ID_GEN_BASIC,
        { // Attribute record
                ATTRID_BASIC_MODEL_ID, ZCL_DATATYPE_CHAR_STR,
                ACCESS_CONTROL_READ, (void *) zswModelId } },
        {
        ZCL_CLUSTER_ID_GEN_BASIC,
        { // Attribute record
                ATTRID_BASIC_DATE_CODE, ZCL_DATATYPE_CHAR_STR,
                ACCESS_CONTROL_READ, (void *) zswDateCode } },
        {
        ZCL_CLUSTER_ID_GEN_BASIC,
        { // Attribute record
                ATTRID_BASIC_POWER_SOURCE, ZCL_DATATYPE_UINT8,
                ACCESS_CONTROL_READ, (void *) &zswPowerSource } },
        {
        ZCL_CLUSTER_ID_GEN_BASIC,
        { // Attribute record
                ATTRID_BASIC_LOCATION_DESC, ZCL_DATATYPE_CHAR_STR,
                        (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
                        (void *) zswLocationDescription } },
        {
        ZCL_CLUSTER_ID_GEN_BASIC,
        { // Attribute record
                ATTRID_BASIC_PHYSICAL_ENV, ZCL_DATATYPE_UINT8,
                        (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
                        (void *) &zswPhysicalEnvironment } },
        {
        ZCL_CLUSTER_ID_GEN_BASIC,
        { // Attribute record
                ATTRID_BASIC_DEVICE_ENABLED, ZCL_DATATYPE_UINT8,
                        (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
                        (void *) &zswDeviceEnable } },

        // *** Identify Cluster Attribute ***
        { ZCL_CLUSTER_ID_GEN_IDENTIFY,
        { // Attribute record
                ATTRID_IDENTIFY_TIME, ZCL_DATATYPE_UINT16, (ACCESS_CONTROL_READ
                        | ACCESS_CONTROL_WRITE), (void *) &zswIdentifyTime } },

        // *** On / Off Cluster Attributes ***
        { ZCL_CLUSTER_ID_GEN_ON_OFF,
        { // Attribute record
                ATTRID_ON_OFF, ZCL_DATATYPE_BOOLEAN, ACCESS_CONTROL_READ,
                        (void *) &zswOnOff } },

        // *** On / Off Switch Configuration Cluster *** //
        { ZCL_CLUSTER_ID_GEN_ON_OFF_SWITCH_CONFIG,
        { // Attribute record
                ATTRID_ON_OFF_SWITCH_TYPE, ZCL_DATATYPE_ENUM8,
                ACCESS_CONTROL_READ, (void *) &zswOnOffSwitchType } },
        {
        ZCL_CLUSTER_ID_GEN_ON_OFF_SWITCH_CONFIG,
        { // Attribute record
                ATTRID_ON_OFF_SWITCH_ACTIONS, ZCL_DATATYPE_ENUM8,
                ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE,
                        (void *) &zswOnOffSwitchActions } }, };

//! \brief Function for processing read attr response from remote read of attributes
//!
static void processZclReadAttributeRsp(afAddrType_t srcAddr, uint8_t zclTransId,
uint16_t clusterId, uint16_t payloadLen, uint8_t *pPayload);

//! \brief AfCallbacks for passing raw AF to ZCL for decoding
//!
static uint_least8_t mtAfDataConfirmCb(DataConfirmFormat_t *msg);
static uint_least8_t mtAfIncomingMsgCb(IncomingMsgFormat_t *msg);
static uint_least8_t mtAfIncomingMsgExtCb(IncomingMsgExtFormat_t *msg);
static mtAfCb_t mtAfCb =
{ mtAfDataConfirmCb,				//MT_AF_DATA_CONFIRM
        mtAfIncomingMsgCb,				//MT_AF_INCOMING_MSG
        mtAfIncomingMsgExtCb,				//MT_AF_INCOMING_MSG_EXT
        NULL,			//MT_AF_DATA_RETRIEVE
        NULL,			    //MT_AF_REFLECT_ERROR
        };

/********************************************************************
 * Local functions
 */

//! \brief AfCallback for handling incoming AF message sent with 16b short addr
//! \param[in]      pInMsg - incoming message
//! \return         status
static uint_least8_t mtAfIncomingMsgCb(IncomingMsgFormat_t *pInMsg)
{
    afIncomingMSGPacket_t afMsg;

    afMsg.groupId = pInMsg->GroupId;
    afMsg.clusterId = pInMsg->ClusterId;
    afMsg.srcAddr.endPoint = pInMsg->SrcEndpoint;
    afMsg.srcAddr.addrMode = afAddr16Bit;
    afMsg.srcAddr.addr.shortAddr = pInMsg->SrcAddr;
    afMsg.endPoint = pInMsg->DstEndpoint;
    afMsg.wasBroadcast = pInMsg->WasVroadcast;
    afMsg.LinkQuality = pInMsg->LinkQuality;
    afMsg.timestamp = pInMsg->TimeStamp;
    //afMsg.radius = pInMsg->radius;
    afMsg.cmd.TransSeqNumber = pInMsg->TransSeqNum;
    afMsg.cmd.DataLength = pInMsg->Len;
    afMsg.cmd.Data = pInMsg->Data;

    zcl_ProcessMessageMSG(&afMsg);

    return 0;
}

//! \brief AfCallback for handling incoming AF message sent with 64bit IEEE
//! addr
//! \param[in]      pInMsg - incoming message
//! \return         status
static uint_least8_t mtAfIncomingMsgExtCb(IncomingMsgExtFormat_t *pInMsg)
{

    afIncomingMSGPacket_t afMsg;

    afMsg.groupId = pInMsg->GroupId;
    afMsg.clusterId = pInMsg->ClusterId;
    afMsg.srcAddr.endPoint = pInMsg->SrcEndpoint;
    afMsg.srcAddr.addrMode = (afAddrMode_t) pInMsg->SrcAddrMode;
    if ((afMsg.srcAddr.addrMode == afAddr16Bit)
            || (afMsg.srcAddr.addrMode == afAddrGroup)
            || (afMsg.srcAddr.addrMode == afAddrBroadcast))
    {
        afMsg.srcAddr.addr.shortAddr = pInMsg->SrcAddr;
    } else if (afMsg.srcAddr.addrMode == afAddr64Bit)
    {
        memcpy(afMsg.srcAddr.addr.extAddr, &(pInMsg->SrcAddr), 8);
    }
    afMsg.endPoint = pInMsg->DstEndpoint;
    afMsg.wasBroadcast = pInMsg->WasVroadcast;
    afMsg.LinkQuality = pInMsg->LinkQuality;
    afMsg.timestamp = pInMsg->TimeStamp;
    //afMsg.radius = pInMsg->radius;
    afMsg.cmd.TransSeqNumber = pInMsg->TransSeqNum;
    afMsg.cmd.DataLength = pInMsg->Len;
    afMsg.cmd.Data = pInMsg->Data;

    zcl_ProcessMessageMSG(&afMsg);

    return 0;
}

//! \brief AfCallback for handling incoming AF confirm message,
//! indicating the result of an AfDataReq based, Base on APS Ack.
//! For messages without APS Ack this is based on MAC Ack.
//! addr
//! \param[in]      msg - data confirm msg
//! \return         status
static uint_least8_t mtAfDataConfirmCb(DataConfirmFormat_t *msg)
{
    if (msg->Status == MT_RPC_SUCCESS)
    {
        // dbg_print(PRINT_LEVEL_INFO, "TransId: %d\n", msg->TransId);
        // dbg_print(PRINT_LEVEL_INFO, "Endpoint: %d\n", msg->Endpoint);
        dbg_print(PRINT_LEVEL_INFO, "ZigBee: Message transmited Succesfully!!\n");
    } else
    {
        dbg_print(PRINT_LEVEL_INFO, "ZigBee: Message failed to transmit\n");
    }

    zWDataResponseConfirm(&msg->Status);
    
    return msg->Status;
}

//! \brief Register empoint with ZCL, used for by ZNP to sending
//!  match Desc and SimpleDesc when requested by a remote device
//! addr
//! \param          none
//! \return         none
static void regEndpoints(void)
{
    RegisterFormat_t regReq =
    { 0 };
    uint_least8_t status;

    // Register an Switch Endpoint
    regReq.EndPoint = ZGW_EP;
    regReq.AppProfId = ZCL_HA_PROFILE_ID;
    regReq.AppDeviceId = ZCL_HA_DEVICEID_ON_OFF_SWITCH;
    regReq.AppDevVer = ZSW_DEVICE_VERSION;
    regReq.AppNumInClusters = sizeof(inputClusters) / sizeof(uint_least16_t);
    memcpy(regReq.AppInClusterList, inputClusters,
            regReq.AppNumInClusters * sizeof(uint_least16_t));
    regReq.AppNumOutClusters = sizeof(outputClusters) / sizeof(uint_least16_t);
    memcpy(regReq.AppOutClusterList, outputClusters,
            regReq.AppNumOutClusters * sizeof(uint_least16_t));
    regReq.LatencyReq = NO_LATENCY_REQS;

    status = afRegister(&regReq);
    dbg_print(PRINT_LEVEL_INFO, "afRegister status: %x\n", status);

    /*build Simple Desc for ZCL*/
    // Initialize the Switch Simple Descriptor
    zswEpDesc.endPoint = ZGW_EP;
    afSimpleDesc.EndPoint = ZGW_EP;
    afSimpleDesc.AppProfId = ZCL_HA_PROFILE_ID;
    afSimpleDesc.AppDeviceId = ZCL_HA_DEVICEID_ON_OFF_SWITCH;
    afSimpleDesc.AppDevVer = ZSW_DEVICE_VERSION;
    afSimpleDesc.AppNumInClusters = sizeof(inputClusters)
            / sizeof(uint_least16_t);
    afSimpleDesc.pAppInClusterList = inputClusters;
    afSimpleDesc.AppNumOutClusters = sizeof(outputClusters)
            / sizeof(uint_least16_t);
    afSimpleDesc.pAppOutClusterList = outputClusters;
    zswEpDesc.simpleDesc = &afSimpleDesc;

}


int8_t waitZclGetRsp(void)
{
    uint_least8_t delayCnt = 0;
    int8_t rtn = 0;

    while ((delayCnt < 10) && (waitZclGetRspFlag))
    {
        //Flush RCP messages
        rpcWaitMqClientMsg(100);
        delayCnt++;
    }

    //did we get the response or are we still waiting
    if (waitZclGetRspFlag)
    {
        rtn = -1;
    }

    return rtn;
}

//! \brief Handles incoming ZCL response protobuf messages.
//! \param[in]      pSrcAddress - protobuf source address information
//! \param[in]      zclTransId - ZCL transaction ID
//! \param[in]      clusterId - attribute list cluster ID
//! \param[in]      payloadLen - length of pPayload
//! \param[in]      pPayload - APS payload from incoming message indication
//! \return         none

attr_response resp;
static void processZclReadAttributeRsp(afAddrType_t srcAddr, uint8_t zclTransId,
uint16_t clusterId, uint16_t payloadLen, uint8_t *pPayload)
{
    uint16_t attrId;

    resp.srcAddr = srcAddr.addr.shortAddr;
    resp.endPoint = srcAddr.endPoint;
    resp.addrMode = srcAddr.addrMode;
    resp.transId = zclTransId;
    resp.clusterId = clusterId;
    resp.payloadLen = payloadLen;

    int k = 0;
    for(k = 0; k < resp.payloadLen; k++) {
        resp.payload[k] = pPayload[k];
    }


    // resp.payload = pPayload;
    // if(payloadLen) {
    //     resp.payload = (uint8_t*) malloc(payloadLen * sizeof(uint8_t));
    //     memcpy(resp.payload, pPayload, payloadLen * sizeof(uint8_t));
    // }


    // printf("\t srcAddr: %d\n",             resp.srcAddr              );
    // printf("\t endPoint: %d\n",           resp.endPoint              );
    // printf("\t addrMode: %d\n",             resp.addrMode              );
    // printf("\t transId: %d\n",           resp.transId              );
    // printf("\t clusterId: %d\n",             resp.clusterId              );
    // printf("\t payloadLen: %d\n",           resp.payloadLen              );
    // printf("\t payload: ");
    // int i = 0;
    // for(i = 0; i < resp.payloadLen; i++) {
    //     printf("%d ", resp.payload[i]);
    // }
    // printf("\n");

    zWInformReadAttritubeRsp(&resp);
    //printf( "Processing Read Attribute Response:" );
    //printf( " zclTransId %d, clusterId %d, payloadlen %d\n",
    //          zclTransId, clusterId, payloadLen );

    //todo: Should we keep ther read req and look up trans ID?

    // Check if response to cluster specific command
    switch (clusterId)
    {
    case ZCL_CLUSTER_ID_GEN_BASIC:
        // printf("ZCL_CLUSTER_ID_GEN_BASIC\n");
        // printf( "Processing Get Basic State Response Indication\n" );

        attrId = BUILD_UINT16(pPayload[0], pPayload[1]);
        pPayload += 2;

        // printf("GOT ATTR ID: %d\n", attrId);
        // Verify attribute ID and read success
        if ((attrId == ATTRID_BASIC_MANUFACTURER_NAME) && (*pPayload == ZSuccess))
        {
            pPayload += 2;  // move pointer past status and data type fields

            // Send response back to app
            if (zclGw_callbacks.pfnZclGetInfoCb != NULL)
            {
                // dbg_print(PRINT_LEVEL_INFO, "Manu: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n", pPayload[0],pPayload[1],pPayload[2],pPayload[3],pPayload[4],pPayload[5],pPayload[6],pPayload[7],pPayload[8],pPayload[9]);
                zclGw_callbacks.pfnZclGetInfoCb(*pPayload,
                        srcAddr.addr.shortAddr, srcAddr.endPoint, zclTransId);
            }
        } else if ((attrId == ATTRID_BASIC_MODEL_ID) && (*pPayload == ZSuccess))
        {
            pPayload += 2;  // move pointer past status and data type fields

            // Send response back to app
            if (zclGw_callbacks.pfnZclGetInfoCb != NULL)
            {
                // dbg_print(PRINT_LEVEL_INFO, "Manu: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n", pPayload[0],pPayload[1],pPayload[2],pPayload[3],pPayload[4],pPayload[5],pPayload[6],pPayload[7],pPayload[8],pPayload[9]);
                zclGw_callbacks.pfnZclGetInfoCb(*pPayload,
                        srcAddr.addr.shortAddr, srcAddr.endPoint, zclTransId);
            }
            //status was error
        } else {

        }
        break;

    case ZCL_CLUSTER_ID_GEN_ON_OFF:
        //printf("ZCL_CLUSTER_ID_GEN_ON_OFF\n");
        //printf( "Processing Get On/Off State Response Indication\n" );

        attrId = BUILD_UINT16(pPayload[0], pPayload[1]);
        pPayload += 2;

        // Verify attribute ID and read success
        if ((attrId == ATTRID_ON_OFF) && (*pPayload == ZSuccess))
        {
            pPayload += 2;  // move pointer past status and data type fields

            // Send response back to app
            if (zclGw_callbacks.pfnZclGetStateCb != NULL)
            {
                zclGw_callbacks.pfnZclGetStateCb(*pPayload,
                        srcAddr.addr.shortAddr, srcAddr.endPoint, zclTransId);
            }
        } else
        {
            //status was error
        }
        break;

    case ZCL_CLUSTER_ID_GEN_LEVEL_CONTROL:
        //printf("processZclReadAttributeRsp: ZCL_CLUSTER_ID_GEN_LEVEL_CONTROL\n");

        attrId = BUILD_UINT16(pPayload[0], pPayload[1]);
        pPayload += 2;

        // Verify attribute ID and read success
        if ((attrId == ATTRID_LEVEL_CURRENT_LEVEL) && (*pPayload == ZSuccess))
        {
            pPayload += 2;  // move pointer past status and data type fields

            // Send response back to app
            if (zclGw_callbacks.pfnZclGetLevelCb != NULL)
            {
                zclGw_callbacks.pfnZclGetLevelCb(*pPayload,
                        srcAddr.addr.shortAddr, srcAddr.endPoint, zclTransId);
            }
        } else
        {
            //status was error
        }
        break;

    case ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL:
        //printf("processZclReadAttributeRsp: ZCL_CLUSTER_ID_GEN_LEVEL_CONTROL\n");

        attrId = BUILD_UINT16(pPayload[0], pPayload[1]);
        pPayload += 2;

        // Verify attribute ID and read success
        if ((attrId == ATTRID_LIGHTING_COLOR_CONTROL_CURRENT_HUE) && (*pPayload == ZSuccess))
        {
            pPayload += 2;  // move pointer past status and data type fields

            // Send response back to app
            if (zclGw_callbacks.pfnZclGetHueCb != NULL)
            {
                zclGw_callbacks.pfnZclGetHueCb(*pPayload,
                        srcAddr.addr.shortAddr, srcAddr.endPoint, zclTransId);
            }
        } else if ((attrId == ATTRID_LIGHTING_COLOR_CONTROL_CURRENT_SATURATION) && (*pPayload == ZSuccess))
        {
            pPayload += 2;  // move pointer past status and data type fields

            // Send response back to app
            if (zclGw_callbacks.pfnZclGetSatCb != NULL)
            {
                zclGw_callbacks.pfnZclGetSatCb(*pPayload,
                        srcAddr.addr.shortAddr, srcAddr.endPoint, zclTransId);
            }
        } else
        {
            //status was error
        }
        break;

    case ZCL_CLUSTER_ID_MS_TEMPERATURE_MEASUREMENT:
        dbg_print(PRINT_LEVEL_INFO, "ZCL_CLUSTER_ID_MS_TEMPERATURE_MEASUREMENT\n");
        break;

        /*case ZCL_CLUSTER_ID_SE_SIMPLE_METERING:
         printf("ZCL_CLUSTER_ID_SE_SIMPLE_METERING\n");
         break;*/

    case ZCL_CLUSTER_ID_MS_RELATIVE_HUMIDITY:
        dbg_print(PRINT_LEVEL_INFO, "ZCL_CLUSTER_ID_MS_RELATIVE_HUMIDITY\n");
        break;

    case ZCL_CLUSTER_ID_CLOSURES_DOOR_LOCK:
        dbg_print(PRINT_LEVEL_INFO, "ZCL_CLUSTER_ID_GEN_LEVEL_CONTROL\n");
        //processDevGetHumidityRspInd( pTransEntry->connection, &getHumidityRsp, payloadLen, pPayload );
        break;

    case ZCL_CLUSTER_ID_HVAC_THERMOSTAT:
        //printf("processZclReadAttributeRsp: ZCL_CLUSTER_ID_HVAC_THERMOSTAT\n");

        attrId = BUILD_UINT16(pPayload[0], pPayload[1]);
        pPayload += 2;

        // Verify attribute ID and read success
        if ((attrId == ATTRID_HVAC_THERMOSTAT_OCCUPIED_HEATING_SETPOINT) && (*pPayload == ZSuccess))
        {
            pPayload += 2;  // move pointer past status and data type fields

            // Send response back to app
            if (zclGw_callbacks.pfnZclGetSetPointCb != NULL)
            {
                zclGw_callbacks.pfnZclGetSetPointCb(*pPayload,
                        srcAddr.addr.shortAddr, srcAddr.endPoint, zclTransId);
            }
        } else
        {
            //status was error
        }
        break;

    default:
        dbg_print(PRINT_LEVEL_INFO, "Processing Read Attribute Response Failed:");
        dbg_print(PRINT_LEVEL_INFO, " zclTransId %d, clusterId %d, payloadlen %d\n", zclTransId,
                clusterId, payloadLen);
        break;
    }

    if (waitZclGetRspTransId == zclTransId)
    {
        waitZclGetRspFlag = false;
    }
}

/*********************************************************************
 * INTERFACE FUNCTIONS
 */

//*****************************************************************************
// Device control helper functions
//*****************************************************************************

//! \brief          Device control API: Add a device to a group
//! \param[in]      groupId - ID of the group to be added to
//! \param[in]      dstAddr - nwk addr of device to be added
//! \param[in]      endpoint - end point on the device to be added to the group
//! \param[in]      addrMode - address mode of dstAddr (afAddr16Bit or afAddrGroup)
//! \return        	status
uint_least8_t zGw_addGroup(uint16_t groupId, uint16_t dstAddr, uint8_t endpoint,
        uint8_t addrMode)
{
    //TODO
    return afStatus_FAILED;
}

//! \brief          Device control API: store a scene on a device
//! \param[in]      groupId - group ID of the scene to be stored
//! \param[in]      sceneId - scene ID of the scene to be stored
//! \param[in]      dstAddr - nwk addr of device to store the scene
//! \param[in]      endpoint - end point on the device to store the scene
//! \param[in]      addrMode - address mode of dstAddr (afAddr16Bit or afAddrGroup)
//! \return        	status
uint_least8_t zGw_storeScene(uint16_t groupId, uint8_t sceneId,
        uint16_t dstAddr, uint8_t endpoint, afAddrMode_t addrMode)
{
    //TODO
    return afStatus_FAILED;
}

//! \brief          Device control API: recall a scene on a device
//! \param[in]      groupId - group ID of the scene to be recalled
//! \param[in]      sceneId - scene ID of the scene to be recalled
//! \param[in]      dstAddr - nwk addr of device to recall the scene
//! \param[in]      endpoint - end point on the device to recall the scene
//! \param[in]      addrMode - address mode of dstAddr (afAddr16Bit or afAddrGroup)
//! \return        	status
uint_least8_t zGw_recallScene(uint16_t groupId, uint8_t sceneId,
        uint16_t dstAddr, uint8_t endpoint, afAddrMode_t addrMode)
{
    //TODO
    return afStatus_FAILED;
}

//! \brief          Device control API: binds a device output cluster (src) to
//                  another devices input cluster (dst)
//! \param[in]      srcNwkAddr - source devices nwk addr
//! \param[in]      srcEndpoint - source devices IEEE address
//! \param[in]      srcIEEE - nwk addr of device to store the scene
//! \param[in]      dstEndpoint - destination devices IEEE address
//! \param[in]      dstIEEE - destination addr of device to store the scene
//! \return        	status
uint_least8_t zGw_bind(uint16_t srcNwkAddr, uint8_t srcEndpoint,
        uint8_t srcIEEE[8], uint8_t dstEndpoint, uint8_t dstIEEE[8],
        uint16_t clusterID)
{
    //TODO
    return afStatus_FAILED;
}

//! \brief          Device control API: sends an identify command to a device
//! \param[in]      identifyTime - time to identify for (10ms units)
//! \param[in]      dstAddr - nwk addr of device to send command to
//! \param[in]      endpoint - end point on the device to send command to
//! \param[in]      addrMode - address mode of dstAddr (afAddr16Bit or afAddrGroup)
//! \return        	status
uint_least8_t zGw_sendIdentify(uint16_t identifyTime, uint16_t dstAddr,
        uint8_t endpoint, afAddrMode_t addrMode)
{
    //TODO
    return afStatus_FAILED;
}

//! \brief          Device control API: sends an identify effect command to a device
//! \param[in]      effect - effect (see ZCL spec)
//! \param[in]      effect - effectVarient (see ZCL spec)
//! \param[in]      identifyTime - time to identify for (10ms units)
//! \param[in]      dstAddr - nwk addr of device to send command to
//! \param[in]      endpoint - end point on the device to send command to
//! \param[in]      addrMode - address mode of dstAddr (afAddr16Bit or afAddrGroup)
//! \return        	status
uint_least8_t zGw_sendIdentifyEffect(uint8_t effect, uint8_t effectVarient,
        uint16_t dstAddr, uint8_t endpoint, afAddrMode_t addrMode)
{
    //TODO
    return afStatus_FAILED;
}

//*****************************************************************************
// Device set API prototypes
//*****************************************************************************

//! \brief          Device set API: sends a set state command to a device
//! \param[in]      0:off, 1:on, 2:toggle
//! \param[in]      dstAddr - nwk addr of device to send command to
//! \param[in]      endpoint - end point on the device to send command to
//! \param[in]      addrMode - address mode of dstAddr (afAddr16Bit or afAddrGroup)
//! \return        	status
uint_least8_t zGw_setState(uint8_t state, uint16_t dstAddr, uint8_t endpoint,
        uint8_t addrMode)
{
    afAddrType_t afDstAddr;
    uint_least8_t status;

    afDstAddr.addr.shortAddr = dstAddr;
    afDstAddr.endPoint = endpoint;
    afDstAddr.addrMode = (afAddrMode_t) addrMode;

    switch (state)
    {
    case 0:
        status = zclGeneral_SendOnOff_CmdOff(ZGW_EP, &afDstAddr, FALSE,
                zgwTransID++);
        break;
    case 1:
        status = zclGeneral_SendOnOff_CmdOn(ZGW_EP, &afDstAddr, FALSE,
                zgwTransID++);
        break;
    case 2:
        status = zclGeneral_SendOnOff_CmdToggle(ZGW_EP, &afDstAddr, FALSE,
                zgwTransID++);
        break;
    default:
        status = afStatus_FAILED;
    }
    return status;
}

//! \brief          Device set API: sends a set level command to a device
//! \param[in]      level - 0-255
//! \param[in]      time - transition time in 10th's of a second
//! \param[in]      dstAddr - nwk addr of device to send command to
//! \param[in]      endpoint - end point on the device to send command to
//! \param[in]      addrMode - address mode of dstAddr (afAddr16Bit or afAddrGroup)
//! \return        	status
uint_least8_t zGw_setLevel(uint8_t level, uint16_t time, uint16_t dstAddr,
        uint8_t endpoint, afAddrMode_t addrMode)
{
    afAddrType_t afDstAddr;

    afDstAddr.addr.shortAddr = dstAddr;
    afDstAddr.endPoint = endpoint;
    afDstAddr.addrMode = addrMode;

    return zclGeneral_SendLevelControlMoveToLevelWithOnOff(ZGW_EP, &afDstAddr, level,
            time, FALSE, zgwTransID++);
}

//! \brief          Device set API: sends a hue level command to a device
//! \param[in]      hue - 0-255
//! \param[in]      time - transition time in 10th's of a second
//! \param[in]      dstAddr - nwk addr of device to send command to
//! \param[in]      endpoint - end point on the device to send command to
//! \param[in]      addrMode - address mode of dstAddr (afAddr16Bit or afAddrGroup)
//! \return        	status
uint_least8_t zGw_setHue(uint8_t hue, uint16_t time, uint16_t dstAddr,
        uint8_t endpoint, afAddrMode_t addrMode)
{
    afAddrType_t afDstAddr;

    afDstAddr.addr.shortAddr = dstAddr;
    afDstAddr.endPoint = endpoint;
    afDstAddr.addrMode = addrMode;

    return zclLighting_ColorControl_Send_MoveToHueCmd( ZGW_EP, &afDstAddr, hue,
    LIGHTING_MOVE_TO_HUE_DIRECTION_SHORTEST_DISTANCE, time, FALSE, zgwTransID++);
}

//! \brief          Device set API: sends a set sturation command to a device
//! \param[in]      sat - saturation 0-255
//! \param[in]      time - transition time in 10th's of a second
//! \param[in]      dstAddr - nwk addr of device to send command to
//! \param[in]      endpoint - end point on the device to send command to
//! \param[in]      addrMode - address mode of dstAddr (afAddr16Bit or afAddrGroup)
//! \return        	status
uint_least8_t zGw_setSat(uint8_t sat, uint16_t time, uint16_t dstAddr,
        uint8_t endpoint, afAddrMode_t addrMode)
{
    afAddrType_t afDstAddr;

    afDstAddr.addr.shortAddr = dstAddr;
    afDstAddr.endPoint = endpoint;
    afDstAddr.addrMode = addrMode;

    return zclLighting_ColorControl_Send_MoveToSaturationCmd( ZGW_EP,
            &afDstAddr, sat, time, FALSE, zgwTransID++);
}

//! \brief          Device set API: sends a set hue and sturation command to a device
//! \param[in]      hue - 0-255
//! \param[in]      sat - saturation 0-255
//! \param[in]      time - transition time in 10th's of a second
//! \param[in]      dstAddr - nwk addr of device to send command to
//! \param[in]      endpoint - end point on the device to send command to
//! \param[in]      addrMode - address mode of dstAddr (afAddr16Bit or afAddrGroup)
//! \return        	status
uint_least8_t zGw_setHueSat(uint8_t hue, uint8_t sat, uint16_t time,
        uint16_t dstAddr, uint8_t endpoint, afAddrMode_t addrMode)
{
    afAddrType_t afDstAddr;

    afDstAddr.addr.shortAddr = dstAddr;
    afDstAddr.endPoint = endpoint;
    afDstAddr.addrMode = addrMode;

    return zclLighting_ColorControl_Send_MoveToHueAndSaturationCmd( ZGW_EP,
            &afDstAddr, hue, sat, time, FALSE, zgwTransID++);
}

//! \brief          Device set setpoint API: sends a set level command to a device
//! \param[in]      setPointValue - 0-255
//! \param[in]      dstAddr - nwk addr of device to send command to
//! \param[in]      endpoint - end point on the device to send command to
//! \param[in]      addrMode - address mode of dstAddr (afAddr16Bit or afAddrGroup)
//! \return        	status
uint_least8_t zGw_setSetPoint(int16_t setPointValue, uint16_t dstAddr,
        uint8_t endpoint, afAddrMode_t addrMode)
{

	afAddrType_t afDstAddr;
    zclWriteCmd_t *writeCmd = malloc(sizeof(zclWriteCmd_t) + sizeof(zclWriteRec_t));

    afDstAddr.addr.shortAddr = dstAddr;
    afDstAddr.endPoint = endpoint;
    afDstAddr.addrMode = addrMode;

    writeCmd->numAttr = 1;
    writeCmd->attrList[0].attrID = ATTRID_HVAC_THERMOSTAT_OCCUPIED_HEATING_SETPOINT;
    writeCmd->attrList[0].dataType = ZCL_DATATYPE_INT16;
    writeCmd->attrList[0].attrData = (uint8_t*) &setPointValue;

    return zcl_SendWrite (ZGW_EP, &afDstAddr,
    		ZCL_CLUSTER_ID_HVAC_THERMOSTAT, writeCmd, ZCL_FRAME_SERVER_CLIENT_DIR,
    		FALSE, zgwTransID++);
}

//*****************************************************************************
// Device get API prototypes
//*****************************************************************************

//! \brief          Device get API: sends a get info command to a device
//! \param[in]      dstAddr - nwk addr of device to send command to
//! \param[in]      endpoint - end point on the device to send command to
//! \param[in]      addrMode - address mode of dstAddr (afAddr16Bit or afAddrGroup)
//! \return        	status
uint_least8_t zGw_getInfo(uint16_t dstAddr, uint8_t endpoint,
        afAddrMode_t addrMode, uint8_t block)
{
    // //TODO: add zcl_lighting
    // return afStatus_FAILED;

    afAddrType_t afDstAddr;
    zclReadCmd_t* readCmd;
    uint_least8_t status;

    afDstAddr.addr.shortAddr = dstAddr;
    afDstAddr.endPoint = endpoint;
    afDstAddr.addrMode = addrMode;

    readCmd = malloc(sizeof(zclReadCmd_t) + sizeof(uint16));

    if (readCmd != NULL)
    {
        readCmd->numAttr = 1;
        readCmd->attrID[0] = ATTRID_BASIC_MODEL_ID;

        status = zcl_SendRead( ZGW_EP, &afDstAddr,
        ZCL_CLUSTER_ID_GEN_BASIC, readCmd,
        ZCL_FRAME_CLIENT_SERVER_DIR, FALSE, zgwTransID++);

        free(readCmd);

        if(block)
        {
            if (waitZclGetRsp() == -1)
            {
                status = ZFailure;
            }
        }
    }

    return status;
}

//! \brief          Device get API: sends a get model ID command to a device
//! \param[in]      dstAddr - nwk addr of device to send command to
//! \param[in]      endpoint - end point on the device to send command to
//! \param[in]      addrMode - address mode of dstAddr (afAddr16Bit or afAddrGroup)
//! \return        	status
uint_least8_t zGw_getModel(uint16_t dstAddr, uint8_t endpoint,
        afAddrMode_t addrMode)
{
    //TODO: add zcl_lighting
    return afStatus_FAILED;
}

//! \brief          Device get API: sends a get state command to a device
//! \param[in]      dstAddr - nwk addr of device to send command to
//! \param[in]      endpoint - end point on the device to send command to
//! \param[in]      addrMode - address mode of dstAddr (afAddr16Bit or afAddrGroup)
//! \param[in]      block - block on response, if set this function will return after
//!                 timeout or after response has been processed and call back returns
//! \return        	status
uint_least8_t zGw_getState(uint16_t dstAddr, uint8_t endpoint,
        afAddrMode_t addrMode, uint8_t block)
{
    afAddrType_t afDstAddr;
    zclReadCmd_t* readCmd;
    uint_least8_t status;

    afDstAddr.addr.shortAddr = dstAddr;
    afDstAddr.endPoint = endpoint;
    afDstAddr.addrMode = addrMode;

    readCmd = malloc(sizeof(zclReadCmd_t) + sizeof(uint16));

    if (readCmd != NULL)
    {
        readCmd->numAttr = 1;
        readCmd->attrID[0] = ATTRID_ON_OFF;

        status = zcl_SendRead( ZGW_EP, &afDstAddr,
        ZCL_CLUSTER_ID_GEN_ON_OFF, readCmd,
        ZCL_FRAME_CLIENT_SERVER_DIR, FALSE, zgwTransID++);

        free(readCmd);

        if(block)
        {
            if (waitZclGetRsp() == -1)
            {
                status = ZFailure;
            }
        }
    }

    return status;
}

//! \brief          Device get API: sends a get level to a device
//! \param[in]      dstAddr - nwk addr of device to send command to
//! \param[in]      endpoint - end point on the device to send command to
//! \param[in]      addrMode - address mode of dstAddr (afAddr16Bit or afAddrGroup)
//! \param[in]      block - block on response, if set this function will return after
//!                 timeout or after response has been processed and call back returns
//! \return        	status
uint_least8_t zGw_getLevel(uint16_t dstAddr, uint8_t endpoint,
        afAddrMode_t addrMode, uint8_t block)
{
    afAddrType_t afDstAddr;
    zclReadCmd_t* readCmd;
    uint_least8_t status;

    afDstAddr.addr.shortAddr = dstAddr;
    afDstAddr.endPoint = endpoint;
    afDstAddr.addrMode = addrMode;

    readCmd = malloc(sizeof(zclReadCmd_t) + sizeof(uint16));

    if (readCmd != NULL)
    {
        readCmd->numAttr = 1;
        readCmd->attrID[0] = ATTRID_LEVEL_CURRENT_LEVEL;

        waitZclGetRspTransId = zgwTransID;
        waitZclGetRspFlag = TRUE;

        status = zcl_SendRead( ZGW_EP, &afDstAddr,
        ZCL_CLUSTER_ID_GEN_LEVEL_CONTROL, readCmd,
        ZCL_FRAME_CLIENT_SERVER_DIR, FALSE, zgwTransID++);

        free(readCmd);

        if(block)
        {
            if (waitZclGetRsp() == -1)
            {
                status = ZFailure;
            }
        }
    }

    return status;

}

//! \brief          Device get API: sends a get hue to a device
//! \param[in]      dstAddr - nwk addr of device to send command to
//! \param[in]      endpoint - end point on the device to send command to
//! \param[in]      addrMode - address mode of dstAddr (afAddr16Bit or afAddrGroup)
//! \param[in]      block - block on response, if set this function will return after
//!                 timeout or after response has been processed and call back returns
//! \return        	status
uint_least8_t zGw_getHue(uint16_t dstAddr, uint8_t endpoint,
        afAddrMode_t addrMode, uint8_t block)
{
    afAddrType_t afDstAddr;
    zclReadCmd_t* readCmd;
    uint_least8_t status;

    afDstAddr.addr.shortAddr = dstAddr;
    afDstAddr.endPoint = endpoint;
    afDstAddr.addrMode = addrMode;

    readCmd = malloc(sizeof(zclReadCmd_t) + sizeof(uint16));

    if (readCmd != NULL)
    {
        readCmd->numAttr = 1;
        readCmd->attrID[0] = ATTRID_LIGHTING_COLOR_CONTROL_CURRENT_HUE;

        waitZclGetRspTransId = zgwTransID;
        waitZclGetRspFlag = TRUE;

        status = zcl_SendRead( ZGW_EP, &afDstAddr,
        		ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL, readCmd,
        ZCL_FRAME_CLIENT_SERVER_DIR, FALSE, zgwTransID++);

        free(readCmd);

        if(block)
        {
            if (waitZclGetRsp() == -1)
            {
                status = ZFailure;
            }
        }
    }

    return status;
}

//! \brief          Device get API: sends a get saturation to a device
//! \param[in]      dstAddr - nwk addr of device to send command to
//! \param[in]      endpoint - end point on the device to send command to
//! \param[in]      addrMode - address mode of dstAddr (afAddr16Bit or afAddrGroup)
//! \param[in]      block - block on response, if set this function will return after
//!                 timeout or after response has been processed and call back returns
//! \return        	status
uint_least8_t zGw_getSat(uint16_t dstAddr, uint8_t endpoint,
        afAddrMode_t addrMode, uint8_t block)
{
    afAddrType_t afDstAddr;
    zclReadCmd_t* readCmd;
    uint_least8_t status;

    afDstAddr.addr.shortAddr = dstAddr;
    afDstAddr.endPoint = endpoint;
    afDstAddr.addrMode = addrMode;

    readCmd = malloc(sizeof(zclReadCmd_t) + sizeof(uint16));

    if (readCmd != NULL)
    {
        readCmd->numAttr = 1;
        readCmd->attrID[0] = ATTRID_LIGHTING_COLOR_CONTROL_CURRENT_SATURATION;

        waitZclGetRspTransId = zgwTransID;
        waitZclGetRspFlag = TRUE;

        status = zcl_SendRead( ZGW_EP, &afDstAddr,
        		ZCL_CLUSTER_ID_LIGHTING_COLOR_CONTROL, readCmd,
        ZCL_FRAME_CLIENT_SERVER_DIR, FALSE, zgwTransID++);

        free(readCmd);

        if(block)
        {
            if (waitZclGetRsp() == -1)
            {
                status = ZFailure;
            }
        }
    }

    return status;
}

//! \brief          Device get API: sends a get saturation to a device
//! \param[in]      dstAddr - nwk addr of device to send command to
//! \param[in]      endpoint - end point on the device to send command to
//! \param[in]      addrMode - address mode of dstAddr (afAddr16Bit or afAddrGroup)
//! \param[in]      block - block on response, if set this function will return after
//!                 timeout or after response has been processed and call back returns
//! \return        	status
uint_least8_t zGw_getSetPoint(uint16_t dstAddr, uint8_t endpoint,
        afAddrMode_t addrMode, uint8_t block)
{
    afAddrType_t afDstAddr;
    zclReadCmd_t* readCmd;
    uint_least8_t status;

    afDstAddr.addr.shortAddr = dstAddr;
    afDstAddr.endPoint = endpoint;
    afDstAddr.addrMode = addrMode;

    readCmd = malloc(sizeof(zclReadCmd_t) + sizeof(uint16));

    if (readCmd != NULL)
    {
        readCmd->numAttr = 1;
        readCmd->attrID[0] = ATTRID_HVAC_THERMOSTAT_OCCUPIED_HEATING_SETPOINT;

        waitZclGetRspTransId = zgwTransID;
        waitZclGetRspFlag = TRUE;

        status = zcl_SendRead( ZGW_EP, &afDstAddr,
                    ZCL_CLUSTER_ID_HVAC_THERMOSTAT, readCmd,
                    ZCL_FRAME_CLIENT_SERVER_DIR, FALSE, zgwTransID++);

        free(readCmd);

        if(block)
        {
            if (waitZclGetRsp() == -1)
            {
                status = ZFailure;
            }
        }
    }

    return status;
}

//! \brief          Initialize MT_AF callbacks
//! \param          none
//! \return        	none
void zclGw_Init(void)
{
    //Register Callbacks MT system callbacks
    afRegisterCallbacks(mtAfCb);
}

//! \brief          Initialize Zcl and registers application endpoint
//! \param          none
//! \return        	none
void zclGw_InitZcl(void)
{
    // Setup the endpoints
    regEndpoints();

    // Register the ZCL General Cluster Library callback functions
    zclGeneral_RegisterCmdCallbacks( ZGW_EP, &cmdCallbacks);

    // Register the application's attribute list
    zcl_registerAttrList( ZGW_EP, ZSW_MAX_ATTRIBUTES, zswAttrs);
}

//! \brief          module management API: register application callbacks
//! \param[in]      zclGw_callbacks: callback table
//! \return        	none
void zclGw_registerCallbacks(zclGw_callbacks_t zclGwCallbacks)
{
    //copy the callback function pointers
    memcpy(&zclGw_callbacks, &zclGwCallbacks, sizeof(zclGw_callbacks_t));
    return;
}

//! \brief          Process incoming ZCL commands specific to this profile
//! \param[in]      pInMsg: pointer to the incoming message
//! \return        	none
void zclGw_processInCmds(zclIncoming_t *pInMsg)
{
    // Check if response acts across entire profile
    if (zcl_ClientCmd(pInMsg->hdr.fc.direction))
    {
        printf( "ZigBee Incoming ZCL Command: CmdId: %d, ClusterId: %04X, TransId: %d\n",
                   pInMsg->hdr.commandID, pInMsg->msg->clusterId, pInMsg->hdr.transSeqNum );

        // int i = 0;
        // printf("Incoming message- ");
        // for(i = 0; i < pInMsg->pDataLen; i++) {
        //     printf("%d ", pInMsg->pData[i]);
        // }
        // printf("\n");

        switch (pInMsg->hdr.commandID)
        {
        case ZCL_CMD_READ_RSP:
            // Process read attribute response

            printf( "ZigBee Incoming ZCL_CMD_READ_RSP\n");
            processZclReadAttributeRsp( pInMsg->msg->srcAddr, pInMsg->hdr.transSeqNum, pInMsg->msg->clusterId,
                                          pInMsg->pDataLen, pInMsg->pData );

            break;

        case ZCL_CMD_WRITE_RSP:
            // Process write attribute response
            printf("ZigBee NOT SUPPORTED: ZCL_CMD_WRITE_RSP\n");
            break;

        case ZCL_CMD_CONFIG_REPORT_RSP:
            // Process read report configuration response
            printf("ZigBee NOT SUPPORTED: ZCL_CMD_CONFIG_REPORT_RSP\n");
            break;

        case ZCL_CMD_DEFAULT_RSP:
            // Process default response
            printf("ZigBee NOT SUPPORTED: ZCL_CMD_DEFAULT_RSP\n");
            break;

        case ZCL_CMD_REPORT:
            // Process attribute report indication
            printf("ZigBee NOT SUPPORTED: ZCL_CMD_REPORT\n");
            break;

        case ZCL_CMD_DISCOVER_ATTRS_RSP:
            // Process discover attributes response
            printf("ZigBee NOT SUPPORTED: ZCL_CMD_DISCOVER_ATTRS_RSP\n");
            break;

        default:
            // Process ZCL frame for unsupported cmd
            printf("ZigBee NOT SUPPORTED: COMMAND- %d\n", pInMsg->hdr.commandID);
            break;
        }
    } else
    {
        // Process ZCL frame for unsupported cmd
    }
}

/*******************************************************************************
 ******************************************************************************/
