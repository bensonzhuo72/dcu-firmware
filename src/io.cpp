#include "io.h"
#include "pins.h"

namespace {

// Index i corresponds to Mission(i + 1).
constexpr uint8_t MISSION_PINS[MISSION_PIN_COUNT] = {
  PIN_MISSION_1_EV,
  PIN_MISSION_2_D,
  PIN_MISSION_3_D,
  PIN_MISSION_4_D,
  PIN_MISSION_5_D,
  PIN_MISSION_6_D,
};

// Set by any mission pin edge. The ISR only timestamps the edge; the
// debounced state is computed in mission_update().
volatile bool     g_mission_dirty = false;
volatile uint32_t g_mission_last_edge_ms = 0;

Mission g_mission = Mission::None;

void mission_isr() {
  g_mission_last_edge_ms = millis();
  g_mission_dirty = true;
}

Mission mission_sample() {
  Mission selected = Mission::None;
  for (uint8_t i = 0; i < MISSION_PIN_COUNT; i++) {
    if (digitalReadFast(MISSION_PINS[i]) == LOW) {
      if (selected != Mission::None) return Mission::Invalid;
      selected = static_cast<Mission>(i + 1);
    }
  }
  return selected;
}

// pinMode(OUTPUT) sets the pad to DSE(7) with slow slew and low speed.
// Switch to fast slew rate and the highest speed setting.
void set_fast_edges(uint8_t pin) {
  volatile uint32_t *pad = portControlRegister(pin);
  *pad = IOMUXC_PAD_DSE(7) | IOMUXC_PAD_SPEED(3) | IOMUXC_PAD_SRE;
}

void output_init(uint8_t pin, bool initial) {
  digitalWriteFast(pin, initial);
  pinMode(pin, OUTPUT);
}

}  // namespace

void io_init() {
  // Mission inputs: internal pull-up (~22k) + interrupt on either edge
  for (uint8_t pin : MISSION_PINS) {
    pinMode(pin, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(pin), mission_isr, CHANGE);
  }

  // Status inputs
  pinMode(PIN_DSMS, INPUT);
  pinMode(PIN_TSAB, INPUT);

  // BP_3.3V analog input
  analogReadResolution(ADC_BITS);
  pinMode(PIN_BP_3V3, INPUT_DISABLE);

  // Trigger outputs with fast edges, start low
  output_init(PIN_TRIG_1_CTRL, LOW);
  set_fast_edges(PIN_TRIG_1_CTRL);
  output_init(PIN_TRIG_2_CTRL, LOW);
  set_fast_edges(PIN_TRIG_2_CTRL);

  // Other outputs, start low (precharge disabled)
  output_init(PIN_PRECHARGE_EN, LOW);
  output_init(PIN_DSR_CTRL, LOW);
  output_init(PIN_REMOTE_CTRL, LOW);

  // LED PWM. analogWriteResolution() is global to all PWM pins.
  analogWriteResolution(LED_PWM_BITS);
  analogWriteFrequency(PIN_LED_BLUE_CTRL, LED_PWM_FREQ_HZ);
  analogWriteFrequency(PIN_LED_YELLOW_CTRL, LED_PWM_FREQ_HZ);
  led_set_blue(0);
  led_set_yellow(0);

  // Start with a debounce window so the boot state is sampled once the
  // pull-ups have settled.
  g_mission_last_edge_ms = millis();
  g_mission_dirty = true;
}

bool mission_update() {
  noInterrupts();
  bool dirty = g_mission_dirty;
  uint32_t last_edge = g_mission_last_edge_ms;
  bool settled = dirty && (millis() - last_edge >= MISSION_DEBOUNCE_MS);
  if (settled) g_mission_dirty = false;
  interrupts();

  if (!settled) return false;

  // Any edge after this point sets dirty again and restarts the window.
  Mission sampled = mission_sample();
  if (sampled == g_mission) return false;
  g_mission = sampled;
  return true;
}

Mission mission_selected() { return g_mission; }

const char *mission_name(Mission m) {
  switch (m) {
    case Mission::None:    return "NONE";
    case Mission::M1_EV:   return "MISSION_1_EV";
    case Mission::M2:      return "MISSION_2";
    case Mission::M3:      return "MISSION_3";
    case Mission::M4:      return "MISSION_4";
    case Mission::M5:      return "MISSION_5";
    case Mission::M6:      return "MISSION_6";
    case Mission::Invalid: return "INVALID";
  }
  return "?";
}

void led_set_blue(uint16_t duty) {
  analogWrite(PIN_LED_BLUE_CTRL, min(duty, LED_PWM_MAX));
}

void led_set_yellow(uint16_t duty) {
  analogWrite(PIN_LED_YELLOW_CTRL, min(duty, LED_PWM_MAX));
}

uint16_t bp3v3_read_raw() { return analogRead(PIN_BP_3V3); }

float bp3v3_read_volts() {
  return bp3v3_read_raw() * (3.3f / ((1u << ADC_BITS) - 1));
}

bool dsms_read() { return digitalReadFast(PIN_DSMS); }
bool tsab_read() { return digitalReadFast(PIN_TSAB); }

void trig1_write(bool level) { digitalWriteFast(PIN_TRIG_1_CTRL, level); }
void trig2_write(bool level) { digitalWriteFast(PIN_TRIG_2_CTRL, level); }
void precharge_en_write(bool level) { digitalWriteFast(PIN_PRECHARGE_EN, level); }
void dsr_ctrl_write(bool level) { digitalWriteFast(PIN_DSR_CTRL, level); }
void remote_ctrl_write(bool level) { digitalWriteFast(PIN_REMOTE_CTRL, level); }
