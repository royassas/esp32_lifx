/*
  LIFX bulb emulator by Kayne Richens (kayno@kayno.net)

  Emulates a LIFX bulb. Connect an RGB LED (or LED strip via drivers)
  to redPin, greenPin and bluePin as you normally would on an
  ethernet-ready Arduino and control it from the LIFX app!

  Notes:
  - Only one client (e.g. app) can connect to the bulb at once

  Set the following variables below to suit your Arduino and network
  environment:
  - mac (unique mac address for your arduino)
  - redPin (PWM pin for RED)
  - greenPin  (PWM pin for GREEN)
  - bluePin  (PWM pin for BLUE)

  Made possible by the work of magicmonkey:
  https://github.com/magicmonkey/lifxjs/ - you can use this to control
  your arduino bulb as well as real LIFX bulbs at the same time!

  And also the RGBMood library by Harold Waterkeyn, which was modified
  slightly to support powering down the LED
*/

#include <EEPROM.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <FastLED.h>
#include <Esp32WifiManager.h>

#include "lifx.h"
#include "color.h"


#define EEPROM_SIZE 256

// define to output debug messages (including packet dumps) to serial (38400 baud)
#define DEBUG

#ifdef DEBUG
#define debug_print(x, ...) Serial.print (x, ## __VA_ARGS__)
#define debug_println(x, ...) Serial.println (x, ## __VA_ARGS__)
#else
#define debug_print(x, ...)
#define debug_println(x, ...)
#endif


// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = {
  0x86, 0x32, 0x98, 0x63, 0x4a, 0x78
};
byte site_mac[] = {
  0x4c, 0x49, 0x46, 0x58, 0x56, 0x32
}; // spells out "LIFXV2" - version 2 of the app changes the site address to this...

//WS2801 for FastLED
#define NUM_LEDS 10
#define DATA_PIN 26
#define CLOCK_PIN 27
CRGB leds[NUM_LEDS];

#include "bulbDefaults.h"

// initial bulb values - warm white!
long power_status = 65535;
long hue = 0;
long sat = 0;
long bri = 65535;
long kel = 2000;
long dim = 0;

// Ethernet instances, for UDP broadcasting, and TCP server and client
WiFiUDP Udp;
WiFiServer TcpServer(LifxPort);
WiFiClient client;
WifiManager wifiManager;

#include "eepromIO.h"
void setup() {

  Serial.begin(115200);
  delay(1000);
  Serial.println("LIFX bulb emulator for Esp32 starting up...");

  //EEPROM
  if (!EEPROM.begin(EEPROM_SIZE))
  {
    Serial.println("failed to initialise EEPROM"); delay(1000000);
  }
  
  // WIFI
  wifiManager.setup();

  // LEDS
  FastLED.addLeds<WS2801, DATA_PIN, CLOCK_PIN>(leds, NUM_LEDS);
  leds[0] = CRGB::Blue;
  FastLED.show();
  debug_println("LEDS initalized");

  // Set mac address
  WiFi.macAddress(mac);

  // set up a UDP and TCP port ready for incoming
  Udp.begin(LifxPort);
  TcpServer.begin();

  if (eepromExists()) {
    readEepromDefaults();
  } else {
    writeEepromDefaults();
  }

dumpEeprom();

  // set the bulb based on the initial colors
  setLight();
}

void loop() {
  wifiManager.loop();
  if (wifiManager.getState() == Connected)
  {
    // buffers for receiving and sending data
    byte PacketBuffer[128]; //buffer to hold incoming packet,

    client = TcpServer.available();
    if (client == true) {
      // read incoming data
      int packetSize = 0;
      while (client.available()) {
        byte b = client.read();
        PacketBuffer[packetSize] = b;
        packetSize++;
      }

      debug_print(F("-TCP "));
      for (int i = 0; i < LifxPacketSize; i++) {
        debug_print(PacketBuffer[i], HEX);
        debug_print(SPACE);
      }

      for (int i = LifxPacketSize; i < packetSize; i++) {
        debug_print(PacketBuffer[i], HEX);
        debug_print(SPACE);
      }
      debug_println();

      // push the data into the LifxPacket structure
      LifxPacket request;
      processRequest(PacketBuffer, packetSize, request);

      //respond to the request
      handleRequest(request);
    }

    // if there's UDP data available, read a packet
    int packetSize = Udp.parsePacket();
    if (packetSize) {
      Udp.read(PacketBuffer, 128);

      debug_print(F("-UDP "));
      for (int i = 0; i < LifxPacketSize; i++) {
        debug_print(PacketBuffer[i], HEX);
        debug_print(SPACE);
      }

      for (int i = LifxPacketSize; i < packetSize; i++) {
        debug_print(PacketBuffer[i], HEX);
        debug_print(SPACE);
      }
      debug_println();

      // push the data into the LifxPacket structure
      LifxPacket request;
      processRequest(PacketBuffer, sizeof(PacketBuffer), request);

      //respond to the request
      handleRequest(request);

    }
  } else {
    Serial.println(wifiManager.getState());
    delay(1000);
  }

  delay (0);
}

void processRequest(byte * packetBuffer, int packetSize, LifxPacket & request) {

  request.size        = packetBuffer[0] + (packetBuffer[1] << 8); //little endian
  request.protocol    = packetBuffer[2] + (packetBuffer[3] << 8); //little endian
  request.reserved1   = packetBuffer[4] + packetBuffer[5] + packetBuffer[6] + packetBuffer[7];

  byte bulbAddress[] = {
    packetBuffer[8], packetBuffer[9], packetBuffer[10], packetBuffer[11], packetBuffer[12], packetBuffer[13]
  };
  memcpy(request.bulbAddress, bulbAddress, 6);

  request.reserved2   = packetBuffer[14] + packetBuffer[15];

  byte site[] = {
    packetBuffer[16], packetBuffer[17], packetBuffer[18], packetBuffer[19], packetBuffer[20], packetBuffer[21]
  };
  memcpy(request.site, site, 6);

  request.reserved3   = packetBuffer[22] + packetBuffer[23];
  request.timestamp   = packetBuffer[24] + packetBuffer[25] + packetBuffer[26] + packetBuffer[27] +
                        packetBuffer[28] + packetBuffer[29] + packetBuffer[30] + packetBuffer[31];
  request.packet_type = packetBuffer[32] + (packetBuffer[33] << 8); //little endian
  request.reserved4   = packetBuffer[34] + packetBuffer[35];

  int i;
  for (i = LifxPacketSize; i < packetSize; i++) {
    request.data[i - LifxPacketSize] = packetBuffer[i];
  }

  request.data_size = i;
}

void handleRequest(LifxPacket & request) {
  debug_print(F("  Received packet type 0x"));
  debug_println(request.packet_type, HEX);

  LifxPacket response;
  switch (request.packet_type) {

    case GET_PAN_GATEWAY:
      {
        // we are a gateway, so respond to this

        // respond with the UDP port
        response.packet_type = PAN_GATEWAY;
        response.protocol = LifxProtocol_AllBulbsResponse;
        byte UDPdata[] = {
          SERVICE_UDP, //UDP
          lowByte(LifxPort),
          highByte(LifxPort),
          0x00,
          0x00
        };

        memcpy(response.data, UDPdata, sizeof(UDPdata));
        response.data_size = sizeof(UDPdata);
        sendPacket(response);

        // respond with the TCP port details
        response.packet_type = PAN_GATEWAY;
        response.protocol = LifxProtocol_AllBulbsResponse;
        byte TCPdata[] = {
          SERVICE_TCP, //TCP
          lowByte(LifxPort),
          highByte(LifxPort),
          0x00,
          0x00
        };

        memcpy(response.data, TCPdata, sizeof(TCPdata));
        response.data_size = sizeof(TCPdata);
        sendPacket(response);

      }
      break;


    case SET_LIGHT_STATE:
      {
        // set the light colors
        hue = word(request.data[2], request.data[1]);
        sat = word(request.data[4], request.data[3]);
        bri = word(request.data[6], request.data[5]);
        kel = word(request.data[8], request.data[7]);

        for (int i = 0; i < request.data_size; i++) {
          debug_print(request.data[i], HEX);
          debug_print(SPACE);
        }
        debug_println();

        setLight();
      }
      break;


    case GET_LIGHT_STATE:
      {
        // send the light's state
        response.packet_type = LIGHT_STATUS;
        response.protocol = LifxProtocol_AllBulbsResponse;
        byte StateData[] = {
          lowByte(hue),  //hue
          highByte(hue), //hue
          lowByte(sat),  //sat
          highByte(sat), //sat
          lowByte(bri),  //bri
          highByte(bri), //bri
          lowByte(kel),  //kel
          highByte(kel), //kel
          lowByte(dim),  //dim
          highByte(dim), //dim
          lowByte(power_status),  //power status
          highByte(power_status), //power status
          // label
          lowByte(bulbLabel[0]),
          lowByte(bulbLabel[1]),
          lowByte(bulbLabel[2]),
          lowByte(bulbLabel[3]),
          lowByte(bulbLabel[4]),
          lowByte(bulbLabel[5]),
          lowByte(bulbLabel[6]),
          lowByte(bulbLabel[7]),
          lowByte(bulbLabel[8]),
          lowByte(bulbLabel[9]),
          lowByte(bulbLabel[10]),
          lowByte(bulbLabel[11]),
          lowByte(bulbLabel[12]),
          lowByte(bulbLabel[13]),
          lowByte(bulbLabel[14]),
          lowByte(bulbLabel[15]),
          lowByte(bulbLabel[16]),
          lowByte(bulbLabel[17]),
          lowByte(bulbLabel[18]),
          lowByte(bulbLabel[19]),
          lowByte(bulbLabel[20]),
          lowByte(bulbLabel[21]),
          lowByte(bulbLabel[22]),
          lowByte(bulbLabel[23]),
          lowByte(bulbLabel[24]),
          lowByte(bulbLabel[25]),
          lowByte(bulbLabel[26]),
          lowByte(bulbLabel[27]),
          lowByte(bulbLabel[28]),
          lowByte(bulbLabel[29]),
          lowByte(bulbLabel[30]),
          lowByte(bulbLabel[31]),
          //tags
          lowByte(bulbTags[0]),
          lowByte(bulbTags[1]),
          lowByte(bulbTags[2]),
          lowByte(bulbTags[3]),
          lowByte(bulbTags[4]),
          lowByte(bulbTags[5]),
          lowByte(bulbTags[6]),
          lowByte(bulbTags[7])
        };

        memcpy(response.data, StateData, sizeof(StateData));
        response.data_size = sizeof(StateData);
        sendPacket(response);
      }
      break;


    case SET_POWER_STATE:
    case SET_POWER_STATE2:
    case GET_POWER_STATE:
    case GET_POWER_STATE2:
      {
        // set if we are setting
        if (request.packet_type == SET_POWER_STATE | request.packet_type == SET_POWER_STATE2) {
          power_status = word(request.data[1], request.data[0]);
          setLight();
        }

        // respond to both get and set commands
        response.packet_type = POWER_STATE;
        response.protocol = LifxProtocol_AllBulbsResponse;
        byte PowerData[] = {
          lowByte(power_status),
          highByte(power_status)
        };

        memcpy(response.data, PowerData, sizeof(PowerData));
        response.data_size = sizeof(PowerData);
        sendPacket(response);
      }
      break;


    case SET_BULB_LABEL:
    case GET_BULB_LABEL:
      {
        // set if we are setting
        if (request.packet_type == SET_BULB_LABEL) {
          int flag = 0;
          for (int i = 0; i < LifxBulbLabelLength; i++) {
            if (bulbLabel[i] != request.data[i]) {
              flag = 1;
              bulbLabel[i] = request.data[i];
              EEPROM.write(EEPROM_BULB_LABEL_START + i, request.data[i]);
            }
          }

          if (flag == 1) {
            debug_print("Bulb label has changed - writing to EEPROM... ");
            EEPROM.commit();
            debug_println("complete!");
          }

        }

        // respond to both get and set commands
        response.packet_type = BULB_LABEL;
        response.protocol = LifxProtocol_AllBulbsResponse;
        memcpy(response.data, bulbLabel, sizeof(bulbLabel));
        response.data_size = sizeof(bulbLabel);
        sendPacket(response);
      }
      break;


    case SET_BULB_TAGS:
    case GET_BULB_TAGS:
      {
        // set if we are setting
        if (request.packet_type == SET_BULB_TAGS) {
          int flag = 0;
          for (int i = 0; i < LifxBulbTagsLength; i++) {
            if (bulbTags[i] != request.data[i]) {
              flag = 1;
              bulbTags[i] = lowByte(request.data[i]);
              EEPROM.write(EEPROM_BULB_TAGS_START + i, request.data[i]);
            }
          }
          if (flag == 1) {
            debug_print("Bulb tags have changed - writing to EEPROM... ");
            EEPROM.commit();
            debug_println("complete!");
          }
        }

        // respond to both get and set commands
        response.packet_type = BULB_TAGS;
        response.protocol = LifxProtocol_AllBulbsResponse;
        memcpy(response.data, bulbTags, sizeof(bulbTags));
        response.data_size = sizeof(bulbTags);
        sendPacket(response);
      }
      break;


    case SET_BULB_TAG_LABELS:
    case GET_BULB_TAG_LABELS:
      {
        // set if we are setting
        if (request.packet_type == SET_BULB_TAG_LABELS) {
          int flag = 0;
          for (int i = 0; i < LifxBulbTagLabelsLength; i++) {
            if (bulbTagLabels[i] != request.data[i]) {
              flag = 1;
              bulbTagLabels[i] = request.data[i];
              EEPROM.write(EEPROM_BULB_TAG_LABELS_START + i, request.data[i]);
            }
          }
          if (flag == 1) {
            debug_print("Bulb tag labels have changed - writing to EEPROM... ");
            EEPROM.commit();
            debug_println("complete!");
          }
        }

        // respond to both get and set commands
        response.packet_type = BULB_TAG_LABELS;
        response.protocol = LifxProtocol_AllBulbsResponse;
        memcpy(response.data, bulbTagLabels, sizeof(bulbTagLabels));
        response.data_size = sizeof(bulbTagLabels);
        sendPacket(response);
      }
      break;
    case DEVICE_SET_LOCATION:
    case DEVICE_GET_LOCATION:
      {
        // set if we are setting
        if (request.packet_type == DEVICE_SET_LOCATION) {
          int flag = 0;
          for (int i = 0; i < (LifxBulbLocationIDLength + LifxBulbLocationLabelLength + LifxBulbLocationUpdatedAtLength); i++) {
            if (i < LifxBulbLocationIDLength) {
              if (location[i] != request.data[i]) {
                flag = 1;
                location[i] = request.data[i];
                EEPROM.write(EEPROM_LOCATION_START + i, request.data[i]);
              }
            } else if (i < LifxBulbLocationLabelLength) {
              if (bulbLocationLabel[i] != request.data[i]) {
                flag = 1;
                bulbLocationLabel[i] = request.data[i];
                EEPROM.write(EEPROM_LOCATION_START + i, request.data[i]);
              }
            } else {
              if (loc_updatedAt[i] != request.data[i]) {
                flag = 1;
                loc_updatedAt[i] = request.data[i];
                EEPROM.write(EEPROM_LOCATION_START + i, request.data[i]);
              }
            }
            

          }
          if (flag == 1) {
            debug_print("Bulb location has changed - writing to EEPROM... ");
            EEPROM.commit();
            debug_println("complete!");
            dumpEeprom();
          }
        }

        // respond to both get and set commands with state
        response.packet_type = DEVICE_STATE_LOCATION;
        response.protocol = LifxProtocol_AllBulbsResponse;
        byte LocationData[] = {
          //Location
          lowByte(location[0]),
          lowByte(location[1]),
          lowByte(location[2]),
          lowByte(location[3]),
          lowByte(location[4]),
          lowByte(location[5]),
          lowByte(location[6]),
          lowByte(location[7]),
          lowByte(location[8]),
          lowByte(location[9]),
          lowByte(location[10]),
          lowByte(location[11]),
          lowByte(location[12]),
          lowByte(location[13]),
          lowByte(location[14]),
          lowByte(location[15]),
          //Label
          lowByte(bulbLocationLabel[0]),
          lowByte(bulbLocationLabel[1]),
          lowByte(bulbLocationLabel[2]),
          lowByte(bulbLocationLabel[3]),
          lowByte(bulbLocationLabel[4]),
          lowByte(bulbLocationLabel[5]),
          lowByte(bulbLocationLabel[6]),
          lowByte(bulbLocationLabel[7]),
          lowByte(bulbLocationLabel[8]),
          lowByte(bulbLocationLabel[9]),
          lowByte(bulbLocationLabel[10]),
          lowByte(bulbLocationLabel[11]),
          lowByte(bulbLocationLabel[12]),
          lowByte(bulbLocationLabel[13]),
          lowByte(bulbLocationLabel[14]),
          lowByte(bulbLocationLabel[15]),
          lowByte(bulbLocationLabel[16]),
          lowByte(bulbLocationLabel[17]),
          lowByte(bulbLocationLabel[18]),
          lowByte(bulbLocationLabel[19]),
          lowByte(bulbLocationLabel[20]),
          lowByte(bulbLocationLabel[21]),
          lowByte(bulbLocationLabel[22]),
          lowByte(bulbLocationLabel[23]),
          lowByte(bulbLocationLabel[24]),
          lowByte(bulbLocationLabel[25]),
          lowByte(bulbLocationLabel[26]),
          lowByte(bulbLocationLabel[27]),
          lowByte(bulbLocationLabel[28]),
          lowByte(bulbLocationLabel[29]),
          lowByte(bulbLocationLabel[30]),
          lowByte(bulbLocationLabel[31]),
          //Updated_at
          lowByte(loc_updatedAt[0]),
          lowByte(loc_updatedAt[1]),
          lowByte(loc_updatedAt[2]),
          lowByte(loc_updatedAt[3]),
          lowByte(loc_updatedAt[4]),
          lowByte(loc_updatedAt[5]),
          lowByte(loc_updatedAt[6]),
          lowByte(loc_updatedAt[7])
        };
        memcpy(response.data, LocationData, sizeof(LocationData));
        response.data_size = sizeof(LocationData); 
        sendPacket(response);
      }
      break;
    case GET_VERSION_STATE:
      {
        // respond to get command
        response.packet_type = VERSION_STATE;
        response.protocol = LifxProtocol_AllBulbsResponse;
        byte VersionData[] = {
          lowByte(LifxBulbVendor),
          highByte(LifxBulbVendor),
          0x00,
          0x00,
          lowByte(LifxBulbProduct),
          highByte(LifxBulbProduct),
          0x00,
          0x00,
          lowByte(LifxBulbVersion),
          highByte(LifxBulbVersion),
          0x00,
          0x00
        };

        memcpy(response.data, VersionData, sizeof(VersionData));
        response.data_size = sizeof(VersionData);
        sendPacket(response);

        /*
          // respond again to get command (real bulbs respond twice, slightly diff data (see below)
          response.packet_type = VERSION_STATE;
          response.protocol = LifxProtocol_AllBulbsResponse;
          byte VersionData2[] = {
          lowByte(LifxVersionVendor), //vendor stays the same
          highByte(LifxVersionVendor),
          0x00,
          0x00,
          lowByte(LifxVersionProduct*2), //product is 2, rather than 1
          highByte(LifxVersionProduct*2),
          0x00,
          0x00,
          0x00, //version is 0, rather than 1
          0x00,
          0x00,
          0x00
          };

          memcpy(response.data, VersionData2, sizeof(VersionData2));
          response.data_size = sizeof(VersionData2);
          sendPacket(response);
        */
      }
      break;


    case GET_MESH_FIRMWARE_STATE:
      {
        // respond to get command
        response.packet_type = MESH_FIRMWARE_STATE;
        response.protocol = LifxProtocol_AllBulbsResponse;
        // timestamp data comes from observed packet from a LIFX v1.5 bulb
        byte MeshVersionData[] = {
          0x00, 0x2e, 0xc3, 0x8b, 0xef, 0x30, 0x86, 0x13, //build timestamp
          0xe0, 0x25, 0x76, 0x45, 0x69, 0x81, 0x8b, 0x13, //install timestamp
          lowByte(LifxFirmwareVersionMinor),
          highByte(LifxFirmwareVersionMinor),
          lowByte(LifxFirmwareVersionMajor),
          highByte(LifxFirmwareVersionMajor)
        };

        memcpy(response.data, MeshVersionData, sizeof(MeshVersionData));
        response.data_size = sizeof(MeshVersionData);
        sendPacket(response);
      }
      break;


    case GET_WIFI_FIRMWARE_STATE:
      {
        // respond to get command
        response.packet_type = WIFI_FIRMWARE_STATE;
        response.protocol = LifxProtocol_AllBulbsResponse;
        // timestamp data comes from observed packet from a LIFX v1.5 bulb
        byte WifiVersionData[] = {
          0x00, 0xc8, 0x5e, 0x31, 0x99, 0x51, 0x86, 0x13, //build timestamp
          0xc0, 0x0c, 0x07, 0x00, 0x48, 0x46, 0xd9, 0x43, //install timestamp
          lowByte(LifxFirmwareVersionMinor),
          highByte(LifxFirmwareVersionMinor),
          lowByte(LifxFirmwareVersionMajor),
          highByte(LifxFirmwareVersionMajor)
        };

        memcpy(response.data, WifiVersionData, sizeof(WifiVersionData));
        response.data_size = sizeof(WifiVersionData);
        sendPacket(response);
      }
      break;


    default:
      {
        debug_print(F("Unknown packet type: 0x"));
        debug_println(request.packet_type, HEX);
      }
      break;
  }
}

void sendPacket(LifxPacket & pkt) {
  sendUDPPacket(pkt);

  if (client.connected()) {
    sendTCPPacket(pkt);
  }
}

unsigned int sendUDPPacket(LifxPacket & pkt) {
  // broadcast packet on local subnet
  IPAddress remote_addr(Udp.remoteIP());
  IPAddress broadcast_addr(remote_addr[0], remote_addr[1], remote_addr[2], 255);

  debug_print(F("+UDP "));
  printLifxPacket(pkt);
  debug_println();

  Udp.beginPacket(broadcast_addr, Udp.remotePort());

  // size
  Udp.write(lowByte(LifxPacketSize + pkt.data_size));
  Udp.write(highByte(LifxPacketSize + pkt.data_size));

  // protocol
  Udp.write(lowByte(pkt.protocol));
  Udp.write(highByte(pkt.protocol));

  // reserved1
  Udp.write(lowByte(0x00));
  Udp.write(lowByte(0x00));
  Udp.write(lowByte(0x00));
  Udp.write(lowByte(0x00));

  // bulbAddress mac address
  for (int i = 0; i < sizeof(mac); i++) {
    Udp.write(lowByte(mac[i]));
  }

  // reserved2
  Udp.write(lowByte(0x00));
  Udp.write(lowByte(0x00));

  // site mac address
  for (int i = 0; i < sizeof(site_mac); i++) {
    Udp.write(lowByte(site_mac[i]));
  }

  // reserved3
  Udp.write(lowByte(0x00));
  Udp.write(lowByte(0x00));

  // timestamp
  Udp.write(lowByte(0x00));
  Udp.write(lowByte(0x00));
  Udp.write(lowByte(0x00));
  Udp.write(lowByte(0x00));
  Udp.write(lowByte(0x00));
  Udp.write(lowByte(0x00));
  Udp.write(lowByte(0x00));
  Udp.write(lowByte(0x00));

  //packet type
  Udp.write(lowByte(pkt.packet_type));
  Udp.write(highByte(pkt.packet_type));

  // reserved4
  Udp.write(lowByte(0x00));
  Udp.write(lowByte(0x00));

  //data
  for (int i = 0; i < pkt.data_size; i++) {
    Udp.write(lowByte(pkt.data[i]));
  }

  Udp.endPacket();

  return LifxPacketSize + pkt.data_size;
}

unsigned int sendTCPPacket(LifxPacket & pkt) {

  debug_print(F("+TCP "));
  printLifxPacket(pkt);
  debug_println();

  byte TCPBuffer[128]; //buffer to hold outgoing packet,
  int byteCount = 0;

  // size
  TCPBuffer[byteCount++] = lowByte(LifxPacketSize + pkt.data_size);
  TCPBuffer[byteCount++] = highByte(LifxPacketSize + pkt.data_size);

  // protocol
  TCPBuffer[byteCount++] = lowByte(pkt.protocol);
  TCPBuffer[byteCount++] = highByte(pkt.protocol);

  // reserved1
  TCPBuffer[byteCount++] = lowByte(0x00);
  TCPBuffer[byteCount++] = lowByte(0x00);
  TCPBuffer[byteCount++] = lowByte(0x00);
  TCPBuffer[byteCount++] = lowByte(0x00);

  // bulbAddress mac address
  for (int i = 0; i < sizeof(mac); i++) {
    TCPBuffer[byteCount++] = lowByte(mac[i]);
  }

  // reserved2
  TCPBuffer[byteCount++] = lowByte(0x00);
  TCPBuffer[byteCount++] = lowByte(0x00);

  // site mac address
  for (int i = 0; i < sizeof(site_mac); i++) {
    TCPBuffer[byteCount++] = lowByte(site_mac[i]);
  }

  // reserved3
  TCPBuffer[byteCount++] = lowByte(0x00);
  TCPBuffer[byteCount++] = lowByte(0x00);

  // timestamp
  TCPBuffer[byteCount++] = lowByte(0x00);
  TCPBuffer[byteCount++] = lowByte(0x00);
  TCPBuffer[byteCount++] = lowByte(0x00);
  TCPBuffer[byteCount++] = lowByte(0x00);
  TCPBuffer[byteCount++] = lowByte(0x00);
  TCPBuffer[byteCount++] = lowByte(0x00);
  TCPBuffer[byteCount++] = lowByte(0x00);
  TCPBuffer[byteCount++] = lowByte(0x00);

  //packet type
  TCPBuffer[byteCount++] = lowByte(pkt.packet_type);
  TCPBuffer[byteCount++] = highByte(pkt.packet_type);

  // reserved4
  TCPBuffer[byteCount++] = lowByte(0x00);
  TCPBuffer[byteCount++] = lowByte(0x00);

  //data
  for (int i = 0; i < pkt.data_size; i++) {
    TCPBuffer[byteCount++] = lowByte(pkt.data[i]);
  }

  //client.write(TCPBuffer, byteCount);

  return LifxPacketSize + pkt.data_size;
}

// print out a LifxPacket data structure as a series of hex bytes - used for DEBUG
void printLifxPacket(LifxPacket & pkt) {
  // size
  debug_print(lowByte(LifxPacketSize + pkt.data_size), HEX);
  debug_print(SPACE);
  debug_print(highByte(LifxPacketSize + pkt.data_size), HEX);
  debug_print(SPACE);

  // protocol
  debug_print(lowByte(pkt.protocol), HEX);
  debug_print(SPACE);
  debug_print(highByte(pkt.protocol), HEX);
  debug_print(SPACE);

  // reserved1
  debug_print(lowByte(0x00), HEX);
  debug_print(SPACE);
  debug_print(lowByte(0x00), HEX);
  debug_print(SPACE);
  debug_print(lowByte(0x00), HEX);
  debug_print(SPACE);
  debug_print(lowByte(0x00), HEX);
  debug_print(SPACE);

  // bulbAddress mac address
  for (int i = 0; i < sizeof(mac); i++) {
    debug_print(lowByte(mac[i]), HEX);
    debug_print(SPACE);
  }

  // reserved2
  debug_print(lowByte(0x00), HEX);
  debug_print(SPACE);
  debug_print(lowByte(0x00), HEX);
  debug_print(SPACE);

  // site mac address
  for (int i = 0; i < sizeof(site_mac); i++) {
    debug_print(lowByte(site_mac[i]), HEX);
    debug_print(SPACE);
  }

  // reserved3
  debug_print(lowByte(0x00), HEX);
  debug_print(SPACE);
  debug_print(lowByte(0x00), HEX);
  debug_print(SPACE);

  // timestamp
  debug_print(lowByte(0x00), HEX);
  debug_print(SPACE);
  debug_print(lowByte(0x00), HEX);
  debug_print(SPACE);
  debug_print(lowByte(0x00), HEX);
  debug_print(SPACE);
  debug_print(lowByte(0x00), HEX);
  debug_print(SPACE);
  debug_print(lowByte(0x00), HEX);
  debug_print(SPACE);
  debug_print(lowByte(0x00), HEX);
  debug_print(SPACE);
  debug_print(lowByte(0x00), HEX);
  debug_print(SPACE);
  debug_print(lowByte(0x00), HEX);
  debug_print(SPACE);

  //packet type
  debug_print(lowByte(pkt.packet_type), HEX);
  debug_print(SPACE);
  debug_print(highByte(pkt.packet_type), HEX);
  debug_print(SPACE);

  // reserved4
  debug_print(lowByte(0x00), HEX);
  debug_print(SPACE);
  debug_print(lowByte(0x00), HEX);
  debug_print(SPACE);

  //data
  for (int i = 0; i < pkt.data_size; i++) {
    debug_print(pkt.data[i], HEX);
    debug_print(SPACE);
  }
}

void setLight() {
  debug_print(F("Set light - "));
  debug_print(F("hue: "));
  debug_print(hue);
  debug_print(F(", sat: "));
  debug_print(sat);
  debug_print(F(", bri: "));
  debug_print(bri);
  debug_print(F(", kel: "));
  debug_print(kel);
  debug_print(F(", power: "));
  debug_print(power_status);
  debug_println(power_status ? " (on)" : "(off)");

  if (power_status) {
    int this_hue = map(hue, 0, 65535, 0, 767);
    int this_sat = map(sat, 0, 65535, 0, 255);
    int this_bri = map(bri, 0, 65535, 0, 255);

    // if we are setting a "white" colour (kelvin temp)
    if (kel > 0 && this_sat < 1) {
      // convert kelvin to RGB
      rgb kelvin_rgb;
      kelvin_rgb = kelvinToRGB(kel);

      // convert the RGB into HSV
      hsv kelvin_hsv;
      kelvin_hsv = rgb2hsv(kelvin_rgb);

      // set the new values ready to go to the bulb (brightness does not change, just hue and saturation)
      this_hue = map(kelvin_hsv.h, 0, 359, 0, 767);
      this_sat = map(kelvin_hsv.s * 1000, 0, 1000, 0, 255); //multiply the sat by 1000 so we can map the percentage value returned by rgb2hsv
    }

    uint8_t rgbColor[3];
    hsb2rgb(this_hue, this_sat, this_bri, rgbColor);

    uint8_t r = map(rgbColor[0], 0, 255, 0, 32);
    uint8_t g = map(rgbColor[1], 0, 255, 0, 32);
    uint8_t b = map(rgbColor[2], 0, 255, 0, 32);

    // LIFXBulb.fadeHSB(this_hue, this_sat, this_bri);
    leds[0].setRGB( r, g, b);
    fill_solid( leds, NUM_LEDS, CRGB( r, g, b) );
  }
  else {

    // LIFXBulb.fadeHSB(0, 0, 0);
    fill_solid( leds, NUM_LEDS, CRGB( 0, 0, 0) );
  }
  FastLED.show();
}

/******************************************************************************
   accepts hue, saturation and brightness values and outputs three 8-bit color
   values in an array (color[])

   saturation (sat) and brightness (bright) are 8-bit values.

   hue (index) is a value between 0 and 767. hue values out of range are
   rendered as 0.

 *****************************************************************************/
void hsb2rgb(uint16_t index, uint8_t sat, uint8_t bright, uint8_t color[3])
{
  uint16_t r_temp, g_temp, b_temp;
  uint8_t index_mod;
  uint8_t inverse_sat = (sat ^ 255);

  index = index % 768;
  index_mod = index % 256;

  if (index < 256)
  {
    r_temp = index_mod ^ 255;
    g_temp = index_mod;
    b_temp = 0;
  }

  else if (index < 512)
  {
    r_temp = 0;
    g_temp = index_mod ^ 255;
    b_temp = index_mod;
  }

  else if ( index < 768)
  {
    r_temp = index_mod;
    g_temp = 0;
    b_temp = index_mod ^ 255;
  }

  else
  {
    r_temp = 0;
    g_temp = 0;
    b_temp = 0;
  }

  r_temp = ((r_temp * sat) / 255) + inverse_sat;
  g_temp = ((g_temp * sat) / 255) + inverse_sat;
  b_temp = ((b_temp * sat) / 255) + inverse_sat;

  r_temp = (r_temp * bright) / 255;
  g_temp = (g_temp * bright) / 255;
  b_temp = (b_temp * bright) / 255;

  color[0]  = (uint8_t)r_temp;
  color[1]  = (uint8_t)g_temp;
  color[2] = (uint8_t)b_temp;
}




