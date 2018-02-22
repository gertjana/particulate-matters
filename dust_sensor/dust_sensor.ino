#include <TheThingsNetwork.h>
    
#define loraSerial Serial1
#define debugSerial Serial

#define DUST_SENSOR_DIGITAL_PIN_PM10  3
#define DUST_SENSOR_DIGITAL_PIN_PM25  6

// Replace REPLACE_ME with TTN_FP_EU868 or TTN_FP_US915
#define freqPlan TTN_FP_EU868

const char *appEui = "70B3D57EF0003899";
const char *appKey = "5F02B508A92A2B6CF839515C63C3692F";
const int port = 3;
    
TheThingsNetwork ttn(loraSerial, debugSerial, freqPlan);

//dust sensor variables
int val = 0;           // variable to store the value coming from the sensor
float valDUSTPM25 =0.0;
float lastDUSTPM25 =0.0;
float valDUSTPM10 =0.0;
float lastDUSTPM10 =0.0;
unsigned long duration;
unsigned long starttime;
unsigned long endtime;
unsigned long sampletime_ms = 30000;
unsigned long lowpulseoccupancy = 0;
float ratio = 0;
long concentrationPM25 = 0;
long concentrationPM10 = 0;
int temp=20; //external temperature, if you can replace this with a DHT11 or better 
long ppmv;

    
void setup() {
  pinMode(DUST_SENSOR_DIGITAL_PIN_PM10,INPUT);
  pinMode(DUST_SENSOR_DIGITAL_PIN_PM25,INPUT);
  
  loraSerial.begin(57600);
  debugSerial.begin(9600);
    
  // Wait a maximum of 10s for Serial Monitor
  while (!debugSerial && millis() < 10000);
    
  debugSerial.println("-- STATUS");
  ttn.showStatus();

  debugSerial.println("-- JOIN");
  ttn.join(appEui, appKey);

  ttn.onMessage(message);
}

void loop() {
  debugSerial.println("-- LOOP");
    
  // Prepare array of 1 byte to indicate LED status
  byte data[1];
  data[0] = (digitalRead(LED_BUILTIN) == HIGH) ? 1 : 0;
    
  // Send it off
  ttn.sendBytes(data, sizeof(data), port) ;
      
  delay(10000);
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
    //Serial.print("lowpulseoccupancy:");
    //Serial.print(lowpulseoccupancy);
    //Serial.print("\n");
    //Serial.print("ratio:");
    //Serial.print(ratio);
    //Serial.print("\n");
    //Serial.print("PPDNS42:");
    //Serial.println(concentration);
    //Serial.print("\n");
    
    lowpulseoccupancy = 0;
    return(concentration);    
    }
  }  
}
