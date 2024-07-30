#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "SSID";
const char* password = "Password";

WebServer server(80);

uint8_t mac[6] = {0x93, 0x29, 0x1B, 0xBF, 0x61, 0x5D};

const int sensorPin = 19;

int counter = 0;
bool gndConnected = false;

unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;
unsigned long lastSettingsChangeTime = 0;
const unsigned long settingsChangeDelay = 3000; // 3 seconds

// Function prototypes
void handleRoot();
void handleCounter();
void handleReset();
void handleSettings();

void handleRoot() {
  // Sayaç değerini sıfırla
  counter = 0;

  String html = "<html><head>";
  html += "<meta charset='UTF-8'>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; text-align: center; }";
  html += "h1 { color: #333; }";
  html += "#counter { font-size: 288px; margin: 20px 0; color: #f39c12; }";
  html += "button { padding: 10px 20px; font-size: 20px; background-color: #3498db; color: white; border: none; cursor: pointer; }";
  html += "button:hover { background-color: #2980b9; }";
  html += "</style>";
  html += "<script>";
  html += "function resetCounter() {";
  html += "  var xhr = new XMLHttpRequest();";
  html += "  xhr.open('POST', '/reset', true);";
  html += "  xhr.onload = function() {";
  html += "    if (xhr.status == 200) {";
  html += "      updateCounter();";
  html += "    }";
  html += "  };";
  html += "  xhr.send();";
  html += "}";
  html += "function updateCounter() {";
  html += "  var xhrUpdate = new XMLHttpRequest();";
  html += "  xhrUpdate.onreadystatechange = function() {";
  html += "    if (xhrUpdate.readyState == 4 && xhrUpdate.status == 200) {";
  html += "      document.getElementById('counter').innerText = xhrUpdate.responseText;";
  html += "    }";
  html += "  };";
  html += "  xhrUpdate.open('GET', '/counter', true);";
  html += "  xhrUpdate.send();";
  html += "}";
  html += "function applySettings() {";
  html += "  var delay = document.getElementById('debounceDelay').value;";
  html += "  var xhr = new XMLHttpRequest();";
  html += "  xhr.open('POST', '/settings?debounceDelay=' + delay, true);";
  html += "  xhr.onload = function() {";
  html += "    if (xhr.status == 200) {";
  html += "      alert('Ayarlar uygulandı. Yeniden başlatmak için sayfayı yenileyin.');";
  html += "    }";
  html += "  };";
  html += "  xhr.send();";
  html += "}";
  html += "setInterval(updateCounter, 50);"; // Her 50 milisaniyede bir sayaç güncelleniyor
  html += "</script>";
  html += "</head><body>";
  html += "<h1>Sayım Kontrol</h1>";
  html += "<h2 id='counter'>" + String(counter) + "</h2>";
  html += "<button onclick='resetCounter()'>Sıfırla</button>";

  // Sensör algılama hızı ayarı
  html += "<h2>Sensör Algılama Hızı Ayarla</h2>";
  html += "<label for='debounceDelay'>Debounce Gecikme (ms):</label>";
  html += "<input type='number' id='debounceDelay' value='" + String(debounceDelay) + "' min='1' max='1000'>";
  html += "<button onclick='applySettings()'>Ayarları Uygula</button>";

  // Bağlantı bilgileri
  html += "<h2>Bağlantı Bilgileri</h2>";
  html += "<p>SSID: " + String(WiFi.SSID()) + "</p>";
  html += "<p>IP Adresi: " + WiFi.localIP().toString() + "</p>";

  html += "</body></html>";

  server.send(200, "text/html", html);
}

void handleCounter() {
  server.send(200, "text/plain", String(counter));
}

void handleReset() {
  counter = 0;
  server.send(200, "text/plain", "Sayaç sıfırlandı ve varsayılan ayarlar uygulandı");
  // Resetten sonra ana sayfaya yönlendirme
  server.sendHeader("Location", "/", true);
  server.send(303);
}

void handleSettings() {
  String response = "{";
  response += "\"debounceDelay\": " + String(debounceDelay);
  response += "}";
  server.send(200, "application/json", response);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  WiFi.begin(ssid, password);
  Serial.print("Bağlanılıyor");
  
  // Wi-Fi bağlantısı başarılı olana kadar bekleniyor
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("");
  Serial.print("Bağlandı: ");
  Serial.println(WiFi.localIP());

  esp_base_mac_addr_set(mac);

  pinMode(sensorPin, INPUT_PULLUP);

  // Sayaç sıfırlama
  counter = 0;

  // Web sunucusunda URL işlemleri tanımlanıyor
  server.on("/", HTTP_GET, handleRoot);
  server.on("/reset", HTTP_POST, handleReset);
  server.on("/counter", HTTP_GET, handleCounter);
  server.on("/settings", HTTP_GET, handleSettings);

  server.begin();
  Serial.println("HTTP sunucu başlatıldı");
}

void loop() {
  server.handleClient();

  int sensorValue = digitalRead(sensorPin);

  unsigned long currentTime = millis();

  if (sensorValue == LOW && !gndConnected && (currentTime - lastDebounceTime) > debounceDelay) {
    counter++;
    gndConnected = true;
    lastDebounceTime = currentTime;
  } else if (sensorValue == HIGH) {
    gndConnected = false;
  }
}
