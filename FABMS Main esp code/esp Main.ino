#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>

#include <OneWire.h>
#include <DallasTemperature.h>

#include <Wire.h>
#include <TinyGPSPlus.h>

#include <esp_now.h>


// =====================================================
//                    WIFI SETTINGS
// =====================================================

const char* WIFI_SSID = "Ratul";
const char* WIFI_PASSWORD = "00000000";


// =====================================================
//                  TELEGRAM SETTINGS
// =====================================================

// IMPORTANT:
// Put your Telegram bot token here.

#define BOT_TOKEN "YOUR_BOT_TOKEN"
#define CHAT_ID "1954882802"

WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);


// =====================================================
//                     PIN SETTINGS
// =====================================================

// -----------------------------------------------------
// Water level sensors
// -----------------------------------------------------

#define WATER1_PIN 32
#define WATER2_PIN 33
#define WATER3_PIN 34
#define WATER4_PIN 35


// -----------------------------------------------------
// PIR
// -----------------------------------------------------

#define PIR_PIN 27


// -----------------------------------------------------
// Vibration
// -----------------------------------------------------

#define VIBRATION_PIN 26


// -----------------------------------------------------
// Soil moisture
// GPIO 39 = VN
// -----------------------------------------------------

#define MOISTURE_PIN 39


// -----------------------------------------------------
// MAIN TDS SENSOR
// GPIO 36 = VP
// -----------------------------------------------------

#define TDS_PIN 36


// -----------------------------------------------------
// DS18B20
// -----------------------------------------------------

#define TEMP_PIN 4


// -----------------------------------------------------
// LED and buzzer
// -----------------------------------------------------

#define LED_PIN 2
#define BUZZER_PIN 5


// -----------------------------------------------------
// GPS NEO-6M
// -----------------------------------------------------

#define GPS_RX_PIN 16
#define GPS_TX_PIN 17


// -----------------------------------------------------
// MPU6500 I2C
// -----------------------------------------------------

#define I2C_SDA 21
#define I2C_SCL 22

#define MPU6500_ADDRESS 0x68


// =====================================================
//                    THRESHOLDS
// =====================================================

#define WATER_THRESHOLD 2000

#define TEMPERATURE_WARNING 35.0

#define MOISTURE_WARNING 1500


// -----------------------------------------------------
// MPU thresholds
// -----------------------------------------------------

#define ACCELERATION_WARNING 3.0
#define GYRO_WARNING 120.0
#define MPU_TILT_WARNING 25.0


// -----------------------------------------------------
// GPS
// -----------------------------------------------------

#define GPS_RADIUS 200.0

#define GPS_CONFIRMATIONS_REQUIRED 5


// -----------------------------------------------------
// MAIN TDS
// -----------------------------------------------------

#define TDS_EXCELLENT 300.0
#define TDS_GOOD 500.0
#define TDS_FAIR 700.0
#define TDS_HIGH 1000.0

#define TDS_CRITICAL 1000.0


// -----------------------------------------------------
// SLAVE TURBIDITY
// -----------------------------------------------------

#define TURBIDITY_THRESHOLD 1800


// =====================================================
//                     SENSOR OBJECTS
// =====================================================

OneWire oneWire(TEMP_PIN);

DallasTemperature temperatureSensor(&oneWire);

TinyGPSPlus gps;

HardwareSerial GPSserial(2);


// =====================================================
//                    GPS VARIABLES
// =====================================================

double referenceLatitude = 23.798881;

double referenceLongitude = 90.442238;


double currentLatitude = 0;

double currentLongitude = 0;

double currentAltitude = 0;

double currentSpeed = 0;

int currentSatellites = 0;

double gpsDistance = 0;


bool gpsValid = false;

bool gpsOutside = false;

int gpsOutsideCounter = 0;


// =====================================================
//                  SENSOR VARIABLES
// =====================================================

int water1 = 0;
int water2 = 0;
int water3 = 0;
int water4 = 0;

int moisture = 0;

float temperature = 0;


// -----------------------------------------------------
// MAIN TDS variables
// -----------------------------------------------------

int tdsRaw = 0;

float tdsVoltage = 0.0;

float tdsValue = 0.0;


// =====================================================
//                  SLAVE TURBIDITY + pH
// =====================================================

// IMPORTANT:
// This structure MUST exactly match the SLAVE.

typedef struct {

  int adcValue;

  char status[20];

  int phRaw;

  int phMilliVolts;

} SensorData;


SensorData slaveData;


bool slaveConnected = false;

unsigned long lastSlaveDataTime = 0;


// -----------------------------------------------------
// Turbidity
// -----------------------------------------------------

int turbidityRaw = 0;

bool turbidityWarning = false;


// -----------------------------------------------------
// pH raw/calibration data
// -----------------------------------------------------

int phRawADC = 0;

int phGPIOmV = 0;

float phGPIOVoltage = 0.0;

float phPoVoltage = 0.0;


// IMPORTANT:
// pH is NOT calibrated yet.
// We will NOT claim an actual pH value
// until buffer calibration is performed.

bool phCalibrated = false;


// =====================================================
//                    MPU6500 VARIABLES
// =====================================================

bool mpuDetected = false;


float accelX = 0;
float accelY = 0;
float accelZ = 0;

float gyroX = 0;
float gyroY = 0;
float gyroZ = 0;

float totalAcceleration = 0;

float totalGyro = 0;

float rollAngle = 0;

float pitchAngle = 0;

bool suddenMovement = false;

bool gyroMovement = false;

bool mpuTilt = false;


// =====================================================
//                 MPU6500 REGISTERS
// =====================================================

#define MPU_WHO_AM_I 0x75

#define MPU_PWR_MGMT_1 0x6B

#define MPU_CONFIG 0x1A

#define MPU_GYRO_CONFIG 0x1B

#define MPU_ACCEL_CONFIG 0x1C

#define MPU_ACCEL_CONFIG2 0x1D

#define MPU_ACCEL_XOUT_H 0x3B


// =====================================================
//                PREVIOUS SENSOR STATES
// =====================================================

bool previousDrowning = false;

bool previousSevereTilt = false;

bool previousTilt = false;

bool previousVibration = false;

bool previousMotion = false;

bool previousTemperatureWarning = false;

bool previousMoistureWarning = false;

bool previousSuddenMovement = false;

bool previousGyroMovement = false;

bool previousMPUTilt = false;

bool previousGPSOutside = false;

bool previousTDSCritical = false;

bool previousTurbidityWarning = false;


// =====================================================
//                 TELEGRAM VARIABLES
// =====================================================

unsigned long lastTelegramCheck = 0;

const unsigned long telegramInterval = 1000;


// =====================================================
//                 SERIAL PRINT TIMER
// =====================================================

unsigned long lastSerialPrint = 0;

const unsigned long serialInterval = 1000;


// =====================================================
//                     WIFI SETUP
// =====================================================

void connectWiFi() {

  Serial.println();

  Serial.println("Connecting to WiFi...");

  WiFi.begin(
    WIFI_SSID,
    WIFI_PASSWORD
  );

  while (
    WiFi.status() != WL_CONNECTED
  ) {

    delay(500);

    Serial.print(".");
  }

  Serial.println();

  Serial.println(
    "WiFi connected!"
  );

  Serial.print(
    "ESP32 IP: "
  );

  Serial.println(
    WiFi.localIP()
  );

  secured_client.setInsecure();

  Serial.println(
    "Telegram connection ready."
  );
}


// =====================================================
//              SEND TELEGRAM MESSAGE
// =====================================================

void sendTelegram(String message) {

  bool result =
    bot.sendMessage(
      CHAT_ID,
      message,
      ""
    );

  if (result) {

    Serial.println(
      "Telegram message sent."
    );

  } else {

    Serial.println(
      "Telegram message FAILED."
    );
  }
}


// =====================================================
//                 MPU6500 WRITE
// =====================================================

void writeMPURegister(
  byte reg,
  byte value
) {

  Wire.beginTransmission(
    MPU6500_ADDRESS
  );

  Wire.write(reg);

  Wire.write(value);

  Wire.endTransmission();
}


// =====================================================
//                 MPU6500 READ
// =====================================================

byte readMPURegister(
  byte reg
) {

  Wire.beginTransmission(
    MPU6500_ADDRESS
  );

  Wire.write(reg);

  Wire.endTransmission(false);

  Wire.requestFrom(
    MPU6500_ADDRESS,
    1
  );

  if (Wire.available()) {

    return Wire.read();
  }

  return 0xFF;
}


// =====================================================
//              INITIALIZE MPU6500
// =====================================================

bool initializeMPU6500() {

  Serial.println();

  Serial.println(
    "======================================"
  );

  Serial.println(
    "       MPU6500 INITIALIZATION"
  );

  Serial.println(
    "======================================"
  );


  Wire.begin(
    I2C_SDA,
    I2C_SCL
  );

  delay(100);


  byte whoAmI =
    readMPURegister(
      MPU_WHO_AM_I
    );


  Serial.print(
    "WHO_AM_I = 0x"
  );


  if (whoAmI < 16) {

    Serial.print("0");
  }


  Serial.println(
    whoAmI,
    HEX
  );


  if (whoAmI != 0x70) {

    Serial.println(
      "❌ MPU6500 NOT DETECTED!"
    );

    mpuDetected = false;

    return false;
  }


  Serial.println(
    "✅ MPU6500 DETECTED!"
  );


  writeMPURegister(
    MPU_PWR_MGMT_1,
    0x00
  );

  delay(100);


  writeMPURegister(
    MPU_CONFIG,
    0x03
  );


  writeMPURegister(
    MPU_GYRO_CONFIG,
    0x00
  );


  writeMPURegister(
    MPU_ACCEL_CONFIG,
    0x00
  );


  writeMPURegister(
    MPU_ACCEL_CONFIG2,
    0x03
  );


  delay(100);


  mpuDetected = true;


  Serial.println(
    "MPU6500 configuration complete."
  );


  return true;
}


// =====================================================
//                  READ MPU6500
// =====================================================

void readMPU() {

  if (!mpuDetected) {

    return;
  }


  Wire.beginTransmission(
    MPU6500_ADDRESS
  );

  Wire.write(
    MPU_ACCEL_XOUT_H
  );


  if (
    Wire.endTransmission(false)
    != 0
  ) {

    mpuDetected = false;

    Serial.println(
      "MPU6500 communication lost!"
    );

    return;
  }


  Wire.requestFrom(
    MPU6500_ADDRESS,
    14
  );


  if (
    Wire.available() < 14
  ) {

    mpuDetected = false;

    Serial.println(
      "MPU6500 data read failed!"
    );

    return;
  }


  int16_t rawAx =
    (Wire.read() << 8)
    | Wire.read();


  int16_t rawAy =
    (Wire.read() << 8)
    | Wire.read();


  int16_t rawAz =
    (Wire.read() << 8)
    | Wire.read();


  Wire.read();

  Wire.read();


  int16_t rawGx =
    (Wire.read() << 8)
    | Wire.read();


  int16_t rawGy =
    (Wire.read() << 8)
    | Wire.read();


  int16_t rawGz =
    (Wire.read() << 8)
    | Wire.read();


  accelX =
    ((float)rawAx / 16384.0)
    * 9.80665;


  accelY =
    ((float)rawAy / 16384.0)
    * 9.80665;


  accelZ =
    ((float)rawAz / 16384.0)
    * 9.80665;


  gyroX =
    (float)rawGx / 131.0;


  gyroY =
    (float)rawGy / 131.0;


  gyroZ =
    (float)rawGz / 131.0;


  totalAcceleration =
    sqrt(
      accelX * accelX +
      accelY * accelY +
      accelZ * accelZ
    );


  totalGyro =
    sqrt(
      gyroX * gyroX +
      gyroY * gyroY +
      gyroZ * gyroZ
    );


  rollAngle =
    atan2(
      accelY,
      accelZ
    )
    * 57.2958;


  pitchAngle =
    atan2(
      -accelX,
      sqrt(
        accelY * accelY +
        accelZ * accelZ
      )
    )
    * 57.2958;


  suddenMovement =
    abs(
      totalAcceleration - 9.80665
    )
    > ACCELERATION_WARNING;


  gyroMovement =
    totalGyro > GYRO_WARNING;


  mpuTilt =
    (
      abs(rollAngle)
      > MPU_TILT_WARNING
    )
    ||
    (
      abs(pitchAngle)
      > MPU_TILT_WARNING
    );
}


// =====================================================
//                    READ GPS
// =====================================================

void readGPS() {

  while (
    GPSserial.available()
  ) {

    char c =
      GPSserial.read();

    gps.encode(c);
  }


  if (
    gps.location.isUpdated()
  ) {

    currentLatitude =
      gps.location.lat();


    currentLongitude =
      gps.location.lng();


    currentAltitude =
      gps.altitude.meters();


    currentSpeed =
      gps.speed.kmph();


    currentSatellites =
      gps.satellites.value();


    gpsValid = true;


    gpsDistance =
      TinyGPSPlus::distanceBetween(

        currentLatitude,
        currentLongitude,

        referenceLatitude,
        referenceLongitude
      );


    if (
      gpsDistance >
      GPS_RADIUS
    ) {

      gpsOutsideCounter++;


      if (
        gpsOutsideCounter >=
        GPS_CONFIRMATIONS_REQUIRED
      ) {

        gpsOutside = true;
      }

    } else {

      gpsOutsideCounter = 0;

      gpsOutside = false;
    }
  }
}


// =====================================================
//                  READ MAIN TDS SENSOR
// =====================================================

void readTDS() {

  const int samples = 30;

  long totalADC = 0;


  for (
    int i = 0;
    i < samples;
    i++
  ) {

    totalADC +=
      analogRead(TDS_PIN);

    delay(2);
  }


  tdsRaw =
    totalADC / samples;


  tdsVoltage =
    (tdsRaw / 4095.0)
    * 3.3;


  float compensationCoefficient =
    1.0;


  if (
    temperature !=
      DEVICE_DISCONNECTED_C &&

    temperature > -20 &&

    temperature < 80
  ) {

    compensationCoefficient =
      1.0 +
      0.02 *
      (temperature - 25.0);
  }


  float compensationVoltage =
    tdsVoltage /
    compensationCoefficient;


  float ec =
    133.42 *
    compensationVoltage *
    compensationVoltage *
    compensationVoltage

    -

    255.86 *
    compensationVoltage *
    compensationVoltage

    +

    857.39 *
    compensationVoltage;


  tdsValue =
    ec * 0.5;


  if (
    tdsValue < 0
  ) {

    tdsValue = 0;
  }
}


// =====================================================
//                TDS QUALITY STATUS
// =====================================================

String getTDSStatus() {

  if (
    tdsValue <= TDS_EXCELLENT
  ) {

    return "EXCELLENT";
  }

  else if (
    tdsValue <= TDS_GOOD
  ) {

    return "GOOD";
  }

  else if (
    tdsValue <= TDS_FAIR
  ) {

    return "FAIR";
  }

  else if (
    tdsValue <= TDS_HIGH
  ) {

    return "HIGH";
  }

  else {

    return "VERY HIGH";
  }
}


// =====================================================
//              READ SLAVE TURBIDITY + pH
// =====================================================

void processSlaveData() {

  if (!slaveConnected) {

    return;
  }


  // ---------------------------------------------------
  // TURBIDITY
  // ---------------------------------------------------

  turbidityRaw =
    slaveData.adcValue;


  turbidityWarning =
    turbidityRaw <
    TURBIDITY_THRESHOLD;


  // ---------------------------------------------------
  // pH RAW DATA
  // ---------------------------------------------------

  phRawADC =
    slaveData.phRaw;


  phGPIOmV =
    slaveData.phMilliVolts;


  // GPIO35 voltage after the divider
  phGPIOVoltage =
    phGPIOmV / 1000.0;


  // Two equal 4.7k resistors divide Po by 2
  phPoVoltage =
    phGPIOVoltage * 2.0;
}


// =====================================================
//              ESP-NOW RECEIVE CALLBACK
// =====================================================

void OnDataRecv(
  const esp_now_recv_info_t *info,
  const uint8_t *incomingData,
  int len
) {

  if (
    len != sizeof(SensorData)
  ) {

    Serial.print(
      "Invalid SLAVE packet size: "
    );

    Serial.println(
      len
    );

    return;
  }


  memcpy(
    &slaveData,
    incomingData,
    sizeof(SensorData)
  );


  slaveConnected = true;

  lastSlaveDataTime =
    millis();


  processSlaveData();
}


// =====================================================
//                  READ SENSOR VALUES
// =====================================================

void readSensors() {

  water1 =
    analogRead(WATER1_PIN);


  water2 =
    analogRead(WATER2_PIN);


  water3 =
    analogRead(WATER3_PIN);


  water4 =
    analogRead(WATER4_PIN);


  moisture =
    analogRead(MOISTURE_PIN);


  temperatureSensor.requestTemperatures();


  temperature =
    temperatureSensor.getTempCByIndex(0);


  readTDS();
}


// =====================================================
//              WATER STATUS
// =====================================================

String getWaterStatus() {

  int crossed = 0;


  if (
    water1 >=
    WATER_THRESHOLD
  )
    crossed++;


  if (
    water2 >=
    WATER_THRESHOLD
  )
    crossed++;


  if (
    water3 >=
    WATER_THRESHOLD
  )
    crossed++;


  if (
    water4 >=
    WATER_THRESHOLD
  )
    crossed++;


  if (
    crossed == 4
  )

    return "DROWNING / SINKING";


  else if (
    crossed == 3
  )

    return "SEVERELY TILTED";


  else if (
    crossed == 1 ||
    crossed == 2
  )

    return "TILTED";


  else

    return "NORMAL";
}


// =====================================================
//              TURBIDITY STATUS
// =====================================================

String getTurbidityStatus() {

  if (!slaveConnected) {

    return "SLAVE OFFLINE";
  }


  if (
    turbidityRaw <
    TURBIDITY_THRESHOLD
  ) {

    return "LOW TURBIDITY";
  }


  return "NORMAL";
}


// =====================================================
//                  pH STATUS
// =====================================================

String getPHStatus() {

  if (!slaveConnected) {

    return "SLAVE OFFLINE";
  }


  return "NOT CALIBRATED";
}


// =====================================================
//                  pH CALIBRATION
// =====================================================

float getCalibratedPH() {

  return 20.88889 - (5.55556 * phPoVoltage);
}


// =====================================================
//                  pH MESSAGE
// =====================================================

String getPHMessage() {

  String message = "";


  message +=
    "🧪 pH SENSOR\n";

  message +=
    "========================\n\n";


  if (
    !slaveConnected ||
    millis() - lastSlaveDataTime >= 5000
  ) {

    message +=
      "SLAVE: OFFLINE\n";

    return message;
  }


  message +=
    "pH Value: ";

  message +=
    String(
      getCalibratedPH(),
      2
    );

  message +=
    "\n\n";


  message +=
    "Raw ADC: ";

  message +=
    String(
      phRawADC
    );

  message +=
    "\n";


  message +=
    "GPIO35 Voltage: ";

  message +=
    String(
      phGPIOVoltage,
      3
    );

  message +=
    " V\n";


  message +=
    "Original Po Voltage: ";

  message +=
    String(
      phPoVoltage,
      3
    );

  message +=
    " V\n\n";


  return message;
}


// =====================================================
//                  STATUS MESSAGE
// =====================================================

String getStatusMessage() {

  String message = "";


  message +=
    "🌱 FLOATING BED STATUS\n";

  message +=
    "========================\n\n";


  // ---------------------------------------------------
  // Water
  // ---------------------------------------------------

  message +=
    "💧 WATER SENSORS\n";


  message +=
    "Sensor 1: ";

  message +=
    String(water1);

  message += "\n";


  message +=
    "Sensor 2: ";

  message +=
    String(water2);

  message += "\n";


  message +=
    "Sensor 3: ";

  message +=
    String(water3);

  message += "\n";


  message +=
    "Sensor 4: ";

  message +=
    String(water4);

  message += "\n";


  message +=
    "Status: ";

  message +=
    getWaterStatus();


  message +=
    "\n\n";


  // ---------------------------------------------------
  // Soil moisture
  // ---------------------------------------------------

  message +=
    "🌱 Soil Moisture: ";

  message +=
    String(moisture);

  message +=
    "\n";


  message +=
    "Threshold: ";

  message +=
    String(MOISTURE_WARNING);

  message +=
    "\n\n";


  // ---------------------------------------------------
  // Temperature
  // ---------------------------------------------------

  message +=
    "🌡️ Temperature: ";


  if (
    temperature ==
    DEVICE_DISCONNECTED_C
  ) {

    message +=
      "SENSOR ERROR";

  } else {

    message +=
      String(
        temperature,
        1
      );

    message +=
      " °C";
  }


  message +=
    "\n";


  message +=
    "Warning threshold: ";

  message +=
    String(
      TEMPERATURE_WARNING,
      1
    );

  message +=
    " °C\n\n";


  // ---------------------------------------------------
  // MAIN TDS
  // ---------------------------------------------------

  message +=
    "💧 MAIN TDS SENSOR\n";


  message +=
    "Raw ADC: ";

  message +=
    String(tdsRaw);

  message += "\n";


  message +=
    "Voltage: ";

  message +=
    String(
      tdsVoltage,
      3
    );

  message +=
    " V\n";


  message +=
    "TDS: ";

  message +=
    String(
      tdsValue,
      1
    );

  message +=
    " ppm\n";


  message +=
    "TDS Level: ";

  message +=
    getTDSStatus();


  message +=
    "\n\n";


  // ---------------------------------------------------
  // SLAVE TURBIDITY
  // ---------------------------------------------------

  message +=
    "🌊 TURBIDITY SENSOR (SLAVE)\n";


  if (
    slaveConnected &&
    millis() - lastSlaveDataTime < 5000
  ) {

    message +=
      "SLAVE: ONLINE\n";


    message +=
      "Raw ADC: ";

    message +=
      String(
        turbidityRaw
      );

    message += "\n";


    message +=
      "Sensor status: ";

    message +=
      String(
        slaveData.status
      );

    message += "\n";


    message +=
      "Turbidity status: ";

    message +=
      getTurbidityStatus();

    message += "\n";


    message +=
      "Threshold: ";

    message +=
      String(
        TURBIDITY_THRESHOLD
      );

  } else {

    message +=
      "SLAVE: OFFLINE";
  }


  message +=
    "\n\n";


  // ---------------------------------------------------
  // pH
  // ---------------------------------------------------

  message +=
    "🧪 pH SENSOR (SLAVE)\n";


  if (
    slaveConnected &&
    millis() - lastSlaveDataTime < 5000
  ) {

    message +=
      "pH Value: ";

    message +=
      String(
        getCalibratedPH(),
        2
      );

    message +=
      "\n";


    message +=
      "Raw ADC: ";

    message +=
      String(
        phRawADC
      );

    message +=
      "\n";


    message +=
      "GPIO35 Voltage: ";

    message +=
      String(
        phGPIOVoltage,
        3
      );

    message +=
      " V\n";


    message +=
      "Original Po Voltage: ";

    message +=
      String(
        phPoVoltage,
        3
      );

    message +=
      " V";

  } else {

    message +=
      "SLAVE: OFFLINE";
  }


  message +=
    "\n\n";


  // ---------------------------------------------------
  // PIR
  // ---------------------------------------------------

  message +=
    "🚶 PIR: ";

  message +=
    digitalRead(PIR_PIN)
    ? "MOTION DETECTED"
    : "NO MOTION";


  message +=
    "\n";


  // ---------------------------------------------------
  // Vibration
  // ---------------------------------------------------

  message +=
    "📳 Vibration: ";

  message +=
    digitalRead(VIBRATION_PIN)
    ? "DETECTED"
    : "NO VIBRATION";


  message +=
    "\n\n";


  // ---------------------------------------------------
  // MPU6500
  // ---------------------------------------------------

  message +=
    "📐 MPU6500\n";


  if (!mpuDetected) {

    message +=
      "Status: NOT DETECTED\n";

    message +=
      "Acceleration: --\n";

    message +=
      "Gyroscope: --\n";

    message +=
      "Tilt: --";

  } else {

    message +=
      "Status: DETECTED\n";


    message +=
      "Acceleration: ";

    message +=
      String(
        totalAcceleration,
        2
      );

    message +=
      " m/s²\n";


    message +=
      "Gyroscope: ";

    message +=
      String(
        totalGyro,
        1
      );

    message +=
      " deg/s\n";


    message +=
      "Roll: ";

    message +=
      String(
        rollAngle,
        1
      );

    message +=
      "°\n";


    message +=
      "Pitch: ";

    message +=
      String(
        pitchAngle,
        1
      );

    message +=
      "°\n";


    message +=
      "Tilt: ";

    message +=
      mpuTilt
      ? "DETECTED"
      : "NORMAL";
  }


  // ---------------------------------------------------
  // GPS
  // ---------------------------------------------------

  message +=
    "\n\n📍 GPS\n";


  if (!gpsValid) {

    message +=
      "GPS: NO FIX";

  } else {

    message +=
      "Latitude: ";

    message +=
      String(
        currentLatitude,
        6
      );

    message +=
      "\n";


    message +=
      "Longitude: ";

    message +=
      String(
        currentLongitude,
        6
      );

    message +=
      "\n";


    message +=
      "Satellites: ";

    message +=
      String(
        currentSatellites
      );

    message +=
      "\n";


    message +=
      "Speed: ";

    message +=
      String(
        currentSpeed,
        2
      );

    message +=
      " km/h\n";


    message +=
      "Altitude: ";

    message +=
      String(
        currentAltitude,
        1
      );

    message +=
      " m\n";


    message +=
      "Distance from reference: ";

    message +=
      String(
        gpsDistance,
        2
      );

    message +=
      " m\n";


    message +=
      "Geofence: ";

    message +=
      gpsOutside
      ? "🚨 OUTSIDE"
      : "✅ INSIDE";
  }


  return message;
}


// =====================================================
//              TELEGRAM COMMAND HANDLER
// =====================================================

void handleTelegramMessages(
  int numNewMessages
) {

  for (
    int i = 0;
    i < numNewMessages;
    i++
  ) {

    String chat_id =
      bot.messages[i].chat_id;


    String text =
      bot.messages[i].text;


    // -------------------------------------------------
    // Security
    // -------------------------------------------------

    if (
      chat_id != CHAT_ID
    ) {

      bot.sendMessage(
        chat_id,
        "Unauthorized user.",
        ""
      );

      continue;
    }


    // =================================================
    // /start
    // =================================================

    if (
      text == "/start"
    ) {

      String message = "";


      message +=
        "🌱 FLOATING BED MONITORING SYSTEM\n\n";


      message +=
        "Commands:\n\n";


      message +=
        "/status - Complete system status\n";


      message +=
        "/tds - Main TDS sensor\n";


      message +=
        "/temperature - Temperature\n";


      message +=
        "/moisture - Soil moisture\n";


      message +=
        "/water - Water sensors\n";


      message +=
        "/turbidity - SLAVE turbidity sensor\n";


      message +=
        "/ph - SLAVE pH raw data\n";


      message +=
        "/location - GPS information\n";


      message +=
        "/setlocation - Set GPS reference\n";


      message +=
        "/sethere - Use current GPS as reference\n";


      message +=
        "/reference - Show reference location\n";


      message +=
        "/help - Show commands\n";


      bot.sendMessage(
        chat_id,
        message,
        ""
      );
    }


    // =================================================
    // /help
    // =================================================

    else if (
      text == "/help"
    ) {

      String message = "";


      message +=
        "COMMANDS\n";

      message +=
        "================\n\n";


      message +=
        "/status\n";

      message +=
        "Complete sensor status.\n\n";


      message +=
        "/tds\n";

      message +=
        "Main TDS sensor.\n\n";


      message +=
        "/turbidity\n";

      message +=
        "SLAVE turbidity sensor.\n\n";


      message +=
        "/ph\n";

      message +=
        "SLAVE pH raw/calibration data.\n\n";


      message +=
        "/temperature\n";

      message +=
        "Current temperature.\n\n";


      message +=
        "/moisture\n";

      message +=
        "Current soil moisture.\n\n";


      message +=
        "/water\n";

      message +=
        "Water sensor readings.\n\n";


      message +=
        "/location\n";

      message +=
        "Current GPS position.\n\n";


      message +=
        "/reference\n";

      message +=
        "Current GPS reference point.\n\n";


      message +=
        "/sethere\n";

      message +=
        "Set current GPS position as reference.\n\n";


      message +=
        "/setlocation\n";

      message +=
        "Set custom latitude and longitude.\n\n";


      message +=
        "Example:\n";

      message +=
        "/setlocation 23.798881 90.442238";


      bot.sendMessage(
        chat_id,
        message,
        ""
      );
    }


    // =================================================
    // /status
    // =================================================

    else if (
      text == "/status"
    ) {

      bot.sendMessage(
        chat_id,
        getStatusMessage(),
        ""
      );
    }


    // =================================================
    // /tds
    // =================================================

    else if (
      text == "/tds"
    ) {

      String message = "";


      message +=
        "💧 MAIN TDS STATUS\n\n";


      message +=
        "Raw ADC: ";


      message +=
        String(tdsRaw);


      message +=
        "\n";


      message +=
        "Voltage: ";


      message +=
        String(
          tdsVoltage,
          3
        );


      message +=
        " V\n";


      message +=
        "TDS: ";


      message +=
        String(
          tdsValue,
          1
        );


      message +=
        " ppm\n";


      message +=
        "Level: ";


      message +=
        getTDSStatus();


      message +=
        "\n\n";


      message +=
        "Project classification:\n";


      message +=
        "0-300 ppm = EXCELLENT\n";


      message +=
        "300-500 ppm = GOOD\n";


      message +=
        "500-700 ppm = FAIR\n";


      message +=
        "700-1000 ppm = HIGH\n";


      message +=
        ">1000 ppm = VERY HIGH";


      bot.sendMessage(
        chat_id,
        message,
        ""
      );
    }


    // =================================================
    // /turbidity
    // =================================================

    else if (
      text == "/turbidity"
    ) {

      String message = "";


      message +=
        "🌊 SLAVE TURBIDITY SENSOR\n\n";


      if (
        slaveConnected &&
        millis() - lastSlaveDataTime < 5000
      ) {

        message +=
          "SLAVE: ONLINE\n";


        message +=
          "Raw ADC: ";


        message +=
          String(
            turbidityRaw
          );


        message +=
          "\n";


        message +=
          "Sensor status: ";


        message +=
          String(
            slaveData.status
          );


        message +=
          "\n";


        message +=
          "Threshold: ";


        message +=
          String(
            TURBIDITY_THRESHOLD
          );


        message +=
          "\n";


        message +=
          "Status: ";


        message +=
          getTurbidityStatus();

      } else {

        message +=
          "SLAVE OFFLINE.";
      }


      bot.sendMessage(
        chat_id,
        message,
        ""
      );
    }


    // =================================================
    // /ph
    // =================================================

    else if (
      text == "/ph"
    ) {

      bot.sendMessage(
        chat_id,
        getPHMessage(),
        ""
      );
    }


    // =================================================
    // /temperature
    // =================================================

    else if (
      text == "/temperature"
    ) {

      String message = "";


      if (
        temperature ==
        DEVICE_DISCONNECTED_C
      ) {

        message =
          "❌ Temperature sensor error.";

      } else {

        message =
          "🌡️ Temperature: ";


        message +=
          String(
            temperature,
            1
          );


        message +=
          " °C";


        if (
          temperature >=
          TEMPERATURE_WARNING
        ) {

          message +=
            "\n⚠️ WARNING: Temperature is above 35°C.";
        }
      }


      bot.sendMessage(
        chat_id,
        message,
        ""
      );
    }


    // =================================================
    // /moisture
    // =================================================

    else if (
      text == "/moisture"
    ) {

      String message = "";


      message +=
        "🌱 Soil Moisture: ";


      message +=
        String(moisture);


      message +=
        "\nThreshold: ";


      message +=
        String(
          MOISTURE_WARNING
        );


      if (
        moisture <
        MOISTURE_WARNING
      ) {

        message +=
          "\n⚠️ Moisture warning.";
      }


      bot.sendMessage(
        chat_id,
        message,
        ""
      );
    }


    // =================================================
    // /water
    // =================================================

    else if (
      text == "/water"
    ) {

      String message = "";


      message +=
        "💧 WATER SYSTEM\n\n";


      message +=
        "Sensor 1: ";


      message +=
        String(water1);


      message +=
        "\n";


      message +=
        "Sensor 2: ";


      message +=
        String(water2);


      message +=
        "\n";


      message +=
        "Sensor 3: ";


      message +=
        String(water3);


      message +=
        "\n";


      message +=
        "Sensor 4: ";


      message +=
        String(water4);


      message +=
        "\n\n";


      message +=
        "Status: ";


      message +=
        getWaterStatus();


      bot.sendMessage(
        chat_id,
        message,
        ""
      );
    }


    // =================================================
    // /location
    // =================================================

    else if (
      text == "/location"
    ) {

      String message = "";


      message +=
        "📍 CURRENT GPS LOCATION\n\n";


      if (!gpsValid) {

        message +=
          "GPS has no valid fix yet.";

      } else {

        message +=
          "Latitude: ";


        message +=
          String(
            currentLatitude,
            6
          );


        message +=
          "\n";


        message +=
          "Longitude: ";


        message +=
          String(
            currentLongitude,
            6
          );


        message +=
          "\n";


        message +=
          "Satellites: ";


        message +=
          String(
            currentSatellites
          );


        message +=
          "\n";


        message +=
          "Speed: ";


        message +=
          String(
            currentSpeed,
            2
          );


        message +=
          " km/h\n";


        message +=
          "Altitude: ";


        message +=
          String(
            currentAltitude,
            1
          );


        message +=
          " m\n";


        message +=
          "Distance from reference: ";


        message +=
          String(
            gpsDistance,
            2
          );


        message +=
          " m\n\n";


        message +=
          "Geofence: ";


        message +=
          gpsOutside
          ? "🚨 OUTSIDE"
          : "✅ INSIDE";


        message +=
          "\n\n";


        message +=
          "https://maps.google.com/?q=";


        message +=
          String(
            currentLatitude,
            6
          );


        message +=
          ",";


        message +=
          String(
            currentLongitude,
            6
          );
      }


      bot.sendMessage(
        chat_id,
        message,
        ""
      );
    }


    // =================================================
    // /reference
    // =================================================

    else if (
      text == "/reference"
    ) {

      String message = "";


      message +=
        "📍 GPS REFERENCE POINT\n\n";


      message +=
        "Latitude: ";


      message +=
        String(
          referenceLatitude,
          6
        );


      message +=
        "\n";


      message +=
        "Longitude: ";


      message +=
        String(
          referenceLongitude,
          6
        );


      message +=
        "\n\n";


      message +=
        "Radius: 200 meters";


      bot.sendMessage(
        chat_id,
        message,
        ""
      );
    }


    // =================================================
    // /sethere
    // =================================================

    else if (
      text == "/sethere"
    ) {

      if (!gpsValid) {

        bot.sendMessage(
          chat_id,
          "❌ GPS does not have a valid location yet.",
          ""
        );

      } else {

        referenceLatitude =
          currentLatitude;


        referenceLongitude =
          currentLongitude;


        gpsOutside = false;

        gpsOutsideCounter = 0;


        String message = "";


        message +=
          "✅ GPS REFERENCE UPDATED\n\n";


        message +=
          "Latitude: ";


        message +=
          String(
            referenceLatitude,
            6
          );


        message +=
          "\n";


        message +=
          "Longitude: ";


        message +=
          String(
            referenceLongitude,
            6
          );


        message +=
          "\n\n";


        message +=
          "Safe radius: 200 meters";


        bot.sendMessage(
          chat_id,
          message,
          ""
        );
      }
    }


    // =================================================
    // /setlocation
    // =================================================

    else if (
      text.startsWith(
        "/setlocation"
      )
    ) {

      String command =
        text;


      command.replace(
        "/setlocation",
        ""
      );


      command.trim();


      int spaceIndex =
        command.indexOf(' ');


      if (
        spaceIndex == -1
      ) {

        bot.sendMessage(
          chat_id,

          "❌ Invalid format.\n\n"
          "Use:\n"
          "/setlocation LATITUDE LONGITUDE\n\n"
          "Example:\n"
          "/setlocation 23.798881 90.442238",

          ""
        );

        continue;
      }


      String latString =
        command.substring(
          0,
          spaceIndex
        );


      String lonString =
        command.substring(
          spaceIndex + 1
        );


      double newLatitude =
        latString.toDouble();


      double newLongitude =
        lonString.toDouble();


      if (
        newLatitude < -90 ||
        newLatitude > 90 ||
        newLongitude < -180 ||
        newLongitude > 180
      ) {

        bot.sendMessage(
          chat_id,

          "❌ Invalid latitude or longitude.",

          ""
        );

        continue;
      }


      referenceLatitude =
        newLatitude;


      referenceLongitude =
        newLongitude;


      gpsOutside = false;

      gpsOutsideCounter = 0;


      String message = "";


      message +=
        "✅ GPS REFERENCE UPDATED\n\n";


      message +=
        "Latitude: ";


      message +=
        String(
          referenceLatitude,
          6
        );


      message +=
        "\n";


      message +=
        "Longitude: ";


      message +=
        String(
          referenceLongitude,
          6
        );


      message +=
        "\n\n";


      message +=
        "Safe radius: 200 meters";


      bot.sendMessage(
        chat_id,
        message,
        ""
      );
    }
  }
}


// =====================================================
//                CHECK TELEGRAM
// =====================================================

void checkTelegram() {

  if (
    millis() -
    lastTelegramCheck <
    telegramInterval
  ) {

    return;
  }


  lastTelegramCheck =
    millis();


  int numNewMessages =
    bot.getUpdates(
      bot.last_message_received + 1
    );


  while (
    numNewMessages
  ) {

    Serial.print(
      "Telegram messages received: "
    );


    Serial.println(
      numNewMessages
    );


    handleTelegramMessages(
      numNewMessages
    );


    numNewMessages =
      bot.getUpdates(
        bot.last_message_received + 1
      );
  }
}


// =====================================================
//              TELEGRAM ALERTS
// =====================================================

void checkAlerts() {

  int crossed = 0;


  if (
    water1 >=
    WATER_THRESHOLD
  )
    crossed++;


  if (
    water2 >=
    WATER_THRESHOLD
  )
    crossed++;


  if (
    water3 >=
    WATER_THRESHOLD
  )
    crossed++;


  if (
    water4 >=
    WATER_THRESHOLD
  )
    crossed++;


  bool drowning =
    crossed == 4;


  bool severeTilt =
    crossed == 3;


  bool tilted =
    crossed == 1 ||
    crossed == 2;


  bool vibrationDetected =
    digitalRead(
      VIBRATION_PIN
    );


  bool motionDetected =
    digitalRead(
      PIR_PIN
    );


  bool temperatureWarning =
    temperature >=
    TEMPERATURE_WARNING;


  bool moistureWarning =
    moisture <
    MOISTURE_WARNING;


  bool tdsCritical =
    tdsValue >
    TDS_CRITICAL;


  bool currentTurbidityWarning =
    turbidityWarning;


  // ---------------------------------------------------
  // Water alerts
  // ---------------------------------------------------

  if (
    drowning &&
    !previousDrowning
  ) {

    sendTelegram(
      "🚨 DROWNING / SINKING ALERT!\n\n"
      "All 4 water sensors crossed 2000."
    );
  }


  if (
    severeTilt &&
    !previousSevereTilt
  ) {

    sendTelegram(
      "⚠️ SEVERE TILT ALERT!\n\n"
      "3 water sensors crossed 2000."
    );
  }


  if (
    tilted &&
    !previousTilt
  ) {

    sendTelegram(
      "⚠️ TILT ALERT!\n\n"
      "1 or 2 water sensors crossed 2000."
    );
  }


  // ---------------------------------------------------
  // Vibration
  // ---------------------------------------------------

  if (
    vibrationDetected &&
    !previousVibration
  ) {

    sendTelegram(
      "📳 VIBRATION ALERT!\n\n"
      "Vibration detected."
    );
  }


  // ---------------------------------------------------
  // PIR
  // ---------------------------------------------------

  if (
    motionDetected &&
    !previousMotion
  ) {

    sendTelegram(
      "🚶 MOTION ALERT!\n\n"
      "PIR detected movement."
    );
  }


  // ---------------------------------------------------
  // Temperature
  // ---------------------------------------------------

  if (
    temperatureWarning &&
    !previousTemperatureWarning
  ) {

    String message =
      "🌡️ TEMPERATURE WARNING!\n\n";


    message +=
      "Temperature: ";


    message +=
      String(
        temperature,
        1
      );


    message +=
      " °C\n\n";


    message +=
      "Threshold: 35 °C";


    sendTelegram(
      message
    );
  }


  // ---------------------------------------------------
  // Moisture
  // ---------------------------------------------------

  if (
    moistureWarning &&
    !previousMoistureWarning
  ) {

    String message =
      "🌱 MOISTURE WARNING!\n\n";


    message +=
      "Moisture reading: ";


    message +=
      String(
        moisture
      );


    message +=
      "\n";


    message +=
      "Threshold: 1500";


    sendTelegram(
      message
    );
  }


  // ---------------------------------------------------
  // MPU sudden movement
  // ---------------------------------------------------

  if (
    suddenMovement &&
    !previousSuddenMovement
  ) {

    sendTelegram(
      "⚡ MPU6500 MOVEMENT ALERT!\n\n"
      "Sudden acceleration detected."
    );
  }


  // ---------------------------------------------------
  // MPU gyro
  // ---------------------------------------------------

  if (
    gyroMovement &&
    !previousGyroMovement
  ) {

    String message =
      "🔄 GYROSCOPE ALERT!\n\n";


    message +=
      "Sudden rotational movement detected.\n";


    message +=
      "Gyro: ";


    message +=
      String(
        totalGyro,
        1
      );


    message +=
      " deg/s";


    sendTelegram(
      message
    );
  }


  // ---------------------------------------------------
  // MPU tilt
  // ---------------------------------------------------

  if (
    mpuTilt &&
    !previousMPUTilt
  ) {

    String message =
      "📐 MPU6500 TILT ALERT!\n\n";


    message +=
      "Tilt greater than approximately 25 degrees detected.\n\n";


    message +=
      "Roll: ";


    message +=
      String(
        rollAngle,
        1
      );


    message +=
      "°\n";


    message +=
      "Pitch: ";


    message +=
      String(
        pitchAngle,
        1
      );


    message +=
      "°";


    sendTelegram(
      message
    );
  }


  // ---------------------------------------------------
  // TDS critical
  // ---------------------------------------------------

  if (
    tdsCritical &&
    !previousTDSCritical
  ) {

    String message =
      "🚨 VERY HIGH WATER TDS WARNING!\n\n";


    message +=
      "TDS: ";


    message +=
      String(
        tdsValue,
        1
      );


    message +=
      " ppm\n\n";


    message +=
      "Level: VERY HIGH\n";


    message +=
      "Threshold: >1000 ppm";


    sendTelegram(
      message
    );
  }


  // ---------------------------------------------------
  // TURBIDITY WARNING
  // ---------------------------------------------------

  if (
    currentTurbidityWarning &&
    !previousTurbidityWarning
  ) {

    String message =
      "🌊 TURBIDITY WARNING!\n\n";


    message +=
      "SLAVE turbidity sensor has dropped below the configured threshold.\n\n";


    message +=
      "Raw ADC: ";


    message +=
      String(
        turbidityRaw
      );


    message +=
      "\n";


    message +=
      "Threshold: ";


    message +=
      String(
        TURBIDITY_THRESHOLD
      );


    message +=
      "\n";


    message +=
      "Status: LOW TURBIDITY";


    sendTelegram(
      message
    );
  }


  // ---------------------------------------------------
  // GPS
  // ---------------------------------------------------

  if (
    gpsOutside &&
    !previousGPSOutside
  ) {

    String message =
      "🚨 GPS GEOFENCE ALERT!\n\n";


    message +=
      "Floating bed moved outside the 200 meter safe radius.\n\n";


    message +=
      "Distance from reference: ";


    message +=
      String(
        gpsDistance,
        2
      );


    message +=
      " meters\n\n";


    message +=
      "Latitude: ";


    message +=
      String(
        currentLatitude,
        6
      );


    message +=
      "\n";


    message +=
      "Longitude: ";


    message +=
      String(
        currentLongitude,
        6
      );


    message +=
      "\n\n";


    message +=
      "https://maps.google.com/?q=";


    message +=
      String(
        currentLatitude,
        6
      );


    message +=
      ",";


    message +=
      String(
        currentLongitude,
        6
      );


    sendTelegram(
      message
    );
  }


  // ---------------------------------------------------
  // Save previous states
  // ---------------------------------------------------

  previousDrowning =
    drowning;


  previousSevereTilt =
    severeTilt;


  previousTilt =
    tilted;


  previousVibration =
    vibrationDetected;


  previousMotion =
    motionDetected;


  previousTemperatureWarning =
    temperatureWarning;


  previousMoistureWarning =
    moistureWarning;


  previousSuddenMovement =
    suddenMovement;


  previousGyroMovement =
    gyroMovement;


  previousMPUTilt =
    mpuTilt;


  previousGPSOutside =
    gpsOutside;


  previousTDSCritical =
    tdsCritical;


  previousTurbidityWarning =
    currentTurbidityWarning;
}


// =====================================================
//                 OUTPUT CONTROL
// =====================================================

void controlOutputs() {

  int crossed = 0;


  if (
    water1 >=
    WATER_THRESHOLD
  )
    crossed++;


  if (
    water2 >=
    WATER_THRESHOLD
  )
    crossed++;


  if (
    water3 >=
    WATER_THRESHOLD
  )
    crossed++;


  if (
    water4 >=
    WATER_THRESHOLD
  )
    crossed++;


  bool waterAlarm =
    crossed > 0;


  bool motion =
    digitalRead(
      PIR_PIN
    );


  bool vibration =
    digitalRead(
      VIBRATION_PIN
    );


  bool tdsAlarm =
    tdsValue >
    TDS_CRITICAL;


  bool alarm =
    waterAlarm ||
    motion ||
    vibration ||
    suddenMovement ||
    gyroMovement ||
    mpuTilt ||
    gpsOutside ||
    tdsAlarm;


  // Turbidity is intentionally NOT added
  // to buzzer/LED logic.

  if (alarm) {

    digitalWrite(
      LED_PIN,
      HIGH
    );


    digitalWrite(
      BUZZER_PIN,
      HIGH
    );

  } else {

    digitalWrite(
      LED_PIN,
      LOW
    );


    digitalWrite(
      BUZZER_PIN,
      LOW
    );
  }
}


// =====================================================
//                 SERIAL MONITOR
// =====================================================

void printSerialStatus() {

  if (
    millis() -
    lastSerialPrint <
    serialInterval
  ) {

    return;
  }


  lastSerialPrint =
    millis();


  Serial.println();

  Serial.println(
    "======================================"
  );

  Serial.println(
    "       FLOATING BED MONITOR"
  );

  Serial.println(
    "======================================"
  );


  // ---------------------------------------------------
  // Water
  // ---------------------------------------------------

  Serial.println(
    "WATER"
  );


  Serial.print("S1: ");

  Serial.println(
    water1
  );


  Serial.print("S2: ");

  Serial.println(
    water2
  );


  Serial.print("S3: ");

  Serial.println(
    water3
  );


  Serial.print("S4: ");

  Serial.println(
    water4
  );


  Serial.print(
    "Status: "
  );


  Serial.println(
    getWaterStatus()
  );


  // ---------------------------------------------------
  // Moisture
  // ---------------------------------------------------

  Serial.println();


  Serial.print(
    "Soil Moisture [GPIO 39]: "
  );


  Serial.println(
    moisture
  );


  // ---------------------------------------------------
  // Temperature
  // ---------------------------------------------------

  Serial.print(
    "Temperature: "
  );


  if (
    temperature ==
    DEVICE_DISCONNECTED_C
  ) {

    Serial.println(
      "ERROR"
    );

  } else {

    Serial.print(
      temperature,
      1
    );


    Serial.println(
      " C"
    );
  }


  // ---------------------------------------------------
  // MAIN TDS
  // ---------------------------------------------------

  Serial.println();

  Serial.println(
    "MAIN TDS SENSOR [GPIO 36]"
  );


  Serial.print(
    "Raw ADC: "
  );


  Serial.println(
    tdsRaw
  );


  Serial.print(
    "Voltage: "
  );


  Serial.print(
    tdsVoltage,
    3
  );


  Serial.println(
    " V"
  );


  Serial.print(
    "TDS: "
  );


  Serial.print(
    tdsValue,
    1
  );


  Serial.println(
    " ppm"
  );


  Serial.print(
    "TDS Level: "
  );


  Serial.println(
    getTDSStatus()
  );


  // ---------------------------------------------------
  // SLAVE TURBIDITY
  // ---------------------------------------------------

  Serial.println();

  Serial.println(
    "SLAVE TURBIDITY"
  );


  Serial.print(
    "SLAVE: "
  );


  Serial.println(
    slaveConnected
    ? "ONLINE"
    : "OFFLINE"
  );


  if (slaveConnected) {

    Serial.print(
      "Raw ADC: "
    );


    Serial.println(
      turbidityRaw
    );


    Serial.print(
      "Sensor Status: "
    );


    Serial.println(
      slaveData.status
    );


    Serial.print(
      "Turbidity Status: "
    );


    Serial.println(
      getTurbidityStatus()
    );


    Serial.print(
      "Threshold: "
    );


    Serial.println(
      TURBIDITY_THRESHOLD
    );


    // -------------------------------------------------
    // pH
    // -------------------------------------------------

    Serial.println();

    Serial.println(
      "SLAVE pH SENSOR"
    );


    Serial.println(
      "Calibration: NOT CALIBRATED"
    );


    Serial.print(
      "Raw ADC: "
    );


    Serial.println(
      phRawADC
    );


    Serial.print(
      "GPIO35 Voltage: "
    );


    Serial.print(
      phGPIOVoltage,
      3
    );


    Serial.println(
      " V"
    );


    Serial.print(
      "Po Voltage: "
    );


    Serial.print(
      phPoVoltage,
      3
    );


    Serial.println(
      " V"
    );
  }


  // ---------------------------------------------------
  // PIR
  // ---------------------------------------------------

  Serial.print(
    "PIR: "
  );


  Serial.println(
    digitalRead(PIR_PIN)
    ? "MOTION"
    : "NO MOTION"
  );


  // ---------------------------------------------------
  // Vibration
  // ---------------------------------------------------

  Serial.print(
    "Vibration: "
  );


  Serial.println(
    digitalRead(VIBRATION_PIN)
    ? "DETECTED"
    : "NORMAL"
  );


  // ---------------------------------------------------
  // MPU6500
  // ---------------------------------------------------

  Serial.println();

  Serial.println(
    "MPU6500"
  );


  Serial.print(
    "Status: "
  );


  Serial.println(
    mpuDetected
    ? "DETECTED"
    : "NOT DETECTED"
  );


  if (mpuDetected) {

    Serial.print(
      "Acceleration: "
    );


    Serial.print(
      totalAcceleration,
      2
    );


    Serial.println(
      " m/s2"
    );


    Serial.print(
      "Gyroscope: "
    );


    Serial.print(
      totalGyro,
      1
    );


    Serial.println(
      " deg/s"
    );


    Serial.print(
      "Roll: "
    );


    Serial.print(
      rollAngle,
      1
    );


    Serial.println(
      " deg"
    );


    Serial.print(
      "Pitch: "
    );


    Serial.print(
      pitchAngle,
      1
    );


    Serial.println(
      " deg"
    );


    Serial.print(
      "Tilt: "
    );


    Serial.println(
      mpuTilt
      ? "DETECTED"
      : "NORMAL"
    );
  }


  // ---------------------------------------------------
  // GPS
  // ---------------------------------------------------

  Serial.println();

  Serial.println(
    "GPS"
  );


  if (!gpsValid) {

    Serial.println(
      "GPS: NO FIX"
    );

  } else {

    Serial.print(
      "Latitude: "
    );


    Serial.println(
      currentLatitude,
      6
    );


    Serial.print(
      "Longitude: "
    );


    Serial.println(
      currentLongitude,
      6
    );


    Serial.print(
      "Satellites: "
    );


    Serial.println(
      currentSatellites
    );


    Serial.print(
      "Speed: "
    );


    Serial.print(
      currentSpeed,
      2
    );


    Serial.println(
      " km/h"
    );


    Serial.print(
      "Distance from reference: "
    );


    Serial.print(
      gpsDistance,
      2
    );


    Serial.println(
      " m"
    );


    Serial.print(
      "Geofence: "
    );


    Serial.println(
      gpsOutside
      ? "OUTSIDE"
      : "INSIDE"
    );
  }


  // ---------------------------------------------------
  // Outputs
  // ---------------------------------------------------

  Serial.println();


  Serial.print(
    "LED: "
  );


  Serial.println(
    digitalRead(LED_PIN)
    ? "ON"
    : "OFF"
  );


  Serial.print(
    "BUZZER: "
  );


  Serial.println(
    digitalRead(BUZZER_PIN)
    ? "ON"
    : "OFF"
  );


  Serial.println(
    "======================================"
  );
}


// =====================================================
//                     ESP-NOW SETUP
// =====================================================

void setupESPNow() {

  Serial.println();

  Serial.println(
    "Initializing ESP-NOW..."
  );


  if (
    esp_now_init() != ESP_OK
  ) {

    Serial.println(
      "❌ ESP-NOW initialization failed!"
    );

    return;
  }


  esp_now_register_recv_cb(
    OnDataRecv
  );


  Serial.println(
    "✅ ESP-NOW receiver ready."
  );


  Serial.println(
    "Waiting for SLAVE broadcast..."
  );
}


// =====================================================
//                       SETUP
// =====================================================

void setup() {

  Serial.begin(
    115200
  );


  delay(1000);


  Serial.println();

  Serial.println(
    "======================================"
  );


  Serial.println(
    " FLOATING BED SYSTEM"
  );


  Serial.println(
    " MAIN ESP32"
  );


  Serial.println(
    "======================================"
  );


  // ---------------------------------------------------
  // Digital sensors
  // ---------------------------------------------------

  pinMode(
    PIR_PIN,
    INPUT
  );


  pinMode(
    VIBRATION_PIN,
    INPUT
  );


  // ---------------------------------------------------
  // Outputs
  // ---------------------------------------------------

  pinMode(
    LED_PIN,
    OUTPUT
  );


  pinMode(
    BUZZER_PIN,
    OUTPUT
  );


  digitalWrite(
    LED_PIN,
    LOW
  );


  digitalWrite(
    BUZZER_PIN,
    LOW
  );


  // ---------------------------------------------------
  // ADC
  // ---------------------------------------------------

  analogReadResolution(12);


  analogSetPinAttenuation(
    TDS_PIN,
    ADC_11db
  );


  analogSetPinAttenuation(
    MOISTURE_PIN,
    ADC_11db
  );


  // ---------------------------------------------------
  // Temperature
  // ---------------------------------------------------

  temperatureSensor.begin();


  // ---------------------------------------------------
  // GPS
  // ---------------------------------------------------

  GPSserial.begin(
    9600,
    SERIAL_8N1,
    GPS_RX_PIN,
    GPS_TX_PIN
  );


  Serial.println(
    "GPS serial started."
  );


  // ---------------------------------------------------
  // I2C
  // ---------------------------------------------------

  Wire.begin(
    I2C_SDA,
    I2C_SCL
  );


  // ---------------------------------------------------
  // MPU6500
  // ---------------------------------------------------

  initializeMPU6500();


  // ---------------------------------------------------
  // WiFi
  // ---------------------------------------------------

  connectWiFi();


  // ---------------------------------------------------
  // ESP-NOW
  // ---------------------------------------------------

  setupESPNow();


  // ---------------------------------------------------
  // System ready
  // ---------------------------------------------------

  Serial.println();

  Serial.println(
    "======================================"
  );


  Serial.println(
    " FLOATING BED SYSTEM ONLINE"
  );


  Serial.println(
    "======================================"
  );


  if (mpuDetected) {

    Serial.println(
      "MPU6500: DETECTED"
    );

  } else {

    Serial.println(
      "MPU6500: NOT DETECTED"
    );
  }


  sendTelegram(
    "🌱 Floating Bed Monitoring System is ONLINE.\n\n"
    "GPS geofence: 200 meters\n"
    "MAIN TDS monitoring: ACTIVE\n"
    "SLAVE turbidity monitoring: ACTIVE\n"
    "SLAVE pH monitoring: RAW DATA MODE\n"
    "pH calibration: NOT SET\n"
    "TDS very high warning: >1000 ppm\n"
    "Turbidity warning: below 1800 ADC\n\n"
    "MPU6500: "
    + String(
      mpuDetected
      ? "DETECTED"
      : "NOT DETECTED"
    )
    + "\n\n"
    "Use /status to view all sensors.\n"
    "Use /ph for pH raw data."
  );
}


// =====================================================
//                       LOOP
// =====================================================

void loop() {

  // ---------------------------------------------------
  // GPS
  // ---------------------------------------------------

  readGPS();


  // ---------------------------------------------------
  // Sensors
  // ---------------------------------------------------

  readSensors();


  // ---------------------------------------------------
  // MPU6500
  // ---------------------------------------------------

  readMPU();


  // ---------------------------------------------------
  // SLAVE TIMEOUT
  // ---------------------------------------------------

  if (
    slaveConnected &&
    millis() -
    lastSlaveDataTime >
    5000
  ) {

    slaveConnected = false;

    turbidityWarning = false;


    Serial.println(
      "⚠️ SLAVE connection timeout."
    );
  }


  // ---------------------------------------------------
  // Process SLAVE
  // ---------------------------------------------------

  processSlaveData();


  // ---------------------------------------------------
  // Outputs
  // ---------------------------------------------------

  controlOutputs();


  // ---------------------------------------------------
  // Serial
  // ---------------------------------------------------

  printSerialStatus();


  // ---------------------------------------------------
  // Telegram alerts
  // ---------------------------------------------------

  checkAlerts();


  // ---------------------------------------------------
  // Telegram commands
  // ---------------------------------------------------

  checkTelegram();


  delay(20);
}