const dgram = require('dgram');
const server = dgram.createSocket('udp4');

const helper = require('@rickkas7/particle-node-cli-helper');
helper
    .withRootDir(__dirname);

const usb = require('particle-usb');

let settings = {
    interval: 1000, // milliseconds
    size: 1024, // bytes of UDP payload
    port: 8200,
};

let usbDevice;
let runTimer;
let remoteAddr;
let remotePort;
let sendSeq = 0;

async function scanForDevices() {
    let usbDevice;

    const scanOptions = {
        showQuitOption: true,
    };

    while (true) {
        const devices = await usb.getDevices();
        if (devices.length == 1) {
            usbDevice = devices[0];
            break;
        }
        else if (devices.length > 0) {
            const deviceMenu = [];

            for (const device of devices) {
                await device.open();
                deviceMenu.push(device.type + ' (' + device.id + ')');
                await device.close();
            }

            const res = await helper.questionMenu('Select device: ', deviceMenu, scanOptions);
            usbDevice = devices[res];

            break;
        }
        else {
            const scanAgainMenu = [
                'Scan again',
            ];

            await helper.questionMenu('No USB devices found. Scan again? ', scanAgainMenu, scanOptions);
        }
    }

    return usbDevice;
}

async function sendControlRequest(reqObj) {
    try {
        const res = await usbDevice.sendControlRequest(10, JSON.stringify(reqObj));

        if (res.result == 0 && typeof res.data !== 'undefined') {
            const json = JSON.parse(res.data);
            return json;
        }
    }
    catch (e) {
        console.log('control request exception', e);
    }

    return null;
}

async function stopTest() {
    if (runTimer) {
        clearInterval(runTimer);
        runTimer = null;

        await sendControlRequest({ op: 'stop' });
    }
}

async function run() {
    usbDevice = await scanForDevices();
    await usbDevice.open();

    helper.rl.output.write('Opened Particle device ' + usbDevice.type + ' (' + usbDevice.id + ') via USB\n');

    let msgTimer;
    msgTimer = setInterval(async function () {

        const res = await sendControlRequest({ op: 'msg' });
        if (res) {
            console.log('msg', res);
            if (typeof res.wifi != 'undefined') {
                if (res.wifi) {
                    remoteAddr = res.ip;
                    remotePort = res.port;
                }
                else {
                    remoteAddr = remotePort = null;
                }
            }
        }

    }, 1000);

    const testOptions = {
        showQuitOption: true,
    };

    const testCommands = [
        {
            title: 'Set interval',
            function: function () {

            },
        },
        {
            title: 'Set packet size',
            function: function () {

            },
        },
        {
            title: 'Run upload test (from device)',
            function: function () {

            },
        },
        {
            title: 'Run download (to device)',
            function: async function () {
                await stopTest();

                await sendControlRequest({ op: 'start' });

                seq = 0;

                runTimer = setInterval(function() {
                    let msg = Buffer.alloc(settings.size);
                    for(let ii = 4; ii < settings.size; ii++) {
                        msg.writeUInt8(ii % 256, ii);
                    }
                    msg.writeInt32LE(sendSeq++, 0);

                    server.send(msg, remotePort, remoteAddr);
                }, settings.interval);
            },
        },
        {
            title: 'Stop test',
            function: stopTest,
        }
    ];

    const res = await sendControlRequest({ op: 'info' });
    if (res) {
        console.log('info', res);
        remoteAddr = res.ip;
        remotePort = res.port;        
    }

    while (true) {
        let titles = [];
        for (const obj of testCommands) {
            titles.push(obj.title);
        }

        const result = await helper.questionMenu('Function? ', titles, testOptions);

        // Note: result is a zero-based index 

        testCommands[result].function();
    }
}

run();

//
// UDP server
//
server.on('error', (err) => {
    console.log(`server error:\n${err.stack}`);
    server.close();
});

server.on('message', (msg, rinfo) => {
    console.log(`server got: ${msg} from ${rinfo.address}:${rinfo.port}`);
});

server.on('listening', () => {
    const address = server.address();
    console.log(`server listening ${address.address}:${address.port}`);
});

server.bind(settings.port);