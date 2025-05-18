#ifndef BUTTON_H
#define BUTTON_H

class Button {
private:
  uint8_t pin;
  unsigned long debounceDelay;
  bool lastButtonState;
  bool buttonPressed;
  unsigned long lastDebounceTime;
  void (*callback)();

public:
  Button(uint8_t buttonPin, void (*onClickCallback)(), unsigned long debounceMs = 50)
    : pin(buttonPin),
      debounceDelay(debounceMs),
      lastButtonState(HIGH),
      buttonPressed(false),
      lastDebounceTime(0),
      callback(onClickCallback) {}

  void begin() {
    pinMode(pin, INPUT_PULLUP);  // Кнопка между пином и GND
  }

  void tick() {
    int reading = digitalRead(pin);

    if (reading != lastButtonState) {
      lastDebounceTime = millis();
    }

    if ((millis() - lastDebounceTime) > debounceDelay) {
      if (reading == LOW && !buttonPressed) {
        buttonPressed = true;
        if (callback) callback();
      } else if (reading == HIGH) {
        buttonPressed = false;
      }
    }

    lastButtonState = reading;
  }
};

#endif // BUTTON_H
