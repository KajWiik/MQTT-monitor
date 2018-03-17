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
#define TINY_GSM_MODEM_SIM800

#include <TinyGsmClient.h>
#include <PubSubClient.h>

#include <SimpleDHT.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>

// Edit BME280_ADDRESS (0x76)
#include <Adafruit_BME280.h>

//#include <Adafruit_SleepyDog.h>

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

const char broker[] = "kw.dy.fi";

const char topicHeater[] = "Karttula/Lammitin/";
const char topicHeaterStatus[] = "Karttula/LammitinStatus/";
const char topicInit[] = "Karttula/init/";
const char topicOlohuone[] = "Karttula/Olohuone/";
const char topicMakuuhuone[] = "Karttula/Makuuhuone/";
const char topicVierashuone[] = "Karttula/Vierashuone/";
const char topicWC[] = "Karttula/WC/";

const char tempprefix[] = "T/";
const char humprefix[] = "RH/";
const char pressprefix[] = "P/";

#define Makuuhuone 4
#define WC 5
#define Vierashuone 6

#define HEATER_PIN 7
#define GSM_RESET_PIN 8

int HeaterStatus = LOW;

long lastReconnectAttempt = 0;
unsigned long lastrun = millis();
int state = 0;
//#define PERIOD 5*60*1000
#define PERIOD 30*1000

bool bme_status;

#define TOPICBUFLEN 36
#define VALBUFLEN 10

char topicbuffer[TOPICBUFLEN];
char valbuffer[VALBUFLEN];

void setup() {

  //  int countdownMS = Watchdog.enable(60000);
  
  pinMode(HEATER_PIN, OUTPUT);
  pinMode(GSM_RESET_PIN, OUTPUT);

  // Turn heater off by default
  digitalWrite(HEATER_PIN, LOW);

  // Reset GSM module
  digitalWrite(GSM_RESET_PIN, LOW);
  delay(100);
  digitalWrite(GSM_RESET_PIN, HIGH);
  delay(500);
  digitalWrite(GSM_RESET_PIN, LOW);
  
  // Set console baud rate
  Serial.begin(115200);
  delay(10);

  // Set GSM module baud rate
  SerialAT.begin(9600);
  delay(3000);

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

  //  bme_status = bme.begin();
  //  Watchdog.reset();
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
  
  Serial.print("millis ");
  Serial.println(millis());
  //  if (mqtt.connected()) {
  if (millis() > (lastrun + PERIOD)) {
      if (mqtt.connect("Karttula", mqtt_user, mqtt_pass)) {
      
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
	  senddht22(Vierashuone, topicVierashuone);
	  state = 0;
	  lastrun = millis();
	}
      } else {
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
      }
    }
    
    delay(1000);
    mqtt.loop();
  /* } else { */
  /*   // Reconnect every 10 seconds */
  /*   unsigned long t = millis(); */
  /*   if (t - lastReconnectAttempt > 10000L) { */
  /*     lastReconnectAttempt = t; */
  /*     if (mqttConnect()) { */
  /*       lastReconnectAttempt = 0; */
  /*     } */
  /*   } */
  /* } */
    
  //  Watchdog.reset();

}

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

void senddht22(int pinDHT22, char* topic) {
  float temperature = 0;
  float humidity = 0;
  int err = SimpleDHTErrSuccess;
  size_t len = strlen(topic);

  if ((err = dht22.read2(pinDHT22, &temperature, &humidity, NULL)) != SimpleDHTErrSuccess) {
    delay(2000);
    Serial.print("DHT init failed, pin ");
    Serial.println(pinDHT22);
    return;
  }

  strcpy(topicbuffer,topic);
  
  strcpy(topicbuffer + len, tempprefix);
  dtostrf(temperature, 0, 1, valbuffer);
  mqtt.publish(topicbuffer, valbuffer);
  
  strcpy(topicbuffer + len, humprefix);
  dtostrf(humidity, 0, 1, valbuffer);
  mqtt.publish(topicbuffer, valbuffer);
  // mqtt.publish(topicbuffer, "Test");
}
  
void sendbme(char* topic) {
  float temperature = 0;
  float humidity = 0;
  bme_status = bme.begin();
  size_t len = strlen(topic);
  Serial.print("BME status ");
  Serial.println(bme_status);

  if (bme_status) {
    strcpy(topicbuffer,topic);
  
    strcpy(topicbuffer + len, tempprefix);
    dtostrf(bme.readTemperature(), 0, 1, valbuffer);
    mqtt.publish(topicbuffer, valbuffer);

    strcpy(topicbuffer + len, humprefix);
    dtostrf(bme.readHumidity(), 0, 1, valbuffer);
    mqtt.publish(topicbuffer, valbuffer);

    strcpy(topicbuffer + len, pressprefix);
    dtostrf(bme.readPressure(), 0, 2, valbuffer);
    mqtt.publish(topicbuffer, valbuffer);
  }
}
