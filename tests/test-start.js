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
var ZNPController = require('./znpController').ZNPController;

var znpHandler = new ZNPController({
	siodev: '/dev/ttyUSB1',
	devType: 0x00,
	newNwk: false,
	channelMask: 0x800,
	baudRate: 115200
});

console.log("Starting ZNP Controller...");

var timeout = setTimeout(function() {
	console.error('ZNPController start failed');
    process.exit(1);
}, 2000); 

znpHandler.start().then(function() {
	clearTimeout(timeout);
	console.log('ZNPController started succesfully');
	// znpHandler.addDevice(60).then(function() {
	// 	console.log('Adding command succesful');
	// });
}, function() {
	console.error('ZNPController start failed');
    process.exit(1);
});

console.log("Trying to connect to znp module...");
znpHandler.connect().then(function() {
	console.log("Connection with znp module established succesfully");
}, function() {
	console.log("Connection could not be established");
    process.exit(1);
});
