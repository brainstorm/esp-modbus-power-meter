  // Yideanqi type Power Meter from Aliexpress
  
  // Relevant holding registers and example values:
  //
  //  0011:    0.193   <--- Amps panel 1
  //  0013:    0.258   <--- Amps panel 2
  //  0015:    0.210   <--- Amps panel 3
  //  0017:  219.600   <--- W total
  //  0019:   57.200   <--- var total

  //  0039:  105.200   <--- VA total
  //  0041:  236.550   <--- Volts panel 1
  //  0043:  236.580   <--- Volts panel 2
  //  0045:  236.480   <--- Volts panel 3

  //  001B:    0.828   <--- PF
  //  001D:   49.951   <--- Hz

  //  001F:    0.000   <---  uh
  //  0021:    0.070   <---  -uh
  //  0023:    0.000   <---  uAh
  //  0025:    0.030   <---  -uAh

// Includes: <Arduino.h> for Serial etc.
#include <Arduino.h>

// Include the header for the ModbusClient RTU style
#include "ModbusClientRTU.h"
#include "Logging.h"

// Definitions for this special case
#define RXPIN GPIO_NUM_8
#define TXPIN GPIO_NUM_6
//#define REDEPIN GPIO_NUM_17
#define BAUDRATE 9600
//#define FIRST_REGISTER 0x002A
#define FIRST_REGISTER 0x017
#define NUM_VALUES 1
#define READ_INTERVAL 1000

bool data_ready = false;
float values[NUM_VALUES];
uint32_t request_time;

// Create a ModbusRTU client instance
// The MAX485-based RS485 shield from LinkSprite has automatic duplexing via transistors,
// so no RE/DE pin is required. Other shields might required the above REDEPIN defined and connected.
ModbusClientRTU MB(Serial1);

// Define an onData handler function to receive the regular responses
// Arguments are received response message and the request's token
void handleData(ModbusMessage response, uint32_t token) 
{
  // First value is on pos 3, after server ID, function code and length byte
  uint16_t offs = 3;
  // The device has values all as IEEE754 float32 in two consecutive registers
  // Read the requested in a loop
  for (uint8_t i = 0; i < NUM_VALUES; ++i) {
    offs = response.get(offs, values[i]);
  }
  // Signal "data is complete"
  request_time = token;
  data_ready = true;
}

// Define an onError handler function to receive error responses
// Arguments are the error code returned and a user-supplied token to identify the causing request
void handleError(Error error, uint32_t token) 
{
  // ModbusError wraps the error code and provides a readable error message for it
  ModbusError me(error);
  LOG_E("Error response: %02X - %s\n", (int)me, (const char *)me);
}

// Setup() - initialization happens here
void setup() {
// Init Serial monitor
  Serial.begin(921600);
  while (!Serial) {}
  Serial.println("CONNECTED");

// Set up Serial1 connected to Modbus RTU
  Serial1.begin(BAUDRATE, SERIAL_8N1, RXPIN, TXPIN);

// Set up ModbusRTU client.
// - provide onData handler function
  MB.onDataHandler(&handleData);
// - provide onError handler function
  MB.onErrorHandler(&handleError);
// Set message timeout to 2000ms
  MB.setTimeout(2000);
// Start ModbusRTU background task
  MB.begin();
}

// Make requests specific to our power meter with holding registers shown above
void queueRequests() {
    // 0x11-0x19
    // 0x39-0x45
    // 0x1B-0x1D
    // 0x1F-0x25

    Error err = MB.addRequest((uint32_t)millis(), 1, READ_HOLD_REGISTER, 0x17, 2);
    if (err!=SUCCESS) {
      ModbusError e(err);
      LOG_E("Error creating request: %02X - %s\n", (int)e, (const char *)e);
    }

    // for (int reg=0x11; reg<=0x19; reg+=2) {
    //   Error err = MB.addRequest((uint32_t)millis(), 1, READ_HOLD_REGISTER, reg, 2);
    //   if (err!=SUCCESS) {
    //     ModbusError e(err);
    //     LOG_E("Error creating request: %02X - %s\n", (int)e, (const char *)e);
    //   }
    // }

    // for (int reg=0x39; reg<=0x45; reg+=2) {
    //   Error err = MB.addRequest((uint32_t)millis(), 1, READ_HOLD_REGISTER, reg, 2);
    //   if (err!=SUCCESS) {
    //     ModbusError e(err);
    //     LOG_E("Error creating request: %02X - %s\n", (int)e, (const char *)e);
    //   }
    // }

    // for (int reg=0x1B; reg<=0x1D; reg+=2) {
    //   Error err = MB.addRequest((uint32_t)millis(), 1, READ_HOLD_REGISTER, reg, 2);
    //   if (err!=SUCCESS) {
    //     ModbusError e(err);
    //     LOG_E("Error creating request: %02X - %s\n", (int)e, (const char *)e);
    //   }
    // }

    // for (int reg=0x1F; reg<=0x25; reg+=2) {
    //   Error err = MB.addRequest((uint32_t)millis(), 1, READ_HOLD_REGISTER, reg, 2);
    //   if (err!=SUCCESS) {
    //     ModbusError e(err);
    //     LOG_E("Error creating request: %02X - %s\n", (int)e, (const char *)e);
    //   }
    // }
}

// loop() - cyclically request the data
void loop() {
  static unsigned long next_request = millis();

  // Shall we do another request?
  if (millis() - next_request > READ_INTERVAL) {
    // Yes.
    data_ready = false;
    // Issue the requests
    queueRequests();
    // Save current time to check for next cycle
    next_request = millis();
  } else {
    // No, but we may have another response
    if (data_ready) {
      // We do. Print out the data
      Serial.printf("Requested at %8.3fs:\n", request_time / 1000.0);
      for (uint8_t i = 0; i < NUM_VALUES; ++i) {
        Serial.printf("   %04X: %8.3f\n", i, values[i]);
      }
      Serial.printf("----------\n\n");
      data_ready = false;
    }
  }
}