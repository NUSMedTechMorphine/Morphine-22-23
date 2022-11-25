//--------------------Useful Links--------------------
//code for wifi connection: https://techtutorialsx.com/2017/04/24/esp32-connecting-to-a-wifi-network/
//code for MPU + ESP + OLED connection: https://randomnerdtutorials.com/esp32-mpu-6050-accelerometer-gyroscope-arduino/



//--------------------Importing Packages--------------------
#include <WiFi.h>
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



//--------------------Define variables--------------------
unsigned long sendDataPrevMillis = 0;
int count = 0;
bool signupOK = false;
const char* ssid = "Iphone (2)";
const char* password =  "12345678";
int LoopCodeStartTime = 0;
int StartUpload = 0;
int EndUpload = 0;
int LoopCodeEndTime = 0;
float UploadSpeed = 0;
float LoopSpeed = 0;



//--------------------Initialising Components--------------------
Adafruit_MPU6050 mpu; 
Adafruit_SSD1306 display = Adafruit_SSD1306(128, 64, &Wire); 


 
void setup() {
  Serial.begin(115200);

//--------------------Wifi Connection-------------------- 
  WiFi.begin(ssid, password);
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }
 
  Serial.println("Connected to the WiFi network");
  
  Serial.print("RRSI: "); 
  Serial.println(WiFi.RSSI()); 



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

  // Everything here is for MPU readings
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  Serial.print("Accelerometer ");
  Serial.print("X: ");
  Serial.print(a.acceleration.x, 1);
  Serial.print(" m/s^2, ");
  Serial.print("Y: ");
  Serial.print(a.acceleration.y, 1);
  Serial.print(" m/s^2, ");
  Serial.print("Z: ");
  Serial.print(a.acceleration.z, 1);
  Serial.println(" m/s^2");
  
  display.println("Accelerometer - m/s^2");
  display.print(a.acceleration.x, 1);
  display.print(", ");
  display.print(a.acceleration.y, 1);
  display.print(", ");
  display.print(a.acceleration.z, 1);
  display.println("");

  Serial.print("Gyroscope ");
  Serial.print("X: ");
  Serial.print(g.gyro.x, 1);
  Serial.print(" rps, ");
  Serial.print("Y: ");
  Serial.print(g.gyro.y, 1);
  Serial.print(" rps, ");
  Serial.print("Z: ");
  Serial.print(g.gyro.z, 1);
  Serial.println(" rps");

  Serial.print("Temperature: ");
  Serial.print(temp.temperature);
  Serial.println(" degC");

  Serial.println(count);

  Serial.println("");
  
  display.println("Gyroscope - rps");
  display.print(g.gyro.x, 1);
  display.print(", ");
  display.print(g.gyro.y, 1);
  display.print(", ");
  display.print(g.gyro.z, 1);
  display.println("");

  display.display();
  


  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 0.0001 || sendDataPrevMillis == 0)){
    sendDataPrevMillis = millis();

    Serial.print("Sending data to cloud......");
    FirebaseJsonArray aXArr;
    FirebaseJsonArray aYArr;
    FirebaseJsonArray aZArr;
    FirebaseJsonArray gXArr;
    FirebaseJsonArray gYArr;
    FirebaseJsonArray gZArr;
    FirebaseJsonArray tempArr;
    
    aXArr.add(a.acceleration.x);
    aYArr.add(a.acceleration.y);
    aZArr.add(a.acceleration.z);
    gXArr.add(g.gyro.x);
    gYArr.add(g.gyro.y);
    gZArr.add(g.gyro.z);
    tempArr.add(temp.temperature);

    StartUpload = millis();
    Serial.printf("Set aXArr array... %s\n", Firebase.RTDB.setArray(&fbdo, "/Test 1/aXarr", &aXArr) ? "ok" : fbdo.errorReason().c_str());
    Serial.printf("Set aYArr array... %s\n", Firebase.RTDB.setArray(&fbdo, "/Test 1/aYArr", &aYArr) ? "ok" : fbdo.errorReason().c_str());
    Serial.printf("Set aZArr array... %s\n", Firebase.RTDB.setArray(&fbdo, "/Test 1/aZArr", &aZArr) ? "ok" : fbdo.errorReason().c_str());
    Serial.printf("Set gXArr array... %s\n", Firebase.RTDB.setArray(&fbdo, "/Test 1/gXArr", &gXArr) ? "ok" : fbdo.errorReason().c_str());
    Serial.printf("Set gYArr array... %s\n", Firebase.RTDB.setArray(&fbdo, "/Test 1/gYArr", &gYArr) ? "ok" : fbdo.errorReason().c_str());
    Serial.printf("Set gZArr array... %s\n", Firebase.RTDB.setArray(&fbdo, "/Test 1/gZArr", &gZArr) ? "ok" : fbdo.errorReason().c_str());
    Serial.printf("Set tempArr array... %s\n", Firebase.RTDB.setArray(&fbdo, "/Test 1/tempArr", &tempArr) ? "ok" : fbdo.errorReason().c_str());
    Serial.printf("Set Accounter... %s\n", Firebase.RTDB.setInt(&fbdo, "/Test 1/Accounter", count) ? "ok" : fbdo.errorReason().c_str());
    EndUpload = millis();
    count++;
    Serial.print("Accounter: ");
    Serial.println(count);
  }   

  //-----------For testing speed (remove in final code)-----------
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
  Serial.printf("Set UploadSpeed array... %s\n", Firebase.RTDB.setArray(&fbdo, "/Test 1/UploadSpeedArr", &UploadSpeedArr) ? "ok" : fbdo.errorReason().c_str());
  Serial.printf("Set LoopSpeed array... %s\n", Firebase.RTDB.setArray(&fbdo, "/Test 1/LoopSpeedArr", &LoopSpeedArr) ? "ok" : fbdo.errorReason().c_str());
  //-----------For testing speed (remove in final code)-----------
  
  
  
}
