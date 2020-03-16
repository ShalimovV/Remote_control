#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <LiquidCrystal.h>

#define menu_delay 100
#define analogInput A6

const int rs = 18, en = 1, d4 = 4, d5 = 3, d6 = 2, d7 = 0;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

const unsigned char BtnUpPin = 7;
const unsigned char BtnDownPin = 8;
const unsigned char BtnLeftPin = 5;
const unsigned char BtnRightPin = 6;
const unsigned char Btn1Pin = 14;
const unsigned char Btn2Pin = 15;
const unsigned char Btn3Pin = 16;
const unsigned char Btn4Pin = 17;
const unsigned char led1 = 0;
const unsigned char led2 = 1;


RF24 radio(9, 10);
//const byte address[6] = "00001";
const uint8_t num_channels = 126;
uint8_t current_chanel = 120;
// A single byte to keep track of the data being sent back and forth
byte counter = 1;

// Topology
const uint64_t pipes[2] = { 0xABCDABCD71LL, 0x544d52687CLL };              // Radio pipe addresses for the 2 nodes to communicate.

bool connected_flag = false;
bool TxFailFlag = 0;
byte data = 0xaa;
int TxData[5] = {0, 0, 0, 0, 0};
enum TxPower {MIN, MID, MAX};

byte BtnState = 0;
bool BtnUp = 1;
bool BtnDown = 1;
bool BtnLeft = 1;
bool BtnRight = 1;
bool Btn1 = 1;
bool Btn2 = 1;
bool Btn3 = 1;
bool Btn4 = 1;

unsigned int trottle = 250;
unsigned long timer1 = 0;
unsigned long ping = 0;
float ackData = 0;
bool lostFlag = 0;


// the setup function runs once when you press reset or power the board
void setup() {
  bool found_ch_flag = false;


  // initialize digital pin.
  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
  pinMode(BtnUpPin, INPUT);
  pinMode(BtnDownPin, INPUT);
  pinMode(BtnLeftPin, INPUT);
  pinMode(BtnRightPin, INPUT);
  pinMode(Btn1Pin, INPUT);
  pinMode(Btn2Pin, INPUT);
  pinMode(Btn3Pin, INPUT);
  pinMode(Btn4Pin, INPUT);
  digitalWrite(BtnUpPin, HIGH);
  digitalWrite(BtnDownPin, HIGH);
  digitalWrite(BtnLeftPin, HIGH);
  digitalWrite(BtnRightPin, HIGH);
  digitalWrite(Btn1Pin, HIGH);
  digitalWrite(Btn2Pin, HIGH);
  digitalWrite(Btn3Pin, HIGH);
  digitalWrite(Btn4Pin, HIGH);

  //Read Bat Voltage
  //pinMode(analogInput, INPUT);
  analogReference(INTERNAL);

  lcd.begin(16, 2);
  // Print a message to the LCD.
  lcd.print("Pult v0.1");
  delay(1000);
  lcd.clear();

  // Инициируем работу nRF24L01+
  radio.begin();
  if (radio.isChipConnected()) {
    lcd.setCursor(0, 0);
    lcd.print("Modem - Ok");
  }
  else {
    lcd.setCursor(0, 0);
    lcd.print("Modem - Fail!");
    while (1);
  }

  if (readBat() < 4) {
    lcd.setCursor(0, 0);
    lcd.print("Bat low");
    lcd.setCursor(0, 1);
    lcd.print("Pls charge");
    delay(1000);
  }

  //radio.setAutoAck(false);
  radio.setChannel      (current_chanel);                       // Указываем канал передачи данных (от 0 до 125), 67 - значит передача данных осуществляется на частоте 2,467 ГГц.
  radio.setDataRate     (RF24_1MBPS);                        // Указываем скорость передачи данных (RF24_250KBPS, RF24_1MBPS, RF24_2MBPS), RF24_1MBPS - 1Мбит/сек.
  radio.setPALevel      (RF24_PA_MIN);                       // Указываем уровень усиления передатчика (RF24_PA_MIN=-18dBm, RF24_PA_LOW=-12dBm, RF24_PA_HIGH=-6dBm, RF24_PA_MAX=0dBm).
  radio.enableDynamicPayloads();
  //radio.setAutoAck(1);                    // Ensure autoACK is enabled
  radio.enableAckPayload();               // Allow optional ack payloads
  radio.setRetries(15, 15);                // Smallest time between retries, max no. of retries
  //radio.setPayloadSize(5);                // Here we are sending 1-byte payloads to test the call-response speed
  radio.openWritingPipe(pipes[0]);        // Both radios listen on the same pipes by default, and switch when writing
  radio.openReadingPipe(1, pipes[1]);
  radio.startListening();

  delay(1000);

  connect_dev();

  lcd.clear();



}

void scan_btn(void) {

  BtnUp = digitalRead(BtnUpPin);
  BtnDown = digitalRead(BtnDownPin);
  BtnLeft = digitalRead(BtnLeftPin);
  BtnRight = digitalRead(BtnRightPin);
  Btn1 = digitalRead(Btn1Pin);
  Btn2 = digitalRead(Btn2Pin);
  Btn3 = digitalRead(Btn3Pin);
  Btn4 = digitalRead(Btn4Pin);
  BtnState = BtnUp << 0 | BtnDown << 1 | BtnLeft << 2 | BtnRight << 3 | Btn1 << 4 | Btn2 << 5 | Btn3 << 6 | Btn4 << 7;
  TxData[0] = BtnState;
}

float readBat(void) {
  float vout = 0.0;
  float vin = 0.0;
  float R1 = 10000.0; // resistance of R1 (100K) -see text!
  float R2 = 998.0; // resistance of R2 (10K) - see text!
  int value = 0;

  value = analogRead(analogInput);
  vout = (value * 1.1) / 1023.0; // see text
  vin = vout / (R2 / (R1 + R2));
  if (vin < 0.09) {
    vin = 0.0; //statement to quash undesired reading !

  }
  return vin;

}

void connect_dev(void) {

  radio.stopListening();
  radio.flush_rx();
  delay(1);
  radio.startListening();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Waiting device");

  while (!radio.available(pipes[1])) {
    scan_btn();
    //delay(5);
    if (!Btn1) {
      radio.stopListening();
      return;
    }
  }
  //radio.read(tmp, 1);
  lcd.setCursor(0, 1);
  lcd.print("Connected");
  delay(1000);
  lcd.clear();

  radio.stopListening();

  //radio.writeAckPayload(pipes[0], &data, sizeof(data));
}


void menu(void) {
  unsigned char pos = 0;
  unsigned char tmp_ch = radio.getChannel();
  unsigned long timer1 = 0;

  while (true) {
    scan_btn();
    if (!BtnRight) {
      pos++;
      if (pos > 2) pos = 2;
    }
    if (!BtnLeft) {
      pos--;
      if (pos < 0) pos = 0;
    }

    switch (pos) {

      case 0:

        scan_btn();
        if (!BtnUp) {
          tmp_ch++;
          if (tmp_ch > 125) tmp_ch = 125;
          TxData[3] = 0x33;
          TxData[4] = tmp_ch;
        }
        if (!BtnDown) {
          tmp_ch--;
          if (tmp_ch > 125) tmp_ch = 125;
          TxData[3] = 0x33;
          TxData[4] = tmp_ch;
        }
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Set Channel");
        lcd.setCursor(0, 1);
        lcd.print(tmp_ch);
        delay(menu_delay);

        break;

      case 1:

        scan_btn();
        if (!BtnUp) trottle = trottle + 10;
        if (!BtnDown) trottle = trottle - 10;
        if (trottle > 250) trottle = 250;
        TxData[1] = trottle;
        TxData[3] = 0x44;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Trottle:");
        lcd.setCursor(0, 1);
        lcd.print(trottle);
        delay(menu_delay);
        break;

      case 2:
        char i;
        scan_btn();
        if (!BtnUp) i++;
        if (!BtnDown) i--;
        if (i > 2) i = 2;
        if (i < 0) i = 0;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("TX Power:");
        lcd.setCursor(0, 1);
        if (i == 0) {
          lcd.print("MIN");
          radio.setPALevel(RF24_PA_MIN);
        }
        if (i == 1) {
          lcd.print("MID");
          radio.setPALevel(RF24_PA_HIGH);
        }
        if (i == 2) {
          lcd.print("MAX");
          radio.setPALevel(RF24_PA_MAX);
        }
        break;

    default:
      break;
  }
  scan_btn();
    if (!Btn1) return;
    delay(menu_delay);
  }

}



// the loop function runs over and over again forever
void loop() {
  unsigned char push_counter = 0;
  

  scan_btn();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Ch");
  lcd.print(radio.getChannel());
  lcd.setCursor(6, 0);
  if (radio.getPALevel() == 0) lcd.print("MIN");
  if (radio.getPALevel() == 2) lcd.print("MID");
  if (radio.getPALevel() == 3) lcd.print("MAX");
  lcd.setCursor(10, 0);
  lcd.print(BtnState);
  lcd.setCursor(0, 1);
  lcd.print("L");
  lcd.print(readBat());
  lcd.setCursor(6, 1);
  lcd.print("R");
  lcd.print(ackData);
  lcd.setCursor(12, 1);
  lcd.print(ping % 1000);
  lcd.setCursor(14, 0);
  if (lostFlag) lcd.print("F");

  scan_btn();
  timer1 = micros();
  while (!radio.write(&TxData, sizeof(TxData))) {
    if (radio.isAckPayloadAvailable()) {
      radio.read(&ackData, sizeof(ackData));
      ping = micros() - timer1;
    }
    if (micros() - timer1 > 200000) {
      lostFlag = 1;
      break;
    }
    lostFlag = 0;
  }
  if (TxData[3] == 0x33) {
    radio.setChannel(TxData[4]);
    TxData[3] = 0;
  }

  while (!Btn2 & !Btn4) {
    push_counter++;
    delay(50);
    scan_btn();
    if (push_counter > 50) {
      push_counter = 0;
      connect_dev();
    }
  }
  while (!Btn3) {
    push_counter++;
    delay(50);
    scan_btn();
    if (push_counter > 50) {
      push_counter = 0;
      menu();
    }
  }

  delay(5);
}







/*
  radio.startListening();
  delayMicroseconds(128);
  radio.stopListening();
  if (radio.testRPD()) {
    lcd.setCursor(0, 1);
    lcd.print("Chanel 1 - Open");
  }
  else {
    lcd.setCursor(0, 1);
    lcd.print("Ch 1 - Bad");
    delay(1000);
    lcd.clear();

    int rep_counter = 10;
    while (rep_counter--) {
      int i = num_channels;
      if (found_ch_flag) {
        break;
      }
      while (i--)
      {
        // Select this channel
        radio.setChannel(i);

        lcd.setCursor(0, 0);
        lcd.print("Start scan ");
        lcd.print(rep_counter);
        lcd.setCursor(0, 1);
        lcd.print("Ch ");
        lcd.print(i);
        lcd.print(" - ");


        // Listen for a little
        radio.startListening();
        delayMicroseconds(128);
        radio.stopListening();

        // Did we get a carrier?
        if ( radio.testRPD() ) {
          lcd.setCursor(0, 1);
          lcd.print("Using ch - ");
          lcd.print(i);
          found_ch_flag = true;
          break;
        }
        else {
          lcd.print("Fail");
        }
        delay(10);
        lcd.clear();
      }
    }
  }
  if (!found_ch_flag) {
    delay(1000);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Scan fail");
    lcd.setCursor(0, 1);
    lcd.print("Using 1 ch");
    radio.setChannel(1);
  }

  bool goodSignal = radio.testRPD();
  if (radio.available()) {
    lcd.setCursor(0, 1);
    lcd.print(goodSignal ? "OK" : "Low signal" );
    radio.read(0, 0);
  }


  if (!radio.write( &counter, 1 )){
      if(!radio.available()){
        lcd.setCursor(0, 1);
      lcd.print("Empty " );
      lcd.print(micros() - time );
      lcd.print("us" );
      }else{
        while(radio.available() ){
          unsigned long tim = micros();
          radio.read( &gotByte, 1 );
          lcd.setCursor(0, 1);
          lcd.print(tim-time);
          lcd.print("us");
          //printf("Got response %d, round-trip delay: %lu microseconds\n\r",gotByte,tim-time);
          counter++;
        }
      }

    }

    else{
      lcd.setCursor(0, 1);
      lcd.print("No signal" );
    }

    //if (!BtnUp or !BtnDown or !BtnLeft or !BtnRight or !Btn1 or !Btn2 or !Btn3 or !Btn4) {

      //radio.write(&BtnState, sizeof(BtnState));
      //while (radio.write(&BtnState, sizeof(BtnState)))
    //}

*/
