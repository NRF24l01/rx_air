#include "AirFreshenerMotor.h"

AirFreshenerMotor::AirFreshenerMotor(uint8_t pinIA, uint8_t pinIB)
  : _pinIA(pinIA), _pinIB(pinIB) {
  pinMode(_pinIA, OUTPUT);
  pinMode(_pinIB, OUTPUT);
  motorStop();
}

void AirFreshenerMotor::tick() {
  if (_isBlowing && millis() >= _stopTime) {
    motorStop();
    _isBlowing = false;
    Serial.println("Motor: stop");
  }
}

void AirFreshenerMotor::blow(uint32_t durationMs) {
  _stopTime = millis() + durationMs + _pushDelay;
  motorForward();
  _isBlowing = true;
  Serial.print("Motor: blow for ");
  Serial.print(durationMs + _pushDelay);
  Serial.println(" ms");
}

void AirFreshenerMotor::motorForward() {
  digitalWrite(_pinIB, LOW);
  digitalWrite(_pinIA, HIGH);
}

void AirFreshenerMotor::motorStop() {
  digitalWrite(_pinIB, LOW);
  digitalWrite(_pinIA, LOW);
}
