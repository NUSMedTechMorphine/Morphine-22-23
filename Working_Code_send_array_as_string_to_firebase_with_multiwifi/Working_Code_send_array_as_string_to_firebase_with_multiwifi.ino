//--------------------Useful Links--------------------
//code for wifi connection: https://techtutorialsx.com/2017/04/24/esp32-connecting-to-a-wifi-network/
//code for MPU + ESP + OLED connection: https://randomnerdtutorials.com/esp32-mpu-6050-accelerometer-gyroscope-arduino/



//--------------------Importing Packages--------------------
#include <WiFi.h>
#include <WiFiMulti.h>
#include <Firebase_ESP_Client.h>
#include <Adafruit_MPU6050.h> 
#include <Adafruit_SSD1306.h> 
#include <Adafruit_Sensor.h> 



//--------------------Accessing to Google Firebase--------------------
//Provide the token generation process info.
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"
// Insert Firebase project API Key
#define API_KEY "AIzaSyDH9SMbhwEBF9rRmpaPMHSakGDCtrobFf4"
// Insert RTDB URLefine the RTDB URL */
#define DATABASE_URL "https://morphine-64cdd-default-rtdb.asia-southeast1.firebasedatabase.app/" 



//--------------------Define Firebase Data object--------------------
FirebaseData fbdo;
FirebaseJson json;
FirebaseAuth auth;
FirebaseConfig config;
FirebaseJsonData result;

//-------------------Define multi wifi object-------------------------
WiFiMulti wifiMulti;

//--------------------Define variables--------------------
unsigned long sendDataPrevMillis = 0;
int count = 0;
bool signupOK = false;
const char* ssid = "Iphone (2)";
const char* password =  "12345678";
float LoopCodeStartTime = 0;
float LoopCodeEndTime = 0;
float StartUpload = 0;
float EndUpload = 0;
float UploadSpeed = 0;
float LoopSpeed = 0;
const uint32_t connectTimeoutMs = 10000; // WiFi connect timeout per AP. Increase when connecting takes longer.
String StringValue = "";


//--------------------Initialising Components--------------------
Adafruit_MPU6050 mpu; 
Adafruit_SSD1306 display = Adafruit_SSD1306(128, 32, &Wire); 


 
void setup() {
  Serial.begin(115200);

//--------------------Wifi Connection-------------------- 
  WiFi.mode(WIFI_STA);
  
  // Add list of wifi networks
  wifiMulti.addAP("Lakshmi's Galaxy S21", "wbmt3054"); //Lakshmi's hotspot
  wifiMulti.addAP("Galaxy S10+f6dc" , "kism8575"); //Lakshmi's hotspot 2
  wifiMulti.addAP("Syarwina's iPhone", "nowifiagain"); //Syarwina's hotspot
  wifiMulti.addAP("kai Jun's phone", "12345678"); //Zhe Li's hotspot
  wifiMulti.addAP("Iphone (2)", "12345678"); //
  wifiMulti.addAP("Rigel wifi", "12345678"); //
 

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
//-------------------------------------------------------

   /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", "")){
    Serial.println("ok");
    signupOK = true;
  }
  else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);



  //This section is for MPU readings
    // while (!Serial);
  Serial.println("MPU6050 OLED demo");

  if (!mpu.begin()) {
    Serial.println("Sensor init failed");
    while (1)
      yield();
  }
  Serial.println("Found a MPU-6050 sensor"); 
 
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally 
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32 
    Serial.println(F("SSD1306 allocation failed")); 
    for (;;) 
      ; // Don't proceed, loop forever 
  } 
  display.clearDisplay(); 
  display.setTextSize(0.5); 
  display.setCursor(0,0); 
} 

 
void loop() {
  LoopCodeStartTime = millis();

  //if the connection to the stongest hotstop is lost, it will connect to the next network on the list
  if (wifiMulti.run() != WL_CONNECTED) {
    Serial.print("WiFi not connected: ");

  }



  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 0.0001 || sendDataPrevMillis == 0)){
    sendDataPrevMillis = millis();

    Serial.println("--------------------");
    Serial.println("Sending data to cloud......");
    //append data to string format
    int i = 0;
    int max_samples = 20;
    String datapoints = "";
    FirebaseJsonArray DatapointsArr;
    
    while (i<max_samples)
    {
      float SetStartTime = millis();
      sensors_event_t a, g, temp;
      mpu.getEvent(&a, &g, &temp);
      datapoints += "Set: ";
      datapoints += String(i);
      datapoints += " ";
      datapoints += "Ax: ";
      datapoints += String(a.acceleration.x);
      datapoints += " ";
      datapoints += "Ay: ";
      datapoints += String(a.acceleration.y);
      datapoints += " ";
      datapoints += "Az: ";
      datapoints += String(a.acceleration.z);
      datapoints += " ";

      datapoints += "gx: ";
      datapoints += String(g.gyro.x);
      datapoints += " ";
      datapoints += "gy: ";
      datapoints += String(g.gyro.y);
      datapoints += " ";
      datapoints += "gz: ";
      datapoints += String(g.gyro.z);
      datapoints += " ";

      datapoints += "temp: ";
      datapoints += String(temp.temperature);
      datapoints += " ";

      float SetEndTime = millis();
      float SetSpeed = SetEndTime - SetStartTime;
      
      datapoints += "timing for this set: ";
      datapoints += String(SetSpeed);
      datapoints += " ";
      
      i++;
    }
    DatapointsArr.add(datapoints);
    
    StartUpload = millis();
    Serial.printf("Uploading Accounter Value... %s\n", Firebase.RTDB.setInt(&fbdo, "/Test 2/Accounter", count) ? "ok" : fbdo.errorReason().c_str());
    Serial.printf("Uploading Data Array ... %s\n", Firebase.RTDB.setArray(&fbdo, "/Test 2/datapoints", &DatapointsArr) ? "ok" : fbdo.errorReason().c_str());
    EndUpload = millis();
    
    count++;
    Serial.print("Accounter: ");
    Serial.println(count);
  }   

  //-----------For testing speed (remove in final code)-----------
  float UploadSpeed = EndUpload - StartUpload;
  Serial.print("DataBase Upload Speed: ");
  Serial.println(UploadSpeed);
  
  //delay(500); //the sensor gets value faster than data could be uploaded
  
  LoopCodeEndTime = millis();
  float LoopSpeed = LoopCodeEndTime - LoopCodeStartTime;
  Serial.print("Loop Code Speed: ");
  Serial.println(LoopSpeed);

  FirebaseJsonArray UploadSpeedArr;
  FirebaseJsonArray LoopSpeedArr;

  UploadSpeedArr.add(UploadSpeed);
  LoopSpeedArr.add(LoopSpeed);
  Serial.printf("Set Upload Speed array... %s\n", Firebase.RTDB.setArray(&fbdo, "/Test 2/UploadSpeedArr", &UploadSpeedArr) ? "ok" : fbdo.errorReason().c_str());
  Serial.printf("Set Loop Speed array... %s\n", Firebase.RTDB.setArray(&fbdo, "/Test 2/LoopSpeedArr", &LoopSpeedArr) ? "ok" : fbdo.errorReason().c_str());
 
  Serial.println("");
  //-----------For testing speed (remove in final code)-----------
  
  if (Firebase.RTDB.getString(&fbdo, "/esp_msg/Code")) {
    Serial.println("Extracting data from cloud...");
    //if (fbdo.dataType() == "String") {
    StringValue = fbdo.stringData();
    Serial.println("Cloud String: "+ StringValue);
    Serial.println("--------------------");
    Serial.println("");

  }

}
