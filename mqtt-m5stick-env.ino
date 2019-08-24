#include <M5StickC.h>
#include <Wire.h>
#include "DHT12.h"
#include <Adafruit_BMP280.h>
#include <WiFi.h>
#include <PubSubClient.h>

#include "config.h"

DHT12 dht12;
Adafruit_BMP280 bme;

float tmp;
float hum;
int pressure;

WiFiClient wifi_client;
PubSubClient mqtt_client(mqtt_host, mqtt_port, nullptr, wifi_client);

void msg(const std::string &str)
{
  M5.Lcd.setCursor(5, 5, 1);
  M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.setTextSize(2);

  M5.Lcd.printf("%s", str.c_str());
  Serial.println(str.c_str());
}

void reboot() {
  M5.Lcd.fillScreen(RED);
  msg("REBOOT!!!!!");
  delay(3000);
  ESP.restart();
}

void setup() {
  M5.begin();
  Wire.begin();
  M5.Lcd.setRotation(1);
  M5.Axp.ScreenBreath(9); // 7-15

  if (!bme.begin(0x76)) {
    M5.Lcd.fillScreen(RED);
    M5.Lcd.setTextColor(WHITE, BLACK);
    M5.Lcd.setTextSize(2);
    M5.Lcd.printf("ENV sensor is not found...");
    reboot();
  }

  WiFi.mode(WIFI_MODE_STA);
  WiFi.begin(wifi_ssid, wifi_password);
  int count = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
    switch (count) {
      case 0:
        msg("|");
        break;
      case 1:
        msg("/");
        break;
      case 2:
        msg("-");
        break;
      case 3:
        msg("\\");
        break;
    }
    count = (count + 1) % 4;
  }
  msg("WiFi connected!");
  delay(1000);

  bool rv = false;
  if (mqtt_use_auth == true) {
    rv = mqtt_client.connect(mqtt_client_id, mqtt_username, mqtt_password);
  }
  else {
    rv = mqtt_client.connect(mqtt_client_id);
  }
  if (rv == false) {
    msg("mqtt connecting failed...");
    reboot();
  }
  msg("MQTT connected!");
  delay(1000);
  M5.Lcd.fillScreen(BLACK);
}

void loop() {
  if (!mqtt_client.connected()) {
    Serial.println("MQTT disconnected...");
    reboot();
  }
  mqtt_client.loop();

  tmp = dht12.readTemperature();
  hum = dht12.readHumidity();
  pressure = (int)(bme.readPressure() / 100.0f);

  updateDisplay();
  publishSensorData();

  delay(3000);
}

void updateDisplay() {
  M5.Lcd.setCursor(5, 5, 1);
  M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.setTextSize(2);

  M5.Lcd.setCursor(5, 5, 1);
  M5.Lcd.printf("tmp: %2.1f C ", tmp);
  M5.Lcd.setCursor(5, 25, 1);
  M5.Lcd.printf("hum: %2.1f% %% ", hum);
  M5.Lcd.setCursor(5, 45, 1);
  M5.Lcd.printf("p: %d hPa", pressure);
}

void publishSensorData() {
  std::string s = "";

  char buf[128];
  memset(buf, 0, 128);

  snprintf(buf, 123, "{\"temperature\":%.1f,\"humidity\":%.1f,\"pressure\":%d}", tmp, hum, pressure);

  Serial.println(buf);
  mqtt_client.publish(mqtt_publish_topic, buf);

  M5.Lcd.fillRect(150, 70, 10, 10, GREEN);
  delay(500);
  M5.Lcd.fillRect(150, 70, 10, 10, BLACK);
}
