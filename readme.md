# Homebridge / Raspberry Pi / Arduino A/C Controller
This is a project that I'm working on to let me control my A/C using [Homebridge](https://github.com/nfarina/homebridge) and [Apple's HomeKit](http://www.apple.com/au/ios/home/).

Thanks to Mattia Rossi for decoding the [Toshiba A/C signals](http://blog.mrossi.com/2016/05/toshiba-air-conditioner-ir-signal.html) so that I didn't have to.

## Device Setup
- I've got Homebridge running on a Raspberry Pi, that allows me to access devices on the network using Apple's HomeKit.
- I've got a basic web server running on the Pi that receives on/off and temperature setting commands from any device on the network - and sends them to the Arduino via a serial connection.
- I've got an Arduino firing the IR signals off to the A/C to control it.


## Files
### irBlaster.ino
This runs on the Arduino that sends the IR signals to the A/C unit.

### irServer.py
This runs on the Raspberry Pi and receives commands (via web requests) to change the state of the A/C. This facilitates the serial communication between the Pi and the Arduino.

### homebridge-ac-over-http.js
This is the Homebride plugin that allows homebridge to talk to send commands and retrieve status from the irServer.