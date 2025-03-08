/***********************************************************
 *  Example: wristbandv1.ino
 *  - Collect ECG (analog pin) and PPG (MAX30102) data
 *  - Collect SCD41 data (CO2, Temp, Humidity) every 5s
 *  - Bundle raw samples into a single chunk + attach SCD41
 *  - Send chunk to iOS for advanced processing
 ***********************************************************/

#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"
#include "SparkFun_SCD4x_Arduino_Library.h"
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

// ========== BLE DEFINITIONS ==========
#define SERVICE_UUID "12345678-1234-1234-1234-123456789abc"
#define DATA_CHARACTERISTIC_UUID "abcd5678-ab12-cd34-ef56-abcdef123456"

BLECharacteristic dataCharacteristic(DATA_CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_NOTIFY);

bool deviceConnected = false;
BLEAdvertising* pAdvertising;

class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
  }
  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    Serial.println("❌ Disconnected! Restarting advertising...");
    pServer->getAdvertising()->start();
  }
};

// ========== SENSOR DEFINITIONS & GLOBALS ==========

// -- MAX30102 (PPG) --
MAX30105 particleSensor;

// -- SCD41 (CO2, Temp, Humidity) --
SCD4x scd41;
float latestCO2 = 0.0;
float latestTemp = 0.0;
float latestHumidity = 0.0;

// We'll read SCD41 every 5 seconds
unsigned long lastSCD41Read = 0;
const unsigned long SCD41_INTERVAL_MS = 5000;

// -- ECG & PPG Data Buffering --
#define SAMPLING_FREQUENCY 100         // e.g., 100 Hz
#define N_SAMPLES 100                  // e.g., 1-second chunk at 100 Hz

float ecgBuffer[N_SAMPLES];
float ppgBuffer[N_SAMPLES];
int sampleIndex = 0;

unsigned long lastSampleTime = 0;
unsigned long sampleInterval = 1000 / SAMPLING_FREQUENCY; // ms per sample

// Helper function to send the big chunk in smaller pieces
void sendInChunks(const String &fullStr, size_t maxChunkSize = 150) {
  // If you're negotiating an MTU of ~185 bytes, you can set maxChunkSize=150 or 180.
  // 150 is safe to account for overhead bytes.
  if (!deviceConnected){
    Serial.println("Device disconnected while sending in chunks");
    return;
  } 

  size_t totalLen = fullStr.length();
  size_t offset = 0;

  while (offset < totalLen) {
    size_t chunkLen = (totalLen - offset > maxChunkSize) ? maxChunkSize : (totalLen - offset);
    String sub = fullStr.substring(offset, offset + chunkLen);

    // Send this piece
    dataCharacteristic.setValue((uint8_t*) sub.c_str(), sub.length());
    dataCharacteristic.notify();

    offset += chunkLen;

    // A short delay helps ensure the BLE stack has time
    // to process before sending the next piece
    delay(20);
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin();

  // ========== Initialize SCD41 ==========
  if (!scd41.begin()) {
    Serial.println("Failed to initialize SCD41! Check wiring.");
  } else {
    Serial.println("SCD41 initialized.");
    scd41.startPeriodicMeasurement(); // Start measuring in periodic mode
  }

  // ========== Initialize MAX30102 ==========
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("MAX30102 not found. Check wiring!");
    while (1);
  }
  particleSensor.setup();
  particleSensor.setPulseAmplitudeRed(0x3F);
  particleSensor.setPulseAmplitudeGreen(0);

  // ========== BLE SETUP ==========
  BLEDevice::deinit();
  delay(100);
  BLEDevice::init("ESP32_Health_Monitor");
  BLEDevice::setMTU(185);
  BLEServer* pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());

  BLEService* pService = pServer->createService(SERVICE_UUID);
  pService->addCharacteristic(&dataCharacteristic);

  BLE2902* descriptor = new BLE2902();
  dataCharacteristic.addDescriptor(descriptor);
  descriptor->setNotifications(true);

  pService->start();
  pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->start();

  Serial.println("📡 BLE Advertising started");
}

void loop() {
  unsigned long currentMillis = millis();

  // ========== 2) Sample ECG & PPG at 100 Hz ==========
  if (currentMillis - lastSampleTime >= sampleInterval) {
    lastSampleTime = currentMillis;

    // Read raw ECG from analog pin
    float ecgValue = analogRead(1);

    // Read raw PPG IR from MAX30102
    float irValue = particleSensor.getIR();

    ecgBuffer[sampleIndex] = ecgValue;
    ppgBuffer[sampleIndex] = irValue;
    sampleIndex++;

    // ========== 3) If we filled one chunk of samples, send it ==========
    if (sampleIndex >= N_SAMPLES) {
      // Create chunk
      // Format (semicolon-separated sections):
      // TIMESTAMP;
      // ECG,ecg0,ecg1,...;
      // PPG,ppg0,ppg1,...;
      // SCD,co2,temp,hum
      unsigned long timestamp = millis(); // or use RTC for real-time seconds

      String chunk = String(timestamp);
      chunk += ";ECG";
      for (int i = 0; i < N_SAMPLES; i++) {
        chunk += ",";
        chunk += String(ecgBuffer[i], 0);
      }
      chunk += ";PPG";
      for (int i = 0; i < N_SAMPLES; i++) {
        chunk += ",";
        chunk += String(ppgBuffer[i], 0);
      }

      if (scd41.readMeasurement()) {
      latestCO2 = scd41.getCO2();          // in ppm
      latestTemp = scd41.getTemperature(); // in °C
      latestHumidity = scd41.getHumidity(); // in %
      /*
      Serial.print("SCD41 --> CO2: ");
      Serial.print(latestCO2);
      Serial.print(" ppm, Temp: ");
      Serial.print(latestTemp);
      Serial.print(" C, Hum: ");
      Serial.print(latestHumidity);
      Serial.println(" %");
      */
    } else {
      Serial.println("Failed to read SCD41.");
    }

      // Append SCD41 data
      chunk += ";SCD," + String(latestCO2, 2) + "," + String(latestTemp, 2) + "," + String(latestHumidity, 2);

      chunk += "*";

      Serial.println("======== SENDING DATA CHUNK ========");
      Serial.println(chunk);

      if (deviceConnected) {
        dataCharacteristic.setValue(chunk.c_str());
        dataCharacteristic.notify();
      }

      // Now send that big chunk in smaller pieces
      if (deviceConnected) {
        sendInChunks(chunk, 150);  // example: chunk size ~150 bytes per notification
      }

      // Reset for next chunk
      sampleIndex = 0;
    }
  }

}
