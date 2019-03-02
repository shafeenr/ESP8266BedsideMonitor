#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ArduinoJson.h> // getWeather
#include <WiFiUdp.h>     // Time
#include <ArduinoOTA.h>

const char *ssid = "";
const char *password = "";

const int NTP_PACKET_SIZE = 48;
byte packetBuffer[NTP_PACKET_SIZE];
WiFiUDP udp;

void getWeather()
{
  WiFiClient client;

  const char *apiKey = "";
  const char *location = "Brampton, CA";

  if (client.connect("api.openweathermap.org", 80))
  {
    client.print("GET /data/2.5/weather?");
    client.print(strcat((char *)"q=", location));
    client.print(strcat((char *)"&appid=", apiKey));
    client.println("&units=metric");
    client.println("Host: api.openweathermap.org");
    client.println("Connection: close");
    client.println();
  }

  while (client.connected())
  {
    DynamicJsonBuffer JsonBuffer;
    JsonObject &root = JsonBuffer.parseObject(client.readString());
    client.stop();
    Serial.println(root["name"].as<char *>());
    delay(500);
    Serial.println(root["weather"][0]["main"].as<char *>());
    delay(500);
    Serial.println(root["main"]["temp"].as<char *>());
  }
}

void getNews()
{
  WiFiClient client;

  const char *apiKey = "";
  const char *newsSource = "the-new-york-times";

  if (client.connect("newsapi.org", 80))
  {
    client.print("GET /v2/top-headlines?");
    client.print(strcat((char *)"sources=", newsSource));
    client.print("&pageSize=1");
    client.print(strcat((char *)"&apiKey=", apiKey));
    client.println(" HTTP/1.1");
    client.println("Host: newsapi.org");
    client.println("Connection: close");
    client.println();
  }

  while (client.connected())
  {
    client.readStringUntil('{');
    std::string data = client.readString().c_str();
    data = data.substr(data.find("\"title\":\""));
    Serial.println(data.substr(9, data.find("\",\"") - 9).c_str());
    /*size_t found = data.find("\"title\":\"");
    size_t foundend = data.find("\",\"", found+9);
    data = data.substr(found+9, foundend);*/
  }
}

void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011; // LI, Version, Mode
  packetBuffer[1] = 0;          // Stratum, or type of clock
  packetBuffer[2] = 6;          // Polling Interval
  packetBuffer[3] = 0xEC;       // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}

void getTime()
{
  IPAddress timeServerIP;
  //get a random server from the pool
  WiFi.hostByName("pool.ntp.org", timeServerIP);

  sendNTPpacket(timeServerIP); // send an NTP packet to a time server
  // wait to see if a reply is available
  delay(500);

  int cb = udp.parsePacket();
  if (cb)
  {
    // We've received a packet, read the data from it
    udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    // subtract seventy years:
    unsigned long epoch = secsSince1900 - 2208988800UL;
    int hour = (epoch % 86400L) / 3600 - 5; //Unix Time -> UTC -> EST
    if (hour <= 0)
    {
      hour += 12;
    }
    Serial.print(hour); // print the hour
    Serial.print(':');

    if (((epoch % 3600) / 60) < 10)
    {
      // In the first 10 minutes of each hour, we'll want a leading '0'
      Serial.print('0');
    }
    Serial.println((epoch % 3600) / 60); // print the minute (3600 equals secs per minute)
  }
}

void getStocks()
{
  WiFiClientSecure client;

  const char *stockTicker = "td";

  if (client.connect("api.iextrading.com", 443))
  {
    client.print(strcat((char *)"GET /1.0/stock/", stockTicker));
    client.println("/quote");
    client.println("Host: api.iextrading.com");
    client.println("Connection: close");
    client.println();
  }

  while (client.connected())
  {
    DynamicJsonBuffer JsonBuffer;
    JsonObject &root = JsonBuffer.parseObject(client.readString());
    client.stop();
    Serial.println(root["symbol"].as<char *>());
    delay(500);
    Serial.println(root["companyName"].as<char *>());
    delay(500);
    Serial.println(root["latestPrice"].as<char *>());
    delay(500);
    Serial.println(root["change"].as<char *>());
    delay(500);
  }
}

void setup()
{
  Serial.begin(9600);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    delay(5000);
    ESP.restart();
  }
  udp.begin(2390);

  ArduinoOTA.begin();

  Serial.println();
  getTime();
  delay(500);
  getWeather();
  delay(500);
  getNews();
  delay(500);
  getStocks();
}

void loop()
{
  ArduinoOTA.handle();
}
