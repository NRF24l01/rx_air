#ifndef SPIFFSMANAGER_H
#define SPIFFSMANAGER_H

#include <SPIFFS.h>
#include <Arduino.h>

class SPIFFSManager {
public:
  bool begin(bool formatOnFail = true) {
    if (!SPIFFS.begin(formatOnFail)) {
      Serial.println("SPIFFS Mount Failed");
      return false;
    }
    return true;
  }

  String readFile(const char* path) {
    File file = SPIFFS.open(path, FILE_READ);
    if (!file) {
      Serial.printf("Failed to open %s for reading\n", path);
      return String();
    }
    String content;
    while (file.available()) {
      content += char(file.read());
    }
    file.close();
    return content;
  }

  bool writeFile(const char* path, const String& data) {
    File file = SPIFFS.open(path, FILE_WRITE);
    if (!file) {
      Serial.printf("Failed to open %s for writing\n", path);
      return false;
    }
    file.print(data);
    file.close();
    return true;
  }
};

#endif // SPIFFSMANAGER_H
