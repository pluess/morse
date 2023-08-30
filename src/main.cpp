#include <ESPmDNS.h>
#include <SPIFFS.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <algorithm>
#include <string>

using namespace std;

WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

#define LED 2
#define USE_SERIAL Serial

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

void hexdump(const void *mem, uint32_t len, uint8_t cols = 16) {
  const uint8_t *src = (const uint8_t *)mem;
  USE_SERIAL.printf("\n[HEXDUMP] Address: 0x%08X len: 0x%X (%d)",
                    (ptrdiff_t)src, len, len);
  for (uint32_t i = 0; i < len; i++) {
    if (i % cols == 0) {
      USE_SERIAL.printf("\n[0x%08X] 0x%08X: ", (ptrdiff_t)src, i);
    }
    USE_SERIAL.printf("%02X ", *src);
    src++;
  }
  USE_SERIAL.printf("\n");
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload,
                    size_t length) {

  switch (type) {
  case WStype_DISCONNECTED:
    USE_SERIAL.printf("[%u] Disconnected!\n", num);
    break;
  case WStype_CONNECTED: {
    IPAddress ip = webSocket.remoteIP(num);
    USE_SERIAL.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0],
                      ip[1], ip[2], ip[3], payload);

    // send message to client
    webSocket.sendTXT(num, "Connected");
  } break;
  case WStype_TEXT:
    USE_SERIAL.printf("[%u] get Text: %s\n", num, payload);

    // send message to client
    webSocket.sendTXT(num, "received a message");

    // send data to all connected clients
    // webSocket.broadcastTXT("message here");
    break;
  case WStype_BIN:
    USE_SERIAL.printf("[%u] get binary length: %u\n", num, length);
    hexdump(payload, length);

    // send message to client
    // webSocket.sendBIN(num, payload, length);
    break;
  case WStype_ERROR:
  case WStype_FRAGMENT_TEXT_START:
  case WStype_FRAGMENT_BIN_START:
  case WStype_FRAGMENT:
  case WStype_FRAGMENT_FIN:
    break;
  }
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

void websocketHtmlPage() {
  File file = SPIFFS.open("/websocket.html", "r");
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
  server.on("/websocket", websocketHtmlPage);

  server.on("/inline",
            []() { server.send(200, "text/plain", "this works as well"); });

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  Serial.println("WebSocket server started");
}

void loop(void) {
  server.handleClient();
  webSocket.loop();

  delay(2); // allow the cpu to switch to other tasks
}