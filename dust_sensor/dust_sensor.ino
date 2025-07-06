#include <TheThingsNetwork.h>
#include <TinyGPSPlus.h>
#include <SoftwareSerial.h>
#include <math.h>

#include "secrets.h"

#define loraSerial Serial1
#define debugSerial Serial

#define DUST_SENSOR_DIGITAL_PIN_PM10  6
#define DUST_SENSOR_DIGITAL_PIN_PM25  3

#define QUEUE_SIZE 10  // Size of the circular queue
#define DATA_SIZE 20   // Size of each data packet

#define freqPlan TTN_FP_EU868

const int port = 3;
unsigned long lastMeasurement = 0;
const unsigned long MEASURE_INTERVAL = 60000; // Measure every minute

// Circular queue implementation
struct Queue {
    byte data[QUEUE_SIZE][DATA_SIZE];
    int front;
    int rear;
    int count;
};

Queue measurementQueue = {{}, 0, 0, 0};

// Function declarations
void initQueue();
bool isQueueFull();
bool isQueueEmpty();
bool enqueue(byte* data);
bool dequeue(byte* data);
    
TheThingsNetwork ttn(loraSerial, debugSerial, freqPlan);

SoftwareSerial gpsSerial(10, 11);
TinyGPSPlus gps;

typedef union
{
 float number;
 uint8_t bytes[4];
} FLOATUNION_t;

// GPS Values
FLOATUNION_t lat;
FLOATUNION_t lng;
signed int altitude;

//dust sensor variables
float lastDUSTPM25 = 0.0;
float lastDUSTPM10 = 0.0;
unsigned long duration;
unsigned long starttime;
unsigned long endtime;
unsigned long sampletime_ms = 15000;
unsigned long lowpulseoccupancy = 0;
float ratio = 0;
long concentrationPM25 = 0;
long concentrationPM10 = 0;

int cnt = 0;

void setup() {
    pinMode(DUST_SENSOR_DIGITAL_PIN_PM10, INPUT);
    pinMode(DUST_SENSOR_DIGITAL_PIN_PM25, INPUT);
    
    loraSerial.begin(57600);
    debugSerial.begin(9600);
    gpsSerial.begin(9600);

    // Wait a maximum of 10s for Serial Monitor
    while (!debugSerial && millis() < 10000);

    debugSerial.println("-- STATUS");
    ttn.showStatus();

    debugSerial.println("-- JOIN");
    ttn.join(appEui, appKey);

    ttn.onMessage(message);
    initQueue();
}

void loop() {
    measureTask();
    sendTask();
}

void measureTask() {
    if (millis() - lastMeasurement < MEASURE_INTERVAL) {
        return;
    }

    byte data[DATA_SIZE] = {0};
    
    debugSerial.println("-- Measure PM");

    concentrationPM25 = getPM(DUST_SENSOR_DIGITAL_PIN_PM25);
    debugSerial.print("PM25: ");
    debugSerial.println(concentrationPM25);

    if ((ceil(concentrationPM25) != lastDUSTPM25) && ((long)concentrationPM25 > 0)) {
        data[0] = (byte)(concentrationPM25 >> 8);
        data[1] = (byte)concentrationPM25;
        lastDUSTPM25 = ceil(concentrationPM25);
    }

    concentrationPM10 = getPM(DUST_SENSOR_DIGITAL_PIN_PM10);
    debugSerial.print("PM10: ");
    debugSerial.println(concentrationPM10);
    
    if ((ceil(concentrationPM10) != lastDUSTPM10) && ((long)concentrationPM10 > 0)) {
        data[2] = (byte)(concentrationPM10 >> 8);
        data[3] = (byte)concentrationPM10;
        lastDUSTPM10 = ceil(concentrationPM10);
    }

    while (gpsSerial.available()) {
        gps.encode(gpsSerial.read());
    }

    lat.number = gps.location.lat();
    lng.number = gps.location.lng();
    altitude = gps.altitude.meters();

    data[4] = lat.bytes[3];
    data[5] = lat.bytes[2];
    data[6] = lat.bytes[1];
    data[7] = lat.bytes[0];
    
    data[8] = lng.bytes[3];
    data[9] = lng.bytes[2];
    data[10] = lng.bytes[1];
    data[11] = lng.bytes[0];
    
    data[12] = altitude;

    data[13] = gps.date.year() >> 8;
    data[14] = gps.date.year();
    data[15] = gps.date.month();
    data[16] = gps.date.day();
    data[17] = gps.time.hour();
    data[18] = gps.time.minute();
    data[19] = gps.time.second();

    if (enqueue(data)) {
        debugSerial.println("Measurement added to queue");
    } else {
        debugSerial.println("Queue full - measurement dropped");
    }

    lastMeasurement = millis();
}

void sendTask() {
    if (isQueueEmpty()) {
        return;  // Nothing to send
    }

    byte dataToSend[DATA_SIZE];
    
    memcpy(dataToSend, measurementQueue.data[measurementQueue.front], DATA_SIZE);
    
    debugSerial.println("-- Sending...");
    if (ttn.sendBytes(dataToSend, DATA_SIZE, port) == TTN_SUCCESSFUL_TRANSMISSION) {
        byte dummy[DATA_SIZE];
        dequeue(dummy);
        cnt = 0;
    } else {
        debugSerial.println("Transmission failed - keeping message in queue");
        cnt++;
    }
}

void initQueue() {
    measurementQueue.front = 0;
    measurementQueue.rear = 0;
    measurementQueue.count = 0;
}

bool isQueueFull() {
    return measurementQueue.count >= QUEUE_SIZE;
}

bool isQueueEmpty() {
    return measurementQueue.count == 0;
}

bool enqueue(byte* data) {
    if (isQueueFull()) {
        return false;
    }
    
    memcpy(measurementQueue.data[measurementQueue.rear], data, DATA_SIZE);
    measurementQueue.rear = (measurementQueue.rear + 1) % QUEUE_SIZE;
    measurementQueue.count++;
    return true;
}

bool dequeue(byte* data) {
    if (isQueueEmpty()) {
        return false;
    }
    
    memcpy(data, measurementQueue.data[measurementQueue.front], DATA_SIZE);
    measurementQueue.front = (measurementQueue.front + 1) % QUEUE_SIZE;
    measurementQueue.count--;
    return true;
}

void message(const byte* payload, int length, int port) {
    debugSerial.println("-- MESSAGE");
    return;
}

long getPM(int DUST_SENSOR_DIGITAL_PIN) {

  starttime = millis();
  while (1) {
  
    duration = pulseIn(DUST_SENSOR_DIGITAL_PIN, LOW);
    lowpulseoccupancy += duration;
    endtime = millis();
    
    if ((endtime-starttime) > sampletime_ms)
    {
    ratio = (lowpulseoccupancy-endtime+starttime)/(sampletime_ms*10.0);  // Integer percentage 0=>100
    long concentration = 1.1*pow(ratio,3)-3.8*pow(ratio,2)+520*ratio+0.62; // using spec sheet curve

    
    lowpulseoccupancy = 0;
    return(concentration);    
    }
  }  
}
