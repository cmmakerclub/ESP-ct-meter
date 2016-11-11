
/* choose ones */
//#define device1
//#define device2
//#define device3


#define LED_vcc   4
#define LED_gnd   5


#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <Ticker.h>

byte dot_state = 0;
unsigned long epoch ;
int hh;
int mm;
int ss;
int force_update = 1;
Ticker second_tick;

const char* ssid     = "ESPERT-3020";
const char* pass     = "espertap";

unsigned int localPort = 2390;      // local port to listen for UDP packets

IPAddress timeServer(129, 6, 15, 28); // time.nist.gov NTP server

const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message

byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets

// A UDP instance to let us send and receive packets over UDP
WiFiUDP udp;

void init_hardware()
{
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LED_vcc, OUTPUT);
  pinMode(LED_gnd, OUTPUT);

  Serial.begin(115200);
  delay(10);
  Serial.println();
  Serial.println();
  //set pins to output so you can control the shift register

  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");

  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.println("Starting UDP");
  udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(udp.localPort());
}

uint32_t data_sum = 1;
uint32_t data_count = 1;
float data = 0;
uint32_t time_now, time_prev_1, time_prev_2, time_prev_3, time_prev_4;

void setup()
{
  init_hardware();

  NTP_get();
  delay(100);

  hh = (epoch % 86400L) / 3600;
  mm = (epoch % 3600) / 60;
  ss = (epoch % 60);

  second_tick.attach(1, tick);
}

void loop()
{
  time_now = epoch;

  if (time_now - time_prev_1 >= 60) { //update time
    time_prev_1 = time_now;
    NTP_get();
  }

  if (time_now % 60 >= 0 && time_now % 60 < 20 ) { //push data ch1
    if (time_now - time_prev_2 >= 30 ) {
      time_prev_2 = time_now;
#ifdef device1
      Serial.println("Push Ch1");
      Push_data();
#endif

    }
  }

  if (time_now % 60 >= 20 && time_now % 60 < 40 ) { //push data ch2
    if (time_now - time_prev_3 >= 30) {
      time_prev_3 = time_now;
#ifdef device2
      Serial.println("Push Ch2");
      Push_data();
#endif

    }
  }

  if (time_now % 60 >= 40 && time_now % 60 < 60 ) { //push data ch3
    if (time_now - time_prev_4 >= 30) {
      time_prev_4 = time_now;
#ifdef device3
      Serial.println("Push Ch3");
      Push_data();
#endif

    }
  }

  data_sum += analogRead(A0);
  data_count++;

  delay(1);
}

void tick (void)
{
  epoch++; //Add a second
  Serial.println(epoch % 60);
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
}


void NTP_get(void)
{

  sendNTPpacket(timeServer); // send an NTP packet to a time server
  delay(1000);
  int cb = udp.parsePacket();
  if (cb) {
    force_update = 0;
    // Serial.print("packet received, length=");
    // Serial.println(cb);
    // We've received a packet, read the data from it
    udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    // Serial.print("Seconds since Jan 1 1900 = " );
    // Serial.println(secsSince1900);

    // now convert NTP time into everyday time:
    // Serial.print("Unix time = ");
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;
    // subtract seventy years:
    epoch = secsSince1900 - seventyYears ;
  }
}


// send an NTP request to the time server at the given address
unsigned long sendNTPpacket(IPAddress& address)
{
  Serial.println("sending NTP packet...");
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}

void Push_data () {
  data = (float)data_sum / (float)data_count;
  int tmp = data / 10;
  data = tmp;
  data /= 10;
  data_sum = 0;
  data_count = 0;
  Serial.println(data);
  uploadThingsSpeak(data);
  digitalWrite(LED_vcc, HIGH);
  delay(3000);
  digitalWrite(LED_vcc, LOW);
}

void uploadThingsSpeak(float data) {
  static const char* host = "api.thingspeak.com";
  static const char* apiKey = "EDEKTGVXZDFN3DO8";

  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    return;
  }

  // We now create a URI for the request
  String url = "/update/";
  //  url += streamId;
  //-----------------------------------------------
  url += "?key=";
  url += apiKey;

#ifdef device1
  url += "&field1=";
  url += data;
#endif

#ifdef device2
  url += "&field2=";
  url += data;
#endif

#ifdef device3
  url += "&field3=";
  url += data;
#endif
  //---------------------------------------------- -


  // This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");
}
