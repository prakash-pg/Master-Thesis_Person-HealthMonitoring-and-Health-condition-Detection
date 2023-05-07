#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "DHT.h"
#include <DFRobot_MAX30102.h>
#include <LiquidCrystal_I2C.h>
#include <MQ135.h>
#include "time.h"
#include "ThingSpeak.h"

// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Insert your network credentials
#define WIFI_SSID "xxxxxxxxxxxxxxx"
#define WIFI_PASSWORD "xxxxxxxxxxx"

// Insert Firebase project API Key
#define API_KEY "xxxxxxxxxxxxxxxxx"

// Insert Authorized Email and Corresponding Password
#define USER_EMAIL "xxxxxxxxxxx"
#define USER_PASSWORD "xxxxxxxxx"

// Insert RTDB URLefine the RTDB URL
#define DATABASE_URL "xxxxxxxxxxxxxxxxxxxxxxxx"

#define LM35_Sensor1 35

DFRobot_MAX30102 particleSensor;

LiquidCrystal_I2C lcd(0x3F,16,2);

WiFiClient  client;

unsigned long myChannelNumber = 1910851;
const char * myWriteAPIKey = "UJBRJU7CDER1WK05";

#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

const int aqsensor = 34;  //output of mq135 connected to A0 pin of Arduino
int threshold = 250;      //Threshold level for Air Quality

float roomtemperature;
float roomhumidity;

float f;


// Define Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Variable to save USER UID
String uid;

// Database main path (to be updated in setup with the user UID)
String databasePath;
// Database child nodes
String bodytempPath = "/bodytemperature";
String heartratePath = "/heartrate";
String bloodoxygenPath = "/bloodoxygen";
String tempPath = "/roomtemperature";
String humPath = "/roomhumidity";
String aqPath = "/airquality";
String timePath = "/timestamp";


// Parent Node (to be updated in every loop)
String parentPath;

int timestamp;
FirebaseJson json;

const char* ntpServer = "pool.ntp.org";


// Timer variables (send new readings every three minutes)
unsigned long sendDataPrevMillis = 0;
unsigned long lastTime = 0;
unsigned long timerDelay = 5000;


// Initialize WiFi
void initWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
  Serial.println();
}

// Function that gets current epoch time
unsigned long getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    //Serial.println("Failed to obtain time");
    return(0);
  }
  time(&now);
  return now;
}

void setup(){
  Serial.begin(115200);

  // Initialize BME280 sensor

  initWiFi();

  lcd.init();         // initialize the lcd

  lcd.backlight();    // open the backlight

  pinMode (aqsensor,INPUT);

  ThingSpeak.begin(client);

   while (!particleSensor.begin()) {
    lcd.print("MAX30102 was not found");
    Serial.println("MAX30102 was not found");
    delay(100);
  }

    particleSensor.sensorConfiguration(/*ledBrightness=*/50, /*sampleAverage=*/SAMPLEAVG_4, \
                        /*ledMode=*/MODE_MULTILED, /*sampleRate=*/SAMPLERATE_100, \
                        /*pulseWidth=*/PULSEWIDTH_411, /*adcRange=*/ADCRANGE_16384);

   
  configTime(0, 0, ntpServer);

  // Assign the api key (required)
  config.api_key = API_KEY;

  // Assign the user sign in credentials
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  // Assign the RTDB URL (required)
  config.database_url = DATABASE_URL;

  Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(4096);

  // Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h

  // Assign the maximum retry of token generation
  config.max_token_generation_retry = 5;

  // Initialize the library with the Firebase authen and config
  Firebase.begin(&config, &auth);

  // Getting the user UID might take a few seconds
  Serial.println("Getting User UID");
  while ((auth.token.uid) == "") {
    Serial.print('.');
    delay(1000);
  }
  // Print user UID
  uid = auth.token.uid.c_str();
  Serial.print("User UID: ");
  Serial.println(uid);

  // Update database path
  databasePath = "/UsersData/" + uid + "/readings";
}

void loop(){

  // Send new readings to database
  if (Firebase.ready() && (millis() - sendDataPrevMillis > timerDelay || sendDataPrevMillis == 0)){
    sendDataPrevMillis = millis();

    
    // Get the voltage reading from the LM35
  int reading = analogRead(LM35_Sensor1);

  // Convert that reading into voltage
  float voltage = reading * (2.0 / 1028.0);

  int airquality = analogRead(aqsensor) / 18;

  // Convert the voltage into the temperature in Celsius
  float bodytemperature = voltage * 25;

  int32_t bloodoxygen; //SPO2
  int8_t blov; //Flag to display if SPO2 calculation is valid
  int32_t heartrate; //Heart-rate
  int8_t hrv; //Flag to display if heart-rate calculation is valid 


   roomtemperature = dht.readTemperature();
   
   roomhumidity = dht.readHumidity();

   f = dht.readTemperature(true);

   // Check if any reads failed and exit early (to try again).
  if (isnan(roomhumidity) || isnan(roomtemperature) || isnan(f)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  
  float hic = dht.computeHeatIndex(roomtemperature, roomhumidity, false);

  Serial.println(F("Wait about four seconds"));
  particleSensor.heartrateAndOxygenSaturation(/**SPO2=*/&bloodoxygen, /**SPO2Valid=*/&blov, /**heartRate=*/&heartrate, /**heartRateValid=*/&hrv);
  //Print result 

    //Get current timestamp
    timestamp = getTime();
    Serial.print ("time: ");
    Serial.println (timestamp);

    parentPath= databasePath + "/" + String(timestamp);
    
    json.set(bodytempPath.c_str(), String(bodytemperature));
    json.set(heartratePath.c_str(), String(heartrate));
    json.set(bloodoxygenPath.c_str(), String(bloodoxygen));
    json.set(tempPath.c_str(), String(roomtemperature));
    json.set(humPath.c_str(), String(roomhumidity));
    json.set(aqPath.c_str(), String(airquality));
    json.set(timePath, String(timestamp));
    Serial.printf("Set json... %s\n", Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &json) ? "ok" : fbdo.errorReason().c_str());

  Serial.print(F("Body Temperature: "));
  Serial.print(bodytemperature);
  Serial.println(" C |");  
  delay(1000);
  
  Serial.print(F("Heart Rate: "));
  Serial.print(heartrate);
  Serial.println(" BPM |");  
  delay(1000);

  Serial.print(F("Blood Oxygen: "));
  Serial.print(bloodoxygen);
  Serial.println(" % |");  
  delay(1000);

  Serial.print(F("Room Humidity: "));
  Serial.print(roomhumidity);
  Serial.println(" % |");  
  delay(1000);

  Serial.print(F("Room Temperature: "));
  Serial.print(roomtemperature);
  Serial.println(F(" C |"));
  delay(1000);

  Serial.print(F("Heat index: "));
  Serial.print(hic);
  Serial.println(F(" C |"));
  delay(1000);

  Serial.print("Air Quality: ");  //print message in serail monitor
  Serial.print(airquality);            //print value of ppm in serial monitor
  Serial.println(F(" PPM |"));
  delay(1000);

  if (isnan(bodytemperature) || isnan(heartrate) || isnan(bloodoxygen) || isnan(roomhumidity) || isnan(roomtemperature)|| isnan(airquality)) {
    lcd.setCursor(0, 0);
    lcd.print("Failed");
  } else {
    lcd.setCursor(1, 1);  // display position
    lcd.print("BodyTemp ");
    lcd.print(bodytemperature);     // display the temperature
    lcd.print("c");
    delay(2000);
    lcd.clear();

    lcd.setCursor(2, 1);  // display position
    lcd.print("HR ");
    lcd.print(heartrate);      // display the humidity
    lcd.print("BPM");
    delay(2000);
    lcd.clear();
    
    lcd.setCursor(1, 1);  // display position
    lcd.print("SPO2 ");
    lcd.print(bloodoxygen);      // display the humidity
    lcd.print("%");
    delay(2000);
    lcd.clear();

    lcd.setCursor(2, 1);  // display position
    lcd.print("RoomHum ");
    lcd.print(roomhumidity);      // display the humidity
    lcd.print("%");
    delay(2000);
    lcd.clear();

    lcd.setCursor(1, 1);  // display position
    lcd.print("RoomTemp ");
    lcd.print(roomtemperature);      // display the humidity
    lcd.print("c");
    delay(2000);
    lcd.clear();

    lcd.setCursor(2, 1);  // display position
    lcd.print("AQ ");
    lcd.print(airquality);      // display the humidity
    lcd.print("PPM");
    delay(2000);
    lcd.clear();

  }

  delay(1000);


   // set the fields with the values
    ThingSpeak.setField(1, bodytemperature);
    ThingSpeak.setField(2, heartrate);
    ThingSpeak.setField(3, bloodoxygen);
    ThingSpeak.setField(4, roomhumidity);
    ThingSpeak.setField(5, roomtemperature);
    ThingSpeak.setField(6, airquality);
    
    
    
    // Write to ThingSpeak. There are up to 8 fields in a channel, allowing you to store up to 8 different
    // pieces of information in a channel.  Here, we write to field 1.
    int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);

    if(x == 200){
      Serial.println("Channel update successful.");
    }
    else{
      Serial.println("Problem updating channel. HTTP error code " + String(x));
    }
    lastTime = millis();
    
  }
}