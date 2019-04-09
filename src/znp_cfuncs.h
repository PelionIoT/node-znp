/*    
    Copyright (c) 2018, Arm Limited and affiliates.
    
    SPDX-License-Identifier: Apache-2.0
    
    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at
    
        http://www.apache.org/licenses/LICENSE-2.0
    
    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#ifndef _ZNP_CFUNCS_H_
#define _ZNP_CFUNCS_H_

#include "znp_mngt.h"
#include "mtZdo.h"

typedef struct {
	uint8_t 	devType;
	uint16_t 	channelMask;
	uint8_t 	newNwk;
	uint32_t	baudRate;
	uint16_t	panId;
} config_options;

typedef struct {
	uint16_t srcAddr;
	uint8_t endPoint;
	uint8_t	addrMode;
	uint8_t transId;
	uint16_t clusterId;
	uint16_t payloadLen;
	uint8_t payload[255];
} attr_response;

typedef struct {
	uint8_t status;
	uint8_t len;
	uint8_t data[248];
} nvRead_response;

#ifdef __cplusplus
extern "C" {
#endif

uint8_t wZAddDevice(uint8_t duration);
uint8_t wZCloseRPC(void);
uint8_t wZEndDeviceAnnce(EndDeviceAnnceIndFormat_t *);
void* wZgetNVItem(uint16_t id);
uint8_t wZsetNVItem(uint16_t id, uint8_t len, uint8_t *value);
uint8_t wZSendLqiReq(uint16_t dstAddr);

void zWNetworkReady(void);
void zWNetworkFailed(void);
uint8_t zWZdoSimpleDescRspCb(epInfo_t *);
void zWDataResponseConfirm(uint8_t*);
void zWInformReadAttritubeRsp(attr_response *);
uint8_t zWUpdateNetworkTopology(Node_t *);
uint8_t zWDeviceJoinedNetwork(EndDeviceAnnceIndFormat_t *);

#ifdef __cplusplus
};
#endif

#endif //_ZNP_CFUNCS_H_