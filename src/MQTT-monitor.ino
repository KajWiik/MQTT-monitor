/**************************************************************
 *
 * For this example, you need to install PubSubClient library:
 *   https://github.com/knolleary/pubsubclient/releases/latest
 *   or from http://librarymanager/all#PubSubClient
 *
 * TinyGSM Getting Started guide:
 *   http://tiny.cc/tiny-gsm-readme
 *
 **************************************************************
 * Use Mosquitto client tools to work with MQTT
 *   Ubuntu/Linux: sudo apt-get install mosquitto-clients
 *   Windows:      https://mosquitto.org/download/
 *
 * Subscribe for messages:
 *   mosquitto_sub -h test.mosquitto.org -t GsmClientTest/init -t GsmClientTest/ledStatus -q 1
 * Toggle led:
 *   mosquitto_pub -h test.mosquitto.org -t GsmClientTest/led -q 1 -m "toggle"
 *
 * You can use Node-RED for wiring together MQTT-enabled devices
 *   https://nodered.org/
 * Also, take a look at these additional Node-RED modules:
 *   node-red-contrib-blynk-websockets
 *   node-red-dashboard
 *
 **************************************************************/

// Select your modem:
// #define TINY_GSM_MODEM_SIM800
// #define TINY_GSM_MODEM_SIM900
// #define TINY_GSM_MODEM_A6
// #define TINY_GSM_MODEM_A7
#define TINY_GSM_MODEM_M590
// #define TINY_GSM_MODEM_ESP8266

#include <TinyGsmClient.h>
#include <PubSubClient.h>
//#include <OneWire.h>
#include <SimpleDHT.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>

// Edit BME280_ADDRESS (0x76)
#include <Adafruit_BME280.h>

// Your GPRS credentials
// Leave empty, if missing user or pass
const char apn[]  = "internet";
const char user[] = "";
const char pass[] = "";
const char mqtt_user[] = "valpuri";
const char mqtt_pass[] = "ammu1001";

// Use Hardware Serial on Mega, Leonardo, Micro
//#define SerialAT Serial1

// or Software Serial on Uno, Nano
#include <SoftwareSerial.h>
SoftwareSerial SerialAT(2, 3); // RX, TX

TinyGsm modem(SerialAT);
TinyGsmClient client(modem);
PubSubClient mqtt(client);
SimpleDHT22 dht22;
Adafruit_BME280 bme;

const char* broker = "kw.dy.fi";

//const String topicLammitin = "Karttula/Lammitin/";
//const String topicLammitinStatus = "Karttula/LammitinStatus/";
const char* topicHeater = "Karttula/Lammitin/";
const char* topicHeaterStatus = "Karttula/LammitinStatus/";
//const String topicInit = "Karttula/init/";
const char* topicInit = "Karttula/init/";
const String topicOlohuone = "Karttula/Olohuone/";
const String topicMakuuhuone = "Karttula/Makuuhuone/";
const String topicVierashuone = "Karttula/Vierashuone/";
const String topicWC = "Karttula/WC/";
//const String topicWClattia = "Karttula/WClattia/";

//#define Olohuone 10
#define Makuuhuone 4
#define WC 5
#define Vierashuone 6

#define HEATER_PIN 13
int HeaterStatus = LOW;

long lastReconnectAttempt = 0;
unsigned long lastrun = millis();
int state = 0;
//#define PERIOD 5*60*1000
#define PERIOD 3000

bool bme_status;
#define WIDTH 6
#define PRECISION 1
#define BUFLEN 36

char charbuffer[BUFLEN];

void setup() {
  pinMode(HEATER_PIN, OUTPUT);

  // Set console baud rate
  Serial.begin(115200);
  delay(10);

  // Set GSM module baud rate
  SerialAT.begin(38400);
  delay(3000);

  if (0) {
    // Restart takes quite some time
    // To skip it, call init() instead of restart()
    Serial.println("Initializing modem...");
    modem.restart();

    // Unlock your SIM card with a PIN
    //modem.simUnlock("1234");

    Serial.print("Waiting for network...");
    if (!modem.waitForNetwork()) {
      Serial.println(" fail");
      while (true);
    }
    Serial.println(" OK");

    Serial.print("Connecting to ");
    Serial.print(apn);
    if (!modem.gprsConnect(apn, user, pass)) {
      Serial.println(" fail");
      while (true);
    }
    Serial.println(" OK");

    // MQTT Broker setup
    mqtt.setServer(broker, 1883);
    mqtt.setCallback(mqttCallback);
  }

  bme_status = bme.begin();
  
}

boolean mqttConnect() {
  Serial.print("Connecting to ");
  Serial.print(broker);
  if (!mqtt.connect("Karttula", mqtt_user, mqtt_pass)) {
    Serial.println(" fail");
    return false;
  }
  Serial.println(" OK");
  mqtt.publish(topicInit, "Karttula started");
  mqtt.subscribe(topicHeater);
  return mqtt.connected();
}

void loop() {

  //  if (mqtt.connected()) {
  if (1) {
    if (millis() > (lastrun + PERIOD)) {
      switch(state) {
      case 0:
	senddht22(Makuuhuone, topicMakuuhuone);
	state++;
	break;

      case 1:
	sendbme(topicOlohuone);
	state++;
	break;

      case 2:
	senddht22(WC, topicWC);
	state++;
	break;

      case 3:
	// Foo
	state = 0;
	lastrun = millis();
      }
    }
    delay(1000);
    //    mqtt.loop();
  } else {
    // Reconnect every 10 seconds
    unsigned long t = millis();
    if (t - lastReconnectAttempt > 10000L) {
      lastReconnectAttempt = t;
      if (mqttConnect()) {
        lastReconnectAttempt = 0;
      }
    }
  }
}

/* { */
/*  lastrun = millis(); */
/*  String pubString = "{"report":{"light": "" + String(lightRead,1) + ""}}"; */
/*  pubString.toCharArray(message_buff, pubString.length()+1); */
/*  //Serial.println(pubString); */
/*  client.publish("io.m2m/arduino/lightsensor", message_buff); */
/* } */
/*     mqtt.loop(); */

void mqttCallback(char* topic, byte* payload, unsigned int len) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.write(payload, len);
  Serial.println();

  // Only proceed if incoming message's topic matches
  if (String(topic) == topicHeater) {
    HeaterStatus = payload[0] == '0' ? LOW : HIGH; 
    digitalWrite(HEATER_PIN, HeaterStatus);
    mqtt.publish(topicHeaterStatus, HeaterStatus ? "1" : "0");
  }
}

void senddht22(int pinDHT22, String topic) {
  float temperature = 0;
  float humidity = 0;
  String message;
  
  int err = SimpleDHTErrSuccess;
  //  Serial.print("In DHT ");
  //Serial.println(topic);
  //Serial.println(pinDHT22);

  if ((err = dht22.read2(pinDHT22, &temperature, &humidity, NULL)) != SimpleDHTErrSuccess) {
    delay(2000);
    Serial.println("DHT init failed");
    return;
  }
  message = topic + "T/" + String(temperature, 1);
  message.toCharArray(charbuffer, BUFLEN);
  Serial.println(charbuffer);
  message = topic + "RH/" + String(humidity, 1);
  message.toCharArray(charbuffer, BUFLEN);
  Serial.println(charbuffer);
}
  
void sendbme(String topic) {
  String message;

  if (bme_status) {
    message = topic + "T/" + String(bme.readTemperature(), 1);
    message.toCharArray(charbuffer, BUFLEN);
    Serial.println(charbuffer);
    message = topic + "RH/" + String(bme.readHumidity(), 1);
    message.toCharArray(charbuffer, BUFLEN);
    Serial.println(charbuffer);
    message = topic + "P/" + String(bme.readPressure()/100, 2);
    message.toCharArray(charbuffer, BUFLEN);
    Serial.println(charbuffer);
  }
}
  

  //  mqtt.publish(topic, xxx);
