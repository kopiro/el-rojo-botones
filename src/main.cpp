#include "Arduino.h"
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <PubSubClient.h>
#include <WiFiManager.h>

char mqtt_server[64] = "";
char mqtt_username[64] = "";
char mqtt_password[64] = "";

#define AVAILABILITY_ONLINE "online"
#define AVAILABILITY_OFFLINE "offline"

char MQTT_TOPIC_AVAILABILITY[80];
char MQTT_TOPIC_CALLBACK[80];
char MQTT_TOPIC_DEBUG[80];

char MQTT_TOPIC_QUIZ[80];

WiFiManager wifiManager;
WiFiClient wifiClient;
PubSubClient mqttClient;

WiFiManagerParameter wifi_param_mqtt_server("mqtt_server", "MQTT server",
                                            mqtt_server, sizeof(mqtt_server));
WiFiManagerParameter wifi_param_mqtt_username("mqtt_user", "MQTT username",
                                              mqtt_username,
                                              sizeof(mqtt_username));
WiFiManagerParameter wifi_param_mqtt_password("mqtt_pass", "MQTT password",
                                              mqtt_password,
                                              sizeof(mqtt_password));

unsigned long mqttLastTime = millis();

void sendMQTTMessage(const char *topic, const char *message, bool retained) {
  Serial.printf("MQTT message - topic: <%s>, message: <%s> -> ", topic,
                message);

  if (mqttClient.publish(topic, message)) {
    Serial.println("sent");
  } else {
    Serial.println("error");
  }
}

void logError(const char *msg) {
  Serial.print("[ERROR] ");
  Serial.println(msg);
  sendMQTTMessage(MQTT_TOPIC_DEBUG, msg, false);
}

void saveConfig() {
  DynamicJsonDocument json(512);
  json["mqtt_server"] = wifi_param_mqtt_server.getValue();
  json["mqtt_username"] = wifi_param_mqtt_username.getValue();
  json["mqtt_password"] = wifi_param_mqtt_password.getValue();

  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println("Failed to open config file for writing");
    return;
  }

  serializeJson(json, configFile);
  configFile.close();

  Serial.printf("Saved JSON: %s", json.as<String>().c_str());
}

void loadConfig() {
  if (!SPIFFS.begin()) {
    Serial.println("Failed to open SPIFFS");
    return;
  }

  if (!SPIFFS.exists("/config.json")) {
    Serial.println("Config file not found");
    return;
  }

  File configFile = SPIFFS.open("/config.json", "r");
  if (!configFile) {
    Serial.println("Failed to open config file");
    return;
  }

  const size_t size = configFile.size();
  std::unique_ptr<char[]> buf(new char[size]);

  configFile.readBytes(buf.get(), size);
  DynamicJsonDocument json(512);

  if (DeserializationError::Ok != deserializeJson(json, buf.get())) {
    Serial.println("Failed to parse config fileDebug");
    return;
  }

  strcpy(mqtt_server, json["mqtt_server"]);
  strcpy(mqtt_username, json["mqtt_username"]);
  strcpy(mqtt_password, json["mqtt_password"]);

  wifi_param_mqtt_server.setValue(mqtt_server, sizeof(mqtt_server));
  wifi_param_mqtt_username.setValue(mqtt_username, sizeof(mqtt_username));
  wifi_param_mqtt_password.setValue(mqtt_password, sizeof(mqtt_password));
}

void setupGeneric() {
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  loadConfig();

  Serial.printf("Board Identifier: %s\n", BOARD_ID);

  snprintf(MQTT_TOPIC_AVAILABILITY, 80, "%s/availability", BOARD_ID);
  snprintf(MQTT_TOPIC_CALLBACK, 80, "%s/callback", BOARD_ID);
  snprintf(MQTT_TOPIC_DEBUG, 80, "%s/debug", BOARD_ID);

  snprintf(MQTT_TOPIC_QUIZ, 80, "%s/quiz", BOARD_ID);
}

void handleRestart(DynamicJsonDocument json) {
  Serial.println("Restarting...");
  ESP.restart();
}

#define WIFI_MAX_TRIES 15

void setupWifi() {
  if (WiFi.status() == WL_NO_SHIELD) {

    Serial.println("WiFi shield not present");
    return;
  }

  WiFi.hostname(BOARD_ID);

  wifiManager.setConfigPortalBlocking(false);
  wifiManager.setDebugOutput(true);
  wifiManager.setSaveParamsCallback(saveConfig);

  wifiManager.addParameter(&wifi_param_mqtt_server);
  wifiManager.addParameter(&wifi_param_mqtt_username);
  wifiManager.addParameter(&wifi_param_mqtt_password);

  if (WiFi.status() == WL_CONNECTED || wifiManager.autoConnect(BOARD_ID)) {
    WiFi.mode(WIFI_STA);
    wifiManager.startWebPortal();
    digitalWrite(LED_BUILTIN, HIGH);
  }
}

void loopWifi() { 
  wifiManager.process(); 
}

void mqttConnect() {
  Serial.printf("Connecting to MQTT server: %s (%s : %s)... ", mqtt_server,
                mqtt_username, mqtt_password);

  if (mqttClient.connect(BOARD_ID, mqtt_username, mqtt_password,
                         MQTT_TOPIC_AVAILABILITY, 1, true,
                         AVAILABILITY_OFFLINE)) {
    Serial.println("connected");
    mqttClient.subscribe(MQTT_TOPIC_CALLBACK);
    sendMQTTMessage(MQTT_TOPIC_AVAILABILITY, AVAILABILITY_ONLINE, true);
  } else {
    Serial.println("failed");
  }
}

void setupMQTT() {
  mqttClient.setClient(wifiClient);

  mqttClient.setServer(mqtt_server, 1883);
  mqttClient.setKeepAlive(10);
  mqttClient.setBufferSize(2048);
  mqttClient.setCallback(mqttCallback);

  mqttConnect();
}

void loopMQTT() {
  if (mqttClient.connected()) {
    mqttClient.loop();
    return;
  }

  if (millis() < mqttLastTime + MQTT_POLLING_TIMEOUT) {
    return;
  }

  Serial.println("Connection to MQTT lost, reconnecting...");
  mqttLastTime = millis();

  mqttConnect();
}

void setupOTA() {
  ArduinoOTA.onStart([]() {});
  ArduinoOTA.onEnd([]() {});
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {});
  ArduinoOTA.onError([](ota_error_t error) {});
  ArduinoOTA.setHostname(BOARD_ID);
  ArduinoOTA.setPassword(BOARD_ID);
  ArduinoOTA.begin();
}

void loopOTA() { 
  ArduinoOTA.handle(); 
}

void setupMDNS() { 
  MDNS.begin(BOARD_ID); 
}

void loopMDNS() { 
  MDNS.update(); 
}

#define LEFT_BUTTON_SWITCH 5
#define RIGHT_BUTTON_SWITCH 4

#define LEFT_BUTTON_LIGHT 14
#define RIGHT_BUTTON_LIGHT 16

unsigned short leftLEDStatus = 0;
unsigned short rightLEDStatus = 0;

unsigned short winner = 0;
bool winnerSent = false;

void quizSwitchLED(int pin, int value) {
  if (pin == LEFT_BUTTON_LIGHT) {
    leftLEDStatus = value;
  } else {
    rightLEDStatus = value;
  }
  digitalWrite(pin, value);
}

void quizReset() {
  quizSwitchLED(LEFT_BUTTON_LIGHT, LOW);
  quizSwitchLED(RIGHT_BUTTON_LIGHT, LOW);
  winner = 0;
  winnerSent = false;
}

void quizAnimation(int delayTime) {
  for (int i = 0; i < 25; i++) {
    digitalWrite(LEFT_BUTTON_LIGHT, i%2 == 0 ? HIGH : LOW);
    digitalWrite(RIGHT_BUTTON_LIGHT, i%2 == 0 ? HIGH : LOW);
    delay(delayTime);
  }
  // Restore initial state
  digitalWrite(LEFT_BUTTON_LIGHT, leftLEDStatus);
  digitalWrite(RIGHT_BUTTON_LIGHT, rightLEDStatus);
}

void setupQuiz() {
  pinMode(LEFT_BUTTON_SWITCH, INPUT);
  pinMode(RIGHT_BUTTON_SWITCH, INPUT);

  pinMode(LEFT_BUTTON_LIGHT, OUTPUT);
  pinMode(RIGHT_BUTTON_LIGHT, OUTPUT);

  quizAnimation(100);
  quizReset();
}

void loopQuiz() {
  if (winnerSent) {
    return;
  }

  int leftSwitchStatus = digitalRead(LEFT_BUTTON_SWITCH);
  int rightSwitchStatus = digitalRead(RIGHT_BUTTON_SWITCH);

  if (leftSwitchStatus == LOW && rightSwitchStatus == LOW) {
    return;
  }

  winnerSent = true;
  
  if (leftSwitchStatus == HIGH) {
    quizSwitchLED(LEFT_BUTTON_LIGHT, HIGH);
    sendMQTTMessage(MQTT_TOPIC_QUIZ, "L", false);
  }
  
  if (rightSwitchStatus == HIGH) {
    quizSwitchLED(RIGHT_BUTTON_LIGHT, HIGH);
    sendMQTTMessage(MQTT_TOPIC_QUIZ, "R", false);
  }
}

void handleQuiz(DynamicJsonDocument json) {
  String method = json["method"].as<String>();

  if (method == "reset") {
    quizReset();
    return;
  }

  if (method == "animation") {
    int delayTime = json["delay"].as<int>();
    if (!delayTime) {
      delayTime = 100;
    }
    quizAnimation(delayTime);
    return;
  }

  logError("INVALID_QUIZ_METHOD");
}

void mqttCallback(char *topic, byte *payload, unsigned int length) {
  char payloadText[length + 1];
  snprintf(payloadText, length + 1, "%s", payload);

  Serial.printf("MQTT callback with topic <%s> and payload <%s>\n", topic,
                payloadText);
  // sendMQTTMessage(MQTT_TOPIC_DEBUG, payloadText, false);

  DynamicJsonDocument commandJson(256);
  DeserializationError err = deserializeJson(commandJson, payloadText);

  if (err) {
    logError("INVALID_JSON");
    return;
  }

  String command = commandJson["command"].as<String>();

  if (command == "availability") {
    sendMQTTMessage(MQTT_TOPIC_AVAILABILITY, AVAILABILITY_ONLINE, true);
    return;
  }

  if (command == "restart") {
    handleRestart(commandJson);
    return;
  }

  if (command == "quiz") {
    handleQuiz(commandJson);
    return;
  }

  Serial.printf("Unknown callback command: %s", command.c_str());
  logError("MQTT_INVALID_COMMAND");
}


void setup() {
  setupGeneric();
  setupQuiz();
  setupWifi();
  setupMDNS();
  setupOTA();
  setupMQTT();
}

void loop() {
  loopQuiz();
  loopWifi();
  loopMDNS();
  loopOTA();
  loopMQTT();
}