#include <Wire.h>
#include "DFRobot_SHT20.h"
DFRobot_SHT20    sht20;

#include <Ethernet.h>
#include <EthernetUdp.h>

const byte relay1 = A0;
const byte relay2 = A1;
const byte relay3 = A2;
const byte relay4 = A3;

const char packetKey[] = "CERES";
const byte roomID[] = {0x77, 0x58, 0xE1, 0x72, 0x8B, 0xD9};//replace with new one for every room
byte envProfile = 0;

byte maxTmp = 25;
byte minHum = 40;
byte maxCO2 = 255;
bool lights = false;


// The IP address will be determined by a dhcp
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};

//Listening for a specific client's IP, security measure
IPAddress listeningFor;

unsigned int localPort = 8888;   // local port to listen on
unsigned int endPort = 8888;     // port to send data to

// buffers for receiving and sending data
char packetBuffer[UDP_TX_PACKET_MAX_SIZE];  // buffer to hold incoming packet
char batchId[] = "IO.2018.11.20";

//Create Ethernet UDP instance
EthernetUDP Udp;

void setup() {

//   Serial Setup
  Serial.begin(9600);
  while (!Serial) {
   ; // wait for serial port to connect. Needed for native USB port only
  }

  pinMode(relay1, INPUT_PULLUP); // first enable pull up, to stop relay chatter during bootup
  pinMode(relay1, OUTPUT); // then set pin to output
  pinMode(relay2, INPUT_PULLUP); // first enable pull up, to stop relay chatter during bootup
  pinMode(relay2, OUTPUT); // then set pin to output
  pinMode(relay3, INPUT_PULLUP); // first enable pull up, to stop relay chatter during bootup
  pinMode(relay3, OUTPUT); // then set pin to output
  pinMode(relay4, INPUT_PULLUP); // first enable pull up, to stop relay chatter during bootup
  pinMode(relay4, OUTPUT); // then set pin to output

  // Ethernet Setup
  Ethernet.init(10);
  Ethernet.begin(mac);

  Serial.println("Initialize Ethernet with DHCP:");
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    } else if (Ethernet.linkStatus() == LinkOFF) {
      Serial.println("Ethernet cable is not connected.");
    }
    // no point in carrying on, so do nothing forevermore:
    while (true) {
      delay(1);
    }
  }
  // print your local IP address:
  Serial.print("My IP address: ");
  Serial.println(Ethernet.localIP());

  // start UDP
  Udp.begin(localPort);

  /// Begin Sensor Setup ///

  //SHT20 Temp & Hum
  sht20.initSHT20();                                  // Init SHT20 Sensor
  delay(100);
  sht20.checkSHT20();                                 // Check SHT20 Sensor

}

void loop() {

  if (envProfile == 0) {

    int packet = Udp.parsePacket();
    if (packet) {
      Udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
      if (packetBuffer[0] == packetKey[0] && packetBuffer[1] == packetKey[1] && packetBuffer[2] == packetKey[2] && packetBuffer[3] == packetKey[3] && packetBuffer[4] == packetKey[4]) {
        envProfile = 1;
        listeningFor = Udp.remoteIP();

        //Reply with roomID and Profile
        byte replyBuffer[] = {9, envProfile};
        Udp.beginPacket(listeningFor, endPort);
        for (int i=0; i<2; i++){
          Udp.write(replyBuffer[i]);
        };
        Udp.endPacket();

      }
    }

  } else {

    byte humd = sht20.readHumidity();                  // Read Humidity
    byte temp = sht20.readTemperature();               // Read Temperature
    byte co2 = 255;

    byte isRelay1 = digitalRead(relay1);
    byte isRelay2 = digitalRead(relay2);
    byte isRelay3 = digitalRead(relay3);
    byte isRelay4 = digitalRead(relay4);

    int packetSize = Udp.parsePacket();

    // Packet Logic
    if (packetSize) {
      if (Udp.remoteIP() == listeningFor) {

        IPAddress remote = Udp.remoteIP();
        Udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);

        // Update listeningFor IP
        if (packetBuffer[0] == 1) {
          Serial.println("Update listeningFor IP");
        }

        // Update Profile
        if (packetBuffer[0] == 2) {
          Serial.println();
          Serial.println("Profile Updated");
          Serial.println();
          envProfile = packetBuffer[1];
          maxTmp = packetBuffer[2];
          minHum = packetBuffer[3];
          maxCO2 = packetBuffer[4];
          if (packetBuffer[5] == 0) {
            lights = true;
          }
          if (packetBuffer[5] == 1) {
            lights = false;
          }

          // Serial.print("Environment Profile ID: ");
          // Serial.print(envProfile);
          // Serial.println();
          // Serial.print("Max Temp: ");
          // Serial.print(maxTmp);
          // Serial.println();
          // Serial.print("Min Hum: ");
          // Serial.print(minHum);
          // Serial.println();
          // Serial.print("Max CO2: ");
          // Serial.print(maxCO2);
          // Serial.println();
          // if (digitalRead(relay4) == 0) {
          //   Serial.print("Lights: On");
          // }
          // if (digitalRead(relay4) == 1) {
          //   Serial.print("Lights: Off");
          // }
          // Serial.println();
        }

        // Send Data
        if (packetBuffer[0] == 3) {
          byte replyBuffer[] = {1, envProfile, temp, humd, co2, isRelay1, isRelay2, isRelay3, isRelay4};
          Udp.beginPacket(listeningFor, endPort);
          Udp.write(batchId);
          for (int i=0; i<9; i++){
            Udp.write(replyBuffer[i]);
          };
          Udp.endPacket();
          Serial.println("Send Data");
        }

      } else {
        Serial.print("Received Packet From Unknown Source ");
        Serial.print(Udp.remoteIP());
      }
    }

    // Environment Logic
    if (temp >= maxTmp) {
      if (digitalRead(relay1) == 1) {
        digitalWrite(relay1, LOW); // relay 'on'
      }
    } else {
      if (digitalRead(relay1) == 0) {
        digitalWrite(relay1, HIGH); // relay 'off'
      }
    }

    if (humd <= minHum) {
      if (digitalRead(relay2) == 1) {
        digitalWrite(relay2, LOW); // relay 'on'
      }
    } else {
      if (digitalRead(relay2) == 0) {
        digitalWrite(relay2, HIGH); // relay 'on'
      }
    }

    if (lights == true) {
      if (digitalRead(relay4) == 1) {
        digitalWrite(relay4, LOW); // relay 'on'
      }
    } else {
      if (digitalRead(relay4) == 0) {
        digitalWrite(relay4, HIGH); // raley 'off'
      }
    }

    delay(10);

  }

}
