#include <IRremote.h>
#include <IRremoteInt.h>

#define TOSHIBA_SPEED 38
#define TOSHIBA_REPEATS 2

#define TOSHIBA_START_MARK 4400
#define TOSHIBA_START_SPACE 4300
#define TOSHIBA_BIT_MARK 543
#define TOSHIBA_HIGH_SPACE 1623
#define TOSHIBA_LOW_SPACE 472
#define TOSHIBA_REPEAT_MARK 440
#define TOSHIBA_REPEAT_SPACE 7048

#define DATALEN 9


IRsend ir;

int repeats_remaining;
int current_temperature;
bool current_power_state;

void setup() {
    Serial.begin(9600);
    
    repeats_remaining = 0;

    current_temperature = 24;
    current_power_state = false;

    pinMode(13, OUTPUT);
}

void loop() {
    // If there is a message waiting
    if (Serial.available() > 0) {
        byte b = Serial.read();

        if (b == 0xFF) { // Status query
            Serial.write(getTemperature(current_temperature) | getOn(current_power_state));
        } else { // Received new instruction
            repeats_remaining = 5; // Repeat this 5 times

            current_temperature = (int)((0xF0 & b) >> 4) + 17;
            current_power_state = !((bool)(0x0F & b));
        }
    }

    // If we still need to send signals to the A/C
    if (repeats_remaining > 0) {
        sendCommand(current_temperature, current_power_state);
        repeats_remaining -= 1;
    }

    delay(500);
}


void sendCommand(int temperature, bool on) {
    // Fixed start bytes                                      Temp      Fan/Mode/Power   Unused    Checksum
    byte signal[DATALEN] = {0xF2, 0x0D, 0x03, 0xFC, 0x01,     0x70,     0x07,            0x00,     0x00};

    // Calculate byte for on/off and temperature
    signal[5] = getTemperature(temperature);
    signal[6] = getOn(on);

    // Calculate checksum and store in final byte
    signal[DATALEN - 1] = calculateCheckSum(signal, sizeof(signal) - 1);

    sendSignal(TOSHIBA_SPEED, TOSHIBA_REPEATS, signal, sizeof(signal));
}

byte getOn(bool on) {
    if (on) {
        return (byte)0;
    } else {
        return (byte)7;
    }
}

byte getTemperature(int temp) {
    if (temp < 17) temp = 17;
    if (temp > 30) temp = 30;

    return (byte)((temp - 17) << 4);
}

void sendSignal(int speed, int numberOfTimes, byte *data, size_t dataLength) {
    ir.enableIROut(speed); //Enable and set speed
    ir.space(0); //Start low


    for (int i = 0; i < numberOfTimes; ++i) {
        sendMessageStart(); // Send start signal

        // Send each of the bytes
        for (int j = 0; j < dataLength; ++j) {
            sendByte(data[j]);
        }

        sendRepeatStart(); // Send repeat signal
    }

    ir.space(0); //Always finish low
}

void sendByte(byte b) {
    // Cycle throgh all of the bits in the byte (MSB first)
    // and send each of them.
    for (byte mask = 10000000; mask > 0; mask >>= 1) {
        if (b & mask) { // If bit is high
            sendHigh();
        } else { // If bit is low
            sendLow();
        }
    }
}

byte calculateCheckSum(byte *data, size_t dataLength) {
    byte checkSum = 0;
    
    // For each of the bytes in data XOR with current checksum.
    for (int i = 0; i < dataLength; ++i) {
        checkSum = data[i] ^ checkSum; // XOR this byte with the current checksum
    }

    return checkSum;
}

void sendMessageStart() {
    send(TOSHIBA_START_MARK, TOSHIBA_START_SPACE);
}

void sendRepeatStart() {
    send(TOSHIBA_REPEAT_MARK, TOSHIBA_REPEAT_SPACE);
}

void sendHigh() {
    send(TOSHIBA_BIT_MARK, TOSHIBA_HIGH_SPACE);
}
  
void sendLow() {
    send(TOSHIBA_BIT_MARK, TOSHIBA_LOW_SPACE);
}

void send(int mark, int space) {
    ir.mark(mark);
    ir.space(space);
}