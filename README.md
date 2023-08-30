# UdpTester
Tool for testing UDP on Particle devices

**This tool is experimental and a little bit flaky.**

It wrote it to test a single thing and got the necessary results so there probably won't be much development on it, but I published it because it does have some neat techniques if you can get past the device code occasionally locking up the device.

It only tests sending data to the Particle device, but I did intend for it to work in either direction but it's missing code for some bits of that.

There's a node.js server component that you run, and it's also an interactive tool that you can make menu selections on to run a test. You can configure the test parameters at the top of app.js.

There's device firmware that you flash to a Particle Wi-Fi device.

And you must connect the Particle device to the computer running the node script via USB.

You will probably want to monitor the device's serial USB debug using `particle serial monitor`.

The way it works is that the node server and device firmware communicate by USB control requests that run independently of the serial monitor. The allows the device to tell the server that it's online and what its IP address is. It's also how the server tells the device it's about to start a new test.


