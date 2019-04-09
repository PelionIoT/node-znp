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
