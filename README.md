# ESP32DeautherTool

This project implements a Wi-Fi deauthentication tool using the ESP32 microcontroller. This project works with the ESP32-S2 and the ESP32-S3 as well. It features both a single-target and an all-network deauth mode, along with a web-based interface for easy control.

## Features

- **Single Target Deauth**: Select a specific Access Point (AP) to target.
- **All Networks Deauth(Jammer-like function)**: Broadcast deauthentication packets to all nearby networks.
- **Web Interface**: Accessible on the ESP32’s AP, allowing easy selection of networks, reason codes, and attack initiation.
- **Wi-Fi Scanning**: Lists all available APs with details such as SSID, BSSID, channel, and RSSI.
- **Stop Attack**: Immediately stop ongoing deauthentication.

## Hardware Requirements

- **ESP32/ESP32-S2/ESP32-S3** microcontroller.
- USB cable for programming and power.
- Optional: LED for visual feedback(ESP32 has an on-board led).

## Software Requirements

- [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp-idf-get-started/) (Espressif IoT Development Framework).
- A terminal or IDE for programming (e.g., Visual Studio Code with the ESP-IDF plugin).

## Setup and Installation

1. **Clone the Repository**:
   ```bash
   git clone https://github.com/claudiunderthehood/ESP32DeautherTool
   cd ESP32DeautherTool
   ```

2. **Set Up ESP-IDF**:
   Follow the [ESP-IDF installation guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp-idf-get-started/) for your operating system.

3. **Configure the Project**:
   ```bash
   idf.py set-target <your-esp-chip>
   ```

4. **Build and Flash the Firmware**:
   ```bash
   idf.py build
   ```

5. **Monitor Serial Output**:
   ```bash
   idf.py flash -p <esp-port> monitor
   ```

6. **Connect to the ESP32 Wi-Fi Network**:
   - Default SSID: `ESP32Deauther`
   - Default Password: `password123`
   - If you want change the SSID and Password you can do that in `main/include/config.h`

7. **Access the Web Interface**:
   - Open a browser and navigate to `http://192.168.4.1/`.

## Usage

### Web Interface

1. **Scan for Wi-Fi Networks**:
   - Click the "Rescan Networks" button to refresh the list of nearby APs.

2. **Single Target Attack**:
   - Select an AP from the list using the radio buttons.
   - Enter a reason code (e.g., `1` for "Unspecified reason").
   - Click "Launch Attack".

3. **Jam all the Wi-Fi Networks**:
   - Enter a reason code.
   - Click "Attack All".

4. **Stop the Attack**:
   - Click the "Stop Attack" button to terminate the ongoing attack.

### Reason Codes

| Code | Meaning                |
|------|------------------------|
| 0    | Reserved               |
| 1    | Unspecified reason     |
| 3    | STA leaving BSS        |
| 4    | Disassociated due to inactivity |
| 5    | Too many STAs          |

## Credits

This project was not the first one, there were other that came before this. I did this version because i saw few Deauthers made with ESP-IDF so i wanted to give it a shot. Special thanks to [stblr](https://github.com/JulianStiebler) who has helped me figure out how to bypass the sanity check function that was blocking the deauth frame. These are the other Deauther projects that inspired this so go and check them out.

- [ESP32-Deauther](https://github.com/tesa-klebeband/ESP32-Deauther) by tesa-klebeband

- [esp32-deauther](https://github.com/GANESH-ICMC/esp32-deauther) by GANESH-ICMC

- [esp8266_deauther](https://github.com/spacehuhntech/esp8266_deauther) by SpacehuhnTech


## Limitations

- The tool works only on 2.4 GHz networks if you are jamming all networks. If you are using the single deauth attack then it probably work with both bands. The Jam attack works well with closer connections, so do not expect it to work on APs that are far from the ESP32.

- Some APs may have protection mechanisms against deauthentication attacks.

## Disclaimer

This tool is for educational and testing purposes only. Unauthorized use of this tool is illegal and unethical. The authors are not responsible for any misuse of this software.

