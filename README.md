# Dual-Role IoT Monitoring System with Role-Based MQTT Dashboards

An enterprise-grade IoT data pipeline simulation built on Wokwi and connected to Adafruit IO. The system splits edge-layer telemetry into two completely isolated dashboards using distinct MQTT topic strings to enforce role-based data security.

## System Features
* **Medical Staff Dashboard (Account: raj12):** Tracks patient vitals (Heart Rate, SpO₂, Body Temp).
* **Facility Management Dashboard (Account: raj13):** Tracks environmental attributes (Room Temp, AQI, O₂ Level).
* **Local Monitoring Edge:** Includes an I2C OLED display for patient bedside reporting and a flashing Red LED/Buzzer emergency alert matrix.

## Hardware Components Simulated
* ESP32-DevKitC-V4
* SSD1306 OLED Display ($128 \times 64$ via I2C)
* DHT22 Climate Sensor
* 4x Potentiometers (Simulating Heart Rate, SpO₂, AQI, O₂)
* Active Buzzer & Red Indicator LED
