// reference for code taken from https://learn.sparkfun.com/tutorials/sparkfun-gps-neo-m9n-hookup-guide#hardware-overview
// and https://github.com/sparkfun/SparkFun_Ublox_Arduino_Library/blob/master/examples/Example12_UseUart/Example12_UseUart.ino
// reference for hardware connections: https://randomnerdtutorials.com/esp32-i2c-communication-arduino-ide/
//--------------------Importing Packages--------------------
#include <WiFi.h>
#include <WiFiMulti.h>
#include <FirebaseESP32.h>
#include <SparkFun_u-blox_GNSS_Arduino_Library.h> //http://librarymanager/All#SparkFun_u-blox_GNSS
SFE_UBLOX_GNSS myGNSS;



//--------------------Accessing to Google Firebase--------------------
//Provide the token generation process info.
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"
// Insert Firebase project API Key
#define API_KEY "AIzaSyDH9SMbhwEBF9rRmpaPMHSakGDCtrobFf4"
// Insert RTDB URLefine the RTDB URL */
#define DATABASE_URL "https://morphine-64cdd-default-rtdb.asia-southeast1.firebasedatabase.app/" 
// Insert Authorized Email and Corresponding Password
#define USER_EMAIL "nusmedtech.morphine@gmail.com"
#define USER_PASSWORD "MorphineAddicts"

/** 5. Define the database secret (optional)
 *
 * This database secret needed only for this example to modify the database rules
 *
 * If you edit the database rules yourself, this is not required.
 */
#define DATABASE_SECRET "DATABASE_SECRET"



//--------------------Define Firebase Data object--------------------
FirebaseData fbdo; //Define Firebase Object
FirebaseJson json; //Firebase store data as JSON objects
FirebaseAuth auth; //FirebaseAuth data for authentication data
FirebaseConfig config; //FirebaseConfig data for config data
FirebaseJsonData result;



//-------------------Define multi wifi object-------------------------
WiFiMulti wifiMulti;
const char* ssid = "SLZ";
const char* password =  "12345678";



//--------------------Define variables--------------------
unsigned long sendDataPrevMillis = 0;
int count = 0;
bool signupOK = false;
float LoopCodeStartTime = 0;
float LoopCodeEndTime = 0;
float StartUpload = 0;
float EndUpload = 0;
float UploadSpeed = 0;
float LoopSpeed = 0;
const uint32_t connectTimeoutMs = 10000; // WiFi connect timeout per AP. Increase when connecting takes longer.
String StringValue = "";
long lastTime = 0; //Simple local timer. Limits amount of I2C traffic to u-blox module.

void setup()
{
  Serial.begin(115200);

//--------------------Wifi Connection-------------------- 
  WiFi.mode(WIFI_STA);
  
  // Add list of wifi networks
  wifiMulti.addAP("Lakshmi's Galaxy S21", "wbmt3054"); //Lakshmi's hotspot
  wifiMulti.addAP("Galaxy S10+f6dc" , "kism8575"); //Lakshmi's hotspot 2
  wifiMulti.addAP("Syarwina's iPhone", "nowifiagain"); //Syarwina's hotspot
  wifiMulti.addAP("kai Jun's phone", "12345678"); //Zhe Li's hotspot
  wifiMulti.addAP("SLZ", "12345678"); //LieZhou's hotspot
  wifiMulti.addAP("Rigel wifi", "12345678"); //Rigel's hotspot
  wifiMulti.addAP("Neh", "1235abcd"); //Ernest's hotspot


  // WiFi.scanNetworks will return the number of networks found
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0) {
      Serial.println("no networks found");
  } 
  else {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i) {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN)?" ":"*");
      delay(10);
    }
  }

  // Connect to Wi-Fi using wifiMulti (connects to the SSID with strongest connection)
  Serial.println("Connecting Wifi...");
  if(wifiMulti.run() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }

  
//--------------------FireBase Login-------------------- 
   /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the user sign in credentials */
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(4096);

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
  Firebase.begin(&config, &auth);

  String base_path = "/UsersData/";

  /** Now modify the database rules (if not yet modified)
   *
   * The user, <user uid> in this case will be granted to read and write
   * at the certain location i.e. "/UsersData/<user uid>".
   *
   * If you database rules has been modified, please comment this code out.
   *
   * The character $ is to make a wildcard variable (can be any name) represents any node key
   * which located at some level in the rule structure and use as reference variable
   * in .read, .write and .validate rules
   *
   * For this case $userId represents any <user uid> node that places under UsersData node i.e.
   * /UsersData/<user uid> which <user uid> is user UID.
   *
   * Please check your the database rules to see the changes after run the below code.
   */
  String var = "$userId";
  String val = "($userId === auth.uid && auth.token.premium_account === true && auth.token.admin === true)";
  Firebase.setReadWriteRules(fbdo, base_path, var, val, val, DATABASE_SECRET);

  /** path for user data is now "/UsersData/<user uid>"
    * The user UID can be taken from auth.token.uid
    * 
    * The refresh token can be accessed from Firebase.getRefreshToken().
    */


//--------------------GPS set-up-------------------- 
  Serial.println("Setting up GPS");

  // Assume that the U-Blox GNSS is running at 9600 baud (the default) or at 38400 baud.
  // Loop until we're in sync and then ensure it's at 38400 baud.
  do {
    Serial.println("GNSS: trying 38400 baud");
    Serial1.begin(38400, SERIAL_8N1, 16, 17);
    if (myGNSS.begin(Serial1) == true) break;

    delay(100);
    Serial.println("GNSS: trying 9600 baud");
    Serial1.begin(9600, SERIAL_8N1, 16, 17);
    if (myGNSS.begin(Serial1) == true) {
        Serial.println("GNSS: connected at 9600 baud, switching to 38400");
        myGNSS.setSerialRate(38400);
        delay(100);
    } else {
        //myGNSS.factoryReset();
        delay(2000); // Wait a bit before trying again to limit the Serial output
    }
  } while(1);
  Serial.println("GNSS serial connected");

  myGNSS.setUART1Output(COM_TYPE_UBX); // Set the UART port to output UBX only
  myGNSS.setI2COutput(COM_TYPE_UBX);   // Set the I2C port to output UBX only (turn off NMEA noise)
  myGNSS.saveConfiguration();          // Save the current settings to flash and BBR

}

void loop(){
  LoopCodeStartTime = millis();
  String path = "/Users Data/";
  path += "Token UID:";
  path += auth.token.uid.c_str();
  path += "/Split Circuit/GPS";
  String user_path_accounter = path + "/GPS Accounter";
  String user_path_datapoints = path + "/GPS Datapoints";
  String UploadSpeedPath = path + "/GPS UploadSpeedArr";
  String LoopSpeedPath = path + "/GPS LoopSpeedArr";

  //if the connection to the stongest hotstop is lost, it will connect to the next network on the list
  if (wifiMulti.run() != WL_CONNECTED) {
    Serial.print("WiFi not connected: ");
  }


//--------------------GPS readings--------------------  
  //Query module only every second (1Hz). Doing it more often will just cause I2C traffic.
  //We have coded the gps to send data at more frequently >1Hz which may cause I2C traffic as we cannot delay mpu6050 readings
  //The module only responds when a new position is available
  
  int latitude = myGNSS.getLatitude();
  int longitude = myGNSS.getLongitude();
  int altitude = myGNSS.getAltitude();
  int SIV = myGNSS.getSIV();


  if (Firebase.ready() && (millis() - sendDataPrevMillis > 10000U || sendDataPrevMillis == 0)){
  
    sendDataPrevMillis = millis();

    Serial.println("--------------------");
    Serial.println("Sending data to cloud......");
    //append data to string format
    String datapoints = "";
    FirebaseJsonArray DatapointsArr;
    
    
    float SetStartTime = millis();
    datapoints += "Latitude: ";
    datapoints += String(latitude);
    datapoints += "(*10^-7) ";
    datapoints += "Longitude: ";
    datapoints += String(longitude);
    datapoints += "(*10^-7) ";
    datapoints += "Altitude: ";
    datapoints += String(altitude);
    datapoints += "(mm) ";
    datapoints += "Satellite-in-view: ";
    datapoints += String(SIV);
    datapoints += " ";

    float SetEndTime = millis();
    float SetSpeed = SetEndTime - SetStartTime;
    
    datapoints += "timing for this set: ";
    datapoints += String(SetSpeed);
    datapoints += " ";
      
   
    DatapointsArr.add(datapoints);
    
    StartUpload = millis();
    
    Serial.printf("Uploading Accounter Value... %s\n", Firebase.RTDB.setInt(&fbdo, user_path_accounter, count) ? "ok" : fbdo.errorReason().c_str());
    Serial.printf("Uploading Data Array ... %s\n", Firebase.RTDB.setArray(&fbdo, user_path_datapoints, &DatapointsArr) ? "ok" : fbdo.errorReason().c_str());
    EndUpload = millis();
    
    count++;
    Serial.print("Accounter: ");
    Serial.println(count);
  }
  //-----------For testing speed-----------
  float UploadSpeed = EndUpload - StartUpload;
  Serial.print("DataBase Upload Speed: ");
  Serial.println(UploadSpeed);

  LoopCodeEndTime = millis();
  float LoopSpeed = LoopCodeEndTime - LoopCodeStartTime;
  Serial.print("Loop Code Speed: ");
  Serial.println(LoopSpeed);

  FirebaseJsonArray UploadSpeedArr;
  FirebaseJsonArray LoopSpeedArr;

  UploadSpeedArr.add(UploadSpeed);
  LoopSpeedArr.add(LoopSpeed);
  Serial.printf("Set Upload Speed array... %s\n", Firebase.RTDB.setArray(&fbdo, UploadSpeedPath, &UploadSpeedArr) ? "ok" : fbdo.errorReason().c_str());
  Serial.printf("Set Loop Speed array... %s\n", Firebase.RTDB.setArray(&fbdo, LoopSpeedPath, &LoopSpeedArr) ? "ok" : fbdo.errorReason().c_str());
 
  Serial.println("");
  //-----------For testing speed-----------
}
