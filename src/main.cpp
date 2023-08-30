#include <ESPmDNS.h>
#include <SPIFFS.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiClient.h>

using namespace std;

WebServer server(80);

#define LED 2

vector<String> readCredentials() {
  File file = SPIFFS.open("/credentials.txt", "r");

  if (!file) {
    Serial.println("Failed to open file for reading");
  }
  vector<String> v;
  while (file.available()) {
    v.push_back(file.readStringUntil('\n'));
  }
  file.close();
  Serial.println("SSID: ");
  Serial.print("[");
  Serial.print(v[0]);
  Serial.print("]");
  Serial.println();
  Serial.print("Password: ");
  Serial.print("[");
  Serial.print(v[1]);
  Serial.print("]");
  Serial.println();
  return v;
}

void handleHello() {
  File file = SPIFFS.open("/hello.html", "r");
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }
  server.streamFile(file, "text/html");
  file.close();
}

void handleRoot() {
  digitalWrite(LED, 1);
  server.send(200, "text/plain", "hello from esp32!");
  digitalWrite(LED, 0);
}

void handleNotFound() {
  digitalWrite(LED, 1);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  digitalWrite(LED, 0);
}

void setup(void) {
  Serial.begin(115200);

  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
  }

  vector<String> credentials = readCredentials();

  pinMode(LED, OUTPUT);
  digitalWrite(LED, 0);

  WiFi.mode(WIFI_STA);
  WiFi.begin(credentials[0], credentials[1]);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(credentials[0]);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("esp32")) {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);

  server.on("/hello", handleHello);

  server.on("/inline",
            []() { server.send(200, "text/plain", "this works as well"); });

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
}

void loop(void) {
  server.handleClient();
  delay(2); // allow the cpu to switch to other tasks
}