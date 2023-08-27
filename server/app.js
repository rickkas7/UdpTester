
const helper = require('@rickkas7/particle-node-cli-helper');
helper
    .withRootDir(__dirname);

const usb = require('particle-usb');


async function scanForDevices() {
    let usbDevice;
    
    const scanOptions = {
        showQuitOption: true,
    };

    while(true) {
        const devices = await usb.getDevices();
        if (devices.length == 1) {
            usbDevice = devices[0];
            break;
        }
        else if (devices.length > 0) {
            const deviceMenu = [];

            for(const device of devices) {
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

async function run() {
    const usbDevice = await scanForDevices();
    await usbDevice.open();

    helper.rl.output.write('Opened Particle device ' + usbDevice.type + ' (' + usbDevice.id + ') via USB\n');
    
    let msgTimer;
    msgTimer = setInterval(async function() {
    
        try {
            let reqObj = {
                op: 'msg'
            };
            const res = await usbDevice.sendControlRequest(10, JSON.stringify(reqObj));

            console.log('res', res);
        }
        catch(e) {
            console.log('control request exception', e);
            /*
            if (msgTimer) {
                clearInterval(msgTimer);
                msgTimer = null;
            }
            */
        }
    
    }, 1000);

    const testOptions = {
        showQuitOption: true,
    };

    const tests = [
        'Upload (from device)',
        'Download (to device)',
    ];
    
    while(true) {
        const test = await helper.questionMenu('Function? ', tests, testOptions);

        // Note: result is a zero-based index 
        console.log('test', test);    
    }
}

run();

