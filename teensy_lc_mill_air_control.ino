struct Key {
  Key(int a, int b = 0) {
    a_ = a;
    b_ = b;
    pressedTime_ = 0;
    timeoutTime_ = 0;
    state_ = 0;
    newstate_ = 0;
  }
  void press();
  void step(uint32_t now);

  int a_;
  int b_;
  uint32_t pressedTime_;
  uint32_t timeoutTime_;
  uint8_t state_;
  uint8_t newstate_;
};

void Key::press() {
  if (state_ == 0) {
    newstate_ = 1;
  }
}

void Key::step(uint32_t now) {
  if ((state_ != 0) && (now - pressedTime_ > timeoutTime_)) {
    newstate_ = state_+1;
  }
  if (state_ != newstate_) {
    pressedTime_ = now;
    state_ = newstate_;
    switch (state_) {
      case 1:
        if (a_ != 0) {
          Keyboard.press(a_);
        }
        timeoutTime_ = 10;
        break;
      case 2:
        if (b_ != 0) {
          Keyboard.press(b_);
        }
        timeoutTime_ = 10;
        break;
      case 3:
        if (b_ != 0) {
          Keyboard.release(b_);
        }
        timeoutTime_ = 10;
        break;
      case 4:
        if (a_ != 0) {
          Keyboard.release(a_);
        }
        timeoutTime_ = 500; //  de-bounce
        break;
      default:
        newstate_ = 0;
        break;
    }
  }
  if (state_ == 0) {
    pressedTime_ = 0;
    timeoutTime_ = 10;
  }
}

Key keyStart(MODIFIERKEY_ALT, KEY_R);
Key keyHold(KEY_SPACE);
Key keyStop(KEY_ESC);

#define TEENSY_LED 13
#define ON_LED 2
#define GO_LED 3
#define AIR_LED 4
#define GO_BTN 5
#define PAUSE_BTN 6
#define STOP_BTN 7
#define AIR_BTN 8
#define LITE_BTN 9
#define AIR_PIN 17
#define LITE_PIN 20

void setup() {
  pinMode(TEENSY_LED, OUTPUT);
  digitalWrite(TEENSY_LED, HIGH);
  pinMode(ON_LED, OUTPUT);
  digitalWrite(ON_LED, HIGH);
  pinMode(GO_LED, OUTPUT);
  digitalWrite(GO_LED, LOW);
  pinMode(AIR_LED, OUTPUT);
  digitalWrite(AIR_LED, LOW);
  pinMode(AIR_PIN, OUTPUT);
  digitalWrite(AIR_PIN, LOW);
  pinMode(LITE_PIN, OUTPUT);
  digitalWrite(LITE_PIN, LOW);

  pinMode(GO_BTN, INPUT_PULLUP);
  pinMode(PAUSE_BTN, INPUT_PULLUP);
  pinMode(STOP_BTN, INPUT_PULLUP);
  pinMode(AIR_BTN, INPUT_PULLUP);
  pinMode(LITE_BTN, INPUT_PULLUP);
}

class InputButton {
  public:
    InputButton(int pin) : changeTime_(0), pin_(pin), state_(true), derivedState_(true) {
    }
    
    bool update(uint32_t now) {
      bool down = digitalRead(pin_) ? true : false;
      if (down != state_) {
        state_ = down;
        changeTime_ = now;
      }
      if ((state_ != derivedState_) && (now - changeTime_ >= 20)) {
        derivedState_ = state_;
        fallingEdge_ = !derivedState_;
        return true;
      }
      fallingEdge_ = false;
      return false;
    }
    
    bool fallingEdge() {
      return fallingEdge_;
    }

    uint32_t changeTime_;
    int pin_;
    bool state_;
    bool derivedState_;
    bool fallingEdge_;
};

InputButton btnGo(GO_BTN);
InputButton btnPause(PAUSE_BTN);
InputButton btnStop(STOP_BTN);
InputButton btnAir(AIR_BTN);
InputButton btnLite(LITE_BTN);

uint32_t airTime_;
uint8_t airIntermittent_;
bool airState_;
bool liteState_;

void nextAirIntermittent() {
  airIntermittent_ = airIntermittent_ + 1;
  if (airIntermittent_ >= 3) {
    airIntermittent_ = 0;
  }
}

void loop() {

  uint32_t now = millis();

  if (btnGo.update(now)) {
    if (btnGo.fallingEdge()) {
      keyStart.press();
    }
  }
  keyStart.step(now);
  
  if (btnPause.update(now)) {
    if (btnPause.fallingEdge()) {
      keyHold.press();
    }
  }
  keyHold.step(now);
  
  if (btnStop.update(now)) {
    if (btnStop.fallingEdge()) {
      keyStop.press();
      airIntermittent_ = 0;
    }
  }
  keyStop.step(now);

  if (btnAir.update(now)) {
    if (btnAir.fallingEdge()) {
      airTime_ = now;
      airState_ = true;
    } else {
      airState_ = false;
      if (now - airTime_ < 300) {
        nextAirIntermittent();
      } else {
        airIntermittent_ = 0;
      }
    }
  }
  bool isIntermittentTime = false;
  if (airIntermittent_ != 0) {
    //  never run for more than two hours?
    if (now - airTime_ > 7200000) {
        airIntermittent_ = 0;
        airState_ = false;
    } else if (((now - airTime_) & (16383 >> airIntermittent_)) < 1024) {
        isIntermittentTime = true;
    }
  }
  if (airState_ || isIntermittentTime) {
    digitalWrite(AIR_PIN, HIGH);
    digitalWrite(GO_LED, HIGH);
  } else {
    digitalWrite(AIR_PIN, LOW);
    digitalWrite(GO_LED, LOW);
  }
  if (airIntermittent_) {
    digitalWrite(AIR_LED, (now & (2047 >> airIntermittent_)) < 128);
  } else {
    digitalWrite(AIR_LED, LOW);
  }

  if (btnLite.update(now)) {
    if (btnLite.fallingEdge()) {
      liteState_ = !liteState_;
    }
  }
  digitalWrite(LITE_PIN, liteState_);
}

