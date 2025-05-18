#ifndef WEBHANDLER_H
#define WEBHANDLER_H

#include <WiFi.h>
#include <SPIFFS.h>
#include <WebServer.h>
#include "SPIFFSManager.h"
#include "AirFreshenerMotor.h"

class WebHandler {
public:
  WebHandler(const char* ssid,
             const char* wifiPassword,
             const char* webPassword,
             SPIFFSManager& spiffs,
             AirFreshenerMotor& motor,
             int* remainingCooldownPtr,
             String* lastActivationPtr)
    : ssid(ssid)
    , wifiPassword(wifiPassword)
    , webPassword(webPassword)
    , fs(spiffs)
    , motor(motor)
    , server(80)
    , remainingCooldown(remainingCooldownPtr)
    , lastActivation(lastActivationPtr)
    , configuredCooldown(0)
  {}

  void begin() {
    // Wi-Fi
    WiFi.begin(ssid, wifiPassword);
    while (WiFi.status() != WL_CONNECTED) delay(500);
    Serial.print("IP: "); Serial.println(WiFi.localIP());

    // SPIFFS
    if (!fs.begin(true)) return;

    // Загрузить configuredCooldown из /cooldown.json
    String buf = fs.readFile("/cooldown.json");
    if (buf.length()) {
      // ищем число после "configured":
      int i = buf.indexOf("configured");
      if (i >= 0) {
        int c1 = buf.indexOf(':', i);
        int c2 = buf.indexOf('}', c1);
        configuredCooldown = buf.substring(c1+1, c2).toInt();
      }
    }
    // если файл пуст или parsing не сработал, оставляем 0

    // Загрузка расписания (как было раньше)
    scheduleJson = fs.readFile("/schedule.json");
    if (! scheduleJson.length()) scheduleJson = "[]";

    // Регистрируем Cookie
    const char* headerKeys[] = { "Cookie" };
    server.collectHeaders(headerKeys, 1);

    // Маршруты
    server.on("/",        HTTP_GET,  [this]() { handleRoot(); });
    server.on("/login",   HTTP_GET,  [this]() { handleLogin(); });
    server.on("/login",   HTTP_POST, [this]() { handleLogin(); });
    server.on("/api/schedule", HTTP_GET,  [this]() { handleApiScheduleGet(); });
    server.on("/api/schedule", HTTP_POST, [this]() { handleApiSchedulePost(); });

    // *** НОВЫЕ ЭНДПОИНТЫ ***
    server.on("/api/cooldown", HTTP_GET,  [this]() { handleCooldownGet(); });
    server.on("/api/cooldown", HTTP_POST, [this]() { handleCooldownPost(); });
    server.on("/api/last_activation", HTTP_GET, [this]() { handleLastActivation(); });
    server.on("/api/activate", HTTP_POST, [this]() { handleActivate(); });

    server.begin();
  }

  void tick() {
    server.handleClient();
  }

private:
  const char*       ssid;
  const char*       wifiPassword;
  const char*       webPassword;
  SPIFFSManager&    fs;
  AirFreshenerMotor& motor;
  WebServer         server;

  // внешние состояния
  int*     remainingCooldown;
  String*  lastActivation;

  // внутренние
  int      configuredCooldown;
  String   scheduleJson;

  String getCookie(const String& name) {
    if (!server.hasHeader("Cookie")) return "";
    String c = server.header("Cookie");
    int s = c.indexOf(name + "=");
    if (s < 0) return "";
    s += name.length()+1;
    int e = c.indexOf(';', s);
    if (e<0) e = c.length();
    return c.substring(s,e);
  }

  bool isAuthenticated() {
    return getCookie("auth") == webPassword;
  }

  void redirect(const char* to) {
    server.sendHeader("Location", to);
    server.send(302);
  }

  // Оригинальные хендлеры...
   void handleRoot() {
    if (!isAuthenticated()) {
      redirect("/login");
      return;
    }
    File f = SPIFFS.open("/index.html", "r");
    if (!f) {
      server.send(500, "text/plain", "File not found");
      return;
    }
    server.streamFile(f, "text/html");
    f.close();
  }

  void handleLogin() {
    // Если уже залогинен — на корень
    if (server.method() == HTTP_GET && isAuthenticated()) {
      redirect("/");
      return;
    }

    if (server.method() == HTTP_GET) {
      server.send(200, "text/html",
        "<!DOCTYPE html><html><body>"
        "<form method='POST' action='/login'>"
        "<label>Пароль: <input name='password' type='password'></label>"
        "<input type='submit' value='Войти'>"
        "</form></body></html>"
      );
      return;
    }

    // POST-проверка пароля
    if (!server.hasArg("password") || server.arg("password") != webPassword) {
      redirect("/login");
      return;
    }
    // Успешно: записываем пароль в куку и переходим на /
    server.sendHeader("Set-Cookie", String("auth=") + webPassword + "; Path=/; HttpOnly");
    redirect("/");
  }

  void handleApiScheduleGet() {
    if (!isAuthenticated()) {
      server.send(401, "application/json", "{\"error\":\"Unauthorized\"}");
      return;
    }
    server.send(200, "application/json", "{\"schedule\":" + scheduleJson + "}");
  }

  void handleApiSchedulePost() {
    if (!isAuthenticated()) {
      server.send(401, "application/json", "{\"error\":\"Unauthorized\"}");
      return;
    }
    if (server.hasArg("plain")) {
      scheduleJson = server.arg("plain");
      // Сохраняем обновлённое расписание
      if (!fs.writeFile("/schedule.json", scheduleJson)) {
        server.send(500, "application/json", "{\"status\":\"error\"}");
        return;
      }
      server.send(200, "application/json", "{\"status\":\"ok\"}");
    } else {
      server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"No body\"}");
    }
  }

  void handleCooldownGet() {
    if (!isAuthenticated()) {
      server.send(401, "application/json", "{\"error\":\"Unauthorized\"}");
      return;
    }
    // {"configured": 30, "remaining": 12}
    String body = "{\"configured\":" + String(configuredCooldown)
                + ",\"remaining\":" + String(*remainingCooldown)
                + "}";
    server.send(200, "application/json", body);
  }

  void handleCooldownPost() {
    if (!isAuthenticated()) {
      server.send(401, "application/json", "{\"error\":\"Unauthorized\"}");
      return;
    }
    if (!server.hasArg("plain")) {
      server.send(400, "application/json", "{\"error\":\"No body\"}");
      return;
    }
    String p = server.arg("plain");
    int i = p.indexOf("configured");
    if (i<0) {
      server.send(400, "application/json", "{\"error\":\"Bad format\"}");
      return;
    }
    int c1 = p.indexOf(':', i);
    int c2 = p.indexOf('}', c1);
    int val = p.substring(c1+1,c2).toInt();
    configuredCooldown = val;
    // сохраняем в SPIFFS
    fs.writeFile("/cooldown.json",
                 "{\"configured\":" + String(val) + "}");
    server.send(200, "application/json", "{\"status\":\"ok\"}");
  }

  void handleLastActivation() {
    if (!isAuthenticated()) {
      server.send(401, "application/json", "{\"error\":\"Unauthorized\"}");
      return;
    }
    // {"time":"13:45:22"}
    String body = "{\"time\":\"" + *lastActivation + "\"}";
    server.send(200, "application/json", body);
  }

  void handleActivate() {
    if (!isAuthenticated()) {
      server.send(401, "application/json", "{\"error\":\"Unauthorized\"}");
      return;
    }
    // Запуск мотора
    motor.blow(300);
    // обновляем внешние переменные
    *remainingCooldown    = configuredCooldown;
    *lastActivation       = getTimeString();
    server.send(200, "application/json", "{\"status\":\"activated\"}");
  }

  String getTimeString() {
    // попробуем uptime→часы:мин:сек
    unsigned long s = millis() / 1000;
    int h = (s/3600)%24, m=(s/60)%60, sec=s%60;
    char buf[9];
    sprintf(buf,"%02d:%02d:%02d", h,m,sec);
    return String(buf);
  }
};

#endif // WEBHANDLER_H
