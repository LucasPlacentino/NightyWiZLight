// https://github.com/LucasPlacentino/NightyWiZLight

#include "SECRETS.h"
#include "UTILS.h"

// #include <HTTPClient.h>     // use ESP8266HTTPClient.h instead
// #include <WifiClient.h>     // use ESP8266WiFi.h instead
// #include <TimeLib.h>        // not used ?
#include <ESP8266WiFi.h> // ESP8266 Wifi
// #include <ESP8266HTTPClient.h> // HTTP client // inside OWM conditional compile
#include <WiFiUdp.h>     // UDP communication to WiZ lights
#include <NTPClient.h>   // get time from NTP server
#include <time.h>        // for time_t
#include <ArduinoJson.h> // for parsing OpenWeatherMap API response
#include <StreamUtils.h> // for ReadLoggingStream the http stream

/*
// Required for LIGHT_SLEEP_T delay mode
extern "C"
{
#include "user_interface.h"
}
extern "C"
{
#include "gpio.h"
}
*/

//* ------- HERE ---------
#define DEBUG 1 // comment to disable debug print

light_t lights[2] = {
    {0, OFF, false, 38899, LIGHT_0_IP}, // light 0, init state, UDP port, changed_state(leave false), ip address
    //{1, OFF, false, 38899, LIGHT_1_IP}, // light 1, init state, UDP port, changed_state(leave false), ip address
};
//! YOU CANNOT USE GPIO 16 BECAUSE INTERRUPTS ARE NOT SUPPORTED FOR THIS PIN
const int LIGHTUP_TIME = 20;   // seconds
const int PIR_PIN = A0;        // 12=D6 // PIR sensor pin
const int BTN_PIN = 13;        // D7 // button pin
const int SWITCH_PIN = 14;     // D5 // switch pin
const int PIR_SWITCH_PIN = 15; // D8 // PIR disable switch pin

//* ------- ^^^^ --------

const unsigned int LOCAL_UDP_PORT = 2444;
const char GET_BUFFER[] = "{\"method\":\"getPilot\"}";                           // packet to get the state of a light
const char ON_BUFFER[] = "{\"method\":\"setPilot\",\"params\":{\"state\": 1}}";  // packet to turn ON a light
const char OFF_BUFFER[] = "{\"method\":\"setPilot\",\"params\":{\"state\": 0}}"; // packet to turn OFF a light
char packet_buffer[255];

const char OWM_URL[] = "http://api.openweathermap.org/data/2.5/weather?units=metric&lang=en";
day_time_t day_time = DAY;
// bool switching_lights = false;
// bool motion = false;
const int TIME_CLIENT_UPDATE = 5000; //! DO NOT GO UNDER 1000ms as OWM is rate-limited at 60 calls/min for the free plan

// TODO: debounce ?
unsigned long now_millis = millis();
unsigned long btn_last = 0;
int btn_debounce = 200; // ms
bool btn_flag = false;
unsigned long pir_last = 0;
int pir_debounce = 200; // ms
bool pir_flag = false;  // not used
unsigned long switch_last = 0;
int switch_debounce = 200; // ms
bool switch_flag = false;

WiFiClient client;
// HTTPClient http; // set only when using OWM
WiFiUDP Udp;
// WiFiUDP ntpUDP; //? needed?
NTPClient timeClient(Udp, "pool.ntp.org");
// NTPClient timeClient(ntpUDP, "pool.ntp.org"); //? needed?

#ifdef OWM_API_KEY
#include <ESP8266HTTPClient.h> // HTTP client
HTTPClient http;
void get_sunset_sunrise_time(time_t &sunrise, time_t &sunset, time_t now)
{
    // String json_buffer;
    String server_path;
    server_path = OWM_URL;
    server_path += "&lat=";
    server_path += LATITUDE;
    server_path += "&lon=";
    server_path += LONGITUDE;
    server_path += "&appid=";
    server_path += OWM_API_KEY;
    Serial.println(F("get_sunset_sunrise_time: server_path = ") + String(server_path));

    Serial.println(F("get_sunset_sunrise_time: [HTTP] begin..."));

    http.useHTTP10(true);                                  // force HTTP/1.0, not 1.1, to be able to get http stream
    //http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS); //! DO NOT USE: BREAKS HTTP REQUEST

    Serial.println(F("get_sunset_sunrise_time: server_path.c_str() = ") + String(server_path.c_str()));
    if (http.begin(client, server_path.c_str()))
    { // HTTP

        Serial.println(F("get_sunset_sunrise_time: [HTTP] GET..."));
        int http_response_code = http.GET();
        // String payload = "{}";

        if (http_response_code > 0)
        {
            Serial.println(F("get_sunset_sunrise_time: [HTTP] GET... code: ") + String(http_response_code));

            if (http_response_code == HTTP_CODE_OK || http_response_code == HTTP_CODE_MOVED_PERMANENTLY)
            {
                // payload = http.getString();
                // Serial.println(payload);

                // const size_t capacity = JSON_ARRAY_SIZE(3) + 2 * JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(2) + 3 * JSON_OBJECT_SIZE(4) + JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(6) + JSON_OBJECT_SIZE(12) + 480;
                // DynamicJsonDocument data(capacity);
                DynamicJsonDocument data(96); // from https://arduinojson.org/v6/assistant/
                // StaticJsonDocument<64> filter;
                // filter["sys"] = true;
                StaticJsonDocument<48> filter; // from https://arduinojson.org/v6/assistant/
                JsonObject filter_sys = filter.createNestedObject("sys");
                filter_sys["sunrise"] = true;
                filter_sys["sunset"] = true;
                // Serial.print(F("JSON filter: "));
                // Serial.println(filter);
                ReadLoggingStream loggingStream(http.getStream(), Serial); // prints stream to Serial
                DeserializationError error = deserializeJson(data, loggingStream, DeserializationOption::Filter(filter));
                if (error)
                {
                    Serial.print(F("get_sunset_sunrise_time: deserializeJson() failed: "));
                    Serial.println(error.f_str());
                    return;
                }
                /*
                Serial.println(F("JSON data:"));
                Serial.println(data);

                // shouldn"t use .containsKey() with ArduinoJson 6
                if (!data.containsKey("sunrise") || !data.containsKey("sunset"))
                {
                    Serial.println("Parsing JSON input failed! No sunrise or sunset data");
                    return;
                }
                else
                {
                    Serial.println("Deserializing JSON doc successful");
                }
                */
                sunrise = data["sys"]["sunrise"].as<long>(); // unsigned long?
                Serial.println(F("get_sunset_sunrise_time: Sunrise time: ") + String(sunrise));
                sunset = data["sys"]["sunset"].as<long>(); // unsigned long?
                Serial.println(F("get_sunset_sunrise_time: Sunset time: ") + String(sunset));
            }
            // payload = http.getString();
            // Serial.println(payload);
        }
        else
        {
            Serial.println(F("get_sunset_sunrise_time: [HTTP] GET... failed, error: ") + String(http_response_code) + F(" - ") + http.errorToString(http_response_code).c_str());
        }
        // json_buffer = payload;
        http.end();
    }
    else
    {
        Serial.println(F("get_sunset_sunrise_time: [HTTP] Unable to connect"));
        return;
    }
    /*
    // capacity of OpenWeatherMap API response from ArduinoJson Assistant
    const size_t capacity = JSON_ARRAY_SIZE(3) + 2 * JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(2) + 3 * JSON_OBJECT_SIZE(4) + JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(6) + JSON_OBJECT_SIZE(12) + 480;
    DynamicJsonDocument data(capacity);
    StaticJsonDocument<64> filter;
    filter["sys"] = true;
    Serial.print("JSON filter: ");
    Serial.println(filter);
    // StaticJsonDocument<1169> jsonBuffer;
    // JsonObject &data = jsonBuffer.parseObject(json_buffer);
    // deserializeJson(data, json_buffer);
    deserializeJson(data, json_buffer, DeserializationOption::Filter(filter));
    Serial.println("JSON data:");
    Serial.println(data);
    */
    /*
    if (!data.containsKey("sunrise") || !data.containsKey("sunset"))
    {
        Serial.println("Parsing JSON input failed! No sunrise or sunset data");
        return;
    }
    else
    {
        Serial.println("Deserializing JSON doc successful");
    }
    */
    /*
    if (!data.success())
    {
        Serial.println("Parsing JSON input failed!");
        return;
    }
    */
    /*
    JSONVar data = JSON.parse(json_buffer);
    if (JSON.typeof(data) == "undefined")
    {
        Serial.println("Parsing JSON input failed!");
        return;
    }
    sunrise = (unsigned long) data["sys"]["sunrise"];
    Serial.println("Sunrise time: " + String(sunrise));
    sunset = (unsigned long) data["sys"]["sunset"];
    Serial.println("Sunset time: " + String(sunset));
    */
    /*
    sunrise = data["sys"]["sunrise"].as<long>(); // unsigned long?
    Serial.println("Sunrise time: " + String(sunrise));
    sunset = data["sys"]["sunset"].as<long>(); // unsigned long?
    Serial.println("Sunset time: " + String(sunset));
    */
}

#else
#include <SolarCalculator.h>
void get_sunset_sunrise_time(time_t &sunrise, time_t &sunset, time_t now)
{
    // TODO:
    double latitude = LATITUDE.toDouble(); // toDouble() not working ?
    double longitude = LONGITUDE.toDouble();
    //? transit?
    calcSunriseSunset(now, latitude, longitude, transit, &sunrise, &sunset);
}
#endif

light_state_t get_light_state(int id)
{
    light_state_t current_state = OFF;
    packet_flush();

    IPAddress ip_address;
    ip_address.fromString(lights[id].ip_addr);
    Serial.println(F("get_light_state: ip_address = ") + ip_address.toString() + F(" _ ") + String(lights[id].ip_addr));
    Serial.println(F("get_light_state: port = ") + String(lights[id].port));

    Udp.beginPacket(ip_address, lights[id].port);
    Udp.write(GET_BUFFER);
    Udp.endPacket();

    int packet_size = Udp.parsePacket();
    int timeout = 10;
    while (!packet_size && timeout > 0)
    {
        packet_size = Udp.parsePacket();
        timeout--;
        delay(200);
    }
    if (timeout == 0)
    {
        Serial.println(F("get_light_state: Light get state timed out, ip ") + String(lights[id].ip_addr));
        return OFF;
    }
    if (packet_size)
    {
        int len = Udp.read(packet_buffer, UDP_TX_PACKET_MAX_SIZE); // 255
        if (len < 0)
        {
            packet_buffer[len] = 0;
        }
        Serial.println(F("get_light_state: Packet received:"));
        Serial.println(packet_buffer);

        const int capacity = JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(7);
        StaticJsonDocument<capacity> response;

        DeserializationError err = deserializeJson(response, packet_buffer);
        if (err.code() == DeserializationError::Ok)
        {
            current_state = response["result"]["state"] ? ON : OFF;
            lights[id].state = response["result"]["state"] ? ON : OFF;
        }
    }
    return current_state;
}

bool switch_light_state(int id, light_state_t new_state)
{
    WiFiUDP Udp;
    packet_flush();
    lights[id].changed_state = true;
    bool res = false;
    IPAddress ip_address;
    ip_address.fromString(lights[id].ip_addr);
    built_in_led(true);
    Udp.beginPacket(ip_address, lights[id].port);
    if (new_state == ON)
    {
        Udp.write(ON_BUFFER);
        res = true;
        Serial.println(F("switch_light_state: Set light to ON"));
    }
    else
    {
        Udp.write(OFF_BUFFER);
        res = true;
        Serial.println(F("switch_light_state: Set light to OFF"));
    }
    Udp.endPacket();
    built_in_led(false);
    return res;
}

void packet_flush()
{
    // parse all packets in the RX buffer to clear it
    while (Udp.parsePacket())
    {
        ;
    }
    debug_println(F("packet_flush: Packets flushed"));
}

bool wifi_setup()
{
    WiFi.mode(WIFI_STA);
    String hostname = "ESP8266-WiZcontroller";
    WiFi.hostname(hostname.c_str());
    Serial.println(F("WiFi hostname: ") + hostname);
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

    Serial.print(F("Signal strength (RSSI): "));
    Serial.print(WiFi.RSSI());
    Serial.println(F(" dBm"));
    return true;
}

bool switch_lights()
{
    // switching_lights = true;
    bool res = false;

    /*
    // --- single light ---
    light_state_t current_state = get_light_state(lights[0]);
    //light.changed_state = current_state == OFF ? true : false;
    Serial.println(F("Switching light ") + String(lights[0].id) + F(" state to ON"));
    if (current_state == ON)
    {
        lights[0].changed_state = false;
        Serial.println(F("(light was already ON, will not turn off after)"));
    } else
    {
        lights[0].changed_state = true;
        res = switch_light_state(lights[0], ON);
        Serial.println(F("Light ") + String(lights[0].id) + F(" will turn back off"));
    }
    Serial.println(lights[0].changed_state);
    // -----------------------
    */

    
    // --- multiple lights ---
    for (int i; i < sizeof(lights) / sizeof(lights[0]); i++)
    {
        //light_state_t current_state = get_light_state(lights[i].id);
        light_state_t current_state = lights[i].state; // light[0].state is updated regularly inside loop()

        ////light.changed_state = current_state == OFF ? true : false;
        Serial.println(F("switch_lights: Switching light ") + String(lights[i].id) + F(" state to ON"));
        if (current_state == ON)
        {
            lights[i].changed_state = false;
            Serial.println(F("switch_lights: (light was already ON, will not turn off after)"));
        } else
        {
            lights[i].changed_state = true;
            res = switch_light_state(lights[i].id, ON);
            Serial.println(F("switch_lights: Light ") + String(lights[i].id) + F(" will turn back off"));
        }
        Serial.println(F("switch_lights: ") + String(lights[i].changed_state));
        //res = switch_light_state(light, ON);
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
    Serial.println(F("switch_lights: Sleeping for ") + String(LIGHTUP_TIME) + F(" seconds"));
    delay(LIGHTUP_TIME * 1000);

    
    for (int j; j < sizeof(lights) / sizeof(lights[0]); j++)
    {
        //light_t light = switching_lights[j];
        Serial.println(F("switch_lights: light") + String(lights[j].id) + String(lights[j].ip_addr) + lights[j].changed_state ? F("changed") : F("not changed"));
        Serial.println(F("switch_lights: Light power OFF"));
        if (lights[j].changed_state)
        {
            Serial.println(F("switch_lights: Switching light ") + String(lights[j].id) + F(" state back to OFF"));
            res = switch_light_state(lights[j].id, OFF);
            lights[j].changed_state = false;
        }
    }
    
    // switching_lights = false;
    Serial.println(F("switch_lights: Finished switching lights"));
    return res;
}

day_time_t check_night_time()
{
    time_t now = timeClient.getEpochTime();
    Serial.println(F("check_night_time: Now time: ") + String(now));
    time_t sunrise;
    time_t sunset;
    get_sunset_sunrise_time(sunrise, sunset, now);

    if (now > sunrise && now < sunset)
    {
        Serial.println(F("check_night_time: It's day time"));
        return DAY;
    }
    else
    {
        Serial.println(F("check_night_time: It's night time"));
        return NIGHT;
    }
}

void IRAM_ATTR pir_motion_isr() // not used
{
    pir_flag = true;
}

void pir_motion_handler()
{
    now_millis = millis();
    if (now_millis - pir_last > pir_debounce)
    {
        Serial.println(F("pir_motion_handler: Motion detected"));
        delay(10);
        /*
        if (digitalRead(PIR_SWITCH_PIN) == HIGH)
        // if (!switching_lights && (digitalRead(PIR_SWITCH_PIN) == HIGH) && !motion) // doesn't have to check if switching is finished because can't interrupt during an ISR
        // prevents the lights from switching again if PIR interrupt during the delay, or if the switch is on
        {
            // motion = true;
            timeClient.update();
            day_time_t current_time = check_night_time();
            if (current_time == NIGHT)
            {
                switch_lights();
            }
        }
        */

        /*
        else if (motion && (digitalRead(PIR_SWITCH_PIN) == LOW))
        {
            motion = false;
        }
        */

        // motion = true;

        //timeClient.update();
        //day_time_t current_time = check_night_time();
        day_time_t current_time = day_time; // day_time is updated regularly inside loop()
        if (current_time == NIGHT)
        {
            switch_lights();
        }
    }
    pir_flag = false; // not used
}

void IRAM_ATTR btn_isr()
{
    btn_flag = true;
}

void btn_handler()
{
    now_millis = millis();
    if (now_millis - btn_last > btn_debounce)
    {
        Serial.println(F("btn_handler: Button pressed"));
        delay(10);
        // same as pir_motion_handler() but without night check
        switch_lights();
        /*
        if (!switching_lights) // doesn't have to check if switching is finished because can't interrupt during an ISR
        {
            switch_lights();
        }
        */
    }
    btn_flag = false;
}

void IRAM_ATTR switch_isr()
{
    switch_flag = true;
}

void switch_handler()
{
    now_millis = millis();
    if (now_millis - switch_last > switch_debounce)
    {
        delay(10);
        Serial.println(F("switch_handler: Switch activated"));
        // TODO: ? put logic outside of interrupt func?
        /*
        now_millis = millis();
        if (now_millis - switch_last < switch_debounce)
        {
            return;
        } else
        {
            switch_last = now_millis;
            switch_interrupted = true;
            finished = false;
        }
        */

        if (digitalRead(SWITCH_PIN) == LOW)
        {
            Serial.println(F("switch_handler: Switching lights ON"));
            for (int i; i < sizeof(lights) / sizeof(lights[0]); i++)
            {
                Serial.println(F("switch_handler: Switching light ") + String(lights[i].id) + F(" state to ON"));
                switch_light_state(lights[i].id, ON);
            }
        }
        else if (digitalRead(SWITCH_PIN) == HIGH)
        {
            Serial.println(F("switch_handler: Switching lights OFF"));
            for (int i; i < sizeof(lights) / sizeof(lights[0]); i++)
            {
                Serial.println(F("switch_handler: Switching light ") + String(lights[i].id) + F(" state to OFF"));
                switch_light_state(lights[i].id, OFF);
            }
        }
    }
    switch_flag = false;
}

void setup()
{
    Serial.begin(115200);
    Serial.println(F("-----------------------------"));
    Serial.println(F("setup() start"));
    delay(200);

    pinMode(PIR_PIN, INPUT); // TODO: ? use an hardware pulldown resistor for PIR?
    pinMode(BTN_PIN, INPUT_PULLUP);
    pinMode(LED_BUILTIN, OUTPUT);
    built_in_led(false);
    pinMode(SWITCH_PIN, INPUT_PULLUP);
    pinMode(PIR_SWITCH_PIN, INPUT_PULLUP);

    wifi_setup();
    // wifi_set_sleep_type(LIGHT_SLEEP_T);
    Udp.begin(LOCAL_UDP_PORT); // begin in switch_lights() or switch_light_state() instead?
    timeClient.begin();

    Serial.println(F("Light(s) setup:"));
    for (int i; i < sizeof(lights) / sizeof(lights[0]); i++)
    {
        Serial.println(F("Light ") + String(lights[i].id) + F(" (IP: ") + String(lights[i].ip_addr) + F(") setup."));
    }

    // attachInterrupt(digitalPinToInterrupt(PIR_PIN), pir_motion_isr, CHANGE); // do polling for PIR sensor instead of interrupt
    attachInterrupt(digitalPinToInterrupt(BTN_PIN), btn_isr, FALLING);
    attachInterrupt(digitalPinToInterrupt(SWITCH_PIN), switch_isr, CHANGE);
    Serial.println(F("Attached interrupts"));

    Serial.println(F("setup() done"));

    // test:
    // motion_detected_interrupt();

    Serial.println(F("-----------------------------"));
    delay(100);
}

long loop_last = 0;
long loop_now = 0;

void loop()
{
    //Serial.println(analogRead(PIR_PIN));
    //if (analogRead(PIR_PIN) > 850) // polling PIR instead of interrupt
    if (digitalRead(PIR_PIN) == HIGH || analogRead(PIR_PIN) > 850) // polling PIR instead of interrupt
    {
        pir_motion_handler();
    }
    /*
    if (pir_flag)
    {
        pir_motion_handler();
    }
    */
    if (btn_flag)
    {
        btn_handler();
    }
    if (switch_flag)
    {
        switch_handler();
    }
    loop_now = millis();
    if (loop_now - loop_last > TIME_CLIENT_UPDATE)
    {
        loop_last = loop_now;
        timeClient.update();
        debug_println(F("loop: Looping, polling PIR..."));
        lights[0].state = get_light_state(lights[0].id);
        debug_println(F("Light 0 state: ") + String(lights[0].state));
        day_time = check_night_time();
        debug_println(F("Day/night time: ") + String((day_time == DAY) ? F("DAY") : F("NIGHT")));
    }

    // wifi_setup(); // in loop() because we use light sleep
    // TODO: sould or should not get here?
    // timeClient.update();
    // Serial.println("~");
    // Serial.println(F("Sleeping..."));
    // light_sleep(); // enter light sleep mode
    // continues here once woken up
}

void built_in_led(bool state)
{
    digitalWrite(LED_BUILTIN, state ? LOW : HIGH);
}

// TODO: add disableable debug print?
void debug_print(String str)
{
#ifdef DEBUG
    Serial.print(str);
#endif
}

void debug_println(String str)
{
#ifdef DEBUG
    Serial.println(str);
#endif
}

// not used
#ifdef LIGHT_SLEEP_ENABLED
void light_sleep()
{
    wifi_station_disconnect();
    wifi_set_opmode_current(NULL_MODE);
    wifi_fpm_set_sleep_type(LIGHT_SLEEP_T); // set sleep type, the above posters wifi_set_sleep_type() doesn't seem to work for me although it did let me compile and upload with no errors
    wifi_set_sleep_type(LIGHT_SLEEP_T);     // doesn't do anything ?
    wifi_fpm_open();                        // Enables force sleep
    wifi_fpm_set_wakeup_cb(light_sleep_wakeup_callback);
    gpio_pin_wakeup_enable(GPIO_ID_PIN(2), GPIO_PIN_INTR_LOLEVEL); // GPIO_ID_PIN(2) corresponds to GPIO2 on ESP8266-01 , GPIO_PIN_INTR_LOLEVEL for a logic low, can also do other interrupts, see gpio.h above
    // wifi_fpm_do_sleep(0xFFFFFFF);                                  // Sleep for longest possible time
    wifi_fpm_do_sleep(FPM_SLEEP_MAX_TIME); // Sleep for longest possible time
}
#endif