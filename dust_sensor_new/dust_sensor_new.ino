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

#define DEBUG

TinyGPSPlus gps;
SoftwareSerial gpsSerial(GPS_TX, GPS_RX);

SparkFunBMV080 bmv080; 

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup()
{
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
        Serial.println(F("SSD1306 allocation failed"));
    }
    display.clearDisplay();
    display.setTextSize(1); // Draw 2X-scale text
    display.setTextColor(WHITE);
    display.println("Particulate Matters");
    display.display();

    Serial.begin(115200);
    gpsSerial.begin(GPS_BAUD);

    while (!Serial && millis() < 10000);

    pinMode(SD_CS, OUTPUT);

    if(!SD.begin()){
        Serial.println("Card Mount Failed");
    }
    uint8_t cardType = SD.cardType();

    if(cardType == CARD_NONE){
        Serial.println("No SD card attached");
        return;
    }


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
    #endif

    uint64_t cardSize = SD.cardSize()/(1024*1024);

    #ifdef DEBUG
    Serial.printf("SD Card Size: %lluMB\n", cardSize);
    Serial.printf("SD Free: %lluMB\n", cardSize-(SD.usedBytes()/(1024*1024)));
    #endif

    wirePort.begin();

    if (bmv080.begin(BMV080_ADDR, wirePort) == false)
    {
        Serial.println(
            "BMV080 not detected at default I2C address. Check your jumpers and the hookup guide. Freezing...");
        while (1)
            ;
    }
    #ifdef DEBUG
        Serial.println("BMV080 found!");
    #endif


    bmv080.init();

    if (bmv080.setMode(SF_BMV080_MODE_CONTINUOUS) == true)
    {   
        #ifdef DEBUG
        Serial.println("BMV080 set to continuous mode");
        #endif  
    }
    else
    {
        Serial.println("Error setting BMV080 mode");
    }
}

String zeroPad(int number, int width) {
    char buffer[10];
    sprintf(buffer, "%0*d", width, number);
    return String(buffer);
}

void appendFile(fs::FS &fs, JsonDocument &doc){
    if (doc["gps"]["date"]) { 
        String path = String("/") + String(doc["gps"]["date"]) + String(".txt");
        
        #ifdef DEBUG
            Serial.printf("Writing to %s\n", path.c_str()); 
        #endif
        
        char output[256];
        serializeJson(doc, output);
        File file = fs.open(path.c_str(), FILE_APPEND);
        if(!file){
            Serial.printf("Failed to open file for appending, creating %s\n", path.c_str());
            File file2 = fs.open(path.c_str(), FILE_WRITE);
            file2.println(output);
            file2.close();
        } else {
            file.println(output);
        }

        file.close();
    }
}

void getGPS(JsonDocument &doc) {
    JsonObject gpsObj = doc.createNestedObject("gps");
    while (gpsSerial.available() > 0) {

        #ifdef DEBUG
        Serial.print(char(gpsSerial.peek()));
        #endif

        if (gps.encode(gpsSerial.read())) {
            if (gps.date.isValid()) {
                gpsObj["date"] = zeroPad(gps.date.day(), 2) + "-" + zeroPad(gps.date.month(), 2) + "-" + String(gps.date.year());
            }
            if (gps.time.isValid()) {
                gpsObj["time"] = zeroPad(gps.time.hour(), 2) + ":" + zeroPad(gps.time.minute(), 2) + ":" + zeroPad(gps.time.second(), 2);
            }
            if (gps.location.isValid()) {
                gpsObj["latitude"] = String(gps.location.lat(),8);
                gpsObj["longitude"] = String(gps.location.lng(),8);
            }
        }
    }

    #ifdef DEBUG
    Serial.println();
    #endif
}

void getPM(JsonDocument &doc) {
    JsonObject pmObj = doc.createNestedObject("pm");

    if (bmv080.readSensor()) {
        pmObj["pm1"] = bmv080.PM1();
        pmObj["pm2.5"] = bmv080.PM25();
        pmObj["pm10"] = bmv080.PM10();
        pmObj["obstructed"] = bmv080.isObstructed();
    } 
}

void updateDisplay(JsonDocument &doc) {
    display.clearDisplay();
    display.setCursor(0,0);
    if (!doc["pm"]["obstructed"].as<bool>()) {
        display.printf("1: %d 2.5: %d 10: %d\n", 
            doc["pm"]["pm1"].as<int>(), 
            doc["pm"]["pm2.5"].as<int>(), 
            doc["pm"]["pm10"].as<int>()
        ); 
    } else {
        display.println("Sensor Obstructed!");
    }
    if (doc["gps"]["longitude"] && doc["gps"]["latitude"]) {
        display.printf("%f, %f\n", 
            doc["gps"]["latitude"].as<float>()),
            doc["gps"]["longitude"].as<float>(); 
    } else {
        display.println("no gps fix");
    }
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
    JsonDocument doc;

    getGPS(doc);
    getPM(doc);

    #ifdef DEBUG
    serializeJson(doc, Serial);
    Serial.println();
    #endif

    updateDisplay(doc);
    appendFile(SD, doc);

    delay(INTERVAL);
}