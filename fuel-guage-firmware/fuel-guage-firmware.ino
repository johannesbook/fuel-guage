/*
 Johannes Book 2014. 
 Fuel guage tool, made for measuring low currents and count energy, 1uA -> 1A.  
 Initially made for Form (formdevices.com) but also used in other projects. 
 
 TODO:
 - Calculate energy metering, and test
 - check clock accuracy
 - check when clock overflows
 - Clean up schematics, write documentation and usage instructions, build copys

 Future improvements:
 Make calibration sticky (may not be neccessary to make on every boot) - store in eeprom
 Increase max voltage range by having a high-impedance divider _before_ the unity-gain OP instead of after. (hits maximum input voltage on the OP; VCC-1,5V)
 Add display output
 Add SD card storage
 
 Tests carried out:
 - step response, made sure LP filters on all three ranges has time constant well above the 10 ms sampling rate. 
 - tested that variations in supply voltage does not affect readings
 - accuracy tests (results below)
 
****************************
*   Test on sample #1      *
****************************

  Multimeter | DUT | Error  

  Range 0:
    9 uA  |    8 uA  |  <12%
   13 uA  |   11 uA  |  <16%
   19 uA  |   17 uA  |  <11%
   30 uA  |   29 uA  |  <4%
   46 uA  |   46 uA  |  <1%
   89 uA  |   89 uA  |  <1%
  212 uA  |  211 uA  |  <1%
  252 uA  |  249 uA  |  <2%
  325 uA  |  320 uA  |  <2%
  424 uA  |  419 uA  |  <2%
  
  Range 1:
  505 uA  |  516 uA  |  <3%
  613 uA  |  626 uA  |  <3%
  917 uA  |  921 uA  |  <1%
  1197 uA |  1198 uA |  <1%
  1704 uA |  1714 uA |  <1%
  11,3 mA |  11,1 mA |  <2%

  Range 2:
  20,8 mA |  21 mA   |  <1%
  51,4 mA |  52 mA   |  <2%
  96,0 mA |  95 mA   |  <2%
  235 mA  |  233 mA  |  <1%
  0,53 A  |  514 mA  |  <3%
  0,71 A  |  697 mA  |  <2%
  0,88 A  |  873 mA  |  <1%
  
  Voltage:
  285 mV  |   275 mV | <4%
  1,726 v |  1755 mV | <2%
  2,327 V |  2375 mV | <1%
  3,34 V  |  3415 mV | <3%
  3,60 V  |  3680 mV | <3%

Generally, better than 4% accuracy can be expected in average. Accuracy is worse in the lower area of each range (due to limited resolution), and at some points it might theoretically be slightly worse than 4%.

*/

signed long rangeZero[3] = {69,27,26}; //default calibration values
signed long currentNow, voltageNow, accCurrent, time; 
signed int range[3], voltage, usedRange;
String unit;

void setup() {
  cli(); //turn off interrupts
  Serial.begin(115200);
  analogReference(INTERNAL); //using 1.1V internal voltage ref to make this independent of supply voltage variations
  analogRead(A0); //to kick A/D in the butt, otherwise the reference change won't happen until later
  pinMode(2,OUTPUT); //debug pin
  time = 0;
  
  //setup interrupts
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;
  OCR1A = 156;
  TCCR1B |= (1 << WGM12); 
  TCCR1B |= (1 << CS12) | (1 << CS10);  //1024 prescaler
  TIMSK1 |= (1 << OCIE1A);   // enable timer compare interrupt  
  sei(); //allow interrupts
}

/***** Calibrate *****
To subtract the OP offset value, which varies with temperature and between units
*********************/
void Calibrate() {
  unsigned int c;
  rangeZero[0] = 0;
  rangeZero[1] = 0;
  rangeZero[2] = 0;
  
  Serial.println("Calibrating, please disconnect all three pins now. Let me know when done.");
  while(Serial.available()==0);
  Serial.print("Thanks. Give me a second... ");  
  for (c=0; c<1000; c++) {
    delayMicroseconds(7000); //since delay() messes up the timer interrupts we use for other things
  }
  for (c=0; c<2048; c++) { //some averaging
    rangeZero[0] += analogRead(A0);
    delay(2); //allow AD to cool down
    rangeZero[1] += analogRead(A1);
    delay(2); //allow AD to cool down
    rangeZero[2] += analogRead(A2);
    delay(2); //allow AD to cool down
  }
  rangeZero[0] = rangeZero[0] >> 11;
  rangeZero[1] = rangeZero[1] >> 11;
  rangeZero[2] = rangeZero[2] >> 11;
  Serial.print("offset values are: ");
  Serial.print(rangeZero[0]);
  Serial.print(", ");
  Serial.print(rangeZero[1]);
  Serial.print(", ");
  Serial.print(rangeZero[2]);
  Serial.println(".");
  for (c=0; c<1000; c++) { //delay to allow reading the last message
    delayMicroseconds(1000); //since delay() messes up the timer interrupts we use for other things
  }
  time = 0; //reset timer when calibrating
}  

/****** ISR ******
sample and accumulate exactly every 10 ms (100 Hz) (which is faster than hw LP filter, which means we won't miss anything in between samples)
Note: After adding stuff here, make sure this routine never closes in on 10ms (will starve main loop). If it exceeds 10 ms, things will break. 
*****************/
ISR(TIMER1_COMPA_vect){ //
  digitalWrite(2,HIGH);
  range[0] = analogRead(A0);
  range[1] = analogRead(A1);
  range[2] = analogRead(A2);
  if (range[0] < 1023) {
    currentNow = (range[0] - rangeZero[0]) / 2; //1 bit <=> 500 nA
    unit = "uA";
    usedRange = 0;
  } else if (range[1] < 1023) {
    currentNow = (range[1] - rangeZero[1]) * 20; //1 bit <=> 20 uA
    unit = "uA";
    usedRange = 1;
  } else {
    currentNow = (range[2] - rangeZero[2]); //1 bit <=> 1 mA
    unit = "mA";
    usedRange = 2;
  }  
  currentNow = currentNow * 118;  // 118/128 ~= -8%, which seems to be the trace resistance adding to the 50 mOhm resistor
  currentNow = currentNow >> 7; 
 
  voltage = analogRead(A3);
  voltageNow = (voltage * 5);
  
  time += 1; //each tick equals 10 ms
  
  digitalWrite(2,LOW);  
}

/****** MAIN ******
only printouts, maths and calibration, no sampling allowed here
*****************/
void loop() {

  int command, seconds, minutes, hours;
  #define TICS_PER_HOUR (360000UL)  
  #define TICS_PER_MINUTE (6000UL)
  #define TICS_PER_SECOND (100UL)

  while(1) {
    //Time:
    seconds = (time / TICS_PER_SECOND) % 60;
    minutes = (time / TICS_PER_MINUTE) % 60;
    hours = time / TICS_PER_HOUR;
    if (hours < 10) {
      Serial.print("0");
    }
    Serial.print(hours); 
    Serial.print(":");
    if (minutes < 10) {
      Serial.print("0");
    }
    Serial.print(minutes); 
    Serial.print(":");
    if (seconds < 10) {
      Serial.print("0");
    }
    Serial.print(seconds); 
    Serial.print(" | ");

    //Momentous values
    Serial.print(currentNow);
    Serial.print(" ");
    Serial.print(unit);
    Serial.print(" | ");
    Serial.print(voltageNow); 
    Serial.print(" mV");
    Serial.print(" | ");
    
    //Accumulated values
    
    //debug prints    
    Serial.print("   debug:");
    if (usedRange == 0) {
      Serial.print("*");
    }
    Serial.print(range[0]);
    Serial.print(" | ");
    if (usedRange == 1) {
      Serial.print("*");
    }
    Serial.print(range[1]);
    Serial.print(" | ");
    if (usedRange == 2) {
      Serial.print("*");
    }
    Serial.print(range[2]);
    Serial.print(" | ");
    Serial.print(voltage);
    Serial.print(" | ");
    Serial.println(time);
    
    if(Serial.available()!=0) {
      command = Serial.read();
      if (command == 0x63) { //0x63 = 'c'
        Calibrate();
      }
    }
    delay(200);
  }
}
