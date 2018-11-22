#include <Wire.h>
#include "DFRobot_SHT20.h"
DFRobot_SHT20    sht20;

#include <Ethernet.h>
#include <EthernetUdp.h>

const byte relay1 = A0;
const byte relay2 = A1;
const byte relay3 = A2;
const byte relay4 = A3;

byte roomID = 1;
byte envProfile = 0;

byte maxTmp = 10;
byte minHum = 90;

// The IP address will be determined by a dhcp
byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};
//Listening for a specific client's IP, security measure
IPAddress listeningFor(172, 16, 89, 189);

unsigned int localPort = 8888;   // local port to listen on
unsigned int endPort = 8888;     // port to send data to

// buffers for receiving and sending data
char packetBuffer[UDP_TX_PACKET_MAX_SIZE];  // buffer to hold incoming packet,
char batchId[] = "IO.2018.11.20";

//Create Ethernet UDP instance
EthernetUDP Udp;

void setup() {

  // Serial Setup
  // Serial.begin(9600);
  // while (!Serial) {
  //   ; // wait for serial port to connect. Needed for native USB port only
  // }

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

  // Serial.println("Initialize Ethernet with DHCP:");
  if (Ethernet.begin(mac) == 0) {
    // Serial.println("Failed to configure Ethernet using DHCP");
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      // Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    } else if (Ethernet.linkStatus() == LinkOFF) {
      // Serial.println("Ethernet cable is not connected.");
    }
    // no point in carrying on, so do nothing forevermore:
    while (true) {
      delay(1);
    }
  }
  // print your local IP address:
  // Serial.print("My IP address: ");
  // Serial.println(Ethernet.localIP());

  // start UDP
  Udp.begin(localPort);

  /// Begin Sensor Setup ///

  //SHT20 Temp & Hum
  sht20.initSHT20();                                  // Init SHT20 Sensor
  delay(100);
  sht20.checkSHT20();                                 // Check SHT20 Sensor

}

void loop() {

    byte humd = sht20.readHumidity();                  // Read Humidity
    byte temp = sht20.readTemperature();               // Read Temperature
    byte co2 = 255;

    byte isRelay1 = digitalRead(relay1);
    byte isRelay2 = digitalRead(relay2);
    byte isRelay3 = digitalRead(relay3);
    byte isRelay4 = digitalRead(relay4);

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

//    Serial.println();
//    Serial.println("Temperature: ");
//    Serial.print(temp);
//    Serial.println();
//    Serial.println("Humidity: ");
//    Serial.print(humd);

//if there's data available, read a packet
  int packetSize = Udp.parsePacket();
  if (packetSize) {
    if (Udp.remoteIP() == listeningFor) {
        // Serial.println();
        // Serial.print("Received packet of size: ");
        // Serial.print(packetSize);
        // Serial.println();
        // Serial.print("From: ");
        IPAddress remote = Udp.remoteIP();
        // for (int i=0; i < 4; i++) {
        //   Serial.print(remote[i], DEC);
        //   if (i < 3) {
        //     Serial.print(".");
        //   }
        // }
        // Serial.print(", port: ");
        // Serial.print(Udp.remotePort());

        // read the packet into packetBufffer
        Udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
        // Serial.println();
        // Serial.print("Contents: ");
        // Serial.print(packetBuffer);


        // Serial.println();
        // Serial.println("Temperature: ");
        // Serial.print(temp);
        // Serial.println();
        // Serial.println("Humidity: ");
        // Serial.print(humd);
        byte replyBuffer[] = {roomID, envProfile, temp, humd, co2, isRelay1, isRelay2, isRelay3, isRelay4};

        // send a reply to the IP address and port that sent us the packet we received
        Udp.beginPacket(listeningFor, endPort);
        Udp.write(batchId);
        for (int i=0; i<9; i++){
          Udp.write(replyBuffer[i]);
        };
        Udp.endPacket();
        // Serial.println();
        // Serial.println("Data Sent");
        // Serial.println();
      }
    // else {
    //   Serial.print("Received Packet From Unknown Source ");
    //   Serial.print(Udp.remoteIP());
    //   }
    }
  delay(10);
}
