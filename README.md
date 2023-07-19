# NightyWiZLight
ESP8266 Arduino program to turn on WiZ lights when motion is detected only if time is after sunset (and before sunrise)
> tested with a **Wemos D1 Mini _clone_** and a **WiZ Linear Light**

## Set up
### Set your secrets in a `SECRETS.h` file, using [`SECRETS.h.template`](SECRETS.h.template) as a template:
- **WiFi SSID**
- **WiFi password**
- **WiZ Light IP address** (please make a DHCP reservation for your WiZ light on your router, to make sure it has a static LAN IP address)
- **OpenWeatherMap API key**, to retrieve local sunrise and sunset data
- **Latitude**, used for getting the sunrise and sunset time locally using OpenWeatherMap
- **Longitude**, used for getting the sunrise and sunset time locally using OpenWeatherMap

```C
// SECRETS.h
#ifndef SECRETS_h
#define SECRETS_h

// ---- Secrets ----

#define LIGHT_0_IP "192.168.0.50" // IP address of light id 0
//#define LIGHT_1_IP "192.168.0.51"  // optional: IP address of light id 1

#define WIFI_SSID "your_wifi_name"
#define WIFI_PASS "your_wifi_password"
#define OWM_API_KEY "your_openweathermap_api_key"
#define LATITUDE "50.85" // your latitudinal coordinate
#define LONGITUDE "4.35" // your longitudinal coordinate
//#define COUNTRY "BE" // NOT USED your contry two-letter code
//#define CITY "Brussels" // NOT USED your city

#endif // SECRETS_h
```

### Change these settings to suit your situation:
- **lights**: add additional lights, only changing their id (first number in the decleration of each instance), and their LIGHT_id_IP (using the ones defined in `SECRETS.h`)
- **LIGHTUP_TIME**: delay during which the lights will stay on before turning off again
- **PIR_PIN**: pin you connected your PIR sensor to, used to detect motion and activate the lights for a delay
- **BTN_PIN**: pin you connected your button (momentary, SPST) to, used to activate the lights for a delay
- **SWITCH_PIN**: pin you connected your switch (SPDT) to, used to turn on and off the lights
- **PIR_SWITCH_PIN**: pin you connected your PIR disabling switch (SPDT) to, used to deactivate the PIR function (deactivate motion detection)
- **DEBUG**: uncomment if your want more debug logs to the serial console
```C
//* ------- HERE ---------
//#define DEBUG 0 // comment to disable debug print

light_t lights[2] = {
    {0, OFF, false, 38899, LIGHT_0_IP}, // light 0, init state, changed_state(leave false), UDP port, ip address
    //{1, OFF, false, 38899, LIGHT_1_IP}, // light 1, init state, changed_state(leave false), UDP port, ip address
};
//! YOU CANNOT USE GPIO 16 BECAUSE INTERRUPTS ARE NOT SUPPORTED FOR THIS PIN
const int LIGHTUP_TIME = 20;   // seconds
const int PIR_PIN = A0;        // A0 // PIR sensor pin
const int BTN_PIN = 13;        // D7 // button pin
const int SWITCH_PIN = 14;     // D5 // switch pin
const int PIR_SWITCH_PIN = 15; // D8 // PIR disable switch pin

//* ------- ^^^^ --------
```

## Author
[Lucas Placentino](https://github.com/LucasPlacentino)

## Acknowledgement
Thanks to Matt Taylor for their example Arduino program showcasing how to interact with a local WiZ light using UDP with an ESP8266 (found on [May Márquez](https://github.com/mmarquez76)'s Gist):  
https://gist.github.com/mmarquez76/a1debed9ed898eafca6f8ce1e4f99ce3

## License
[MIT](/LICENSE)
