#ifndef AIR_FRESHENER_MOTOR_H
#define AIR_FRESHENER_MOTOR_H

#include <Arduino.h>

class AirFreshenerMotor {
  public:
    AirFreshenerMotor(uint8_t pinIA, uint8_t pinIB);

    void tick();                        // Проверка, пора ли выключить мотор
    void blow(uint32_t durationMs);    // Включить мотор на заданное время

  private:
    uint8_t _pinIA, _pinIB;
    bool _isBlowing = false;
    uint32_t _stopTime = 0;
    const uint32_t _pushDelay = 250;

    void motorForward();
    void motorStop();
};

#endif // AIR_FRESHENER_MOTOR_H
