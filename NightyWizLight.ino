// https://github.com/LucasPlacentino/NightyWiZLight

#define USE_OPENWEATHERMAP 1

#include <WiFi.h>
#include <HTTPClient.h>
#include <Time>
#include "SECRETS.h"

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

void setup()
{
  //
}

void loop()
{
  //
}
