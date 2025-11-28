# 🛠️ TriBot – A Versatile Three-Mode Robot

![ESP8266](https://img.shields.io/badge/ESP8266-Compatible-blue)
![Flutter](https://img.shields.io/badge/Flutter-App-blue)
![Arduino](https://img.shields.io/badge/Arduino-Firmware-green)
![License](https://img.shields.io/badge/License-MIT-green)
![Last Commit](https://img.shields.io/github/last-commit/onlyvaibhav/Tribot)
![Repo Size](https://img.shields.io/github/repo-size/onlyvaibhav/Tribot)

**TriBot** is a multi-mode robotic system built using the **ESP8266** microcontroller and a custom **Flutter mobile application**. The project demonstrates IoT-based control, autonomous navigation, and a clean firmware + app architecture.

---

## 📌 Features

### 🔹 Robot Modes
1.  **Wi-Fi Controlled Car**
    * Control movement and direction using the mobile app over HTTP/Wi-Fi.
2.  **Obstacle Avoiding Mode**
    * Uses an ultrasonic sensor (HC-SR04) to automatically avoid obstacles.
3.  **Line Following Mode**
    * IR sensors detect and follow line paths autonomously.

### 🔹 Flutter Mobile App
* Mode switching (Car / Obstacle Avoid / Line Follow)
* Real-time directional control
* ESP8266 IP configuration
* Clean, modern, and responsive UI

### 🔹 Modular Firmware
* **`tribot.ino`** – Original version.
* **`tribot_2.0.ino`** – AI-refactored version.
* **`tribot_3.0.ino`** – Further improved version with manual fixes.

---

## 📂 Project Structure

```text
Tribot/
├── app/
│   └── tribot_app_v2/       # Flutter control application
│
├── firmware/
│   ├── tribot.ino           # Original ESP8266 firmware
│   ├── tribot_2.0.ino       # AI-assisted refactored version
│   └── tribot_3.0.ino       # Further improvements
│
├── .gitignore
└── README.md

---

## 🔧 Hardware Requirements

- **ESP8266 (NodeMCU)**
- **L298N Motor Driver**
- **DC Motors + Wheels + Chassis**
- **Ultrasonic Sensor (HC-SR04)**
- **IR Line Sensors**
- **Li-ion Battery Pack or Power Bank**
- **Jumper wires and connectors**

---

## 📱 Running the Flutter App

### Prerequisites
- Flutter SDK installed  
- Android device / emulator  
- ESP8266 and phone connected to **same Wi-Fi network**

### Commands
```bash
cd app/tribot_app_v2
flutter pub get
flutter run

---

- Note: Configure your ESP8266 IP address inside the app’s configuration file (if applicable).

##⚡Uploading the ESP8266 Firmware

- Install Arduino IDE or PlatformIO

- Install ESP8266 board package

- Open any firmware version from:

- Tribot/firmware/


Select board:
Tools → Board → ESP8266 → NodeMCU 1.0

Connect ESP8266 via USB

Click Upload


## 📝 License

This project is open-source and available under the MIT License.
You may modify, distribute, and use it for academic or personal purposes.

## 👤 Author

Vaibhav (onlyvaibhav)
GitHub: https://github.com/onlyvaibhav