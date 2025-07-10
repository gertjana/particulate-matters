#include "SparkFun_BMV080_Arduino_Library.h" 
#include <Wire.h>
#include <TinyGPSPlus.h>
#include <SoftwareSerial.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 32 
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)

#define BMV080_ADDR 0x57
#define wirePort Wire

#define SD_CS 44
#define SD_MOSI 9
#define SD_MISO 8
#define SD_CLK 7

#define GPS_RX 2
#define GPS_TX 3
#define GPS_BAUD 9600

#define INTERVAL 10000 
#define JSON_DOC_SIZE 512  // Explicit JSON document size

#define DEBUG

TinyGPSPlus gps;
SoftwareSerial gpsSerial(GPS_TX, GPS_RX);

SparkFunBMV080 bmv080; 

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Global flags for system status
bool sdCardAvailable = false;
bool sensorAvailable = false;
bool displayAvailable = false;

void setup()
{
    Serial.begin(115200);
    while (!Serial && millis() < 10000);

    // Initialize display
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
        Serial.println(F("SSD1306 allocation failed"));
    } else {
        displayAvailable = true;
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(WHITE);
        display.println("Particulate Matters");
        display.display();
    }

    // Initialize GPS
    gpsSerial.begin(GPS_BAUD);

    // Initialize SD card
    pinMode(SD_CS, OUTPUT);
    if(!SD.begin()){
        Serial.println("Card Mount Failed");
    } else {
        uint8_t cardType = SD.cardType();
        if(cardType == CARD_NONE){
            Serial.println("No SD card attached");
        } else {
            sdCardAvailable = true;
            #ifdef DEBUG
                Serial.print("SD Card Type: ");
                if(cardType == CARD_MMC){
                    Serial.println("MMC");
                } else if(cardType == CARD_SD){
                    Serial.println("SDSC");
                } else if(cardType == CARD_SDHC){
                    Serial.println("SDHC");
                } else {
                    Serial.println("UNKNOWN");
                }
                
                uint64_t cardSize = SD.cardSize()/(1024*1024);
                Serial.printf("SD Card Size: %lluMB\n", cardSize);
                Serial.printf("SD Free: %lluMB\n", cardSize-(SD.usedBytes()/(1024*1024)));
            #endif
        }
    }

    // Initialize I2C
    wirePort.begin();

    // Initialize BMV080 sensor
    if (bmv080.begin(BMV080_ADDR, wirePort) == false) {
        Serial.println("BMV080 not detected at default I2C address. Check your jumpers and the hookup guide.");
    } else {
        sensorAvailable = true;
        #ifdef DEBUG
            Serial.println("BMV080 found!");
        #endif

        bmv080.init();

        if (bmv080.setMode(SF_BMV080_MODE_CONTINUOUS) == true) {   
            #ifdef DEBUG
                Serial.println("BMV080 set to continuous mode");
            #endif  
        } else {
            Serial.println("Error setting BMV080 mode");
            sensorAvailable = false;
        }
    }

    // Display system status
    if (displayAvailable) {
        display.clearDisplay();
        display.setCursor(0,0);
        display.println("System Status:");
        display.printf("SD: %s\n", sdCardAvailable ? "OK" : "FAIL");
        display.printf("Sensor: %s\n", sensorAvailable ? "OK" : "FAIL");
        display.display();
        delay(2000);
    }
}

String zeroPad(int number, int width) {
    char buffer[10];
    if (width > 9) width = 9; // Prevent buffer overflow
    sprintf(buffer, "%0*d", width, number);
    return String(buffer);
}

bool appendFile(fs::FS &fs, JsonDocument &doc) {
    if (!sdCardAvailable) {
        return false;
    }
    
    if (!doc["gps"]["date"]) { 
        return false;
    }
    
    String path = String("/") + String(doc["gps"]["date"]) + String(".txt");
    
    #ifdef DEBUG
        Serial.printf("Writing to %s\n", path.c_str()); 
    #endif
    
    char output[256];
    if (serializeJson(doc, output, sizeof(output)) == 0) {
        Serial.println("JSON serialization failed");
        return false;
    }
    
    File file = fs.open(path.c_str(), FILE_APPEND);
    if(!file){
        Serial.printf("Failed to open file for appending, creating %s\n", path.c_str());
        File file2 = fs.open(path.c_str(), FILE_WRITE);
        if (!file2) {
            Serial.println("Failed to create new file");
            return false;
        }
        file2.println(output);
        file2.close();
    } else {
        file.println(output);
        file.close();
    }
    
    return true;
}

void getGPS(JsonDocument &doc) {
    JsonObject gpsObj = doc.createNestedObject("gps");
    bool hasValidData = false;
    
    while (gpsSerial.available() > 0) {
        #ifdef DEBUG
            Serial.print(char(gpsSerial.peek()));
        #endif

        if (gps.encode(gpsSerial.read())) {
            if (gps.date.isValid()) {
                gpsObj["date"] = zeroPad(gps.date.day(), 2) + "-" + zeroPad(gps.date.month(), 2) + "-" + String(gps.date.year());
                hasValidData = true;
            }
            if (gps.time.isValid()) {
                gpsObj["time"] = zeroPad(gps.time.hour(), 2) + ":" + zeroPad(gps.time.minute(), 2) + ":" + zeroPad(gps.time.second(), 2);
                hasValidData = true;
            }
            if (gps.location.isValid()) {
                gpsObj["latitude"] = String(gps.location.lat(), 8);
                gpsObj["longitude"] = String(gps.location.lng(), 8);
                hasValidData = true;
            }
        }
    }

    #ifdef DEBUG
        if (hasValidData) {
            Serial.println("GPS data updated");
        }
        Serial.println();
    #endif
}

void getPM(JsonDocument &doc) {
    JsonObject pmObj = doc.createNestedObject("pm");

    if (sensorAvailable && bmv080.readSensor()) {
        pmObj["pm1"] = bmv080.PM1();
        pmObj["pm2.5"] = bmv080.PM25();
        pmObj["pm10"] = bmv080.PM10();
        pmObj["obstructed"] = bmv080.isObstructed();
    }
}

void updateDisplay(JsonDocument &doc) {
    if (!displayAvailable) {
        return;
    }
    
    display.clearDisplay();
    display.setCursor(0,0);
    
    // Display PM data
    if (doc["pm"]["obstructed"].as<bool>()) {
        display.println("Sensor Obstructed!");
    } else {
        int pm1 = doc["pm"]["pm1"].as<int>();
        int pm25 = doc["pm"]["pm2.5"].as<int>();
        int pm10 = doc["pm"]["pm10"].as<int>();
        
        if (pm1 >= 0 && pm25 >= 0 && pm10 >= 0) {
            display.printf("1: %d 2.5: %d 10: %d\n", pm1, pm25, pm10);
        } else {
            display.println("Sensor Error");
        }
    }
    
    // Display GPS data
    if (doc["gps"]["longitude"] && doc["gps"]["latitude"]) {
        float lat = doc["gps"]["latitude"].as<float>();
        float lng = doc["gps"]["longitude"].as<float>();
        display.printf("%.4f, %.4f\n", lat, lng);
    } else {
        display.println("no gps fix");
    }
    
    // Display date and time
    if (doc["gps"]["date"]) {
        display.printf("%s ", doc["gps"]["date"].as<String>());
    }
    if (doc["gps"]["time"]) {
        display.printf("%s", doc["gps"]["time"].as<String>());
    }
    
    display.display();
} 

void loop()
{
    StaticJsonDocument<JSON_DOC_SIZE> doc;

    getGPS(doc);
    getPM(doc);

    #ifdef DEBUG
        serializeJson(doc, Serial);
        Serial.println();
    #endif

    updateDisplay(doc);
    
    if (!appendFile(SD, doc)) {
        #ifdef DEBUG
            Serial.println("Failed to save data to SD card");
        #endif
    }

    delay(INTERVAL);
} 