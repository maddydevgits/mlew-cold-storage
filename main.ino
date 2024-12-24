#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>

// Wi-Fi credentials
const char* ssid = "warriors";
const char* password = "9603036054";

// API Endpoints
String apiRoute = "https://web-api-zw0x.onrender.com/store?label=";
String alertRoute = "https://web-api-zw0x.onrender.com/alert?type=";

const String alertRoute1 = "https://web-api-zw0x.onrender.com/alert?type=warning&message=RFID tag not rescanned in 30 seconds";

// Sensor pinse
const int dhtPin = 23;    // DHT11 data pin
const int firePin = 12;   // GPIO12
const int gasPin = 13;    // GPIO13
const int buzzerPin = 14; // GPIO14

// Initialize 
DHT dht(dhtPin, DHT11);

bool fireAlertSent = false;
bool gasAlertSent = false;

String tagIDs[] = {
  "01006A7A3322", // Replace with your actual RFID card data
  "1700732A5618",
  "4A00A6852841",
  "180059F29625",
  "190082A3C3FB",
  "4C002126F5BE"
};

unsigned long lastScanTimes[6] = {0};  // Tracks last scan time for each tag
const unsigned long alertInterval = 30000; // 30 seconds


void setup() {
  Serial.begin(9600);

  // Initialize sensors
  dht.begin();
  pinMode(firePin, INPUT);
  pinMode(gasPin, INPUT);
  pinMode(buzzerPin, OUTPUT);

  // Connect to Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConnected to WiFi");
}

void sendAlert(String tagID) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String apiUrl = alertRoute + "&tagID=" + tagID; // Include tag ID in the alert message
    http.begin(apiUrl);
    int responseCode = http.GET();

    if (responseCode > 0) {
      String response = http.getString();
      Serial.println("Alert sent for " + tagID + ": " + response);
    } else {
      Serial.print("Error sending alert for ");
      Serial.println(tagID);
    }
    http.end();
  } else {
    Serial.println("WiFi not connected, alert not sent");
  }
}

// Function to send data to API
void sendDataToDashboard(String api) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(api);
    int responseCode = http.GET();

    if (responseCode > 0) {
      String response = http.getString();
      Serial.println(response);
    } else {
      Serial.print("Error sending data: ");
      Serial.println(responseCode);
    }
    http.end();
  } else {
    Serial.println("WiFi not connected");
  }
}

void loop() {
  // Read DHT11 sensor values
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("DHT11 read error");
    return;
  }

  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println("°C");

  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.println("%");

  // Read fire sensor
  int fireValue = digitalRead(firePin);
  Serial.print("Fire: ");
  Serial.println(fireValue);

  // Read gas sensor
  int gasValue = analogRead(gasPin);
  Serial.print("Gas: ");
  Serial.println(gasValue);


  // Send data to API
  sendDataToDashboard(apiRoute + "Temperature&value=" + String(temperature));
  delay(100);
  sendDataToDashboard(apiRoute + "Humidity&value=" + String(humidity));
  delay(100);
  sendDataToDashboard(apiRoute + "Fire&value=" + String(fireValue));
  delay(100);
  sendDataToDashboard(apiRoute + "Gas&value=" + String(gasValue));
  delay(100);

  if (digitalRead(firePin) == LOW) {
    digitalWrite(buzzerPin,HIGH);
    delay(1000);
    digitalWrite(buzzerPin,LOW);
  }

  // Send alerts if thresholds are exceeded
  if (digitalRead(firePin) == LOW && !fireAlertSent) {
    sendDataToDashboard(alertRoute + "danger&message=Fire Detected");
    fireAlertSent = true;
    digitalWrite(buzzerPin, HIGH);
    delay(1000);
    digitalWrite(buzzerPin, LOW);
  }
  if (gasValue > 400 && !gasAlertSent) {
    sendDataToDashboard(alertRoute + "danger&message=Gas Leak Detected");
    gasAlertSent = true;
    digitalWrite(buzzerPin, HIGH);
    delay(1000);
    digitalWrite(buzzerPin, LOW);
  }

  // Reset alerts if conditions are normal
  if (fireValue == LOW) {
    fireAlertSent = false;
    
  }
  if (gasValue <= 400) {
    gasAlertSent = false;
  }

  if (Serial.available()) {
      String cardData = "";
  
      // Read all available data
      while (Serial.available()) {
        char c = Serial.read();
        cardData += c;
        delay(10); // Ensure complete data is read
      }
  
      // Print the RFID card data to the Serial Monitor
      Serial.println("RFID Card Data: " + cardData);
    }

    // Match the scanned tag with known tags
    for (int i = 0; i < 6; i++) {
      if (cardData == tagIDs[i]) {
        Serial.println("Tag " + tagIDs[i] + " detected. Resetting timer.");
        lastScanTimes[i] = millis(); // Update the last scan time for this tag
        return;
      }
    }

  // Check for expired timers
  unsigned long currentTime = millis();
  for (int i = 0; i < 6; i++) {
    if (lastScanTimes[i] != 0 && currentTime - lastScanTimes[i] >= alertInterval) {
      Serial.println("No scan for tag " + tagIDs[i] + " within 30 seconds, sending alert...");
      sendAlert(tagIDs[i]);
      lastScanTimes[i] = 0; // Reset timer after sending alert
    }
  }

  delay(100); // Small delay to reduce CPU usage

  delay(1000);
}
