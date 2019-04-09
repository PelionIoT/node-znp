/*
 * Copyright (c) 2018, Arm Limited and affiliates.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/*
 * ZNP test program.
 */

var ZNP = require('./../index.js');
var EventEmitter = require('events').EventEmitter;
var Promise = require('promise');

/*
 * Default options
 */
var _options = {
	siodev: "/dev/ttyUSB0",
	devType: 0x00,
	newNwk: false,
	channelMask: 0x800,
	baudRate: 115200
}

var ZNPController = function(options) {
	this.configuration = options || {};
	this.configuration.devType = _options.devType || options.devType;
	this.configuration.newNwk = _options.newNwk || options.newNwk;
	this.configuration.channelMask = _options.channelMask || options.channelMask;
	this.configuration.baudRate = _options.baudRate || options.baudRate;
	this.siodev = options.siodev || _options.siodev;	
};

ZNPController.prototype = Object.create(EventEmitter.prototype);

ZNPController.prototype.start = function() {
	var self = this;

	return new Promise(function(resolve, reject) {
		self.znp = new ZNP(self.siodev, self.configuration);

		var znp = self.znp = self.znp.znp;

		znp.onNetworkReady(function() {
			console.log('Network Ready');
			resolve();
		});

		znp.onNetworkFailed(function() {
			console.log('Network failed');
			reject();
		});

		var seq = 0;
		znp.onNodeDiscovered(function(nodeInfo, IEEEAddr, inClusterList, outClusterList) {
			console.log('nodeInfo: ', nodeInfo);
			console.log('IEEEAddr: ', IEEEAddr);
			console.log('inClusterList: ', inClusterList);
			console.log('outClusterList: ', outClusterList);
			//var i = 0;
			//var interval;
			//interval = setInterval(function() {
			//	console.log('Sending state change request with seq id: ', seq);
			//	if(i <= 7) {
			//		self.zclWork({workCode: 1, srcEp: 6, dstAddr: nodeInfo.nwkAddr, endPoint: nodeInfo.endpoint, addrMode: 2, clusterId: 0x0000, numAttr: 1, attrId: i, 
			//			direction: 0x00, disableDefaultRsp: false, seqNumber: seq++}).then(function(status) {
			//				console.log("GOT STATUS: ", status);
			//			});
			//	} else {
			//		clearInterval(interval);
			//	}

				// if(i % 2 == 0) {
				// 	self.zclWork({workCode: 0, srcEp: 6, dstAddr: nodeInfo.nwkAddr, endPoint: nodeInfo.endpoint, addrMode: 2, clusterId: 0x0006, cmdId: 0, specific: 1, 
				// 		direction: 0x00, disableDefaultRsp: false, manuCode: 0, seqNumber: seq++, cmdFormatLen: 0, cmdFormat: null}).then(function(status) {
				// 			console.log("GOT STATUS: ", status);
				// 		});
				// } else {
				// 	self.zclWork({workCode: 0, srcEp: 6, dstAddr: nodeInfo.nwkAddr, endPoint: nodeInfo.endpoint, addrMode: 2, clusterId: 0x0006, cmdId: 1, specific: 1, 
				// 		direction: 0x00, disableDefaultRsp: false, manuCode: 0, seqNumber: seq++, cmdFormatLen: 0, cmdFormat: null}).then(function(status) {
				// 			console.log("GOT STATUS: ", status);
				// 		});
				// }
			//	i++;
			//}, 2000);
		});

		znp.onCmdResponse(function(status, seqId) {
			console.log('GOT RESPONSE: ', status + ' for command seq: ', seqId);
		});

		znp.onAttrResponse(function(info, payload) {
			console.log('info: ', info);
			console.log(' with payload: ', payload); 
			console.log(' in string: ', payload.toString('ascii'));
			console.log('******************************************');
		});
	});
};

ZNPController.prototype.addDevice = function(duration) {
	var self = this;
	return new Promise(function(resolve, reject) {
		self.znp.addDevice(duration, function() {
			console.log('addDevice successful');
			resolve();
		}, function() {
			console.error('addDevice call failed');
			reject();
		});
	});
};

ZNPController.prototype.connect = function() {
	var self = this;
	return new Promise(function(resolve, reject) {
		self.znp.connect(self.siodev, function(buf) {
			if(buf == true){
				resolve();
			} else {
				reject();
			}
		});
	});
};

ZNPController.prototype.disconnect = function() {
	this.znp.disconnect();
};

ZNPController.prototype.zclWork = function(buf) {
	var self = this;
	return new Promise(function(resolve, reject) {
		self.znp.doZCLWork(buf, function(status) {
			resolve(status);
		});
	});
};

module.exports = {
	ZNPController: ZNPController
};
