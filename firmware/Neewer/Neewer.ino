#include <avr/io.h>
#include <util/atomic.h>

const char GRB[3]        { 'G', 'R', 'B' }; // wire color codes

const float TRIGGER[3]   { 0.f, 0.5f, 0.5f }; // G, R, B

const uint8_t LED[3]     {  5,  6,  7 }; // G, R, B
const uint8_t POT[3]     { A0, A1, A2 }; // G, R, B

const uint8_t BALANCE    { 0 }; // G
const uint8_t COLOR      { 1 }; // R
const uint8_t BRIGHTNESS { 2 }; // B

#define DEBUG            false // enables serial output
#define SLOW             false // true: ~30kHz, false: ~60kHz
#define ALIGN            true  // force PWM outputs to go LOW at the boundary

#if SLOW && ALIGN
#error SLOW and ALIGN are incompatible!
#endif

struct Reading
{
  float filt    { -1    };
  int   step    { -1    };
  float value   { -1    };
  bool  changed { false };
};

void setup_led()
{
  pinMode(LED[0], OUTPUT);
  pinMode(LED[1], OUTPUT);
  pinMode(LED[2], OUTPUT);

  digitalWrite(LED[0], LOW);
  digitalWrite(LED[1], LOW);
  digitalWrite(LED[2], LOW);
}

void setup_pot()
{ 
  pinMode(POT[0], INPUT);
  pinMode(POT[1], INPUT);
  pinMode(POT[2], INPUT);
}

void setup_pwm()
{
  // halt timers and reset prescalers so they can start together
  GTCCR = _BV(TSM) | _BV(PSRSYNC) | _BV(PSRASY);
  
  DDRB = 0;
  DDRD = 0;

  TCCR1A = 0;
  TCCR1B = 0;

  TCCR2A = 0;
  TCCR2B = 0;

  DDRB |= _BV(PB1); // OC1A => D9
  DDRB |= _BV(PB2); // OC1B => D10
  DDRB |= _BV(PB3); // OC2A => D11
  DDRD |= _BV(PD3); // OC2B => D3

  if (SLOW)
  {
    // 8-bit phase correct PWM ~30kHz
    TCCR1A |= _BV(WGM10);
    TCCR2A |= _BV(WGM20);
  }
  else
  {
    // 8-bit fast PWM ~60kHz
    TCCR1A |= _BV(WGM10);
    TCCR1B |= _BV(WGM12);
    TCCR2A |= _BV(WGM20) | _BV(WGM21);
  }

  TCCR1A |= _BV(COM1A1) | _BV(COM1B1);
  TCCR2A |= _BV(COM2A1) | _BV(COM2B1);

  if (ALIGN)
  {
    TCCR1A |= _BV(COM1A0) | _BV(COM1B0);
    TCCR2A |= _BV(COM2A0) | _BV(COM2B0);
  }

  TCCR1B |= _BV(CS10);
  TCCR2B |= _BV(CS20);

  if (ALIGN)
  {
    OCR1A = 0xFF;
    OCR1B = 0xFF;
    OCR2A = 0xFF;
    OCR2B = 0xFF;
  }
  else
  {
    OCR1A = 0;
    OCR1B = 0;
    OCR2A = 0;
    OCR2B = 0;
  }

  TCNT1 = 0;
  TCNT2 = 0;

  // start all timers simultaneously
  GTCCR = 0;
}

void setup()
{
  setup_pwm();
  setup_pot();
  setup_led();

  if (DEBUG)
  {
    Serial.begin(9600);
    while (!Serial);
    Serial.println("DEBUG");
  }
}

void read(uint8_t pot, Reading& reading)
{
  const float ALPHA = 0.25f; // EMA smoothing (0..1), higher = faster, lower = smoother
  const float HYST  = 0.01f; // 1% hysteresis around each 10% step

  const int previous = reading.step;

  float value = analogRead(POT[pot]);

  value = value / 1023.f;
  value = constrain(value, 0.f, 1.f);
  value = 1.f - value;

  // init
  if (reading.filt < 0 || reading.step < 0)
  {
    reading.filt = value;
    reading.step = round(value * 10.f);
  }

  // smooth
  reading.filt += ALPHA * (value - reading.filt);

  // thresholds to step up and down
  const float up = min(1.f, ((reading.step + 0.5f) / 10.f) + HYST);
  const float dn = max(0.f, ((reading.step - 0.5f) / 10.f) - HYST);

  // hysteretic step update
  if      (reading.step < 10 && reading.filt >= up) reading.step++;
  else if (reading.step >  0 && reading.filt <= dn) reading.step--;

  reading.changed = reading.step != previous;
  reading.value = reading.step / 10.f; // 0.0, 0.1, ..., 1.0
}

void write(float duty1, float duty2, float duty3, float duty4)
{
  uint8_t a = constrain(duty1 * 0xFF, 0, 0xFF);
  uint8_t b = constrain(duty2 * 0xFF, 0, 0xFF);
  uint8_t c = constrain(duty3 * 0xFF, 0, 0xFF);
  uint8_t d = constrain(duty4 * 0xFF, 0, 0xFF);

  if (ALIGN)
  {
    a = 0xFF - a;
    b = 0xFF - b;
    c = 0xFF - c;
    d = 0xFF - d;
  }
  
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
  {
    OCR1A = a;
    OCR1B = b;
    OCR2A = c;
    OCR2B = d;
  }
}

void loop()
{
  const unsigned long tic = micros();
  
  static Reading READINGS[3] {};

  static bool RAMP { true };

  for (uint8_t i = 0; i < 3; ++i)
  {
    read(i, READINGS[i]);

    if (i == BALANCE)
    {
      READINGS[i].value *= 2;
      READINGS[i].value -= 1;
    }
  }

  const float color      = READINGS[COLOR].value;
  const float brightness = READINGS[BRIGHTNESS].value;
  const float balance    = READINGS[BALANCE].value;

  float a = (0 + color) * brightness;
  float b = (1 - color) * brightness;

  float c = a;
  float d = b;

  if (balance < 0)
  {
    const float f = 1.f - abs(balance);

    a *= f;
    b *= f;
  }

  if (balance > 0)
  {
    const float f = 1.f - abs(balance);

    c *= f;
    d *= f;
  }

  if (!RAMP)
  {
    write(a, b, c, d);
  }

  for (uint8_t i = 0; i < 3; ++i)
  {
    if (READINGS[i].value == TRIGGER[i])
    {
      digitalWrite(LED[i], HIGH);
    }
    else if (READINGS[i].changed)
    {
      digitalWrite(LED[i], HIGH);
      delay(5);
      digitalWrite(LED[i], LOW);
    }
    else
    {
      digitalWrite(LED[i], LOW);
    }
  }

  if (RAMP)
  {
    for (float f = 0.f; f < 1.f; f += 0.01f)
    {
      write(a * f, b * f, c * f, d * f);
      delay(10);
    }

    write(a, b, c, d);

    RAMP = false;
  }

  if (DEBUG)
  {
    if (false)
    {
      Serial.println();
  
      for (uint8_t i = 0; i < 3; ++i)
      {
        Serial.print(GRB[i]);
        Serial.print(": ");
        Serial.println(int(READINGS[i].value * 100));
      }
    }

    if (false)
    {
      Serial.println();
  
      Serial.print("A: ");
      Serial.println(a);
  
      Serial.print("B: ");
      Serial.println(b);
  
      Serial.print("C: ");
      Serial.println(c);
  
      Serial.print("D: ");
      Serial.println(d);
    }
  }

  const unsigned long toc = micros();

  delay(50 - min((toc - tic) / 1000, 50));
}
