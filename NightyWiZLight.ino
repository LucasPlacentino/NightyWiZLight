// https://github.com/LucasPlacentino/NightyWiZLight

#include "SECRETS.h"
#include "UTILS.h"

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
// #include <HTTPClient.h>
// #include <WifiClient.h>
// #include <WiFiUdp.h>
// #include <Time>
#include <NTPClient.h>

light_t lights[2] = {
    {0, OFF, 38899, false, "192.168.0.50"}, // light 0, init state, UDP port, changed_state(leave false), ip address
    {1, OFF, 38899, false, "192.168.0.51"}, // light 1, init state, UDP port, changed_state(leave false), ip address
};
const int LIGHTUP_TIME = 20; // seconds
const int PIR_PIN = 12;      // PIR sensor pin
const int SWITCH_PIN = 13;   // switch pin

// --------------

const unsigned int LOCAL_UDP_PORT = 2444;
const char GET_BUFFER[] = "{\"method\":\"getPilot\"}";                           // packet to get the state of a light
const char ON_BUFFER[] = "{\"method\":\"setPilot\",\"params\":{\"state\": 1}}";  // packet to turn ON a light
const char OFF_BUFFER[] = "{\"method\":\"setPilot\",\"params\":{\"state\": 0}}"; // packet to turn OFF a light
char packet_buffer[255];

const char OWM_URL[] = "https://api.openweathermap.org/data/2.5/weather?units=metric&lang=en";
// day_time_t time = DAY;
bool swiching_lights = false;

WiFiClient client;
// HTTPClient http;
WifiUDP Udp;
// WiFiUDP ntpUDP; //? needed?
NTPClient timeClient(Udp, "pool.ntp.org");

#if USE_OPENWEATHERMAP || defined(OWM_API_KEY)
#include <Arduino_JSON>
HTTPClient http;
void get_sunset_sunrise_time(time_t *sunrise, time_t *sunset)
{
    String json_buffer;
    String server_path = OWM_URL + "&lat=" + LATITUDE + "&lon=" + LONGITUDE + "&appid=" + OWM_API_KEY;

    Serial.print("[HTTP] begin...\n");
    if (http.begin(client, server_path.c_str()))
    { // HTTP

        Serial.print("[HTTP] GET...\n");
        int http_response_code = http.GET();
        String payload = "{}";

        if (http_response_code > 0)
        {
            Serial.printf("[HTTP] GET... code: %d\n", http_response_code);

            if (http_response_code == HTTP_CODE_OK || http_reponse_code == HTTP_CODE_MOVED_PERMANENTLY)
            {
                payload = http.getString();
                Serial.println(payload);
            }
            // payload = http.getString();
            // Serial.println(payload);
        }
        else
        {
            Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(http_response_code).c_str());
        }
        json_buffer = payload;
        http.end();
    }
    else
    {
        Serial.println("[HTTP] Unable to connect");
        return {1, 1};
    }
    JSONVar data = JSON.parse(json_buffer);
    if (JSON.typeof(data) == "undefined")
    {
        Serial.println("Parsing JSON input failed!");
        return {1, 1};
    }
    sunrise = data["sys"]["sunrise"];
    Serial.println("Sunrise time: %u"; sunrise);
    sunset = data["sys"]["sunset"];
    Serial.println("Sunset time: %u"; sunset);
}

#else
#include <SolarCalculator>
sunrise_sunset_time_t get_sunset_sunrise_time()
{
    // TODO:
}
#endif

light_state_t get_light_state(light_t light)
{
    light_state_t current_state = OFF;
    packet_flush();

    Udp.beginPacket(&light.ip_addr, &light.port);
    Udp.write(GET_BUFFER)
        Udp.endPacket();

    int packet_size = Udp.parsePacket();
    int timeout = 50;
    while (!packet_size && timeout > 0)
    {
        packet_size = Udp.parsePacket();
        timeout--;
        delay(200);
    }
    if (timeout == 0)
    {
        Serial.println("Light get state timed out: %s", light.ip_addr);
        return OFF;
    }
    if (packet_size)
    {
        int len = Udp.read(packet_buffer, UDP_TX_PACKET_MAX_SIZE); // 255
        if (len < 0)
        {
            packer_buffer[len] = 0;
        }
        Serial.println("Packet received:");
        Serial.println(packer_buffer);

        const int capacity = JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(7);
        StaticJsonDocument<capacity> response;

        DeserializationError err = deserializeJson(response, packet_buffer);
        if (err.code() == DeserializationError::Ok)
        {
            current_state = response["result"]["state"] ? ON : OFF;
        }
    }
    return current_state;
}

bool switch_light_state(light_t light, light_state_t new_state)
{
    WiFiUDP Udp;
    bool res = false;
    Udp.beginPacket(&light.ip_addr, &light.port);
    if (new_state == ON)
    {
        Udp.write(ON_BUFFER);
        res = true;
    }
    else
    {
        Udp.write(OFF_BUFFER);
        res = true;
    }
    Udp.endPacket();
    return res;
}

void packet_flush()
{
    // parse all packets in the RX buffer to clear it
    while (Udp.parsePacket())
    {
        ;
    }
}

bool wifi_setup()
{
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    Serial.println(F("Connecting to WiFi"));
    int i = 0;
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(F("."));
        i++;
        if (i > 50)
        {
            Serial.println(F("Connection failed"));
            return false;
        }
    }
    Serial.print(F("Connected to the WiFi network: "));
    Serial.println(WIFI_SSID);

    Serial.print(F("IP: "));
    Serial.println(WiFi.localIP());

    Serial.print("Signal strength (RSSI):");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
    return true;
}

bool switch_lights()
{
    bool res = false;
    for (int i, i < sizeof(lights) / sizeof(lights[0]), i++)
    {
        light_t light = lights[i];
        light_state_t current_state = get_light_state(light);

        light.changed_state = current_state == OFF ? true : false;
        Serial.println("Switching light %d state to ON", light.id);
        !light.changed_state ? Serial.println("(light was already ON)");
        res = switch_light_state(light, ON);
        /*
        if (current_state == OFF)
        {
            Serial.println("Switching light %d state to ON", light.id);
            res = switch_light_state(light, ON);
            light.changed_state = true;
        }
        else
        {
            Serial.println("Light %d state already ON, not switching",light.id);
            res true;
            light.changed_state = false;
        }
        */
    }

    delay(LIGHTUP_TIME * 1000);

    for (int i, i < sizeof(lights) / sizeof(lights[0]), i++)
    {
        light_t light = lights[i];
        if (light.changed_state)
        {
            Serial.println("Switching light %d state back to OFF", light.id);
            res = switch_light_state(light, OFF);
        }
    }

    return res;
}

day_time_t check_night_time()
{
    time_t now = timeClient.getEpochTime();
    Serial.println("Now time: %u", now);
    time_t sunrise;
    time_t sunset;
    get_sunset_sunrise_time(&sunrise, &sunset);

    if (now > sunrise && now < sunset)
    {
        Serial.println("It's day time");
        return DAY;
    }
    else
    {
        Serial.println("It's night time");
        return NIGHT;
    }
}

void motion_detected_interrupt()
{
    if (!swiching_lights || !(digitalRead(SWITCH_PIN) == LOW))
    // prevents the lights from switching again if PIR interrupt during the delay, or if the switch is on
    {
        day_time_t current_time = check_night_time();
        if (current_time == NIGHT)
        {
            swiching_lights = true;
            switch_lights();
            swiching_lights = false;
        }
    }
}

void setup()
{
    Serial.begin(115200);
    pinMode(PIR_PIN, INPUT);
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(SWITCH_PIN, INPUT_PULLUP);
    wifi_setup();

    Udp.begin(LOCAL_UDP_PORT);
    timeClient.begin();

    // TODO:
    attachInterrupt(digitalPinToInterrupt(PIR_PIN), motion_detected_interrupt, CHANGE);
}

void loop()
{
    // TODO:
    timeClient.update();
    delay(2000);
}
