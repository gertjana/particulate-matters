#include <TheThingsNetwork.h>
#include <TinyGPSPlus.h>
#include <SoftwareSerial.h>
#include "secrets.h"

#define loraSerial Serial1
#define debugSerial Serial

#define DUST_SENSOR_DIGITAL_PIN_PM10  6
#define DUST_SENSOR_DIGITAL_PIN_PM25  3

#define freqPlan TTN_FP_EU868

const int port = 3;
    
TheThingsNetwork ttn(loraSerial, debugSerial, freqPlan);

SoftwareSerial gpsSerial(10, 11);
TinyGPSPlus gps;

typedef union
{
 float number;
 uint8_t bytes[4];
} FLOATUNION_t;

FLOATUNION_t lat;
FLOATUNION_t lng;

//dust sensor variables
int val = 0;           // variable to store the value coming from the sensor
float valDUSTPM25 =0.0;
float lastDUSTPM25 =0.0;
float valDUSTPM10 =0.0;
float lastDUSTPM10 =0.0;
unsigned long duration;
unsigned long starttime;
unsigned long endtime;
unsigned long sampletime_ms = 15000;
unsigned long lowpulseoccupancy = 0;
float ratio = 0;
long concentrationPM25 = 0;
long concentrationPM10 = 0;
int temp=20; //external temperature, if you can replace this with a DHT11 or better 
long ppmv;
int cnt = 0; 

    
void setup() {
  pinMode(DUST_SENSOR_DIGITAL_PIN_PM10,INPUT);
  pinMode(DUST_SENSOR_DIGITAL_PIN_PM25,INPUT);
  
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
}

void loop() {
  debugSerial.println("-- Measure PM");
    
  // Prepare array of 20 bytes to send data
  byte data[20];
  
  //get PM 2.5 density of particles over 2.5 μm.
  concentrationPM25=getPM(DUST_SENSOR_DIGITAL_PIN_PM25);
  debugSerial.print("PM25: ");
  debugSerial.println(concentrationPM25);
  debugSerial.print("\n");
  //ppmv=mg/m3 * (0.08205*Tmp)/Molecular_mass
  //0.08205   = Universal gas constant in atm·m3/(kmol·K)
  ppmv=(concentrationPM25*0.0283168/100/1000) *  (0.08205*temp)/0.01;

  if ((ceil(concentrationPM25) != lastDUSTPM25)&&((long)concentrationPM25>0)) {
      data[0] = (byte) concentrationPM25; 
      data[1] = (byte) concentrationPM25 >> 8;
      lastDUSTPM25 = ceil(concentrationPM25);
  }

  //get PM 1.0 - density of particles over 1 μm.
  concentrationPM10=getPM(DUST_SENSOR_DIGITAL_PIN_PM10);
  debugSerial.print("PM10: ");
  debugSerial.println(concentrationPM10);
  debugSerial.print("\n");
  //ppmv=mg/m3 * (0.08205*Tmp)/Molecular_mass
  //0.08205   = Universal gas constant in atm·m3/(kmol·K)
  
  ppmv=(concentrationPM10*0.0283168/100/1000) *  (0.08205*temp)/0.01;
  
  if ((ceil(concentrationPM10) != lastDUSTPM10)&&((long)concentrationPM10>0)) {
      //make bytes to send off
      data[2] = (byte) concentrationPM10; 
      data[3] = (byte) concentrationPM10 >> 8;
      lastDUSTPM10 = ceil(concentrationPM10);
  }

  debugSerial.println("-- GPS");

  while(gpsSerial.available()) {
    char c = gpsSerial.read();
    debugSerial.write(c);
    gps.encode(c);
  }
  debugSerial.println("");
  debugSerial.print("LAT=");  debugSerial.println(gps.location.lat(), 6);
  debugSerial.print("LONG="); debugSerial.println(gps.location.lng(), 6);
  debugSerial.print("ALT=");  debugSerial.println(gps.altitude.meters());

  lat.number = gps.location.lat(); 
  lng.number = gps.location.lng(); 

  data[4] = lat.bytes[0];
  data[5] = lat.bytes[1];
  data[6] = lat.bytes[2];
  data[7] = lat.bytes[3];
  
  data[8] = lng.bytes[0];
  data[9] = lng.bytes[1];
  data[10] = lng.bytes[2];
  data[11] = lng.bytes[3];
  
  data[12] = gps.date.year() >> 8;
  data[13] = gps.date.year();
  data[14] = gps.date.month();
  data[15] = gps.date.day();
  data[16] = gps.time.hour();
  data[17] = gps.time.minute();
  data[18] = gps.time.second();
  data[19] = gps.time.centisecond();
  
  debugSerial.println("-- Sending...");
  // Send it off
  if (ttn.sendBytes(data, sizeof(data), port) == TTN_SUCCESSFUL_TRANSMISSION) {
    cnt = 0;    
  } else {
    cnt = cnt + 1;
  }
}

void message(const byte* payload, int length, int port) {
  debugSerial.println("-- MESSAGE");
    
  // Only handle messages of a single byte
  if (length != 1) {
    return;
  }
    
  if (payload[0] == 0) {
    debugSerial.println("LED: off");
    digitalWrite(LED_BUILTIN, LOW);
          
  } else if (payload[0] == 1) {
    debugSerial.println("LED: on");
    digitalWrite(LED_BUILTIN, HIGH);
  }
}

float conversion25(long concentrationPM25) {
  double pi = 3.14159;
  double density = 1.65 * pow (10, 12);
  double r25 = 0.44 * pow (10, -6);
  double vol25 = (4/3) * pi * pow (r25, 3);
  double mass25 = density * vol25;
  double K = 3531.5;
  return (concentrationPM25) * K * mass25;
}

float conversion10(long concentrationPM10) {
  double pi = 3.14159;
  double density = 1.65 * pow (10, 12);
  double r10 = 0.44 * pow (10, -6);
  double vol10 = (4/3) * pi * pow (r10, 3);
  double mass10 = density * vol10;
  double K = 3531.5;
  return (concentrationPM10) * K * mass10;
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
//    debugSerial.print("lowpulseoccupancy:");
//    debugSerial.print(lowpulseoccupancy);
//    debugSerial.print("\n");
//    debugSerial.print("ratio:");
//    debugSerial.print(ratio);
//    debugSerial.print("\n");
//    debugSerial.print("PPDNS42:");
//    debugSerial.println(concentration);
//    debugSerial.print("\n");
    
    lowpulseoccupancy = 0;
    return(concentration);    
    }
  }  
}
