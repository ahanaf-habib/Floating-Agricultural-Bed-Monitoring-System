#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

// =====================================================
// PIN DEFINITIONS
// =====================================================

#define TURBIDITY_PIN 34
#define PH_PIN 35


// =====================================================
// ESP-NOW DATA STRUCTURE
// MUST MATCH THE MAIN ESP32
// =====================================================

typedef struct {
  int adcValue;
  char status[20];

  int phRaw;
  int phMilliVolts;
} SensorData;

SensorData sensorData;


// =====================================================
// BROADCAST ADDRESS
// =====================================================

uint8_t broadcastAddress[] = {
  0xFF,
  0xFF,
  0xFF,
  0xFF,
  0xFF,
  0xFF
};


// =====================================================
// SETUP
// =====================================================

void setup() {

  Serial.begin(115200);

  delay(1000);

  Serial.println();
  Serial.println("================================");
  Serial.println("          SLAVE ESP32");
  Serial.println("================================");


  // ===================================================
  // ADC SETUP
  // ===================================================

  analogReadResolution(12);

  analogSetPinAttenuation(
    TURBIDITY_PIN,
    ADC_11db
  );

  analogSetPinAttenuation(
    PH_PIN,
    ADC_11db
  );


  // ===================================================
  // WIFI STATION MODE
  // ===================================================

  WiFi.mode(WIFI_STA);

  delay(500);


  // ===================================================
  // FORCE WIFI CHANNEL 1
  // SAME CHANNEL AS MAIN
  // ===================================================

  esp_wifi_set_channel(
    1,
    WIFI_SECOND_CHAN_NONE
  );

  uint8_t channel;
  wifi_second_chan_t second;

  esp_wifi_get_channel(
    &channel,
    &second
  );


  // ===================================================
  // PRINT SLAVE INFORMATION
  // ===================================================

  Serial.print("Slave MAC: ");
  Serial.println(WiFi.macAddress());

  Serial.print("Channel: ");
  Serial.println(channel);

  Serial.println("================================");


  // ===================================================
  // START ESP-NOW
  // ===================================================

  if (esp_now_init() != ESP_OK) {

    Serial.println(
      "ESP-NOW initialization FAILED!"
    );

    return;
  }

  Serial.println(
    "ESP-NOW initialized successfully"
  );


  // ===================================================
  // ADD BROADCAST PEER
  // ===================================================

  esp_now_peer_info_t peerInfo = {};

  memcpy(
    peerInfo.peer_addr,
    broadcastAddress,
    6
  );

  peerInfo.channel = 1;

  peerInfo.encrypt = false;


  if (
    esp_now_add_peer(&peerInfo) != ESP_OK
  ) {

    Serial.println(
      "Broadcast peer ADD FAILED!"
    );

    return;
  }

  Serial.println(
    "Broadcast peer added successfully"
  );


  // ===================================================
  // READY
  // ===================================================

  Serial.println();
  Serial.println("================================");
  Serial.println("          ESP-NOW READY");
  Serial.println("================================");
  Serial.println(
    "Broadcasting turbidity + pH..."
  );
  Serial.println("================================");

  delay(1000);
}


// =====================================================
// LOOP
// =====================================================

void loop() {


  // ===================================================
  // READ TURBIDITY
  // EXISTING SENSOR — GPIO 34
  // ===================================================

  int adcValue = analogRead(
    TURBIDITY_PIN
  );


  // ===================================================
  // READ pH SENSOR
  // GPIO 35
  // ===================================================

  int phRaw = analogRead(
    PH_PIN
  );


  // ===================================================
  // READ pH ADC VOLTAGE
  // ===================================================

  int phMilliVolts = analogReadMilliVolts(
    PH_PIN
  );


  // ===================================================
  // CREATE SENSOR DATA
  // ===================================================

  sensorData.adcValue = adcValue;

  strcpy(
    sensorData.status,
    "TEST OK"
  );

  sensorData.phRaw = phRaw;

  sensorData.phMilliVolts =
    phMilliVolts;


  // ===================================================
  // PRINT DATA
  // ===================================================

  Serial.println();

  Serial.println("--------------------------------");

  Serial.print(
    "Turbidity ADC: "
  );

  Serial.println(
    sensorData.adcValue
  );


  Serial.print(
    "pH Raw ADC: "
  );

  Serial.println(
    sensorData.phRaw
  );


  Serial.print(
    "pH GPIO Voltage: "
  );

  Serial.print(
    phMilliVolts
  );

  Serial.println(
    " mV"
  );


  // Because we have two equal 4.7k
  // resistors, the voltage is divided by 2.

  float gpioVoltage =
    phMilliVolts / 1000.0;

  float poVoltage =
    gpioVoltage * 2.0;


  Serial.print(
    "Estimated Po Voltage: "
  );

  Serial.print(
    poVoltage,
    3
  );

  Serial.println(
    " V"
  );


  Serial.print(
    "Status: "
  );

  Serial.println(
    sensorData.status
  );


  Serial.println(
    "Send request..."
  );


  // ===================================================
  // SEND DATA TO BROADCAST ADDRESS
  // ===================================================

  esp_err_t result = esp_now_send(
    broadcastAddress,
    (uint8_t *)&sensorData,
    sizeof(sensorData)
  );


  // ===================================================
  // CHECK WHETHER SEND REQUEST WAS ACCEPTED
  // ===================================================

  if (result == ESP_OK) {

    Serial.println(
      "Send request: OK"
    );

  } else {

    Serial.print(
      "Send request: FAILED | Error code: "
    );

    Serial.println(
      result
    );
  }


  // ===================================================
  // SEND EVERY 1 SECOND
  // ===================================================

  delay(1000);
}