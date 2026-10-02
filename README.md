# IoT-Based Smart Vehicle Breakdown Detection and Assistance System

An ESP32-based smart vehicle monitoring and breakdown detection system integrating sensors, GPS, motor control, LCD alerts, buzzer alerts, Wi-Fi connectivity, and ThingSpeak cloud monitoring.

## Project Overview

This project is designed to monitor vehicle conditions and identify possible abnormal movement or breakdown conditions.

The ESP32 collects data from the MPU6050 accelerometer, obstacle sensor, fuel switch, and NEO-6M GPS. The system provides local status information through a 16x2 I2C LCD and buzzer.

The vehicle data can also be uploaded to ThingSpeak through Wi-Fi for cloud-based monitoring.

## Main Features

- Vehicle movement and abnormal vibration detection
- Breakdown condition detection using MPU6050
- GPS-based vehicle location monitoring
- Obstacle detection
- Fuel-low detection
- Automatic motor stopping during safety conditions
- LCD-based vehicle status indication
- Buzzer warning for abnormal conditions
- Wi-Fi connectivity
- ThingSpeak cloud monitoring
- Serial Monitor status and sensor information

## Components Used

- ESP32 Development Board
- MPU6050 Accelerometer/Gyroscope
- NEO-6M GPS Module
- Obstacle Sensor
- Fuel/Oil Level Switch
- 16x2 I2C LCD
- Buzzer
- L298N Motor Driver
- DC Motors
- Battery/Power Supply

## Technologies Used

- ESP32
- Arduino IDE
- C/C++
- MPU6050
- GPS
- I2C
- Wi-Fi
- ThingSpeak IoT Platform
- Sensor Interfacing
- Motor Control

## Pin Connections

| Component | ESP32 GPIO |
|---|---:|
| I2C SDA | GPIO 21 |
| I2C SCL | GPIO 22 |
| GPS RX | GPIO 16 |
| GPS TX | GPIO 17 |
| Motor IN1 | GPIO 32 |
| Motor IN2 | GPIO 33 |
| Motor IN3 | GPIO 18 |
| Motor IN4 | GPIO 19 |
| Buzzer | GPIO 25 |
| Obstacle Sensor | GPIO 26 |
| Fuel Switch | GPIO 27 |

## Working Principle

1. The ESP32 initializes the connected sensors, LCD, GPS, motor driver, and communication interfaces.
2. The MPU6050 continuously provides acceleration readings.
3. The system calculates the change in acceleration between consecutive readings.
4. Repeated abnormal movement readings are used to identify a possible breakdown condition.
5. The obstacle sensor detects obstacles around the vehicle.
6. The fuel switch indicates a low-fuel condition.
7. If an obstacle, low-fuel condition, or abnormal movement is detected, the motors are stopped and the buzzer is activated.
8. The LCD displays the corresponding vehicle status.
9. The NEO-6M GPS provides latitude, longitude, and satellite information when a valid GPS location is available.
10. Sensor and vehicle-status information is periodically uploaded to ThingSpeak through Wi-Fi.

## Breakdown Detection

The MPU6050 is used to monitor vehicle movement.

The system ignores the first 30 MPU6050 readings during startup. After that, abnormal acceleration changes are counted.

A breakdown condition is detected when the required number of abnormal readings is reached.

After a breakdown alert is detected, the system waits until the vibration remains normal continuously for the configured time before clearing the alert.

## Safety Response

The system stops the vehicle motors when any of the following conditions occurs:

- Obstacle detected
- Fuel level is low
- Abnormal vehicle movement / breakdown detected

The buzzer is activated to provide a local warning.

## LCD Status

The LCD provides different status messages, including:

- `OBSTACLE!`
- `VEHICLE STOP`
- `FUEL LOW!`
- `BREAKDOWN!`
- `ASSISTANCE`
- `VEHICLE MOVING`
- `GPS OK`
- `GPS SEARCH...`

## ThingSpeak Monitoring

The ESP32 sends selected vehicle information to ThingSpeak through Wi-Fi.

The uploaded data includes:

- Acceleration X
- Acceleration Y
- Obstacle status
- Fuel status
- Latitude
- Longitude
- Breakdown status

The system is configured to upload data periodically.

## Source Code

The ESP32 source code is available in:

`IoT_Smart_Vehicle_Breakdown_Assistance_System.ino`

## Result

The prototype demonstrates ESP32-based vehicle monitoring, safety-condition detection, breakdown detection, GPS location monitoring, motor control, local alerts, and cloud-based data monitoring.

## Future Scope

- Mobile application for vehicle monitoring
- Emergency notification to a predefined contact
- Automatic assistance-request system
- Improved breakdown classification
- Real-time location sharing
- Cloud-based historical data analysis
- Additional vehicle health sensors
