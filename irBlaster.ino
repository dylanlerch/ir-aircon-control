#include <IRremote.h>
#include <IRremoteInt.h>
#include <math.h>

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

#define SIGNAL_SEND_REPEATS 3

#define AMBIENT_TEMP_PIN 0


IRsend ir;

int repeats_remaining;
int current_temperature;
float current_ambient_temperature;
bool current_power_state;

void setup() {
    Serial.begin(9600);
    
    repeats_remaining = 0;

    current_temperature = 24;
    current_power_state = false;

    current_ambient_temperature = 0;

    pinMode(13, OUTPUT);
}

void loop() {
    // If there is a message waiting
    if (Serial.available() > 0) {
        byte b = Serial.read();

        if (b == 0xFF) { // AC status query
            byte message= getTemperature(current_temperature) | getOn(current_power_state);
            Serial.write(message);
        } else if (b = 0xF0) { // Ambient temp query
            byte message = getAmbientTemperature();
            Serial.write(message);
        } else if ((0x10 & b)) { // If bit 5 is high, it's an on or off command
            repeats_remaining = SIGNAL_SEND_REPEATS; // Reset the repeat count
            current_power_state = ((bool)(0x0F & b));
        } else { // received a temperature instruction
            repeats_remaining = SIGNAL_SEND_REPEATS; // Reset the repeat count
            current_temperature = (int)((0x0F & b)) + 17;
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

byte getAmbientTemperature() {
    // From the temperature sensor data sheet
    float voltage = (analogRead(AMBIENT_TEMP_PIN) * 0.004882814);
    float degrees = (voltage - 0.5) * 100.0;
    
    // Get the whole number for the degrees, this will be stored in the top 6
    // bits of the byte that is returned.
    int wholeDegrees = floor(degrees);

    // We only have 4 bits left for the decimal, so we will map the decimals to
    // one of 4 values, 0->0, 1->0.25, 2->0.5, 3->0.75
    int decimal = round((wholeDegrees * 4) - (degrees * 4));

    // If the decimal is greater than 3, then the decimal is so high that we 
    // should just round the temerature up to the next degree, and set decimal
    // to 0.
    if (decimal > 3) wholeDegrees += 1;

    // Offset by 10 so we can support between -10 - 53 degrees rather than 
    // 0 - 63.
    wholeDegrees += 10;

    // If we're exceeing limits, just set it back to the thresholds
    if (wholeDegrees < 0) {
        wholeDegrees = 0;
        decimal = 0;
    } else if (wholeDegrees > 63) {
        wholeDegrees = 63;
        decimal = 0;
    }

    // Top six bits store the whole value, bottom two bits store the decimal
    return (wholeDegrees << 2) | (decimal & 0b00000011);
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