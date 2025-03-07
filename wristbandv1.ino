#include <Wire.h>
#include "SparkFun_SCD4x_Arduino_Library.h"  // SCD41 Library
#include "MAX30105.h"                        // MAX30102 Library
#include "heartRate.h"                        // Heart Rate Algorithm
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

#define SERVICE_UUID "12345678-1234-1234-1234-123456789abc"
#define EEG_CHARACTERISTIC "abcd5678-ab12-cd34-ef56-abcdef123456"

BLECharacteristic eegCharacteristic(
  EEG_CHARACTERISTIC,
  BLECharacteristic::PROPERTY_NOTIFY);

bool deviceConnected = false;
BLEAdvertising *pAdvertising;

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

// Create sensor objects
SCD4x scd41;           // CO2, Temperature, Humidity Sensor
MAX30105 particleSensor;  // MAX30102 Heart Rate Sensor

// Heart rate calculation variables
const byte RATE_SIZE = 4; // Number of readings for averaging BPM
byte rates[RATE_SIZE];    // Array to store heart rate values
byte rateSpot = 0;
long lastBeat = 0;        // Time of last beat detection
float beatsPerMinute;
int beatAvg;

unsigned long lastSCD41Read = 0;

const unsigned long nestedLoopDuration = 1000;  // Nested loop runs for 1 second

float co2 = 0, temp = 0, humidity = 0;


void setup() {
    Serial.begin(115200);
    Wire.begin();  // Initialize I2C communication

    // Initialize SCD41
    if (!scd41.begin()) {
        Serial.println("Failed to initialize SCD41! Check wiring.");
    } else {
        Serial.println("SCD41 initialized.");
        scd41.startPeriodicMeasurement();
    }

    // Initialize MAX30102
    if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {  // 400kHz I2C speed
        Serial.println("MAX30102 was not found! Check wiring.");
        while (1);  // Halt if sensor not found
    }
    

    // MAX30102 sensor setup
    particleSensor.setup();  // Configures sensor with default settings
    particleSensor.setPulseAmplitudeRed(0x3F);  //Turn Red LED to low to indicate sensor is running
    particleSensor.setPulseAmplitudeGreen(0);   //Turn off Green LED

    Serial.println("Initialized sensors.");

  Serial.println("🚀 ESP32 BLE Setup Starting...");

  BLEDevice::deinit(); // Ensures a clean restart of BLE
  delay(100);

  // ✅ Initialize BLE
  BLEDevice::init("ESP32_Health_Monitor");
  BLEServer* pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());

  BLEService* pService = pServer->createService(SERVICE_UUID);
  if (pService == nullptr) {
      Serial.println("❌ Failed to create BLE Service!");
  } else {
      Serial.println("✅ BLE Service Created Successfully");
  }

  pService->addCharacteristic(&eegCharacteristic);

  // ✅ Add BLE2902 Descriptor for EEG
  BLE2902* eegDescriptor = new BLE2902();
  eegCharacteristic.addDescriptor(eegDescriptor);
  eegDescriptor->setNotifications(true);

  pService->start();
  Serial.println("✅ BLE Service Started");

  pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(BLEUUID(SERVICE_UUID));
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // Helps with iOS compatibility
  pAdvertising->start();
  Serial.println("📡 BLE Advertising Started with Service UUID");

}

void loop() {
    
    unsigned long nestedLoopStart = millis();
        
    // Run nested loop for 1 second without delay
    while (millis() - nestedLoopStart < nestedLoopDuration) {
    
    // Read MAX30102 Heart Rate Data
    long irValue = particleSensor.getIR();

    if (checkForBeat(irValue)) {
        // Detected a beat
        Serial.println("Heartbeat detected!");  
        long delta = millis() - lastBeat;
        Serial.print("Delta: "); Serial.println(delta);  // Debugging

        lastBeat = millis();

        beatsPerMinute = 60 / (delta / 1000.0);

        //averaging
        if (beatsPerMinute < 255 && beatsPerMinute > 20) {
            rates[rateSpot++] = (byte)beatsPerMinute;  // Store reading
            rateSpot %= RATE_SIZE;  // Wrap variable

            // Average the readings
            beatAvg = 0;
            for (byte x = 0; x < RATE_SIZE; x++)
                beatAvg += rates[x];
            beatAvg /= RATE_SIZE;
        }
    }else{
      Serial.println("no heartbeat detected");
    }

    // Print Heart Rate Data
    Serial.print("IR: "); Serial.print(irValue);
    Serial.print(", BPM: "); Serial.print(beatsPerMinute);
    Serial.print(", Avg BPM: "); Serial.print(beatAvg);

    if (irValue < 50000) Serial.print(" (No Finger Detected)");

    Serial.println();

    }
    

    
    // Read SCD41 (CO2, Temperature, Humidity)
    if (millis() - lastSCD41Read >= 5000) { 
        lastSCD41Read = millis();
        if (scd41.readMeasurement()) {
            co2 = scd41.getCO2();
            temp = scd41.getTemperature();
            humidity = scd41.getHumidity();
        } else {
            Serial.println("Failed to read SCD41.");
        }
    }

    Serial.print("CO2: "); Serial.print(co2); Serial.print(" ppm, ");
    Serial.print("Temp: "); Serial.print(temp, 2); Serial.print(" C, ");
    Serial.print("Humidity: "); Serial.print(humidity, 2); Serial.println(" %");

    //ecg
    int ecgSignal = analogRead(1);
    String ecgString = String(ecgSignal);
    Serial.println("ECG: " + ecgString);

    // **Combine all data into a single string**
    String dataString = String(ecgSignal) + "," + 
                        String(beatAvg) + "," + 
                        String(co2) + "," +
                        String(temp) + "," + 
                        String(humidity);

    Serial.println("BLE Data: " + dataString);

    eegCharacteristic.setValue(dataString.c_str());
    eegCharacteristic.notify();

    delay(1000);  
}
