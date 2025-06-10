# Particulate Matter Bike Monitor

This project is a mobile air quality monitoring system designed to be mounted on a bicycle. It measures fine particulate matter (PM2.5 and PM10) and records GPS location data during your commute. The collected data is transmitted over LoRaWAN to The Things Network (TTN), enabling you to create heatmaps of air pollution along your cycling routes.

## Features

- **Real-time air quality monitoring:** Measures PM2.5 and PM10 concentrations using a dust sensor.
- **GPS tracking:** Captures latitude, longitude, altitude, and timestamp for each measurement.
- **LoRaWAN connectivity:** Sends data packets to TTN for remote collection and analysis.
- **Data buffering:** Uses a queue to ensure no measurements are lost if transmission is temporarily unavailable.
- **Open hardware:** Designed for Arduino Leonardo (ATmega32u4) with easy-to-source components.

## Hardware Requirements

- The Things Uno (Arduino Leonardo with integrated RN2483 LoRaWAN module)
- Dust sensor (e.g., PPD42NS or similar)
- GPS module (compatible with TinyGPS++)
- Battery pack
- Bicycle mounting hardware

## How It Works

1. **Measurement:**
   - The device measures PM2.5 and PM10 concentrations and reads GPS data at regular intervals (default: every minute).
   - Each measurement is packed into a 20-byte array containing sensor values and timestamp/location info.
   - Measurements are added to a queue for reliable transmission.

2. **Transmission:**
   - The device attempts to send the oldest measurement in the queue to TTN.
   - If transmission fails, the data remains in the queue and is retried later.

3. **Data Format:**
   - Each LoRaWAN uplink is a 20-byte packet:
     - PM2.5 (2 bytes, INT16)
     - PM10 (2 bytes, INT16)
     - Latitude (4 bytes, FLOAT32)
     - Longitude (4 bytes, FLOAT32)
     - Altitude (1 byte, INT8)
     - Year (2 bytes, INT16)
     - Month, Day, Hour, Minute, Second (1 byte each, INT8)
   - See `example.md` for a detailed breakdown and example.

4. **Decoding:**
   - Use the provided TTN payload formatter (`ttn_formatters/fmt.js`) to decode the binary data into readable values.

5. **Visualization:**
   - Export the decoded data and use mapping tools to create heatmaps of air quality along your routes.

## Getting Started

1. Clone this repository.
2. Copy `dust_sensor/secrets_example.h` to `dust_sensor/secrets.h` and fill in your TTN credentials.
3. Upload `dust_sensor/dust_sensor.ino` to your Arduino Leonardo.
4. Mount the device on your bike and power it up before your ride.
5. View your data in the TTN Console and process it for mapping.

## Example Data

See [`example.md`](example.md) for a sample data packet and its decoded output.

## License

MIT License

## Contributing

Contributions are welcome! Please open an issue or submit a pull request.
