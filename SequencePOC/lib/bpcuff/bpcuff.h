#ifndef BPCUFF_H
#define BPCUFF_H

#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_LPS35HW.h>

// -----------------------------------------------------------------------------
// Default pins
// Change these to match your wiring, or use the constructor with explicit pins.
// -----------------------------------------------------------------------------
#ifndef BP_CUFF_IN1_PIN
#define BP_CUFF_IN1_PIN 3
#endif

#ifndef BP_CUFF_IN2_PIN
#define BP_CUFF_IN2_PIN 2
#endif

#ifndef BP_CUFF_EEP_PIN
#define BP_CUFF_EEP_PIN 4
#endif

#ifndef BP_CUFF_VALVE_PIN
#define BP_CUFF_VALVE_PIN 1
#endif

#ifndef BP_CUFF_LPS_CS_PIN
#define BP_CUFF_LPS_CS_PIN 5

// Valve pin
#define VALVE 1

// LPS35HW SPI pins
#define LPS_CS   5
#define LPS_SCK  28
#define LPS_MISO 16
#define LPS_MOSI 19

#endif

struct BPReading {
  float systolic;
  float diastolic;
  float map;
  bool valid;
  bool complete;
};

enum class BPCuffState {
  IDLE,
  BP_ZERO_DEFLATE,
  BP_ZERO_SETTLE,
  BP_INFLATE,
  BP_SETTLE,
  BP_DEFLATE_SAMPLE,
  BP_FINAL_DEFLATE,
  BP_DONE,
  BP_ERROR,
  MANUAL_INFLATE,
  HOLD_ACTIVE,
  MANUAL_DEFLATE
};

class CuffControl {
public:
  CuffControl();
  CuffControl(uint8_t in1Pin,
              uint8_t in2Pin,
              uint8_t eepPin,
              uint8_t valvePin,
              uint8_t lpsCsPin);

  bool bpCuffBegin();

  // Immediate hardware helpers
  float pressure_hPa = 0.0f;
  

  void pumpForward();
  void stopPump();
  void valveOpen();
  void valveClose();
  void readPressure_hPa();
  float getPressureMmHg();
  void zeroAmbientNow();

  // ---------------------------------------------------------------------------
  // Nonblocking blood-pressure measurement.
  // Call GetBloodPressure() every loop. The first call starts the sequence.
  // It returns complete=false while running. When complete=true, valid tells
  // whether systolic/diastolic are usable.
  // ---------------------------------------------------------------------------
  BPReading GetBloodPressure();
  void StartBloodPressure();
  bool IsBusy() const;
  BPCuffState getState() const;
  void Reset();

  // ---------------------------------------------------------------------------
  // Nonblocking cuff-control phases for the 5-minute hold workflow.
  // Call each function repeatedly from loop until it returns true.
  // Inflate() target defaults to last measured systolic, or 140 mmHg if no BP
  // reading exists yet.
  // ---------------------------------------------------------------------------
  bool Inflate();
  bool Inflate(float targetMmHg);
  bool Hold(void (*updateHRV)() = nullptr);
  bool Deflate();

  BPReading lastReading() const;
  unsigned long holdElapsedMs() const;
  float holdStartPressureMmHg() const;
  float holdEndPressureMmHg() const;
  float holdPressureDropMmHg() const;

private:
  static const int MAX_POINTS = 1200;

  // Oscillometric tuning from tested code
  static constexpr float SBP_RATIO = 0.55f;
  static constexpr float DBP_RATIO = 0.70f;

  static constexpr float BP_INFLATE_TARGET_MMHG = 185.0f;
  static constexpr float BP_SAMPLE_HIGH_MMHG = 155.0f;
  static constexpr float BP_SAMPLE_LOW_MMHG = 45.0f;
  static constexpr float BP_STOP_DEFLATE_MMHG = 40.0f;
  static constexpr float FULL_DEFLATE_DONE_MMHG = 5.0f;
  static constexpr float HOLD_FALLBACK_TARGET_MMHG = 140.0f;

  static constexpr unsigned long ZERO_DEFLATE_MS = 3000UL;
  static constexpr unsigned long ZERO_SETTLE_MS = 500UL;
  static constexpr unsigned long BP_SETTLE_MS = 1000UL;
  static constexpr unsigned long PUMP_TIMEOUT_MS = 30000UL;
  static constexpr unsigned long BP_DEFLATE_TIMEOUT_MS = 60000UL;
  static constexpr unsigned long FULL_DEFLATE_TIMEOUT_MS = 30000UL;
  static constexpr unsigned long HOLD_DURATION_MS = 10000UL;
  static constexpr unsigned long BP_SAMPLE_PERIOD_MS = 10UL;

  uint8_t _in1Pin;
  uint8_t _in2Pin;
  uint8_t _eepPin;
  uint8_t _valvePin;
  uint8_t _lpsCsPin;

  Adafruit_LPS35HW sensor;

  BPCuffState state;
  unsigned long stateStartMs;
  unsigned long lastSampleMs;
  float ambientPressure_hPa;

  unsigned long lastSampleTime; // For limiting pressure sampling frequency

  float pressureData[MAX_POINTS];
  float oscData[MAX_POINTS];
  int dataCount;

  BPReading result;

  float manualInflateTargetMmHg;
  bool manualInflateStarted;

  bool holdStarted;
  unsigned long holdStartMs;
  float holdStartPressure;
  float holdEndPressure;

  void resetBPData();
  void savePoint(float pressureMmHg);
  void startState(BPCuffState newState);
  void failMeasurement();
  bool calculateBP(BPReading &out);
};

#endif
