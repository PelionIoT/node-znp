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
var util = require('util');
var znp = null;
try {
	znp = require(__dirname + '/build/Debug/obj.target/znp.node').ZNP;
} catch(e) {
	if(e.code == 'MODULE_NOT_FOUND')
		znp = require(__dirname + '/build/Release/obj.target/znp.node').ZNP;
	else
		console.error("ERROR: " + util.inspect(e));
}
var events = require('events');

/*
 * Extend prototype.
 */
function inherits(target, source) {
	for (var k in source.prototype)
		target.prototype[k] = source.prototype[k];
}

/*
 * Default options
 */

//  // ZCD_LOGICAL_TYPE values
// #define DEVICETYPE_COORDINATOR 0x00
// #define DEVICETYPE_ROUTER 0x01
// #define DEVICETYPE_ENDDEVICE 0x02

var _options = {
	devType: 0x00,
	newNwk: false,
	channelMask: 0x800,
	baudRate: 115200,
	panIdSelection: "random",
	panId: 65535
}

var ZNP = function(path, options) {
	options = options || {};
	options.devType = options.devType || _options.devType;
	options.newNwk = options.newNwk || _options.newNwk;
	options.channelMask = options.channelMask || _options.channelMask;
	options.baudRate = options.baudRate || _options.baudRate;
	options.panId = options.panId || _options.panId;
	this.path = path;
	this.znp = new znp(options);
	this.znp.emit = this.emit.bind(this);
}

inherits(ZNP, events.EventEmitter);
inherits(ZNP, znp);

module.exports = ZNP;
