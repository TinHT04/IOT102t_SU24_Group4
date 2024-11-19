#include <SoftwareSerial.h>
#include <LedControl.h>
#include <TimerOne.h> // Thư viện TimerOne

SoftwareSerial bluetooth(0, 2);  // kết nối chân TX, RX của Bluetooth module
                                 //TX: Transmits - HC05 NHẬN TÍN HIỆU ĐƯỢC GỬI từ thiết bị (điện thoại -> bluetooth )
                                 //RX: Receives  - HC05 GỬI TÍN HIỆU VỀ thiết bị           ( Bluetooth -> điện thoại)
                                 //ko hiểu tại sao nhưng chân 1 KO GỬI/NHẬN được
                                 //còn chân 0 thì KO GỬI VỀ dc nhưng NHẬN TÍN HIỆU ĐƯỢC
                                 //                                                        --> chân 0 dùng cho TX
                                 //                                                        --> chân 1 PHẾ TOÀN TẬP
                                 
                                 //Lưu ý siêu qtrong: trước khi upload thì tháo chân 0 với 1 ra rồi upload code
                                 //đợi code upload xong hoàn toàn (mất cái thanh loading) thì mới cắm chân 0 với 1 vào
                                 //nếu ko tháo 2 chân đó hoặc không đợi code upload xong hoàn toàn mà ghim vô thì 100% lỗi
                                 //code sẽ ko thể upload lên và Arduino sẽ xài lại code được upload thành công trước đó
                                 
LedControl lc = LedControl(10, 11, 12);    //kết nối chân DIN, CLK, CS
                                           //lỗi mao phắc: thực tế cắm như trên nhưng éo hiểu kiểu gì
                                           //mà đôi lúc bị lỗi nên cắm ngược lại: 10 -> DIN
                                           //                                     11 -> CS
                                           //                                     12 -> CLK

                                           //Lưu ý về Module 1 LED Matrix:
                                           //nó có tổng 10 chân cắm, 1 bên ở dưới IC MAX7219 (Mạch tích hợp MAX7219)
                                           //và 1 bên đối diện  
                                           //2 bên này đều có cùng chức năng chân cắm của chân đối diện tương ứng
                                           //TUY NHIÊN: Chân đối diện của DIN là DOUT !!!
                                           //cắm ngược thì đèn sáng lỗi ráng chịu, sửa dc mà lâu
                                           
#define red_led A2                // Chân kết nối LED đỏ
#define yellow_led A1             // Chân kết nối LED vàng
#define green_led A0              // Chân kết nối LED xanh

#define red_led2 A5               // Chân kết nối LED đỏ 2
#define yellow_led2 A4            // Chân kết nối LED vàng 2
#define green_led2 A3             // Chân kết nối LED xanh 2
//#define light_sensor A0         // Chân kết nối cảm biến ánh sáng
//tạm thời loại bỏ cảm biến A

const int buttonPin = 13;         //Chân kết nối button
int buttonState = 0;

// Chân kết nối 7 segment LED
#define segA 5 
#define segB 6
#define segC 9
#define segD 8
#define segE 7
#define segF 4
#define segG 3

unsigned long previousMillis = 0;
long interval = 30000; // Thời gian chờ để kiểm tra kết nối (30 giây), const là khai báo biến và KHÔNG THỂ THAY ĐỔI GIÁ TRỊ ĐƯỢC GÁN
bool automaticMode = false;
int currentLightState = 0;
bool autoStopped = false;
volatile int counter = 0;                     // Biến đếm cho ngắt timer
int flag = 0;                                 //biến cờ để bật đèn đỏ đi bộ cho Auto
int flagS = 0;                                //biến cờ để tắt 7 segment khi Auto tắt
int flagL = 0;                                //biến cờ để lúc nhập r hoặc g thì nó sáng (1 cách bình thường) - cờ Logic
int flag1 = 0;                                //biến cờ khống chế lần chạy đầu tiên
int flagY = 0;                                //biến cờ cho đèn vàng
int flagR = 0;                                //biến cờ lệnh trùng cho R
int flagG = 0;
void setup() {
  lc.shutdown(0, false);
  lc.setIntensity(0, 8);
  lc.clearDisplay(0);
  
  pinMode(red_led, OUTPUT);
  pinMode(green_led, OUTPUT);
  pinMode(yellow_led, OUTPUT);
  pinMode(red_led2, OUTPUT);
  pinMode(green_led2, OUTPUT);
  pinMode(yellow_led2, OUTPUT);
  Serial.begin(9600);
  bluetooth.begin(9600);
  bluetooth.write("Input R to turn on RED light\n");    //In ra thông báo ở điện thoại lần đầu chạy code
  bluetooth.write("Input Y to turn on YELLOW light\n");
  bluetooth.write("Input G to turn on GREEN light\n");
  bluetooth.write("Or A to turn on AUTOMATIC MODE\n");
  

  pinMode(segA, OUTPUT);
  pinMode(segB, OUTPUT);
  pinMode(segC, OUTPUT);
  pinMode(segD, OUTPUT);
  pinMode(segE, OUTPUT);
  pinMode(segF, OUTPUT);
  pinMode(segG, OUTPUT);

  pinMode(buttonPin, INPUT);
  
  turnOffLights();
  Timer1.initialize(1000000);                   // 1 giây
  Timer1.attachInterrupt(timerIsr);             // Gắn hàm xử lý ngắt
}


void loop() {
  unsigned long currentMillis = millis();
  // Kiểm tra nếu không có dữ liệu từ Bluetooth trong một khoảng thời gian nhất định
  if (currentMillis - previousMillis >= interval || flag1 == 0) {
      
      previousMillis = currentMillis;
      flagL = 0; //tắt cờ logic
      // Nếu không có dữ liệu từ Bluetooth trong một khoảng thời gian
      flagS = 1; // Biến cờ 7 Segment = 1 sẽ cho LED 7 SEGMENT đếm số countdown
      bluetooth.write("AUTOMATIC MODE is ON\n");

      while (!bluetooth.available()) { // Bật vòng lặp vô hạn khi không có dữ liệu từ Bluetooth
        interval = 30000; // Thời gian chờ để kiểm tra kết nối (1 giây)
        automa(); // Chạy hàm automa (automatic)
        if  (bluetooth.available()) {
          break;
        }
      }
  }
    if (bluetooth.available()) {
      previousMillis = currentMillis;
      flag1 = 1;
    char command = bluetooth.read(); 
      if (command == 'Y' || command == 'y') {   //đèn chớp tắt vàng (chạy vô hạn)
         flagS = 0;                             //ko cho chạy countdown
         flagL = 0;                             //tắt cờ logic
         flagY = 1;
         automaticMode = true;
         countdown(10);                         //tắt 7 Segment
         displayNone();                         //tắt 8x8
         bluetooth.write("-----Yellow LED turned on-----\n");
         bluetooth.write("Input R to turn on RED light\n");
         bluetooth.write("Input G to turn on GREEN light\n");
         bluetooth.write("Or A to turn on AUTOMATIC MODE\n");
         while (automaticMode) {
            if (bluetooth.available()) {
                command = bluetooth.read();     //nếu nhận ký tự tương ứng thì phá vòng lặp
                if (command == 'R' || command == 'A' || command == 'G' ||
                    command == 'r' || command == 'a' || command == 'g') {
                  automaticMode = false;        //phá while
                  break;                        //cấm chạy automaY
                } 
            }
            automaY();                          //chạy automaY (automatic Yellow) nếu ko có j được nhận từ Bluetooth
         }
      }
      if (command == 'R' || command == 'r') {
        flagS = 0;
        if (flagG = 1) {
          flagG = 0;
        }
        if (flagR == 1) {
          flagL = 0;
          flagR = 0;
        }
        countdown(10);
        bluetooth.write("-----Red LED turned on-----\n");
        bluetooth.write("Input Y to turn on YELLOW light\n");
        bluetooth.write("Input G to turn on GREEN light\n");
        bluetooth.write("Or A to turn on AUTOMATIC MODE\n"); 
        turnOffLights();
        redOn();                            //chạy 1 cách logic - đọc ở hàm redOn để hiểu thêm
        displayX();                         //8x8 bật O
        flagL = 1;                          //cắm cờ logic
        flagR = 1;                          //cắm cờ trùng
      } 
 
      if (command == 'G' || command == 'g') {
        flagS = 0;
        if (flagR == 1) {
          flagR = 0;
        }
        if (flagY == 0) {
          flagL = 1; // Cờ logic bật
        } else {
          flagY = 0;
        }
        if (flagG == 1) {
          flagL = 0;
          flagG = 0;
        }
        bluetooth.write("-----Green LED turned on-----\n");
        bluetooth.write("Input R to turn on RED light\n");
        bluetooth.write("Input Y to turn on YELLOW light\n");
        bluetooth.write("Or A to turn on AUTOMATIC MODE\n");
        countdown(10);
        turnOffLights();
        greenOn();
        displayO();
        flagG = 1;                          //cắm cờ trùng
        flagL = 1;                          //cắm cờ logic
      } 
      if (command == 'A' || command == 'a') {
       interval = 1; // Thời gian chờ để kiểm tra kết nối (1 giây), để khởi động lại chức năng Auto
       bluetooth.write("AUTOMATIC MODE will start after 1 second if no further input.\n");
    }
  } 
}

void greenOn() {
   if (flagL == 0) {                              //nếu cờ logic ko có
       digitalWrite(red_led2, HIGH);              //đèn cột 2 sáng đỏ
       digitalWrite(green_led, HIGH);             //đèn cột 1 sáng xanh
   } else {                                       //nếu có cờ logic
       digitalWrite(green_led2, LOW);             //cột 2 tắt xanh bật vàng
       digitalWrite(red_led, HIGH);               //cột 1 giữ đỏ
       digitalWrite(yellow_led2, HIGH);
       startCountdown(2);                         //đợi 3s
       waitForCountdownX();
       turnOffLights();
       digitalWrite(red_led2, HIGH);              //đèn cột 2 sáng đỏ
       digitalWrite(green_led, HIGH);             //đèn cột 1 sáng xanh
   }
}

void redOn() {                                //code đỏ sáng vv cải tiến
   if (flagL == 0) {                          //nếu cờ logic ko có
       digitalWrite(red_led, HIGH);           //đèn cột 1 sáng đỏ
       digitalWrite(green_led2, HIGH);        //đèn cột 2 sáng xanh
   } else {                                       //nếu có cờ logic
       digitalWrite(green_led, LOW);              //cột 1 tắt xanh bật vàng
       digitalWrite(red_led2, HIGH);               //cột 2 giữ đỏ
       digitalWrite(yellow_led, HIGH);
       startCountdown(2);               //đợi 3s
       waitForCountdownX();
       turnOffLights();
       digitalWrite(red_led, HIGH);           //đèn cột 1 sáng đỏ
       digitalWrite(green_led2, HIGH);        //đèn cột 2 sáng xanh
   }
}

void automaY() {
   turnOffLights();
   digitalWrite(yellow_led, HIGH);
   digitalWrite(yellow_led2, HIGH);
   startCountdown(1);               //đợi 2s
   waitForCountdownX(); 
   turnOffLights();                 //hết 2s thì tắt đèn
   startCountdown(1);               //đợi 2s
   waitForCountdownX();
}
void automa() {
  displayO();                       //8x8 hiện chữ X (ờ t đặt ngược vậy đó chứ ko phải ghi nhầm đâu, tại t lười sửa, xin lỗi)
  turnOffLights();                  //hàm tắt đèn
  digitalWrite(green_led, HIGH);    //đèn cột 1 sáng xanh
  digitalWrite(red_led2, HIGH);     //đèn cột 2 sáng đỏ
  countdown(3);
  startCountdown(2);                //đếm ngược 3s
  waitForCountdownSP();             //hàm countdown đặc biệt, tự chuyển đỏ (bộ) ngay lập tức nếu button được nhấn

  if (flag == 1) {                  //nếu button được nhấn thì flag = 1 và sẽ dẹp luôn cái hàm else
    flag = 0;                       //nếu không có cái flag thì khi đèn đỏ (bộ) đếm xong sẽ chuyển vàng rồi tới đỏ            -> ko đúng logic
  } else {                          //cái flag tồn tại là để khi đỏ (bộ) đếm xong sẽ chuyển xanh rồi tới vàng như bình thường -> đúng logic
                                    //khi flag = 1 thì chỉnh flag = 0 để lúc button ko dc nhấn thì đèn chạy bình thường 
    
    displayO();                     //8x8 hiện X
    turnOffLights();                //chị Dậu
    digitalWrite(red_led2, HIGH);   //cột 1 sáng vàng, cột 2 sáng đỏ
    digitalWrite(yellow_led, HIGH);
    startCountdown(2);
    waitForCountdown();
    
    displayX();
    turnOffLights();                //hàm tắt đèn
    digitalWrite(green_led2, HIGH); //cột 1 sáng đỏ, cột 2 sáng xanh
    digitalWrite(red_led, HIGH);
    countdown(6);
    startCountdown(5);
    waitForCountdownR();            //gọi hàm countdown đặc biệt thứ 2, cột 1 đang countdown màu đỏ mà còn 3s thì cột 2 sẽ chuyển vàng
  }                                 //hàm này tồn tại là để đảm bảo cả 2 cột đèn đều chạy đúng logic đỏ -> xanh -> vàng -> đỏ
}                                   //đảm bảo cả việc cả 2 đếm cùng tgian

void waitForCountdownSP() {
  while (counter >= -1) {
    countdown(counter + 1);
                                            // Đợi cho đến khi ngắt đếm ngược xong
    buttonState = digitalRead(buttonPin);   //đọc button
    if (buttonState == HIGH) {              //nếu button được nhấn (ờ nó ngược vậy đó, LOW là thả còn HIGH là ấn, tại thư viện chứ ko phải tại t)
    turnOffLights();                        //chị Dậu
    digitalWrite(red_led2, HIGH);   //cột 1 sáng vàng, cột 2 sáng đỏ
    digitalWrite(yellow_led, HIGH);
    countdown(3);
    startCountdown(2);
    waitForCountdown();

    turnOffLights();                        //hàm tắt đèn
    displayX();                             //8x8 hiện O
    digitalWrite(red_led, HIGH);            //cột 1 sáng đỏ, cột 2 sáng xanh
    digitalWrite(green_led2, HIGH);
    countdown(6);
    startCountdown(5);
    waitForCountdownR();                    //gọi hàm countdown đặc biệt thứ 2
    flag = 1;                               //chỉnh flag = 1 để khóa 2 đèn kia lại (đỡ bị sai logic)
  }
  }
}


void waitForCountdownR() {               //hàm countdown đặc biệt thứ 2 
  while (counter >= -1) {                //đếm ngược
    if (counter == 2) {                  //nếu còn 3s thì cột 2 tắt đèn xanh và chuyển vàng
        digitalWrite(green_led2, LOW);   //ko thể gọi hàm chị Dậu (turnOffLight) vì nó sẽ tắt cả 2 cột
        digitalWrite(yellow_led2, HIGH);
    }
    countdown(counter + 1);
  }
}

void waitForCountdown() {
  while (counter >= -1) {
    countdown(counter + 1);
  }
}
void waitForCountdownX() {
  while (counter >= -1) {
  }
}
void startCountdown(int seconds) {
  counter = seconds;
}

void timerIsr() {
  if (counter >= -1) {
    if (flagS == 1) {
      
      counter--;
    } else {
      counter--;
    }
  }
}

void displayX() {               //8x8 bật O
  byte X[8] = {
    B00111100,
    B01000010,
    B10000001,
    B10000001,
    B10000001,
    B10000001,
    B01000010,
    B00111100
  };

  for (int i = 0; i < 8; i++) {
    lc.setRow(0, i, X[i]);
  }
}

void displayNone() {
  byte X[8] = {                 //8x8 tắt
    B00000000,
    B00000000,
    B00000000,
    B00000000,
    B00000000,
    B00000000,
    B00000000,
    B00000000
  };

  for (int i = 0; i < 8; i++) {
    lc.setRow(0, i, X[i]);
  }
}

void displayO() {
  byte O[8] = {                 //8x8 bật X
    B10000001,
    B01000010,
    B00100100,
    B00011000,
    B00011000,
    B00100100,
    B01000010,
    B10000001
  };

  for (int i = 0; i < 8; i++) {
    lc.setRow(0, i, O[i]);
  }
}

void countdown(int count) {
  switch (count) {
    case 0:
      digitalWrite(segA, LOW);
      digitalWrite(segB, LOW);
      digitalWrite(segC, LOW);
      digitalWrite(segD, LOW);
      digitalWrite(segE, LOW);
      digitalWrite(segF, LOW);
      digitalWrite(segG, HIGH);
      break;
    case 1:
      digitalWrite(segA, HIGH);
      digitalWrite(segB, LOW);
      digitalWrite(segC, LOW);
      digitalWrite(segD, HIGH);
      digitalWrite(segE, HIGH);
      digitalWrite(segF, HIGH);
      digitalWrite(segG, HIGH);
      break;
    case 2:
      digitalWrite(segA, LOW);
      digitalWrite(segB, LOW);
      digitalWrite(segC, HIGH);
      digitalWrite(segD, LOW);
      digitalWrite(segE, LOW);
      digitalWrite(segF, HIGH);
      digitalWrite(segG, LOW);
      break;
    case 3:
      digitalWrite(segA, LOW);
      digitalWrite(segB, LOW);
      digitalWrite(segC, LOW);
      digitalWrite(segD, LOW);
      digitalWrite(segE, HIGH);
      digitalWrite(segF, HIGH);
      digitalWrite(segG, LOW);
      break;
    case 4:
      digitalWrite(segA, HIGH);
      digitalWrite(segB, LOW);
      digitalWrite(segC, LOW);
      digitalWrite(segD, HIGH);
      digitalWrite(segE, HIGH);
      digitalWrite(segF, LOW);
      digitalWrite(segG, LOW);
      break;
    case 5:
      digitalWrite(segA, LOW);
      digitalWrite(segB, HIGH);
      digitalWrite(segC, LOW);
      digitalWrite(segD, LOW);
      digitalWrite(segE, HIGH);
      digitalWrite(segF, LOW);
      digitalWrite(segG, LOW);
      break;
    case 6:
      digitalWrite(segA, LOW);
      digitalWrite(segB, HIGH);
      digitalWrite(segC, LOW);
      digitalWrite(segD, LOW);
      digitalWrite(segE, LOW);
      digitalWrite(segF, LOW);
      digitalWrite(segG, LOW);
      break;
    case 7:
      digitalWrite(segA, LOW);
      digitalWrite(segB, LOW);
      digitalWrite(segC, LOW);
      digitalWrite(segD, HIGH);
      digitalWrite(segE, HIGH);
      digitalWrite(segF, HIGH);
      digitalWrite(segG, HIGH);
      break;
    case 8:
      digitalWrite(segA, LOW);
      digitalWrite(segB, LOW);
      digitalWrite(segC, LOW);
      digitalWrite(segD, LOW);
      digitalWrite(segE, LOW);
      digitalWrite(segF, LOW);
      digitalWrite(segG, LOW);
      break;
    case 9:
      digitalWrite(segA, LOW);
      digitalWrite(segB, LOW);
      digitalWrite(segC, LOW);
      digitalWrite(segD, LOW);
      digitalWrite(segE, HIGH);
      digitalWrite(segF, LOW);
      digitalWrite(segG, LOW);
      break;
    case 10:
      digitalWrite(segA, HIGH);
      digitalWrite(segB, HIGH);
      digitalWrite(segC, HIGH);
      digitalWrite(segD, HIGH);
      digitalWrite(segE, HIGH);
      digitalWrite(segF, HIGH);
      digitalWrite(segG, HIGH);
      break;
  }
}

void turnOffLights() {            //hàm tắt đèn
  digitalWrite(red_led, LOW);
  digitalWrite(yellow_led, LOW);
  digitalWrite(green_led, LOW);
  digitalWrite(red_led2, LOW);
  digitalWrite(yellow_led2, LOW);
  digitalWrite(green_led2, LOW);
}
