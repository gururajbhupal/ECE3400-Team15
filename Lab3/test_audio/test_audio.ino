#define LOG_OUT 1 // use the log output function
#define FFT_N 256 // set to 256 point fft

#include <FFT.h> // include the library

int audio_threshold = 155;

void setup() {
  pinMode(7, OUTPUT);
  //MUX SELECT
  pinMode(13, OUTPUT); // S0 LSB
  pinMode(12, OUTPUT); //S1
  pinMode(11, OUTPUT); //S2 MSB

  Serial.begin(115200); // use the serial port
  TIMSK0 = 0; // turn off timer0 for lower jitter
  ADCSRA = 0xe5; // set the adc to free running mode
  ADMUX = 0x40; // use adc0
  DIDR0 = 0x01; // turn off the digital input for adc0
}

void loop() {
  while (1) { // reduces jitter
    digitalWrite(13, 0); //SO
    digitalWrite(12, 0); //S1
    digitalWrite(11, 0); //S2
    cli();  // UDRE interrupt slows this way down on arduino1.0
    for (int i = 0 ; i < 512 ; i += 2) { // save 256 samples
      while (!(ADCSRA & 0x10)); // wait for adc to be ready
      ADCSRA = 0xf5; // restart adc
      byte m = ADCL; // fetch adc data
      byte j = ADCH;
      int k = (j << 8) | m; // form into an int
      k -= 0x0200; // form into a signed int
      k <<= 6; // form into a 16b signed int
      fft_input[i] = k; // put real data into even bins
      fft_input[i + 1] = 0; // set odd bins to 0
    }
    fft_window(); // window the data for better frequency response
    fft_reorder(); // reorder the data before doing the fft
    fft_run(); // process the data in the fft
    fft_mag_log(); // take the output of the fft
    sei();
    Serial.println("start");
    for (byte i = 0 ; i < FFT_N / 2 ; i++) {
      Serial.println(fft_log_out[i]); // send out the data
      if (i == 5 && fft_log_out[i] >= audio_threshold) {
        digitalWrite(7, HIGH);
        //        Serial.println(999999999);
        //        Serial.println(999999999);
        //        Serial.println(999999999);
        //        Serial.println(999999999);
      }
      if (i == 5 && fft_log_out[i] < audio_threshold) {
        digitalWrite(7, LOW);
      }
    }
  }
}
