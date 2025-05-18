#include "AirFreshenerMotor.h"
#include "SPIFFSManager.h"
#include "WebHandler.h"
#include "Button.h"

#define MOTOR_PIN_IA 18
#define MOTOR_PIN_IB 19

const char* ssid         = "robotx";
const char* wifiPassword = "78914040";
const char* webPassword  = "show_niger";

AirFreshenerMotor motor(MOTOR_PIN_IA, MOTOR_PIN_IB);
SPIFFSManager      spiffs;          

int remainingCooldown;
String lastActivation;

WebHandler web(ssid,
               wifiPassword,
               webPassword,
               spiffs,
               motor,
               &remainingCooldown,
               &lastActivation);

// Timer for 1-second ticks
unsigned long lastSecondTick = 0;

// Helper: Increment time string by 1 second
String incrementTime(String timeStr) {
  int h = timeStr.substring(0, 2).toInt();
  int m = timeStr.substring(3, 5).toInt();
  int s = timeStr.substring(6, 8).toInt();

  s++;
  if (s >= 60) {
    s = 0;
    m++;
  }
  if (m >= 60) {
    m = 0;
    h++;
  }
  if (h >= 24) {
    h = 0;
  }

  char buffer[9];
  snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d", h, m, s);
  return String(buffer);
}

void buttonPressed(){
  if (remainingCooldown > 0) {
    return;
  }
  String buf = spiffs.readFile("/cooldown.json");
  int configuredCooldown = 0;

  if (buf.length()) {
    int i = buf.indexOf("configured");
    if (i >= 0) {
      int c1 = buf.indexOf(':', i);
      int c2 = buf.indexOf('}', c1);
      configuredCooldown = buf.substring(c1+1, c2).toInt();
    }
  }

  motor.blow(200);
  remainingCooldown = configuredCooldown;
  lastActivation = "00:00:00";
}

Button pushButton(14, buttonPressed);

void setup() {
  Serial.begin(115200);
  remainingCooldown = 0;
  lastActivation    = "00:00:00";
  web.begin();
  Serial.println("Ready");
}

void loop() {
  motor.tick();
  web.tick();
  pushButton.tick();

  // Handle 1-second updates
  if (millis() - lastSecondTick >= 1000) {
    lastSecondTick = millis();

    if (remainingCooldown > 0) {
      remainingCooldown--;
    }

    lastActivation = incrementTime(lastActivation);
  }
}
