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
