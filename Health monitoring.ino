/*IOT project:: */
/*Author:: Abd-Alfatah Alodainy*/
/*E-mail:: ua3agw@stud.uni-obuda.hu*/
/* This program is used to Run an Arduino based device in which 3  sensors are connected to a  microcontroller
to measure the different human physiological states (HR, TEMP, HUMIDITY, O2 level and support the ECG function) */
/* Comment this out to disable prints and save space */
#define BLYNK_PRINT Serial

/* Fill-in your Template ID (only if using Blynk.Cloud) */
//#define BLYNK_TEMPLATE_ID   "YourTemplateID"
#define BLYNK_TEMPLATE_ID "TMPLRIIITE2O"
#define BLYNK_DEVICE_NAME "Health monitoring"

#include <WiFiManager.h>
#include <BlynkSimpleEsp32.h>
BlynkTimer  timer;
/*Necessary libraries */
#include <Wire.h>
#include <BlynkSimpleEsp32.h>
#include <WiFiManager.h>
#include <SHT21.h>
//#include <Adafruit_Sensor.h>
#include "time.h"
// Provide the token generation process info.
//Define SHT21 SENSOR objects
SHT21 sht;
// You should get Auth Token in the Blynk App.
// Go to the Project Settings (nut icon).
char auth[] = "UvFrfRvtLGO55EPU7xKWD_LKwEqxUx-L";
//declaration of time stamp of the Arduino running time:::: to be used in case of continous monitoring  
int timestamp;

const char* ntpServer = "pool.ntp.org";
//LEDs ot show different opperation signs
#define LED2 17 // if it is ON then Wifi is connected
#define LED3 18 // if it is blinking then data is being sent
#define LED1 4 //if it is blinking then hear rate is being measured 
#define LED4 19 // if it is ON then ECG is connected
/**************************************************************************/
// TIME OF HEART RATE SENISNG
#define REPORTING_PERIOD_MS 1000
//initializing the pulse_oximeter senosr 
#include "MAX30100_PulseOximeter.h"
PulseOximeter pox;
uint32_t tsLastReport = 0;
////initializing the temprature varilable
float temp;
float humidity;
//Non blocking delays variable
uint16_t delayingTime = 300;
uint16_t delayingTime1 = 750;
uint16_t timeRunning = 0;
unsigned long sendDataPrevMillis = 0;
unsigned long timerDelay = 5000;
//initializing the strings for the Blynk path variable 
String firetemp=String("");
float hr=0.0;
String fireOxi =String("");
String fireheart=String("");
String firehum=String("");
//blinking and updating the HR in case of sensing any beats from huamn finger
void onBeatDetected()
{
  pox.update();
  digitalWrite(LED1,!digitalRead(LED1));
}
//getting the timestamp of the Arduino
unsigned long getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return(0);
  }
  time(&now);
  return now;
}
// temp and hum measuring
void temp_and_humi() {
  if (millis() - timeRunning > 85){
  pox.update();
  temp = sht.getTemperature();  // get temp from SHT
  humidity = sht.getHumidity(); // get temp from SHT
 /* Serial.print("Temp: ");      // printing the readings to the serial output
  Serial.print(temp);
  Serial.print("\t Humidity: ");
  Serial.println(humidity);*/
  firetemp = String(temp) + String("°C");
  firehum = String(humidity) + String("%");
  }
  pox.update();
}
// HR and O2 measuring 
void pulse_oximeter() {
  pox.update();
  if (millis() - tsLastReport > REPORTING_PERIOD_MS) {
    // printing the readings to the serial monitor
   /* Serial.print("Heart rate:");
    Serial.print(pox.getHeartRate());
    Serial.print("bpm / SpO2:");
    Serial.print(pox.getSpO2());
    Serial.println("%");*/
    hr=pox.getHeartRate();
    fireheart = String(pox.getHeartRate()) + String("bpm");
    fireOxi = String ("SpO2:") + String(pox.getSpO2()) + String("%");
    tsLastReport = millis();
  }
}
void connect_to_Wifi() {
      WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
    // it is a good practice to make sure your code sets wifi mode how you want it.
    //WiFiManager, Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wm;

    // reset settings - wipe stored credentials for testing
    // these are stored by the esp library
    //wm.resetSettings();

    // Automatically connect using saved credentials,
    // if connection fails, it starts an access point with the specified name ( "AutoConnectAP"),
    // if empty will auto generate SSID, if password is blank it will be anonymous AP (wm.autoConnect())
    // then goes into a blocking loop awaiting configuration and will return success result

    bool res;
    // res = wm.autoConnect(); // auto generated AP name from chipid
    // res = wm.autoConnect("AutoConnectAP"); // anonymous ap
    res = wm.autoConnect("ESP32","AbdAlfatah"); // password protected ap

    if(!res) {
        Serial.println("Failed to connect");
        // ESP.restart(); //This can be used in case we want to restart the device each time the connection failed
    } 
    else {
        //here it means that we are connected to the WiFi    
        Serial.println("connected...yeey :)");
        digitalWrite( LED2, !digitalRead(LED2));//this LED  will tell us whehter the device is connected to wifi or not
        //if the LED is ON then it is connected
       //this contains the password
        const char* ssid="Vodafone-0B3A";//=WiFi.SSID().c_str();
        const char* pass =WiFi.psk().c_str();
        Serial.println(ssid);
        Serial.println(pass);
      
    }
}

void Sending_new_readings(){

// Send new readings to database
    //Get current timestamp
    //timestamp = getTime();
    /*Serial.print ("time: ");
    Serial.println (timestamp);*/ //print the current time stamp to the serial monitor
    //Creating path in the database for data sending
    Blynk.virtualWrite(V0,temp);
    Blynk.virtualWrite(V1,firehum);
    Blynk.virtualWrite(V2,fireheart);
    Blynk.virtualWrite(V3,fireOxi);
    Blynk.virtualWrite(V4,hr);
    digitalWrite(LED3,!digitalRead(LED3)); //THIS led will blink in case of data being sent
    Serial.println("it is sending");
  //}
}
TaskHandle_t Task1;
TaskHandle_t Task2;
void setup()
{
  // Debug console
  Serial.begin(9600);
  pinMode(LED1,OUTPUT);  //LEDs setup
  pinMode(LED2,OUTPUT);
  pinMode(LED3,OUTPUT);
  pinMode(LED4,OUTPUT);
  timeRunning = millis();
  xTaskCreatePinnedToCore(
                   Task1code,   /* Task function. */
                   "Task1",     /* name of task. */
                   10000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                   1,           /* priority of the task */
                   &Task1,      /* Task handle to keep track of created task */
                    0);          /* pin task to core 0 */                  
  delay(500); 

  //This task will be executed in the Task2code() function, with priority 1 and executed on core 1
 xTaskCreatePinnedToCore(
                    Task2code,   /* Task function. */
                    "Task2",     /* name of task. */
                   10000,       /* Stack size of task */
                   NULL,        /* parameter of the task */
                    1,           /* priority of the task */
                   &Task2,      /* Task handle to keep track of created task */
                    1
                    );          /* pin task to core 1 */
   delay(500); 
}
//task 1 code (In core 0 we will run the all sensors)
void Task1code( void * pvParameters ){
  //Serial printing
//Serial.print("Initializing pulse oximeter..");
  // Initialize the PulseOximeter instance
  // Failures are generally due to an improper I2C wiring, missing power supply
  // or wrong target chip
  if (!pox.begin()) {
    Serial.println("FAILED");
  } else {
    Serial.println("SUCCESS");
  }
  pox.update();
  pox.setOnBeatDetectedCallback(onBeatDetected);
  for(;;){
    pulse_oximeter();
    if (millis() - timeRunning > delayingTime1) {
        pox.update();
        temp_and_humi();
      timeRunning = millis();
  } 
}
}
//Task2code: Connect to wifi and send the data to the firebase

void Task2code( void * pvParameters ){
    connect_to_Wifi();
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Blynk.config(auth);
  timer.setInterval(100L,Sending_new_readings);
    
       
  for(;;){

      Blynk.run();
      timer.run();
   
  }

}
  

void loop()
{
///Since we used a multi tasking method this sould be empty
}