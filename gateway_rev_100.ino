#include <WiFi.h>
#include "esp_wifi.h"
#include <esp_now.h>
#include "freertos/queue.h"
#include "time.h"
#include <FB_Const.h>
#include <FB_Error.h>
#include <FB_Network.h>
#include <FB_Utils.h>
#include <Firebase.h>
#include <FirebaseFS.h>
#include <Firebase_ESP_Client.h>
#include <WebServer.h>
#include <EEPROM.h>
#include "esp_task_wdt.h"
#include "SRF05.h"
const int trigger = 26;
const int echo    = 25;

SRF05 SRF(trigger, echo);

#define ind 13
#define buzz 19

// variabel
bool signupOK;
int chan, bedengan, node, n;
String id;
#define MAX_NODES 10
enum MessageType {PAIRING, DATA};
MessageType messageType;

// setting Wifi
// #define WIFI_SSID "Elkapride"
// #define WIFI_PASSWORD "ngiritbos"
#define WIFI_SSID "TAbawang"
#define WIFI_PASSWORD "bawang123"

// RTOS
TaskHandle_t DataTaskHandle = NULL;
const uint32_t Data_Ready = 0x01; // Battery ready flag
const uint32_t Data_Kapal = Data_Ready; // All sensors ready

// antrian
QueueHandle_t dataQueue;
QueueHandle_t kapalQueue;
QueueHandle_t antriKirim;
QueueHandle_t espNow;

// EEPROM addresses for storing email and password
#define EEPROM_SIZE 512
#define EEPROM_EMAIL_ADDR 0
#define EEPROM_PASSWORD_ADDR 100

//datastoredDay
#define MAX_ENTRIES 24 // Jumlah maksimum entri per hari

int dailyHumidity[MAX_ENTRIES];
float dailyTemperature[MAX_ENTRIES];
float dailypH[MAX_ENTRIES];
int dailyN[MAX_ENTRIES];
int dailyP[MAX_ENTRIES];
int dailyK[MAX_ENTRIES];
int entryIndex = 0;

// Web server on port 80
WebServer server(80);

// setting AP
const char *ssid = "Master_GATEWAY";
const char *password = "12345678";


// sett time
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 7 * 3600; // GMT+7
const int daylightOffset_sec = 0;    // Tidak ada perubahan daylight saving

// setting FIREBASE
#define API_KEY "AIzaSyAksG3S-WQ5L1zKJknPXSQAIl-TNlqQZ8w"
#define DATABASE_URL "https://tabawangmerah-default-rtdb.firebaseio.com/"

String user_email = "";
String user_password = "";

// format waktu
unsigned long timestamp;
String formattedDate;
String formattedTime;

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// data rata rata satu gateway
int tHum;
float tTemp;
float tpH;
int tN;
int tP;
int tK;

// pesan node
typedef struct struct_message {
  uint8_t msgType;
  char id[6];
  int humidity;
  float temperature;
  float pH;
  int N;
  int P;
  int K;
  float baterai;
  double Latitude;
  double Longitude;
} struct_message;

struct_message nodeData[MAX_NODES][MAX_NODES];
struct_message inData;

// pesan kapal
typedef struct data_kapal {
  uint8_t msgType;
  int id;
  int humidity;
  float temperature;
  double Latitude;
  double Longitude;
} data_kapal;

data_kapal nodeKapal[MAX_NODES];

typedef struct struct_pairing {
  uint8_t msgType;
  char id[6];
  uint8_t macAddr[6];
  uint8_t channel;
} struct_pairing;

struct_pairing pairingData;
struct_pairing pairingMatrix[MAX_NODES][MAX_NODES];

// Handle root URL
void handleRoot() {
  EEPROM.begin(EEPROM_SIZE);
  String storedEmail = EEPROM.readString(EEPROM_EMAIL_ADDR);
  String storedPassword = EEPROM.readString(EEPROM_PASSWORD_ADDR);

  if (storedEmail.length() > 0 && storedPassword.length() > 0) {
    // Redirect to /login if email and password are already stored
    server.sendHeader("Location", "/login", true);
    server.send(302, "text/plain", "");
    EEPROM.end();
  } else {
    // Display the login form if not logged in
    String html = "<html>\
                    <head>\
                      <style>\
                        body {\
                          display: flex;\
                          justify-content: center;\
                          align-items: center;\
                          height: 100vh;\
                          margin: 0;\
                          font-family: Arial, sans-serif;\
                        }\
                        form {\
                          display: flex;\
                          flex-direction: column;\
                          align-items: center;\
                          background-color: #f7f7f7;\
                          padding: 20px;\
                          border-radius: 10px;\
                          box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);\
                        }\
                        input[type='text'], input[type='password'] {\
                          font-size: 1.2em;\
                          margin: 10px 0;\
                          padding: 10px;\
                          border: 1px solid #ccc;\
                          border-radius: 5px;\
                          width: 100%;\
                          max-width: 300px;\
                        }\
                        input[type='submit'], button {\
                          font-size: 1.2em;\
                          padding: 10px 20px;\
                          color: #fff;\
                          background-color: #007bff;\
                          border: none;\
                          border-radius: 5px;\
                          cursor: pointer;\
                          transition: background-color 0.3s;\
                          margin: 5px;\
                        }\
                        input[type='submit']:hover, button:hover {\
                          background-color: #0056b3;\
                        }\
                        .show-password {\
                          display: flex;\
                          align-items: center;\
                          margin: 10px 0;\
                        }\
                        .show-password input {\
                          margin-left: 10px;\
                        }\
                      </style>\
                      <script>\
                        function togglePassword() {\
                          var passwordField = document.getElementById('password');\
                          if (passwordField.type === 'password') {\
                            passwordField.type = 'text';\
                          } else {\
                            passwordField.type = 'password';\
                          }\
                        }\
                      </script>\
                    </head>\
                    <body>\
                      <form action=\"/login\" method=\"POST\">\
                        <h2>Login to Firebase</h2>\
                        <label for=\"email\">Email:</label>\
                        <input type=\"text\" id=\"email\" name=\"email\"><br>\
                        <label for=\"password\">Password:</label>\
                        <input type=\"password\" id=\"password\" name=\"password\"><br>\
                        <div class=\"show-password\">\
                          <label for=\"showPassword\">Show Password</label>\
                          <input type=\"checkbox\" id=\"showPassword\" onclick=\"togglePassword()\">\
                        </div>\
                        <input type=\"submit\" value=\"Login\">\
                      </form>\
                    </body>\
                  </html>";
    server.send(200, "text/html", html);
    EEPROM.end();
  }
}


void handleLogin() {
  if (server.method() == HTTP_GET) {
    // Redirect from handleRoot to here if email and password are stored
    EEPROM.begin(EEPROM_SIZE);
    user_email = EEPROM.readString(EEPROM_EMAIL_ADDR);
    user_password = EEPROM.readString(EEPROM_PASSWORD_ADDR);
    EEPROM.end();
  } else {
    // Handle POST request to log in
    user_email = server.arg("email");
    user_password = server.arg("password");

    // Save email and password to EEPROM
    EEPROM.begin(EEPROM_SIZE);
    EEPROM.writeString(EEPROM_EMAIL_ADDR, user_email);
    EEPROM.writeString(EEPROM_PASSWORD_ADDR, user_password);
    EEPROM.commit();
    EEPROM.end();
  }

  // Display the login success page
  String html = "<html>\
                  <head>\
                    <style>\
                      body {\
                        display: flex;\
                        justify-content: center;\
                        align-items: center;\
                        height: 100vh;\
                        margin: 0;\
                        font-family: Arial, sans-serif;\
                        background-color: #f7f7f7;\
                      }\
                      .container {\
                        text-align: center;\
                        background-color: #fff;\
                        padding: 20px;\
                        border-radius: 10px;\
                        box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);\
                      }\
                      button {\
                        font-size: 1.2em;\
                        padding: 10px 20px;\
                        color: #fff;\
                        background-color: #007bff;\
                        border: none;\
                        border-radius: 5px;\
                        cursor: pointer;\
                        transition: background-color 0.3s;\
                        margin: 5px;\
                      }\
                      button:hover {\
                        background-color: #0056b3;\
                      }\
                    </style>\
                  </head>\
                  <body>\
                    <div class=\"container\">\
                      Login credentials received, connecting to Firebase...<br>\
                      <button onclick=\"window.location.href='/changeAccount'\">Ganti Akun</button>\
                    </div>\
                  </body>\
                </html>";
  server.send(200, "text/html", html);

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  auth.user.email = user_email.c_str();
  auth.user.password = user_password.c_str();
  if (Firebase.signUp(&config, &auth, user_email.c_str(), user_password.c_str())) {
    Serial.println("Sign up succeeded");
  } else {
    Serial.printf("Sign up failed: %s\n", config.signer.signupError.message.c_str());
  }
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  if (Firebase.ready()) {
    if (auth.token.uid.length() > 0) {
      Serial.println("Sign in succeeded");
      Serial.print("User UID: ");
      Serial.println(auth.token.uid.c_str());
      id = auth.token.uid.c_str();
      for (int i = 0; i < 2; i++) {
        digitalWrite(ind, LOW);
        vTaskDelay(pdMS_TO_TICKS(200));
        digitalWrite(ind, HIGH);
        vTaskDelay(pdMS_TO_TICKS(200));
      }
    } else {
      Serial.println("Failed to authenticate after sign in");
    }
  } else {
    Serial.println("Failed to initialize Firebase");
  }
}


void handleChangeAccount() {
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.writeString(EEPROM_EMAIL_ADDR, "");
  EEPROM.writeString(EEPROM_PASSWORD_ADDR, "");
  EEPROM.commit();
  Serial.println("ID dihapus");
  digitalWrite(buzz, HIGH);
  vTaskDelay(pdMS_TO_TICKS(1000));
  digitalWrite(buzz, LOW);
  Serial.println("berhasil");
  String html = "<html>\
                  <head>\
                    <style>\
                      body {\
                        display: flex;\
                        justify-content: center;\
                        align-items: center;\
                        height: 100vh;\
                        margin: 0;\
                        font-family: Arial, sans-serif;\
                        background-color: #f7f7f7;\
                      }\
                      .container {\
                        text-align: center;\
                        background-color: #fff;\
                        padding: 20px;\
                        border-radius: 10px;\
                        box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);\
                      }\
                      button {\
                        font-size: 1.2em;\
                        padding: 10px 20px;\
                        color: #fff;\
                        background-color: #007bff;\
                        border: none;\
                        border-radius: 5px;\
                        cursor: pointer;\
                        transition: background-color 0.3s;\
                        margin: 5px;\
                      }\
                      button:hover {\
                        background-color: #0056b3;\
                      }\
                    </style>\
                  </head>\
                  <body>\
                    <div class=\"container\">\
                      Akun dihapus. Silakan login kembali.<br>\
                      <button onclick=\"window.location.href='/'\">Kembali ke Login</button>\
                    </div>\
                  </body>\
                </html>";
  server.send(200, "text/html", html);
}

void setupFirebase() {
  EEPROM.begin(EEPROM_SIZE);
  user_email = EEPROM.readString(EEPROM_EMAIL_ADDR);
  user_password = EEPROM.readString(EEPROM_PASSWORD_ADDR);

  if (user_email.length() > 0 && user_password.length() > 0) {
    config.api_key = API_KEY;
    config.database_url = DATABASE_URL;
    auth.user.email = user_email.c_str();
    auth.user.password = user_password.c_str();

    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);
    if (Firebase.ready()) {
      if (auth.token.uid.length() > 0) {
        Serial.println("Sign in succeeded");
        Serial.print("User UID: ");
        Serial.println(auth.token.uid.c_str());
        id = auth.token.uid.c_str();
        for (int i = 0; i < 2; i++) {
          digitalWrite(ind, LOW);
          vTaskDelay(pdMS_TO_TICKS(200));
          digitalWrite(ind, HIGH);
          vTaskDelay(pdMS_TO_TICKS(200));
        }
      } else {
        Serial.println("Failed to authenticate after sign in");
      }
    } else {
      Serial.println("Failed to initialize Firebase");
    }
    server.on("/", handleRoot);
    server.on("/login", HTTP_GET, handleLogin);
    server.on("/login", HTTP_POST, handleLogin);
    server.on("/changeAccount", HTTP_GET, handleChangeAccount);
    server.begin();
    Serial.println("HTTP server started");
  } else {
    // Start web server for user to input credentials if no email/password in EEPROM
    server.on("/", handleRoot);
    server.on("/login", HTTP_GET, handleLogin);
    server.on("/login", HTTP_POST, handleLogin);
    server.on("/changeAccount", HTTP_GET, handleChangeAccount);
    server.begin();
    Serial.println("HTTP server started");
  }
}

// tambahkan peer address
bool addPeer(const uint8_t * mac_addr) {
  esp_now_peer_info_t peer;
  memset(&peer, 0, sizeof(esp_now_peer_info_t));
  peer.channel = chan;
  peer.encrypt = false;
  memcpy(peer.peer_addr, mac_addr, 6); // Gunakan 6 untuk jelas menyatakan ukuran alamat MAC
  if (esp_now_is_peer_exist(peer.peer_addr)) {
    Serial.println("Already Paired");
    return true;
  } else {
    // Hanya tambahkan peer jika belum ada
    esp_err_t addStatus = esp_now_add_peer(&peer); // Gunakan peer yang sudah dikonfigurasi, bukan hanya alamat MAC
    if (addStatus == ESP_OK) {
      Serial.println("Pair success");
      return true;
    } else {
      Serial.println("Pair failed");
      return false;
    }
  }
}

void printMAC(const uint8_t * mac_addr) {
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.print(macStr);
}

// callback espNOW
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("Last Packet Send Status: ");
  Serial.print(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success to " : "Delivery Fail to ");
  printMAC(mac_addr);
  Serial.println();
}

void OnDataRecv(const uint8_t * mac_addr, const uint8_t *incomingData, int len) {
  Serial.print(len);
  Serial.print(" bytes of data received from : ");
  printMAC(mac_addr);
  Serial.println();
  uint8_t type = incomingData[0];       // first message byte is the type of message
  switch (type) {
  case DATA :                           // the message is data type
    memcpy(&inData, incomingData, sizeof(inData));
    Serial.print("ID: ");
    Serial.println(inData.id);
    if(strcmp(inData.id, "-1") == 0){
      Serial.println("kirim Data lokasi");
      for(int i = 0;i < MAX_NODES; i++){
        if(nodeKapal[i].id != 0){
          nodeKapal[i].msgType = DATA;
          esp_now_send(pairingData.macAddr, (uint8_t *) &nodeKapal[i], sizeof(data_kapal));
          Serial.print("id = ");
          Serial.println(i);
        }
        Serial.println("kosong");
      }
    }else{
      sscanf(inData.id, "%d.%d", &bedengan, &node);
      xQueueSend(dataQueue, (void *)&inData, portMAX_DELAY);
      if (bedengan < MAX_NODES && node < MAX_NODES) {
          memcpy(&nodeData[bedengan][node], &inData, sizeof(struct_message));
      }
      esp_now_send(pairingData.macAddr, (uint8_t *) &inData, sizeof(pairingData));
    }
    break;

  case PAIRING:                            // the message is a pairing request
    memcpy(&pairingData, incomingData, sizeof(pairingData));
    sscanf(pairingData.id, "%d.%d", &bedengan, &node);
    Serial.println(pairingData.msgType);
    Serial.println(pairingData.id);
    Serial.print("Pairing request from: ");
    printMAC(pairingData.macAddr);
    pairingData.channel = chan;
    Serial.println(bedengan);
    Serial.println(node);
    Serial.println();
    Serial.println(pairingData.channel);
    addPeer(pairingData.macAddr);
    if (strcmp(pairingData.id, "-1") == 0) {
      digitalWrite(buzz, HIGH);
      vTaskDelay(pdMS_TO_TICKS(500));
      digitalWrite(buzz, LOW);
      Serial.println("kirim data ke kapal");
      xQueueSend(antriKirim, &pairingData, portMAX_DELAY);
    }else if (strcmp(pairingData.id, "0.0") != 0) {     // do not replay to server itself
      if ((strcmp(pairingMatrix[bedengan][node].id, "0") == 0 || pairingMatrix[bedengan][node].id[0] == '\0') || memcmp(pairingMatrix[bedengan][node].macAddr, pairingData.macAddr, 6) == 0) {
        strcpy(pairingData.id, "0");       // 0 is server
        // Server is in AP_STA mode: peers need to send data to server soft AP MAC address
        Serial.println("send response");
        digitalWrite(ind, LOW);
        vTaskDelay(500);
        digitalWrite(ind, HIGH);
        esp_now_send(pairingData.macAddr, (uint8_t *) &pairingData, sizeof(pairingData));
        memcpy(&pairingMatrix[bedengan][node], &pairingData, sizeof(struct_pairing));
      }else{
        Serial.print("Node ID: ");
        Serial.println(nodeData[bedengan][node].id);
        strcpy(pairingData.id, "-1");
        Serial.println("send salah");
        esp_now_send(pairingData.macAddr, (uint8_t *) &pairingData, sizeof(pairingData));
      }
    }
    break;
  }
}

unsigned long getTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return 0;
  }
  return mktime(&timeinfo); // Convert struct tm to time_t (timestamp)
}

String getFormattedDate() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return "";
  }

  char dateBuffer[11];
  strftime(dateBuffer, sizeof(dateBuffer), "%Y-%m-%d", &timeinfo);
  return String(dateBuffer);
}

String getFormattedTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return "";
  }

  char timeBuffer[6];
  strftime(timeBuffer, sizeof(timeBuffer), "%H:%M", &timeinfo);
  return String(timeBuffer);
}

bool isIt6PM() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return false;
  }
  // Cek apakah sekarang antara jam 18:00 dan 18:59
  if (timeinfo.tm_hour == 18 && timeinfo.tm_min >= 0 && timeinfo.tm_min < 60) {
    return true;
  }
  return false;
}

void storeDailyData() {
  dailyHumidity[entryIndex] = tHum;
  dailyTemperature[entryIndex] = tTemp;
  dailypH[entryIndex] = tpH;
  dailyN[entryIndex] = tN;
  dailyP[entryIndex] = tP;
  dailyK[entryIndex] = tK;
  entryIndex = (entryIndex + 1) % MAX_ENTRIES; // Increment and wrap around
}


void hitungRataRataKeseluruhan() {
  int totalHumidity = 0;
  float totalTemperature = 0;
  float totalpH = 0;
  int totalN = 0;
  int totalP = 0;
  int totalK = 0;
  int validNodes = 0;

  for (int bedengan = 1; bedengan < MAX_NODES; bedengan++) {
    if (strcmp(nodeData[bedengan][0].id, "") != 0) {
      totalHumidity += nodeData[bedengan][0].humidity;
      totalTemperature += nodeData[bedengan][0].temperature;
      totalpH += nodeData[bedengan][0].pH;
      totalN += nodeData[bedengan][0].N;
      totalP += nodeData[bedengan][0].P;
      totalK += nodeData[bedengan][0].K;
      validNodes++;
    }
  }

  if (validNodes > 0) {
    // Hitung rata-rata keseluruhan
    int avgHumidity = totalHumidity / validNodes;
    float avgTemperature = totalTemperature / validNodes;
    float avgpH = totalpH / validNodes;
    int avgN = totalN / validNodes;
    int avgP = totalP / validNodes;
    int avgK = totalK / validNodes;

    // masukkan data ke total
    tHum = avgHumidity;
    tTemp = avgTemperature;
    tpH = avgpH;
    tN = avgN;
    tP = avgP;
    tK = avgK;

    // Opsi: Cetak rata-rata keseluruhan ke Serial
    Serial.println("Rata-rata Keseluruhan:");
    Serial.print("Kelembaban = ");
    Serial.print(avgHumidity);
    Serial.print(", Suhu = ");
    Serial.print(avgTemperature, 2);
    Serial.print(", pH = ");
    Serial.println(avgpH, 2);

    sendAverageDataToFirebase();
  } else {
    Serial.println("Tidak ada data valid untuk menghitung rata-rata keseluruhan");
  }
}


//fungsi mengirim data rata2 /hari
void sendDailyAverageToFirebase() {
  if (Firebase.ready()) {
    if (entryIndex > 0) {
      int sumHumidity = 0;
      float sumTemperature = 0;
      float sumpH = 0;
      int sumN = 0;
      int sumP = 0;
      int sumK = 0;

      for (int i = 0; i < entryIndex; i++) {
        sumHumidity += dailyHumidity[i];
        sumTemperature += dailyTemperature[i];
        sumpH += dailypH[i];
        sumN += dailyN[i];
        sumP += dailyP[i];
        sumK += dailyK[i];
      }

      int avgHumidity = sumHumidity / entryIndex;
      float avgTemperature = sumTemperature / entryIndex;
      float avgpH = sumpH / entryIndex;
      int avgN = sumN / entryIndex;
      int avgP = sumP / entryIndex;
      int avgK = sumK / entryIndex;

      FirebaseJson dailyJson;
      dailyJson.set("kelembapan", avgHumidity);
      dailyJson.set("suhu", avgTemperature);
      dailyJson.set("ph", avgpH);
      dailyJson.set("nitrogen", avgN);
      dailyJson.set("phosphor", avgP);
      dailyJson.set("kalium", avgK);

      String datePath = "/" + id + "/DataRecord/" + getFormattedDate();
      if (Firebase.RTDB.setJSON(&fbdo, datePath, &dailyJson)) {
        Serial.println("Daily average data successfully sent to Firebase");
        for (int i = 0; i < 2; i++) {
          digitalWrite(ind, LOW);
          vTaskDelay(pdMS_TO_TICKS(200));
          digitalWrite(ind, HIGH);
          vTaskDelay(pdMS_TO_TICKS(200));
        }
      } else {
        Serial.println("Failed to send daily average data to Firebase: " + fbdo.errorReason());
      }

      // Kosongkan array setelah pengiriman data
      memset(dailyHumidity, 0, sizeof(dailyHumidity));
      memset(dailyTemperature, 0, sizeof(dailyTemperature));
      memset(dailypH, 0, sizeof(dailypH));
      memset(dailyN, 0, sizeof(dailyN));
      memset(dailyP, 0, sizeof(dailyP));
      memset(dailyK, 0, sizeof(dailyK));
      entryIndex = 0; // Reset indeks
    } else {
      Serial.println("Tidak ada data harian untuk dikirim ke Firebase");
    }
  }
}

// fungsi untuk mengirim data rata2 perjam
void sendAverageDataToFirebase() {
  FirebaseJson avgJson;
  avgJson.set("kelembapan", tHum);
  avgJson.set("suhu", tTemp);
  avgJson.set("ph", tpH);
  avgJson.set("nitrogen", tN);
  avgJson.set("phosphor", tP);
  avgJson.set("kalium", tK);

  String path = "/" + id + "/Average";
  if (Firebase.RTDB.setJSON(&fbdo, path, &avgJson)) {
    Serial.println("Average data successfully sent to Firebase");
  } else {
    Serial.println("Failed to send average data to Firebase: " + fbdo.errorReason());
  }
}

// Fungsi untuk memeriksa dan menghapus node jika ada
bool deleteFirebasePath(const String& path) {
  if (Firebase.RTDB.pathExisted(&fbdo, path)) {
    if (Firebase.RTDB.deleteNode(&fbdo, path)) {
      Serial.println("Path deleted successfully.");
      digitalWrite(buzz, HIGH);
      vTaskDelay(pdMS_TO_TICKS(500));
      digitalWrite(buzz, LOW);
      return true;
    }
  }
  return false;
}

// Fungsi utama yang menggunakan fungsi helper
void deletePath() {
  if (Firebase.ready()) {
    Serial.println("masuk");
    for (int i = 1; i < 5; i++) {
      Serial.printf("i = %d\n", i);
      for (int j = 1; j < MAX_NODES; j++) {
        String pathID1 = "/" + String(id) + "/Data/bedengan" + String(i) + "/node" + String(i);
        String pathID2 = pathID1 + "_" + String(j);
        deleteFirebasePath(pathID2);
        deleteFirebasePath(pathID1);
      }
    }
  }
}

void hitungAverage(int bedengan) {
  Serial.print("Calculating average for bedengan: ");
  Serial.println(bedengan);

  int totalHumidity = 0;
  float totalTemperature = 0;
  float totalpH = 0;
  int totalN = 0;
  int totalP = 0;
  int totalK = 0;
  int validNodes = 0;

  // Iterasi melalui semua node di bedengan
  for (int node = 1; node < MAX_NODES; node++) { // Mulai dari 1 karena 0 akan digunakan untuk menyimpan rata-rata
    if (strcmp(nodeData[bedengan][node].id, "000000") != 0 && strcmp(nodeData[bedengan][node].id, "") != 0) {
      totalHumidity += nodeData[bedengan][node].humidity;
      totalTemperature += nodeData[bedengan][node].temperature;
      totalpH += nodeData[bedengan][node].pH;
      totalN += nodeData[bedengan][node].N;
      totalP += nodeData[bedengan][node].P;
      totalK += nodeData[bedengan][node].K;
      validNodes++;
    }
  }

  if (validNodes > 0) {
    // Hitung rata-rata
    int avgHumidity = totalHumidity / validNodes;
    float avgTemperature = totalTemperature / validNodes;
    float avgpH = totalpH / validNodes;
    int avgN = totalN / validNodes;
    int avgP = totalP / validNodes;
    int avgK = totalK / validNodes;

    // Menyiapkan ID unik berdasarkan 'bedengan'
    char uniqueID[6]; // Sesuaikan ukuran array sesuai dengan panjang maksimum ID Anda
    snprintf(uniqueID, sizeof(uniqueID), "%d", bedengan); // Konversi int ke string

    // Simpan rata-rata ke nodeData[bedengan][0]
    strcpy(nodeData[bedengan][0].id, uniqueID);
    nodeData[bedengan][0].humidity = avgHumidity;
    nodeData[bedengan][0].temperature = avgTemperature;
    nodeData[bedengan][0].pH = avgpH;
    nodeData[bedengan][0].N = avgN;
    nodeData[bedengan][0].P = avgP;
    nodeData[bedengan][0].K = avgK;
    nodeData[bedengan][0].baterai = 0; // Atau set nilai yang menunjukkan data ini valid
    nodeKapal[bedengan].id = bedengan;
    nodeKapal[bedengan].humidity =  avgHumidity;
    nodeKapal[bedengan].temperature = avgTemperature;
    // Serial.print("Bedengan ");
    // Serial.print(bedengan);
    // Serial.print(": Rata-rata Kelembaban = ");
    // Serial.print(avgHumidity);
    // Serial.print(", Rata-rata Suhu = ");
    // Serial.println(avgTemperature, 2);
    updateLocData(bedengan);
    displayAllNodeData();
    sendNodeDataToFirebase(bedengan);
  }
}

void sendNodeDataToFirebase(int bedengan) {
  if (Firebase.ready()) {
    Serial.print("Sending data to Firebase for bedengan: ");
    Serial.println(bedengan);

    // Kirim data rata-rata bedengan ke Firebase
    FirebaseJson avgJson;
    createBedenganJson(avgJson, nodeData[bedengan][0]);
    String avgPath = "/" + id + "/Data/bedengan" + String(bedengan);
    sendJsonToFirebase(avgPath, avgJson);

    // Kirim data setiap node dalam bedengan ke Firebase
    for (int node = 1; node < MAX_NODES; node++) {
      if (strcmp(nodeData[bedengan][node].id, "000000") != 0 && strcmp(nodeData[bedengan][node].id, "") != 0) {
        String nodeIdStr = String(nodeData[bedengan][node].id);
        nodeIdStr.replace(".", "_");
        timestamp = getTime();
        Serial.print("Timestamp: ");
        Serial.println(timestamp);
        formattedDate = getFormattedDate();
        formattedTime = getFormattedTime();
        Serial.print("Formatted Date: ");
        Serial.println(formattedDate);
        Serial.print("Formatted Time: ");
        Serial.println(formattedTime);
        String path = "/" + id + "/Data/bedengan" + String(bedengan) + "/node" + nodeIdStr;
        FirebaseJson nodeJson;
        createNodeJson(nodeJson, nodeData[bedengan][node]);
        sendJsonToFirebase(path, nodeJson);
        delay(100); // Tambahkan delay untuk memberikan waktu pengiriman
      }
    }
  }
}

void createBedenganJson(FirebaseJson& nodeJson, const struct_message& node) {
  nodeJson.clear();
  nodeJson.set("id", node.id);
  nodeJson.set("kelembapan", node.humidity);
  nodeJson.set("suhu", node.temperature);
  nodeJson.set("ph", node.pH);
  nodeJson.set("N", node.N);
  nodeJson.set("P", node.P);
  nodeJson.set("K", node.K);
  nodeJson.set("latitude", node.Latitude);
  nodeJson.set("longitude", node.Longitude);
  nodeJson.set("baterai", node.baterai);
  nodeJson.set("timestamp", timestamp);
  nodeJson.set("date", formattedDate);
  nodeJson.set("time", formattedTime);
}

void createNodeJson(FirebaseJson& nodeJson, const struct_message& node) {
  nodeJson.clear();
  nodeJson.set("id", node.id);
  nodeJson.set("kelembapan", node.humidity);
  nodeJson.set("suhu", node.temperature);
  nodeJson.set("ph", node.pH);
  nodeJson.set("N", node.N);
  nodeJson.set("P", node.P);
  nodeJson.set("K", node.K);
  nodeJson.set("baterai", node.baterai);
  nodeJson.set("timestamp", timestamp);
  nodeJson.set("date", formattedDate);
  nodeJson.set("time", formattedTime);
}

void sendJsonToFirebase(const String& path, FirebaseJson& nodeJson) {
  Serial.print("Sending JSON to Firebase: ");
  Serial.println(path);

  if (Firebase.RTDB.setJSON(&fbdo, path, &nodeJson)) {
    Serial.println("Data berhasil dikirim ke Firebase: " + path);
    for (int i = 0; i < 2; i++) {
      digitalWrite(ind, LOW);
      vTaskDelay(pdMS_TO_TICKS(200));
      digitalWrite(ind, HIGH);
      vTaskDelay(pdMS_TO_TICKS(200));
    }
  } else {
    Serial.println("Gagal mengirim data ke Firebase: " + fbdo.errorReason());
  }
}

//mengupdate data lokasi untuk pengiriman data lanjutan
void updateLocData(int bedengan){
  String bedenganPath = "/" + String(id) + "/Data/bedengan" + String(bedengan);
  if (Firebase.RTDB.pathExisted(&fbdo, bedenganPath)){
    String latitudePath = bedenganPath + "/latitude";
    String longitudePath = bedenganPath + "/longitude";
    if (Firebase.RTDB.getDouble(&fbdo, latitudePath)) {
        nodeKapal[bedengan].Latitude = fbdo.doubleData();
        nodeData[bedengan][0].Latitude = fbdo.doubleData();
        Serial.print("Bedengan ");
        Serial.print(bedengan);
        Serial.print(" Latitude: ");
        Serial.println(nodeKapal[bedengan].Latitude, 6);
    }
    if (Firebase.RTDB.getDouble(&fbdo, longitudePath)) {
        nodeKapal[bedengan].Longitude = fbdo.doubleData();
        nodeData[bedengan][0].Longitude = fbdo.doubleData();
        Serial.print("Bedengan ");
        Serial.print(bedengan);
        Serial.print(" Longitude: ");
        Serial.println(nodeKapal[bedengan].Longitude, 6);
    }
  }
}

void retrieveLocationData() {
  for (int i = 1; i <= MAX_NODES; i++) {
    Serial.print("Memeriksa data ke: ");
    Serial.println(i);
    String bedenganPath = "/" + String(id) + "/Data/bedengan" + String(i);
    if (Firebase.RTDB.pathExisted(&fbdo, bedenganPath)){
      String latitudePath = bedenganPath + "/latitude";
      String longitudePath = bedenganPath + "/longitude";

      bool isLatitudeAvailable = Firebase.RTDB.getDouble(&fbdo, latitudePath);
      bool isLongitudeAvailable = Firebase.RTDB.getDouble(&fbdo, longitudePath);
      if (Firebase.RTDB.getDouble(&fbdo, latitudePath)) {
          nodeKapal[i].Latitude = fbdo.doubleData();
          nodeData[i][0].Latitude = fbdo.doubleData();
          Serial.print("Bedengan ");
          Serial.print(i);
          Serial.print(" Latitude: ");
          Serial.println(nodeKapal[i].Latitude, 6);
      }else {
        Serial.print("Bedengan ");
        Serial.print(i);
        Serial.println(" Latitude not available.");
      }
      if (Firebase.RTDB.getDouble(&fbdo, longitudePath)) {
          nodeKapal[i].Longitude = fbdo.doubleData();
          nodeData[i][0].Longitude = fbdo.doubleData();
          Serial.print("Bedengan ");
          Serial.print(i);
          Serial.print(" Longitude: ");
          Serial.println(nodeKapal[i].Longitude, 6);
      }else {
        Serial.print("Bedengan ");
        Serial.print(i);
        Serial.println(" Longitude not available.");
      }
    }else {
      Serial.println("Path does not exist: " + bedenganPath);
    }
    vTaskDelay(pdMS_TO_TICKS(10)); // Memberi cukup waktu untuk task lain dan mengurangi beban CPU
  }
}

void displayAllNodeData() {
  Serial.println("Menampilkan data dari semua node:");
  for (int bedengan = 0; bedengan < MAX_NODES; bedengan++) {
    for (int node = 0; node < MAX_NODES; node++) {
      if (strcmp(nodeData[bedengan][node].id, "NODATA") != 0 && strcmp(nodeData[bedengan][node].id, "") != 0) {
        Serial.print("Node ID: ");
        Serial.println(nodeData[bedengan][node].id);
        Serial.print("Kelembapan: ");
        Serial.println(nodeData[bedengan][node].humidity);
        Serial.print("Suhu: ");
        Serial.println(nodeData[bedengan][node].temperature, 2);
        Serial.print("pH: ");
        Serial.println(nodeData[bedengan][node].pH, 2);
        Serial.print("N: ");
        Serial.println(nodeData[bedengan][node].N);
        Serial.print("P: ");
        Serial.println(nodeData[bedengan][node].P);
        Serial.print("baterai: ");
        Serial.println(nodeData[bedengan][node].baterai);
      }
    }
  }
}

bool isDataAvailable() {
  for (int bedengan = 0; bedengan < MAX_NODES; bedengan++) {
    for (int node = 0; node < MAX_NODES; node++) {
      if (strcmp(nodeData[bedengan][node].id, "000000") != 0 && strcmp(nodeData[bedengan][node].id, "") != 0) {
        return true;
      }
    }
  }
  return false;
}

void disableWDT() {
  // Disable Core 0 WDT
  esp_task_wdt_deinit();

  // Disable Core 1 WDT
  esp_task_wdt_deinit();
}

void setup() {
  Serial.begin(115200);
  pinMode(ind, OUTPUT);
  pinMode(buzz, OUTPUT);

  // Set memori matrix
  memset(pairingMatrix, 0, sizeof(pairingMatrix));
  memset(nodeData, 0, sizeof(nodeData));
  memset(nodeKapal, 0, sizeof(nodeKapal));
  
  // Matikan Watchdog Timer
  disableWDT();

  Serial.print("Server MAC Address: ");
  Serial.println(WiFi.macAddress());

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ssid, password);
  Serial.println("Access Point Setup Done.");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  // Konfigurasi Firebase
  setupFirebase();

  // Aktifkan ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_wifi_config_espnow_rate(WIFI_IF_STA, WIFI_PHY_RATE_LORA_500K);
  esp_wifi_config_espnow_rate(WIFI_IF_AP, WIFI_PHY_RATE_LORA_500K);
  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);

  antriKirim = xQueueCreate(10, sizeof(struct_pairing));
  dataQueue = xQueueCreate(10, sizeof(struct_message));
  kapalQueue = xQueueCreate(10, sizeof(struct_message));
  if (dataQueue == NULL) {
    Serial.println("Failed to create the queue");
  }

  // Tes data
  // for (int i = 1; i <= 4; ++i) {
  //   nodeKapal[i].id = i;
  //   Serial.println(nodeKapal[i].id);
  //   nodeKapal[i].humidity = random(1, 100);
  //   nodeKapal[i].temperature = random(25, 40);
  // }
  Serial.println("done");
  digitalWrite(ind, LOW);

  BaseType_t xReturned;
  Serial.printf("Free heap size before creating tasks: %d bytes\n", xPortGetFreeHeapSize());
  xReturned = xTaskCreate(processDataTask, "ProcessData", 30000, NULL, 1, NULL);
  if (xReturned == pdPASS) {
    Serial.println("Task processDataTask berhasil dibuat");
  } else {
    Serial.println("Task processDataTask gagal dibuat");
  }

  xReturned = xTaskCreate(dataProcessingTask, "DataProcTask", 30000, NULL, 1, NULL);
  if (xReturned == pdPASS) {
    Serial.println("Task dataProcessingTask berhasil dibuat");
  } else {
    Serial.println("Task dataProcessingTask gagal dibuat");
  }

  xReturned = xTaskCreate(avgData, "sendDaily", 50000, NULL, 1, NULL);
  if (xReturned == pdPASS) {
    Serial.println("Task avgData berhasil dibuat");
  } else {
    Serial.println("Task avgData gagal dibuat");
  }

  xReturned = xTaskCreate(sensorUltra, "sensor ultrasonik", 5000, NULL, 3, NULL);
  if (xReturned == pdPASS) {
    Serial.println("Task sensorUltra berhasil dibuat");
  } else {
    Serial.println("Task sensorUltra gagal dibuat");
  }
  
  vTaskDelay(pdMS_TO_TICKS(500));
  retrieveLocationData();
  digitalWrite(ind, HIGH);
}

void loop() {
  server.handleClient();
}

void sensorUltra(void *parameter){
  for(;;){
    int pipa = 75;
    int y = SRF.getCentimeter();
    int kedalamanAir = pipa-y;
    SRF.setSpeedOfSound(340);
    Serial.printf("Jarak Sensor ke permukaan: %d\r\n", y);
    Serial.printf("kedalaman air: %d\r\n", kedalamanAir);
    if (kedalamanAir < 30){
      n++;
    }
    if(n < 40 && kedalamanAir < 30){
      digitalWrite(buzz, HIGH);
      vTaskDelay(pdMS_TO_TICKS(500));
      digitalWrite(buzz, LOW);
    }else if(n == 100){
      n = 0;
    }
    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}

void avgData(void *parameter) {
  while (true) {
    if (isDataAvailable()) {
      if (isIt6PM()) {
        // Kirim rata-rata harian ke Firebase
        sendDailyAverageToFirebase();
        // Tunggu selama 1 jam sebelum pengecekan berikutnya
        vTaskDelay(pdMS_TO_TICKS(3600000)); // Delay 1 jam
      } else {
        // Hitung rata-rata bedengan dan simpan ke array
        hitungRataRataKeseluruhan();
        storeDailyData();
        // Tunggu selama 30 menit sebelum pengecekan berikutnya
        vTaskDelay(pdMS_TO_TICKS(600000)); // Delay 30 menit
        // vTaskDelay(pdMS_TO_TICKS(1800000)); // Delay 30 menit
      }
    } else {
      // Jika belum ada data, cek lagi dalam 10 menit
      Serial.printf("belum ada data\r\n");
      vTaskDelay(pdMS_TO_TICKS(60000)); // Delay 10 menit
    }
  }
}

void processDataTask(void *parameter) {
  struct_message dataToProcess;
  while (true) {
    if (xQueueReceive(dataQueue, &dataToProcess, portMAX_DELAY) == pdTRUE) {
      // Serial.println("Received data from queue");
      if (bedengan < MAX_NODES && node < MAX_NODES) {
        // Serial.print("Processing data for bedengan: ");
        // Serial.println(bedengan);
        hitungAverage(bedengan);
        // displayAllNodeData();
      }
    }
    vTaskDelay(pdMS_TO_TICKS(200));
  }
}

void dataProcessingTask(void *params) {
  while (true) {
    struct_pairing data;
    if (xQueueReceive(antriKirim, &data, portMAX_DELAY)) {
      Serial.println("Mengambil data lokasi");
      retrieveLocationData();
      strcpy(pairingData.id, "0");
      Serial.println("send response");
      esp_now_send(pairingData.macAddr, (uint8_t *) &pairingData, sizeof(pairingData));
    }
    vTaskDelay(pdMS_TO_TICKS(200));
  }
}

