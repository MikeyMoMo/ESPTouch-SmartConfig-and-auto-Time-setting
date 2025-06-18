//#define CONFIG_FOR_JOE

#define STRCMP(a, b) (!strcmp((a), (b)))  // Thank you, CoPilot!  I am weak on macros.

#include <WiFi.h>
String chSSID;
String chPassword;
bool   CredsSet;
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "zones.h"

#include "Preferences.h"
Preferences preferences;
#define RO_MODE true     // Preferences calls Read-only mode
#define RW_MODE false    // Preferences calls Read-write mode
#include <esp_sntp.h>    // Get UTC epoch here.

#include <GeoIP.h>
GeoIP  geoip;            // create GeoIP object 'geoip'
location_t loc;          // data structure to hold geoip results
bool   zoneFound = false;

int    looper    = 100;  // Over 40 means connection problem.
int    iDOM, iMonth, iYear, iHour, iMin, iSec, iPrev_Sec = -1;
time_t TS_Epoch = 0, UTC;
struct tm timeinfo;
char   chBuffer[100];    // Work area for sprintf, etc.
/*******************************************************************************************/
void setup()
/*******************************************************************************************/
{
  Serial.begin(115200); delay(5000);

  Serial.println("Initializing WiFi");
  initWiFi();
  Serial.println(); Serial.println("Done with initWifi\r\n");

  Serial.println("Getting Geo information by IP address.");
  IPbyGeo();
  Serial.println(); Serial.println("Done with IPbyGeo");

  Serial.println("Finding geoIP zone, setting that zone and getting time.");
  initTime();
  Serial.println("Done with initTime");
}
/*******************************************************************************************/
void loop()
/*******************************************************************************************/
{
  // Just to prove something happened.
  getLocalTime(&timeinfo);  // This is what I was after.  Local time!
  
  iMonth = timeinfo.tm_mon + 1;
  iDOM   = timeinfo.tm_mday;
  iYear  = timeinfo.tm_year + 1900;

  iHour  = timeinfo.tm_hour;
  iMin   = timeinfo.tm_min;
  iSec   = timeinfo.tm_sec;
  
  if (iPrev_Sec == iSec) return;  // Wait for the next second to roll around...
  iPrev_Sec = iSec;

  Serial.printf("It's %02i/%02i/%02i %02i:%02i:%02i\r\n",
                iMonth, iDOM, iYear, iHour, iMin, iSec);

  // Keep checking connection status
  // I am not sure SmartConfig is the correct process to use.  It might be better to do this:
  //   WiFi.begin(chSSID, chPassword); delay(1500);
  //   looper = 0;
  //   while (WiFi.status() != WL_CONNECTED) {
  //     delay(500); Serial.print("W");
  //     if (looper++ > 40) {
  //       Serial.println();
  //       break;  // 20 seconds should be enough!
  //     }
  //   }
  //  ...but this is what was coded in the sample this is based off of from Espressif.
  //  You can decide and change it if you wish.  I will test this and update this
  //   source, later, if it appears better the other way.  The source for WiFi is above
  //   my pay grade so I will leave this until proven wrong.

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi lost, reconnecting...");
    WiFi.beginSmartConfig();
    while (!WiFi.smartConfigDone()) {
      delay(500);
      Serial.print(".");
    }
  }
}
/*******************************************************************************************/
void initTime()
/*******************************************************************************************/
{
  sntp_set_sync_interval(21601358);  // Get updated time every 6 hours plus a little.
  sntp_set_time_sync_notification_cb(timeSyncCallback);
  sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH);

  int zoneCt = sizeof(zones) / sizeof(zones[0]);
  Serial.printf("There are %i zones in the table.\r\n", zoneCt);

  for (int i = 0; i < zoneCt; i++) {
    Serial.printf("Comparing %s to %s\r\n", zones[i].name, loc.timezone);
    if (STRCMP(zones[i].name, loc.timezone)) {
      Serial.println("Found a match.");
      configTzTime(zones[i].zone, "pool.ntp.org");  // Use the env string from the match.
      zoneFound = true;
      break;
    }
  }
  if (!zoneFound) {
    Serial.printf("Zone %s not found. Cannot continue.\r\n", loc.timezone);
    while (1);
  }
  Serial.print("Waiting for correct time..."); delay(5000);
  while (!TS_Epoch); // Wait for time hack. Probably sufficient but...
  // I will double check by checking the year is reasonable.
  strftime(chBuffer, sizeof(chBuffer), "%Y", localtime(&UTC));
  iYear = atoi(chBuffer);
  int iLooper = 0;
  while (iYear < 2025) {
    Serial.print(".");
    time(&UTC);
    strftime (chBuffer, 100, "%Y", localtime(&UTC));
    iYear = atoi(chBuffer);
    if (iLooper++ > 30) {  // Wait one minute for time hack.
      Serial.println("\r\nCannot get time set. Rebooting.");
      ESP.restart();
    }
    delay(2000);
  }
  Serial.println();
}
/*******************************************************************************************/
void initWiFi()
/*******************************************************************************************/
{
  // First, I will see if there are saved credentials.  I have to do this myself
  //  since, after 6 years, this tiny thing has not been coded into the ESP
  //  SmartConfig code.  I guess they have been busy... FOR SIX YEARS!
  Serial.println("Reading from saved preferences.");
  preferences.begin("WiFiConf", RO_MODE);
  CredsSet = preferences.getBool("CredsSet", false);  // Have they been set already?
  if (CredsSet) {  // If so, use what is there.
    Serial.println("Credentials claim to be set.");
    chSSID = preferences.getString("SSID", "NoWAP");
    chPassword = preferences.getString("Password", "NoPass");
    preferences.end();
    Serial.printf("SSID: %s, PW: %s\r\n", chSSID, chPassword);

    // If we have something reasonable, try it.
    if (chSSID != "NoWAP") {
      WiFi.begin(chSSID, chPassword); delay(1500);
      looper = 0;
      while (WiFi.status() != WL_CONNECTED) {
        delay(500); Serial.print("W");
        if (looper++ > 40) {
          Serial.println();
          break;  // 20 seconds should be enough!
        }
      }
    }
  }
  // looper init'd to 100.  If still over 40, there was some error. Use SmartConfig.
  // Either credentials were not set or they are outdated and no connection is
  //  possible and it ran out of the 40 tries.
  if (looper > 40) {
    Serial.println("Credentials not set or cannot connect to a WAP.  "
                   "Starting SmartConfig.");
    // Set to STA mode and initialize SmartConfig
    WiFi.mode(WIFI_STA); // Station mode
    WiFi.beginSmartConfig();

    Serial.println("Waiting for SmartConfig...");
    while (!WiFi.smartConfigDone()) {
      delay(500);
      Serial.print("S");
    }
    Serial.print("\nSmartConfig info received. Connected to: ");
    Serial.printf("%s/%s\r\n", WiFi.SSID(), WiFi.psk());
    Serial.print("at IP Address: "); Serial.println(WiFi.localIP());

    preferences.begin("WiFiConf", RW_MODE);
    preferences.putString("SSID", WiFi.SSID());
    preferences.putString("Password", WiFi.psk());
    preferences.putBool("CredsSet", true);
    preferences.end();
    ESP.restart();
  }
}
/*******************************************************************************************/
void IPbyGeo()
/*******************************************************************************************/
{
  loc.status = false;
  uint8_t count = 0;

  while (!loc.status && count < 21)     // Try up to 20 times to connect to geoip server.
    // Usually it happens on the first try.
  {
    count++;
    Serial.printf("Attempt %u of 20 to get Geo info.\r\n", count);

    // Use one of the following function calls.
    // If using API key it must be inside double quotation marks.

    // no key, results not shown on serial monitor
    loc = geoip.getGeoFromWiFi();

    // no key, results not shown on serial monitor
    //loc = geoip.getGeoFromWiFi(false);

    // no key, show results on on serial monitor
    //loc = geoip.getGeoFromWiFi(true);

    // use API key, results not shown on serial monitor
    //loc = geoip.getGeoFromWiFi("Your API Key");

    // use API key, results not shown on serial monitor
    //loc = geoip.getGeoFromWiFi("Your API Key", false);

    // use API key, show results on on serial monitor
    //loc = geoip.getGeoFromWiFi("Your API Key", true);
  }
  if (loc.status)      // Check to see if the data came in from the server.
  {
    // Display information from GeoIP.
    // The library can do this too if true is added to the function call.

    Serial.print("\nLatitude: ");               Serial.println(loc.latitude);   // float
    Serial.print("Longitude: ");                Serial.println(loc.longitude);  // float
    Serial.print("City: ");                     Serial.println(loc.city);       // char[24]
    Serial.print("Region: ");                   Serial.println(loc.region);     // char[24]
    Serial.print("Country: ");                  Serial.println(loc.country);    // char[24]
    Serial.print("Timezone: ");                 Serial.println(loc.timezone);   // char[24]
    // int  (eg. -1000 means -10 hours, 0 minutes)
    Serial.print("UTC Offset: ");               Serial.println(loc.offset);
    Serial.print("Offset Seconds: ");           Serial.println(loc.offsetSeconds); // long
  }
  else
  {
    Serial.println("Data received was not valid.");
  }
}
/*******************************************************************************************/
void timeSyncCallback(struct timeval * tv)
/*******************************************************************************************/
{
  //  struct timeval {  // Instantiated as "*tv"
  //    Number of whole seconds of elapsed time.
  //   time_t      tv_sec;
  //    Number of microseconds of rest of elapsed time minus tv_sec.
  //     Always less than one million.
  //   long int    tv_usec;
  //  Serial.print("\n** > Time Sync Received at ");
  //  Serial.println(ctime(&tv->tv_sec));
  TS_Epoch = tv->tv_sec;
}
