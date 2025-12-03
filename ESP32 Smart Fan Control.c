#define BLYNK_TEMPLATE_ID   "TEMPLATE_ID"  // <-- REPLACE WITH YOUR TEMPLATE ID 
#define BLYNK_TEMPLATE_NAME   "TEMPLATE_NAME" // <-- REPLACE WITH YOUR TEMPLATE NAME
#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>

// --- AUTH & WIFI ---
char auth[] = "YOUR_BLYNK_API"; // <-- REPLACE WITH YOUR AUTH TOKEN
char ssid[] = "WiFi_NAME"; // <-- REPLACE WITH YOUR WIFI SSID
char pass[] = "WiFi_PASSWORD"; // <-- REPLACE WITH YOUR WIFI PASSWORD

// --- PIN DEFINITIONS ---
#define DHTPIN 4        // Digital pin connected to the DHT sensor
#define RELAY_PIN 5     // Digital pin connected to the Relay's IN pin (Active-LOW assumed)

// --- SENSOR SETUP ---
#define DHTTYPE DHT22   // DHT 22 (AM2302)
DHT dht(DHTPIN, DHTTYPE);

// --- VIRTUAL PINS ---
#define VIRTUAL_PIN_TEMP V0             // Display Temperature (Float)
#define VIRTUAL_PIN_FAN_STATUS V1       // Display Fan Status (String)
#define VIRTUAL_PIN_THRESHOLD V2        // Temperature Threshold Slider (Float)
#define VIRTUAL_PIN_FAN_MANUAL_SWITCH V3// Manual Fan Control Switch (Integer 0/1)
#define VIRTUAL_PIN_CONTROL_MODE V4     // Display Auto/Manual Mode (String)
#define VIRTUAL_PIN_HUMIDITY V5         // NEW: Display Humidity (Float)

// --- VARIABLES & TIMER ---
float temperature_threshold = 30; 
BlynkTimer timer;
bool manual_fan_override = false;
bool manual_fan_state = false;    

// --- BLYNK WRITE FUNCTIONS ---

// Function to handle data received from Blynk (Manual Switch V3)
BLYNK_WRITE(VIRTUAL_PIN_FAN_MANUAL_SWITCH) {
  int value = param.asInt();

  manual_fan_override = true; 

  if (value == 1) {
    manual_fan_state = true;
    digitalWrite(RELAY_PIN, LOW);
    Blynk.virtualWrite(VIRTUAL_PIN_FAN_STATUS, "ON (Manual)");
    Blynk.virtualWrite(VIRTUAL_PIN_CONTROL_MODE, "Manual");
    Serial.println("Manual Fan ON");
  } else {
    manual_fan_state = false;
    digitalWrite(RELAY_PIN, HIGH);
    Blynk.virtualWrite(VIRTUAL_PIN_FAN_STATUS, "OFF (Manual)");
    Blynk.virtualWrite(VIRTUAL_PIN_CONTROL_MODE, "Manual");
    Serial.println("Manual Fan OFF");
  }
}

// Function to handle data received from Blynk (Temperature Threshold V2)
BLYNK_WRITE(VIRTUAL_PIN_THRESHOLD) {
  temperature_threshold = param.asFloat();
  Serial.print("New Temperature Threshold: ");
  Serial.println(temperature_threshold);

  manual_fan_override = false;
  Blynk.virtualWrite(VIRTUAL_PIN_CONTROL_MODE, "Auto");
}

// --- MAIN CONTROL FUNCTION ---

void readAndControlFan() {
  float h = dht.readHumidity(); // NEW: Read humidity
  float t = dht.readTemperature(); 

  // Check if any reads failed (for either T or H)
  if (isnan(t) || isnan(h)) {
    Serial.println("Failed to read from DHT sensor!");
    Blynk.virtualWrite(VIRTUAL_PIN_FAN_STATUS, "Sensor Error");
    return;
  }

  // Send sensor data to Blynk
  Blynk.virtualWrite(VIRTUAL_PIN_TEMP, t);
  Blynk.virtualWrite(VIRTUAL_PIN_HUMIDITY, h); // NEW: Send humidity to V5

  // Print to Serial Monitor for debugging
  Serial.print("Temperature: ");
  Serial.print(t);
  Serial.print(" *C\tHumidity: ");
  Serial.print(h);
  Serial.println(" %");

  // Control Logic: Check if in Manual or Auto Mode
  if (manual_fan_override) {
    // MANUAL MODE: Update status display
    Blynk.virtualWrite(VIRTUAL_PIN_CONTROL_MODE, "Manual");
    
    // Ensure the status text matches the manual state
    if (manual_fan_state) {
        Blynk.virtualWrite(VIRTUAL_PIN_FAN_STATUS, "ON (Manual)");
    } else {
        Blynk.virtualWrite(VIRTUAL_PIN_FAN_STATUS, "OFF (Manual)");
    }
  } 
  else {
    // AUTOMATIC MODE: Control based on temperature threshold
    Blynk.virtualWrite(VIRTUAL_PIN_CONTROL_MODE, "Auto");

    if (t >= temperature_threshold) {
      digitalWrite(RELAY_PIN, LOW); // Fan ON
      Blynk.virtualWrite(VIRTUAL_PIN_FAN_STATUS, "ON (Auto)");
    } else {
      digitalWrite(RELAY_PIN, HIGH); // Fan OFF
      Blynk.virtualWrite(VIRTUAL_PIN_FAN_STATUS, "OFF (Auto)");
    }
  }
}

// --- ARDUINO SETUP & LOOP ---

void setup() {
  Serial.begin(115200);

  // Initialize Relay Pin
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); // Start with fan OFF (HIGH for Active-LOW relay)

  // Initialize DHT sensor
  dht.begin();

  // Connect to Blynk
  Blynk.begin(auth, ssid, pass);

  // Setup function to run every 5000 milliseconds (5 seconds)
  timer.setInterval(5000L, readAndControlFan);
}

void loop() {
  Blynk.run();
  timer.run();
}
