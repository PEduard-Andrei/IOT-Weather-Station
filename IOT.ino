#include <ESP8266WiFi.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <ArduinoJson.h>

// Pins
#define TFT_CS   D8
#define TFT_DC   D4
#define TFT_RST  -1
#define UV_PIN   A0

// WiFi ID & Pass
const char* ssid = "";
const char* password = "";


// ThingSpeak API Key
const char* server = "api.thingspeak.com";
String apiKey = ""; 

// OpenWeather API Key
const char* openWeatherServer = "api.openweathermap.org";
const char* city = "";
const char* apiKeyOW = "";  

// Weather WU ID & Pass
const char* wuServer = "weatherstation.wunderground.com";
const char* stationID = "";     
const char* wuPassword = "";    

// Initialize components
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
Adafruit_BME280 bme;
WiFiClient client;

// Variables for sensor readings
float temperature, humidity, pressure;
float oldTemp = 0, oldHum = 0, oldPress = 0;
float uvIndex = 0, oldUV = -1;

// Variables for OpenWeather data
float tempOW = 0, humidityOW = 0, pressureOW = 0, uvIndexOW = 0;
unsigned long lastUpdate = 0;
const unsigned long updateInterval = 5000; // 15s

void setup() {
  Serial.begin(115200);
  
  // Initialize BME280
  Wire.begin(D2, D1); // SDA = D2, SCL = D1
  if (!bme.begin(0x76)) {
    Serial.println("BME280 not found!");
    while (1);
  }
  Serial.println("BME280 OK!");

  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(ILI9341_BLACK);

  tft.setTextSize(2);
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(10, 100);
  tft.print("Connecting to WiFi...");
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");

  // Clear screen and display success message
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(10, 100);
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_GREEN);
  tft.print("WiFi Connected!");

  // Read sensors and OpenWeather data
  readSensors();
  getWeatherData();
  getUVIndexFromOpenWeather();

  // Keep success message visible for a moment
  delay(1500);

  // Clear screen and display initial values, including OW UV
  tft.fillScreen(ILI9341_BLACK);
  displayInitialScreen();
  updateDisplayValues(); // Update screen with all values, including OW UV
}

void loop() {
  // Regular update
  if (millis() - lastUpdate >= updateInterval) {
    // Read local sensors
    readSensors();

    // Get OpenWeather data
    getWeatherData();
    getUVIndexFromOpenWeather();

    // Update display
    updateDisplayValues();

    // Send data to services
    sendToWeatherUnderground(temperature, humidity, pressure, uvIndex);
    sendToThingSpeak();

    lastUpdate = millis();
  }


  // Frequent updates for local display values
  delay(100);
}

void readSensors() {
  temperature = bme.readTemperature();
  humidity = bme.readHumidity();
  pressure = bme.readPressure() / 100.0F;
  
  int uvRaw = analogRead(UV_PIN);
  uvIndex = map(uvRaw, 0, 1024, 0, 11);
}

void getWeatherData() {
    WiFiClient client;
    if (client.connect(openWeatherServer, 80)) {
        String url = String("GET /data/2.5/weather?q=") + city + "&appid=" + apiKeyOW + "&units=metric";
        
        client.print(url + " HTTP/1.1\r\n" +
                    "Host: api.openweathermap.org\r\n" +
                    "Connection: close\r\n\r\n");

        bool jsonStarted = false;
        String payload;
        
        while (client.connected() || client.available()) {
            String line = client.readStringUntil('\n');
            if (line == "\r") {
                jsonStarted = true;
                continue;
            }
            if (jsonStarted) {
                payload = line; 
                break;
            }
        }

        Serial.println("JSON primit:");
        Serial.println(payload);

        DynamicJsonDocument doc(2048);
        DeserializationError error = deserializeJson(doc, payload);

        if (error) {
            Serial.print("Eroare la parsarea JSON: ");
            Serial.println(error.c_str());
            return;
        }

        tempOW = doc["main"]["temp"];
        pressureOW = doc["main"]["pressure"];
        humidityOW = doc["main"]["humidity"];
        
        
        Serial.println("Date extrase:");
        Serial.print("Temperatura: "); Serial.println(tempOW);
        Serial.print("Presiune: "); Serial.println(pressureOW);
        Serial.print("Umiditate: "); Serial.println(humidityOW);
    }
}

void getUVIndexFromOpenWeather() {
    WiFiClient client;
    const char* openWeatherUVServer = "api.openweathermap.org";
    String lat = "44.4268";
    String lon = "26.1025";

    if (client.connect(openWeatherUVServer, 80)) {
        String url = "/data/2.5/onecall?lat=" + lat + "&lon=" + lon + 
                    "&exclude=minutely,hourly,daily,alerts&appid=" + apiKeyOW +
                    "&units=metric";
                    
        client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                     "Host: " + openWeatherUVServer + "\r\n" +
                     "Connection: close\r\n\r\n");

        bool jsonStarted = false;
        String payload;
        
        while (client.connected() || client.available()) {
            String line = client.readStringUntil('\n');
            if (line == "\r") {
                jsonStarted = true;
                continue;
            }
            if (jsonStarted) {
                payload = line;
                break;
            }
        }

        DynamicJsonDocument doc(2048);
        DeserializationError error = deserializeJson(doc, payload);

        if (error) {
            Serial.print("Eroare la parsarea JSON UV: ");
            Serial.println(error.c_str());
            uvIndexOW = -1.0;
            return;
        }

        uvIndexOW = doc["current"]["uvi"];
        Serial.print("UV Index primit: ");
        Serial.println(uvIndexOW);
    }
}


void sendToWeatherUnderground(float temperature, float humidity, float pressure, float uvIndex) {
  if (WiFi.status() == WL_CONNECTED) {
      WiFiClient client;
      if (client.connect(wuServer, 80)) {
          String url = "/weatherstation/updateweatherstation.php?";
          url += "ID=" + String(stationID);
          url += "&PASSWORD=" + String(wuPassword);
          url += "&dateutc=now";
          url += "&tempf=" + String(temperature * 1.8 + 32);
          url += "&humidity=" + String(humidity);
          url += "&baromin=" + String(pressure * 0.02953);
          url += "&uv=" + String(uvIndex);
          url += "&softwaretype=ESP8266";
          url += "&action=updateraw";

          client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                       "Host: " + wuServer + "\r\n" +
                       "Connection: close\r\n\r\n");

          Serial.println("Trimit date către Weather Underground:");
          Serial.println("URL-ul trimis către Weather Underground: " + url);
          while (client.connected() || client.available()) {
              if (client.available()) {
                  String line = client.readStringUntil('\n');
                  Serial.println("Răspuns server: " + line);
              }
          }
          client.stop();
      } else {
          Serial.println("Eroare: Nu s-a conectat la Weather Underground!");
      }
  }
}


void sendToThingSpeak() {
  WiFiClient client;
  if (client.connect(server, 80)) {
    String postStr = apiKey;
    postStr += "&field1=" + String(temperature);
    postStr += "&field2=" + String(tempOW);
    postStr += "&field3=" + String(pressure);
    postStr += "&field4=" + String(pressureOW);
    postStr += "&field5=" + String(humidity);
    postStr += "&field6=" + String(humidityOW);
    postStr += "&field7=" + String(uvIndex);
    postStr += "&field8=" + String(uvIndexOW);

    client.print("POST /update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: " + apiKey + "\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: " + String(postStr.length()) + "\n\n");
    client.print(postStr);
    
    Serial.println("Data sent to ThingSpeak");
  }
  client.stop();
}

void displayInitialScreen() {
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextSize(2);

  // Local data labels
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(10, 10);
  tft.print("Local Temp:");
  
  tft.setCursor(10, 40);
  tft.print("Local Hum:");
  
  tft.setCursor(10, 70);
  tft.print("Local Press:");
  
  tft.setCursor(10, 100);
  tft.print("Local UV:");
  
  // OpenWeather labels
  tft.setTextColor(ILI9341_YELLOW);
  tft.setCursor(10, 130);
  tft.print("OW Temp:");
  
  tft.setCursor(10, 160);
  tft.print("OW Hum:");
  
  tft.setCursor(10, 190);
  tft.print("OW Press:");
  
  tft.setCursor(10, 220);
  tft.print("OW UV:");
}

void updateDisplayValues() {
  tft.setTextSize(2);
  
  // Update local temperature
  if (temperature != oldTemp) {
    tft.setTextColor(ILI9341_BLACK);
    tft.setCursor(170, 10);
    tft.print(oldTemp, 1);
    tft.print(" C");
    
    tft.setTextColor(ILI9341_GREEN);
    tft.setCursor(170, 10);
    tft.print(temperature, 1);
    tft.print(" C");
    oldTemp = temperature;
  }
  
  // Update local humidity
  if (humidity != oldHum) {
    tft.setTextColor(ILI9341_BLACK);
    tft.setCursor(170, 40);
    tft.print(oldHum, 1);
    tft.print(" %");
    
    tft.setTextColor(ILI9341_GREEN);
    tft.setCursor(170, 40);
    tft.print(humidity, 1);
    tft.print(" %");
    oldHum = humidity;
  }
  
  // Update local pressure
  if (pressure != oldPress) {
    tft.setTextColor(ILI9341_BLACK);
    tft.setCursor(170, 70);
    tft.print(oldPress, 1);
    tft.print(" hPa");
    
    tft.setTextColor(ILI9341_GREEN);
    tft.setCursor(170, 70);
    tft.print(pressure, 1);
    tft.print(" hPa");
    oldPress = pressure;
  }
  
  // Update local UV
  if (uvIndex != oldUV) {
    tft.setTextColor(ILI9341_BLACK);
    tft.setCursor(170, 100);
    tft.print(oldUV, 1);
    
    tft.setTextColor(ILI9341_GREEN);
    tft.setCursor(170, 100);
    tft.print(uvIndex, 1);
    oldUV = uvIndex;
  }
  
  // Update OpenWeather values
  static float oldTempOW = 0, oldHumOW = 0, oldPressOW = 0, oldUVOW = -1;
  
  // Temperature OW
  if (tempOW != oldTempOW) {
    tft.setTextColor(ILI9341_BLACK);
    tft.setCursor(170, 130);
    tft.print(oldTempOW, 1);
    tft.print(" C");
    
    tft.setTextColor(ILI9341_ORANGE);
    tft.setCursor(170, 130);
    tft.print(tempOW, 1);
    tft.print(" C");
    oldTempOW = tempOW;
  }
  
  // Humidity OW
  if (humidityOW != oldHumOW) {
    tft.setTextColor(ILI9341_BLACK);
    tft.setCursor(170, 160);
    tft.print(oldHumOW, 1);
    tft.print(" %");
    
    tft.setTextColor(ILI9341_ORANGE);
    tft.setCursor(170, 160);
    tft.print(humidityOW, 1);
    tft.print(" %");
    oldHumOW = humidityOW;
  }
  
  // Pressure OW
  if (pressureOW != oldPressOW) {
    tft.setTextColor(ILI9341_BLACK);
    tft.setCursor(170, 190);
    tft.print(oldPressOW, 1);
    tft.print(" hPa");
    
    tft.setTextColor(ILI9341_ORANGE);
    tft.setCursor(170, 190);
    tft.print(pressureOW, 1);
    tft.print(" hPa");
    oldPressOW = pressureOW;
  }
  
  // UV OW
  if (uvIndexOW != oldUVOW) {
  tft.setTextColor(ILI9341_BLACK);
  tft.setCursor(170, 220);
  tft.print(oldUVOW, 1);

  tft.setTextColor(ILI9341_ORANGE);
  tft.setCursor(170, 220);
  tft.print(uvIndexOW, 1);
  oldUVOW = uvIndexOW;
}
}