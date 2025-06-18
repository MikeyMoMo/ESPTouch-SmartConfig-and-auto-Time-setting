# ESPTouch-SmartConfig-and-auto-Time-setting
How to use the ESPTouch app to supply an SSID/PW pair to an ESP32, save it and set the time based on IP geolocation.

ESPTouch is a parallel process to WiFiManager and has similarities and differences.  In this example, it is used to submit SSID and password for your WAP so it can connect to the internet.  The original code does not store nor recall these two items.  It was reported SIX YEARS ago and has not been addressed and, of course, not fixed.  Maybe they have been busy for SIX YEARS!!!  This code remedies that problem.  It uses the SSID and password to connect and, if successful, saves them and uses them after a reboot or power cycle.  If you change SSID or password, the code will attempt to use the old for 20 seconds then will drop into SmartConfig mode to accept the new credentials and save them.

To use this code, you will need the ESPTouch app for your phone.  Start the code and then start the ESPTouch app.  Be sure to be in Broadcast mode in the app.  It will assume the SSID you are connected to on your phone is the one you want to use.  If that is not the one you want to use, change to a different WAP on your phone then exit and restart the app so it can pick up the new SSID.  Be sure it is a 2G WAP (ESP32 limitation as of this writing) and supply the password.  For WAPs that use the same SSID for 2G and 5G, you don't have to choose.  The correct one will be used.  The credentials will be sent to the ESP32 device and a connection attempt will be made.  If it is successful, the credentials will be saved and used next time the code is run.  The process will wait for credentials to be sent forever or until the power goes off or the earth goes flat.

This example code will (hopefully) connect to your desired WAP.  Then, it will send a query to ipapi.co to obtain your location and the timezone identifier for that location.  It will then look up the environmental time string in the zones array for your location and activate it.

It will then query ntp.org to get the local time for your location and print it each second.  That's all it does and that is simply to prove that the entire process worked.  Add your own code after that to accomplish your mission objective.

As always, if you see any error or want any updates or produce any updates, please let me know.  I am always glad to talk about programming.

Mikey the Midnight Coder

Additional information on SmartConfig (not needed to use this example):
https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/network/esp_smartconfig.html
https://espressif.github.io/esp32-c3-book-en/chapter_7/7.3/7.3.3.html
