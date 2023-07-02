/*

For ESP32 UWB or ESP32 UWB Pro

*/

#include <SPI.h>
#include <DW1000Ranging.h>
#include "link.h"

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>



#define WIFI_SSID "NETGEAR31"
#define WIFI_PASSWD "fluffywind2904"


#define SPI_SCK 18
#define SPI_MISO 19
#define SPI_MOSI 23
#define DW_CS 4


// connection pins
const uint8_t PIN_RST = 27;  // reset pin
const uint8_t PIN_IRQ = 34;  // irq pin
const uint8_t PIN_SS = 4;    // spi select pin


String macAddress;
String ipAddress;


struct MyLink *uwb_data;
int index_num = 0;
long runtime = 0;
String all_json = "";

WebSocketsClient webSocket;

void webSocketEvent(WStype_t type, uint8_t *payload, size_t length) {

  switch (type) {
    case WStype_DISCONNECTED:
      Serial.printf("[WSc] Disconnected!\n");
      break;
    case WStype_CONNECTED:
      {
        Serial.printf("[WSc] Connected to url: %s\n", payload);
      }
      break;
    case WStype_TEXT:
      Serial.printf("[WSc] get text: %s\n", payload);
      break;
    case WStype_BIN:
      Serial.printf("[WSc] get binary length: %u\n", length);
      break;
    case WStype_PING:
      // pong will be send automatically
      Serial.printf("[WSc] get ping\n");
      break;
    case WStype_PONG:
      // answer to a ping we send
      Serial.printf("[WSc] get pong\n");
      break;
  }
}

void setupNetwork() {
  macAddress = "";
#ifdef SOFTAP_MODE
  WiFi.mode(WIFI_AP);
  macAddress += WiFi.softAPmacAddress().substring(0, 5);
  WiFi.softAP(macAddress.c_str());
  ipAddress = WiFi.softAPIP().toString();
#else
  WiFi.begin(WIFI_SSID, WIFI_PASSWD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  ipAddress = WiFi.localIP().toString();
  macAddress = String(WiFi.macAddress());
#endif
}




void setup()
{
    Serial.begin(115200);

      setupNetwork();


  webSocket.begin("mocap.local", 3000, "/uwb32");
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000);
  
    delay(1000);

    //init the configuration
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
    DW1000Ranging.initCommunication(PIN_RST, DW_CS, PIN_IRQ);
    DW1000Ranging.attachNewRange(newRange);
    DW1000Ranging.attachNewDevice(newDevice);
    DW1000Ranging.attachInactiveDevice(inactiveDevice);

    //we start the module as a tag
    DW1000Ranging.startAsTag("7D:00:22:EA:82:60:3B:9C", DW1000.MODE_SHORTDATA_FAST_ACCURACY);

    uwb_data = init_link();
}

void loop()
{
    DW1000Ranging.loop();
      webSocket.loop();

    if ((millis() - runtime) > 50)
    {
        make_link_json(uwb_data, &all_json);
        Serial.println(all_json);
        webSocket.sendTXT(all_json);
        runtime = millis();
    }
}

void newRange()
{
    char c[30];

    Serial.print("from: ");
    Serial.print(DW1000Ranging.getDistantDevice()->getShortAddress(), HEX);
    Serial.print("\t Range: ");
    Serial.print(DW1000Ranging.getDistantDevice()->getRange());
    Serial.print(" m");
    Serial.print("\t RX power: ");
    Serial.print(DW1000Ranging.getDistantDevice()->getRXPower());
    Serial.println(" dBm");
    fresh_link(uwb_data, DW1000Ranging.getDistantDevice()->getShortAddress(), DW1000Ranging.getDistantDevice()->getRange(), DW1000Ranging.getDistantDevice()->getRXPower());
}

void newDevice(DW1000Device *device)
{
    Serial.print("ranging init; 1 device added ! -> ");
    Serial.print(" short:");
    Serial.println(device->getShortAddress(), HEX);

    add_link(uwb_data, device->getShortAddress());
}

void inactiveDevice(DW1000Device *device)
{
    Serial.print("delete inactive device: ");
    Serial.println(device->getShortAddress(), HEX);

    delete_link(uwb_data, device->getShortAddress());
}

void send_udp(String *msg_json)
{

}
