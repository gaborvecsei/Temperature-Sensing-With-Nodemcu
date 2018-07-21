/*
 * Display the Temperature/Humidity/Heat Index on a mini OLED display
 * and log the values to a private Thingspeak.com channel
 * 
 * NodeMcu v0.9 board
 * 
 * DHT22 temp/humid Sensor
 * GND -> GND
 * VCC -> V3.3
 * DAT -> D6
 * 
 * 128x64 OLED I2C
 * GND -> GND
 * VCC -> V3.3
 * SCL -> D1
 * SDA -> D2
 * 
 */
 

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>
#include <DHT.h>

// OLED setup
#define OLED_RESET D5
#if (SSD1306_LCDHEIGHT != 64)
	#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

// DHT22 setup
#define DHTPIN D6
#define DHTTYPE DHT22
const boolean IS_METRIC = true;
const int UPDATE_INTERVAL_SECONDS = 60;

// WiFi setup
const char* SSID = <YOUR_SSID>;
const char* PASSWORD = <YOUR_PASSWORD>;
const int PORT = 80;

// Thingspeak setup
const char* THINGSPEAK_HOST = "api.thingspeak.com";
const char* THINGSPEAK_API_KEY = <YOUR_THINGSPEAK_API_KEY>;

//////////////////////////////////////////////////////////////////////////////////

DHT dht(DHTPIN, DHTTYPE);
Adafruit_SSD1306 display(OLED_RESET);

void connectToWiFi(const char* ssid, const char* password){
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
}

void setupDisplay(){
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
}

void drawSensorData(const String temperature, const String humidity, const String heatIndex){
  display.clearDisplay();
  drawStr(0, 10, String("Temperature: ") + temperature);
  drawStr(0, 30, String("Humidity: ") + humidity);
  drawStr(0, 50, String("HeatIndex: ") + heatIndex);
  display.display();
}

void drawStr(const uint8_t x, const uint8_t y, const String str){
  display.setCursor(x, y);
  display.println(str);
}

String createThingspeakRequestUrl(const char* api_key, const String temperature, const String humidity){
  String url = "/update?api_key=";
  url += api_key;
  url += "&field1=";
  url += temperature;
  url += "&field2=";
  url += humidity;
  return url;
}

void setup() {
  Serial.begin(115200);
  delay(10);
  
  setupDisplay();
  connectToWiFi(SSID, PASSWORD);
}

void loop() {
  WiFiClient client;
  if (!client.connect(THINGSPEAK_HOST, PORT)) {
    Serial.println("connection failed");
    return;
  }
  
  // Read Sensor Values
  float temperature = dht.readTemperature(!IS_METRIC);
  float humidity = dht.readHumidity();
  float heatIndex = dht.computeHeatIndex(temperature, humidity, !IS_METRIC);

  Serial.println(String("Temperature ") + String(temperature));
  Serial.println(String("Humidity ") + String(humidity));
  Serial.println(String("Heat Index ") + String(heatIndex));

  // Display values on OLED
  drawSensorData(String(temperature), String(humidity), String(heatIndex));

  // Send request to Thingspeak
  String url = createThingspeakRequestUrl(THINGSPEAK_API_KEY, String(temperature), String(humidity));
  client.print(String("GET ") + url + " HTTP/1.1\r\n" + "Host: " + THINGSPEAK_HOST + "\r\n" + "Connection: close\r\n\r\n");
  delay(10);
    while(!client.available()){
      delay(100);
    }

  // ESP.deepSleep(1e6 * UPDATE_INTERVAL_SECONDS);  // For this we need to connect the D0 pint and RST pin
  delay(1000 * UPDATE_INTERVAL_SECONDS);
}
