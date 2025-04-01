# IOT Weather Station – Weather Underground

This project is an IOT-based weather station designed to measure and transmit environmental data to an online platform like Weather Underground. It utilizes sensors and an ESP8266-based microcontroller to monitor temperature, humidity, pressure, and UV index, displaying the data on a 2.4-inch SPI TFT screen.

---

### ⚙️Features:

- Environmental Monitoring: Measures temperature, humidity, pressure, and UV index.

- Wireless Data Transmission: Sends data to an IOT platform (ThingSpeak).

- Real-time Display: Shows sensor readings on a 240x320 SPI TFT display, alongside real data (OpenWeather).

- Wi-Fi Connectivity: Uses ESP8266 NodeMCU for wireless communication.

- Compact & Portable: Powered independently via battery.

---

### 🧩Components:

- Microcontroller: ESP8266 NodeMCU V3 (Wi-Fi)

- Environmental Sensor: Bosch BME280 (Temperature, Humidity, Pressure)

- UV Sensor: RoboFun DC 3.3V-5V

- Display: Adafruit_ILI9341 2.4-inch (240x320 SPI)

---

### ⚒️Working on:

- Integrating a PM sensor to measure particulate matter levels for air quality monitoring.

- Developing data analysis features to provide explanations for environmental variations (e.g., why humidity is higher on certain days).







