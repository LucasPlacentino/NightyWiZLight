// https://github.com/LucasPlacentino/NightyWiZLight

#include "SECRETS.h"

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
// #include <HTTPClient.h>
// #include <WifiClient.h>
// #include <WiFiUdp.h>
#include <Time> // or just NTPClient?

WiFiClient client;
WiFiUDP Udp;
unsigned int localUdpPort = 2444;

#if USE_OPENWEATHERMAP || defined(OWM_API_KEY)
#include <Arduino_JSON>
def get_sunset_sunrise_time()
{
    //
}
#else
#include <SolarCalculator>
def get_sunset_sunrise_time()
{
    //
}
#endif

bool wifi_setup()
{
    //
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    Serial.println(F("Connecting to WiFi"));
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(F("."));
    }
    Serial.print(F("Connected to the WiFi network: "));
    Serial.println(WIFI_SSID);

    Serial.print(F("IP: "));
    Serial.println(WiFi.localIP());

    Serial.print("Signal strength (RSSI):");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
}

void setup()
{
    //
}

void loop()
{
    //
}
