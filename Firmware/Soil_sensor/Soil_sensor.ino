#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>

#define MEASURE_DELAY   30000UL
#define PING_DELAY_MAX  30000UL
#define PING_DELAY_MIN  1000UL

#define MEASURE_THRESHOLD_4 180//640
#define MEASURE_THRESHOLD_3 170//150
#define MEASURE_THRESHOLD_2 160//640

#define LED_PIN_GREEN   0
#define LED_PIN_RED     5
#define USB_P           4
#define USB_N           3
#define MEASURE_C_PIN   2
#define MEASURE_C_ADC   1
#define MEASURE_PWM_PIN 1

volatile unsigned long measurementTimer = 0;
volatile unsigned int pingTimer = 0;

volatile uint16_t lastMeasurement = 0;
uint16_t currentPingDelay = 0;

uint8_t mcucr1, mcucr2;
int vcc = 0;

uint8_t measure_threshold = MEASURE_THRESHOLD_2;

void setup() {
  pinMode(MEASURE_PWM_PIN, OUTPUT);
  pinMode(LED_PIN_GREEN, OUTPUT);
  pinMode(LED_PIN_RED, OUTPUT);

  digitalWrite(LED_PIN_GREEN, HIGH); // LED is active LOW
  digitalWrite(LED_PIN_RED, HIGH); // LED is active LOW
  digitalWrite(MEASURE_PWM_PIN, LOW);

  currentPingDelay = PING_DELAY_MIN;
  vcc = measureVCC();
  
  ACSR |= _BV(ACD);                         //disable the analog comparator

  measurementTimer = MEASURE_DELAY+1;
  pingTimer = currentPingDelay+1;
}


void loop() {
  
  if(measurementTimer >= MEASURE_DELAY){
    
    vcc = measureVCC();
    if(vcc >= 4000){
      measure_threshold = MEASURE_THRESHOLD_4;
    }else if(vcc >= 3000){
      measure_threshold = MEASURE_THRESHOLD_3;
    }else{
      measure_threshold = MEASURE_THRESHOLD_2;
    }

    uint8_t measurement = measureSoilMoisture();
    measurementTimer = 0;
    if(measurement > measure_threshold && lastMeasurement < measure_threshold){
      // Dry soil, and we went from wet to dry
      currentPingDelay = PING_DELAY_MIN;
    }
    else if(measurement <= measure_threshold){
      // Wet soil, no need to go blinking
      currentPingDelay = PING_DELAY_MAX;
    }
        lastMeasurement = measurement;
  }
  if(pingTimer >= currentPingDelay){
    if(lastMeasurement > measure_threshold){
      for(uint8_t nBlinks = 0; nBlinks < 3; nBlinks++){
        digitalWrite(LED_PIN_GREEN, LOW);
        sleepNow(125);
        digitalWrite(LED_PIN_GREEN, HIGH);
        sleepNow(125);
      }
      currentPingDelay += 1000;
      if(currentPingDelay > PING_DELAY_MAX){
        currentPingDelay = PING_DELAY_MAX;
      }
    }else{
      digitalWrite(LED_PIN_GREEN, HIGH);
    }
    if(vcc <= 2800){
      for(uint8_t nBlinks = 0; nBlinks < 3; nBlinks++){
        digitalWrite(LED_PIN_RED, LOW);
        sleepNow(125);
        digitalWrite(LED_PIN_RED, HIGH);
        sleepNow(125);
      }
    }
    
    pingTimer = 0;
  }
  //uint16_t sleep = currentPingDelay;
  if(currentPingDelay >= 8000UL){
    sleepNow(8000);
  }else if(currentPingDelay >= 4000UL){
    sleepNow(4000);
  }else if(currentPingDelay >= 2000UL){
    sleepNow(2000);
  }else{
    sleepNow(1000);
  }
}

uint8_t measureSoilMoisture(){
  // Set PWM output
  // Note: PWM frequency must be set to 64kHz. Change:
  //       in arduino-1.x/hardware/digispark/cores/tiny/wiring.c MS_TIMER_TICK_EVERY_X_CYCLES to 1
  //       in arduino-1.x/hardware/digispark/cores/tiny/core_build_options.h FAVOR_PHASE_CORRECT_PWM  to 0 

  analogWrite(MEASURE_PWM_PIN,40);
  delay(50);
  uint8_t sensorValue = (uint8_t)(analogRead(MEASURE_C_ADC)>>2); // read ADC1
  analogWrite(MEASURE_PWM_PIN,0);
  return sensorValue;

}

int measureVCC(){
  uint8_t mux = ADMUX;
  uint8_t sra = ADCSRA;
  
  //reads internal 1V1 reference against VCC
  ADMUX = _BV(MUX3) | _BV(MUX2); // For ATtiny85/45
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA, ADSC));
  uint8_t low = ADCL;
  unsigned int val = (ADCH << 8) | low;
  //discard previous result
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA, ADSC));
  low = ADCL;
  val = (ADCH << 8) | low;

  ADMUX = mux;
  ADCSRA = sra;
  
  return ((long)1024 * 1100) / val;  
}

void sleepNow (uint16_t wdt_delay){

  ADCSRA &= ~_BV(ADEN);                     //disable ADC
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  
  wdtEnable(wdt_delay);              //start the WDT
  
  //turn off the brown-out detector.
  //must have an ATtiny45 or ATtiny85 rev C or later for software to be able to disable the BOD.
  //current while sleeping will be <0.5uA if BOD is disabled, <25uA if not.
  cli();
  mcucr1 = MCUCR | _BV(BODS) | _BV(BODSE);  //turn off the brown-out detector
  mcucr2 = mcucr1 & ~_BV(BODSE);
  MCUCR = mcucr1;
  MCUCR = mcucr2;
  sei();                         //ensure interrupts enabled so we can wake up again
  sleep_cpu();                   //go to sleep
                                 //----zzzz----zzzz----zzzz----zzzz
  cli();                         //wake up here, disable interrupts
  sleep_disable();
  wdtDisable();                  //don't need the watchdog while we're awake
  sei();                         //enable interrupts again (but INT0 is disabled above)


  ADCSRA |= _BV(ADEN);                     //enable ADC
  
  measurementTimer += wdt_delay;
  pingTimer += wdt_delay;
}

void wdtEnable(uint16_t wdt_delay){
    wdt_reset();
    cli();
    MCUSR = 0x00;
    WDTCR |= _BV(WDCE) | _BV(WDE);
    WDTCR = _BV(WDIE);
    switch(wdt_delay){
      case 8000: {
        WDTCR |= (1 << WDP3) | (0 << WDP2) | (0 << WDP1) | (1 << WDP0); // 8 seconds sleep before WDT
      }break;
      case 4000: {
        WDTCR |= (1 << WDP3) | (0 << WDP2) | (0 << WDP1) | (0 << WDP0);
      }break;
      case 2000: {
        WDTCR |= (0 << WDP3) | (1 << WDP2) | (1 << WDP1) | (1 << WDP0);
      }break;
      case 1000: {
        WDTCR |= (0 << WDP3) | (1 << WDP2) | (1 << WDP1) | (0 << WDP0);
      }break;
      case 500: {
        WDTCR |= (0 << WDP3) | (1 << WDP2) | (0 << WDP1) | (1 << WDP0);
      }break;
      case 250: {
         WDTCR |= (0 << WDP3) | (1 << WDP2) | (0 << WDP1) | (0 << WDP0);
      }break;
      case 125: {
        WDTCR |= (0 << WDP3) | (0 << WDP2) | (1 << WDP1) | (1 << WDP0);
      }break;
      case 64: {
        WDTCR |= (0 << WDP3) | (0 << WDP2) | (1 << WDP1) | (0 << WDP0);
      }break;
      case 32: {
        WDTCR |= (0 << WDP3) | (0 << WDP2) | (0 << WDP1) | (1 << WDP0);
      }break;
      case 16: {
        WDTCR |= (0 << WDP3) | (0 << WDP2) | (0 << WDP1) | (0 << WDP0);
      };
      default: {
        WDTCR |= (0 << WDP3) | (1 << WDP2) | (1 << WDP1) | (0 << WDP0);
      };
    }  
    sei();
}

void wdtDisable(void){
    wdt_reset();
    cli();
    MCUSR = 0x00;
    WDTCR |= _BV(WDCE) | _BV(WDE);
    WDTCR = 0x00;
    sei();
}

ISR(WDT_vect) {
  wdt_disable();
}
