#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Preferences.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL), ...

#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

const byte analog_botons_port = 4; //Buttons rack connected to GPIO 19
int Button_Data = 0;
int buttons_value;
int velA = 512;
int velB = 512;
int velC = 512;
int MENU = 1;
int MENU_SELECTOR_CHOICE = 1;
int LEG_SELECTED = 0;
int LEG_SELECTOR_CHOICE = 1;
int LEG_MODE = 0;

int PREVIOUS_BUTTON = 0;
int SELECTED_MOTOR = 0;
String MOTOR_STATUS = "Stop";
int TIME_LIST[10] = {};

// Motor B es la pata cerntral.

const int MotorA_RPWM = 27;
const int MotorA_LPWM = 14;
const int MotorB_RPWM = 33;
const int MotorB_LPWM = 32;
const int MotorC_RPWM = 25;
const int MotorC_LPWM = 26;

const int  Channel_15 = 0;   //Canal virtual para valor PWM.
const int  Channel_14 = 1;   //Canal virtual para valor PWM.
const int  Channel_13 = 2;   //Canal virtual para valor PWM.
const int  Channel_12 = 3;   //Canal virtual para valor PWM.
const int  Channel_11 = 4;   //Canal virtual para valor PWM.
const int  Channel_10 = 5;   //Canal virtual para valor PWM.

int D1_H = 830;
int D1_L = 780;
int D2_H = 680;
int D2_L = 620;
int D3_H = 580;
int D3_L = 520;
int D4_H = 510;
int D4_L = 440;

int MotorA_Speed = 0;  //Resolucion a 10bits, (2^10)-1 = 1023.
int MotorB_Speed = 0;  //Resolucion a 10bits, (2^10)-1 = 1023.
int MotorC_Speed = 0;  //Resolucion a 10bits, (2^10)-1 = 1023.

const int freq = 20000;      //  Set up PWM Frequency
const int res = 10;          //  Set up PWM Resolution. Resolucion a 10bits, (2^10)-1 = 1023 PWM analógica motor. 

// Current Sensors configuration:

#define Current_Sensor_A 36
#define Current_Sensor_B 39
#define Current_Sensor_C 34

float CS_A_Value;
float CS_B_Value;
float CS_C_Value;

float R1 = 6800.0;
float R2 = 10000.0;
float ajuste_entrada = 0.20;
float Sensibilidad = 0.185; //For -5/+5 Limit Sensor Ampers.

Preferences preferences;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup() {
    Serial.begin(9600);
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    analogReadResolution(10);
    ledcSetup(Channel_15, freq,res); // setup PWM channel for BST A L_PWM
    ledcSetup(Channel_14, freq,res); // setup PWM channel for BST A R_PWM
    ledcSetup(Channel_13, freq,res); // setup PWM channel for BST B L_PWM
    ledcSetup(Channel_12, freq,res); // setup PWM channel for BST B R_PWM
    ledcSetup(Channel_11, freq,res); // setup PWM channel for BST C L_PWM
    ledcSetup(Channel_10, freq,res); // setup PWM channel for BST C R_PWM
    ledcAttachPin(MotorA_LPWM , Channel_15); // Attach BST A L_PWM
    ledcAttachPin(MotorA_RPWM , Channel_14); // Attach BST A R_PWM
    ledcAttachPin(MotorB_LPWM , Channel_13); // Attach BST B L_PWM
    ledcAttachPin(MotorB_RPWM , Channel_12); // Attach BST B R_PWM
    ledcAttachPin(MotorC_LPWM , Channel_11); // Attach BST C L_PWM
    ledcAttachPin(MotorC_RPWM , Channel_10); // Attach BST C R_PWM
    display.setRotation(2); //Rotate command: 0 = 0º, 1 = 90º, 2 = 180º, 3 = 270º
    display.clearDisplay();
    // Open namespace "MIOT", RW mode
    preferences.begin("MIOT", false);
    // Read previous value (default to 0)
    int counter = preferences.getInt("counter", 0);
    Serial.println("Current counter: " + String(counter));
    // Increment and store it back
    counter++;
    preferences.putInt("counter", counter);
    Serial.println("New counter saved: " + String(counter));
    // Close the Preferences
    preferences.end();
}

void loop() {
  set_button_data();
  get_menu_buttons();
  CS_A_Value = get_Current_Sensor(Current_Sensor_A, ajuste_entrada-1.67, Sensibilidad);
  CS_B_Value = get_Current_Sensor(Current_Sensor_B, ajuste_entrada, Sensibilidad);
  CS_C_Value = get_Current_Sensor(Current_Sensor_C, ajuste_entrada-0.1, Sensibilidad);
  PREVIOUS_BUTTON = Button_Data;
  delay(10);
}

void analog_value_screen(){
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.print("Test button: ");
  display.println(Button_Data);
  display.setCursor(0, 10);
  display.print("Analog Value: ");
  display.println(buttons_value);
  display.setCursor(0,20);
  display.print("CS: ");
  display.print(CS_A_Value);
  display.print(", ");
  display.print(CS_B_Value);
  display.print(", ");
  display.print(CS_C_Value);
  display.display();
}

void get_menu_buttons(){
  switch (MENU) {
      case 1:
          default_buttons();
          break;
      case 0:
          menu_selector();
          break;
      case 2:
          leg_selector();
          break;
      case 3:
          special_buttons();
          break;
      case 4:
          control_leg();
          break;
  }
}

void set_button_data(){
  buttons_value = analogRead(analog_botons_port);
  if (buttons_value <= D1_H && buttons_value >= D1_L) {
      Button_Data = 1;
    }
  else if (buttons_value <= D2_H && buttons_value >= D2_L) {
      Button_Data = 2;
    }  
  else if (buttons_value <= D3_H && buttons_value >= D3_L) {
      Button_Data = 3;
    }
  else if (buttons_value <= D4_H && buttons_value >= D4_L) {
      Button_Data = 4;
    }  
  else {
      Button_Data = 0;
    }
  Serial.print("Buttons data: ");
  Serial.println(Button_Data);
}

void default_buttons(){
  analog_value_screen();
  switch (Button_Data) {
    case 0:
      break;
    case 1:
      MotorA_Left(velA);
      MotorB_Left(velB);
      MotorC_Left(velC);
      Serial.println("TODOS ACTIVOS+");
      break;
    case 2:
      MotorA_Right(velA);
      MotorB_Right(velB);
      MotorC_Right(velC);
      Serial.println("TODOS ACTIVOS-");
      break;
    case 3:
      MotorA_Left(0);
      MotorB_Left(0);
      MotorC_Left(0);
      break;
    case 4:
      MENU = 0;
      break;
    }
}

void menu_selector(){
  int number_of_menus = 4;
  menu_display();

  if (PREVIOUS_BUTTON == Button_Data) {
    return;
  }
  switch (Button_Data) {
    case 0:
      break;
    case 1:
      MENU_SELECTOR_CHOICE = (MENU_SELECTOR_CHOICE + 1) % number_of_menus;
      break;
    case 2:
      MENU_SELECTOR_CHOICE = abs((MENU_SELECTOR_CHOICE - 1) % number_of_menus);
      break;
    case 3:
      MENU = MENU_SELECTOR_CHOICE;
      MENU_SELECTOR_CHOICE = 1;
      break;
    case 4:
      MENU = 1;
      break;
    }
}

void menu_display(){
    String menu_name = "Unavaible";
  switch (MENU_SELECTOR_CHOICE)
  {
  case 0:
    menu_name = "MENUS";
    break;
  case 1:
    menu_name = "HOME";
    break;
  case 2:
    menu_name = "LEG BASIC CONTROL";
    break;
  case 3:
    menu_name = "LEG ADVANCE CONTROL";
    break;
  
  default:
    break;
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.print("MENU: ");
  display.println(MENU_SELECTOR_CHOICE);
  display.display();
}

void leg_selector(){
  int number_of_legs = 3;
  legs_display();

  if (PREVIOUS_BUTTON == Button_Data) {
    return;
  }
  switch (Button_Data) {
    case 0:
      break;
    case 1:
      LEG_SELECTOR_CHOICE = (LEG_SELECTOR_CHOICE + 1) % number_of_legs;
      break;
    case 2:
      LEG_SELECTOR_CHOICE = abs((LEG_SELECTOR_CHOICE - 1) % number_of_legs);
      break;
    case 3:
      LEG_SELECTED = LEG_SELECTOR_CHOICE;
      LEG_SELECTOR_CHOICE = 1;
      MENU = 4
      break;
    case 4:
      MENU = 1;
      break;
    }
}

void legs_display(){
    String menu_name = "Unavaible";
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Change leg");
  display.println("Select Leg");
  display.display();
}

void control_leg(){

  if (PREVIOUS_BUTTON == Button_Data) {
    return;
  }
  switch (Button_Data) {
    case 0:
      MotorA_Left(0);
      MotorB_Left(0);
      MotorC_Left(0);
      MOTOR_STATUS = "Stop";
      break;
    case 1:
    switch (LEG_MODE)
    {
    case 0:
        switch (LEG_SELECTED) {
        case 0:
          MotorA_Left(velA);
          break;
        case 1:
          MotorB_Left(velB);
          break;
        case 2:
          MotorC_Left(velC);
          break;
      }
        break;
    case 1:
        switch (LEG_SELECTED) {
            case 0:
            MotorA_Rigth(velA);
            break;
            case 1:
            MotorB_Rigth(velB);
            break;
            case 2:
            MotorC_Rigth(velC);
            break;
        }
        break;
    }
      break;
    case 2:
        switch (LEG_MODE)
        {
        case 0:
            switch (LEG_SELECTED) {
                case 0:
                velA = velA + 5;
                break;
                case 1:
                velB = velB + 5;
                break;
                case 2:
                velC = velC + 5;
                break;
            break;
        
        case 1:
            switch (LEG_SELECTED) {
                case 0:
                velA = velA - 5;
                break;
                case 1:
                velB = velB - 5;
                break;
                case 2:
                velC = velC - 5;
                break;
            break;
            }
        }
      }
      break;
    case 3:
      LEG_MODE = (LEG_MODE + 1) % 3;
      break;
    case 4:
      MENU = 2;
      break;
    }
    leg_display();
}

void leg_display(){
    String mode_name = "Unavaible";
    int leg_voltage = 0;
  switch (LEG_MODE)
  {
  case 0:
    mode_name = "Move UP AND add voltage";
    break;
  case 1:
    mode_name = "Move DOWN And reduce voltage";
    break;
  }

  switch (LEG_SELECTED)
  {
  case 0:
    leg_voltage = velA;
    break;
  case 1:
    leg_voltage = velB;
    break;
  case 2:
    leg_voltage = velC;
    break;
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.print("Selected leg: ");
  display.println(SELECTED_LEG);
  display.print("Mode: ");
  display.println(mode_name);
  display.print("Leg voltage: ");
  display.println(leg_voltage);
  display.display();
}

void special_buttons(){

  if (PREVIOUS_BUTTON == Button_Data) {
    return;
  }
  switch (Button_Data) {
    case 0:
      MotorA_Left(0);
      MotorB_Left(0);
      MotorC_Left(0);
      MOTOR_STATUS = "Stop";
      break;
    case 1:
      switch (SELECTED_MOTOR) {
        case 0:
          MotorA_Left(velA);
          break;
        case 1:
          MotorB_Left(velB);
          break;
        case 2:
          MotorC_Left(velC);
          break;
      }
      MOTOR_STATUS = "Active UP";
      break;
    case 2:
      switch (SELECTED_MOTOR) {
        case 0:
          MotorA_Right(velA);
          Serial.println("MotorA-");
          break;
        case 1:
          MotorB_Right(velB);
          break;
        case 2:
          MotorC_Right(velC);
          Serial.println("MotorC-");
          break;
      }
      MOTOR_STATUS = "Active DOWN";
      break;
    case 3:
      SELECTED_MOTOR = (SELECTED_MOTOR + 1) % 3;
      break;
    case 4:
      MENU = 0;
      break;
    }
    special_display(SELECTED_MOTOR, MOTOR_STATUS);

}

void special_display(int seleted_motor, String motor_status){
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.print("Seleted Motor: ");
  display.println(seleted_motor);
  display.print("Motor Status: ");
  display.println(motor_status);
  display.display();
}

float get_Current_Sensor(int currentpin, float ajuste_entrada, float Sensibilidad){
  int adc = analogRead(currentpin);
  float adc_voltage = adc * (3.3 / 4096.0);
  float current_voltage = (adc_voltage * (R1+R2)/R2);
  float current = (current_voltage - 2.5 + ajuste_entrada) / Sensibilidad;
  return current;
}

void Motor_Moved(int ChannelA, int ChannelB, int SpeedA, int SpeedB){ // SpeedA o SpeedB uno de los dos tiene que ser 0.
  ledcWrite(ChannelA, SpeedA);
  ledcWrite(ChannelB, SpeedB);
}

void MotorA_Left(int MotorA_Speed){
  Motor_Moved(Channel_14, Channel_15, 0, MotorA_Speed);
}

void MotorA_Right(int MotorA_Speed){
  Motor_Moved(Channel_14, Channel_15, MotorA_Speed, 0);
}

void MotorB_Left(int MotorB_Speed){
  Motor_Moved(Channel_12, Channel_13, 0, MotorB_Speed);
}

void MotorB_Right(int MotorB_Speed){
  Motor_Moved(Channel_12, Channel_13, MotorB_Speed, 0);
}

void MotorC_Left(int MotorC_Speed){
  Motor_Moved(Channel_10, Channel_11, 0, MotorC_Speed);
}

void MotorC_Right(int MotorC_Speed){
  Motor_Moved(Channel_10, Channel_11, MotorC_Speed, 0);
}

void get_saved_times(int arr[], int listSize) {
    preferences.begin("miot", false);
    size_t size = sizeof(TIME_LIST);
    // Save as binary blob
    preferences.putBytes("TIME_LIST", TIME_LIST, size);
    preferences.end();

void get_saved_times(int arr[], int listSize) {
    preferences.begin("miot", false);

    // Read
    size_t bytesRead = preferences.getBytes("TIME_LIST", TIME_LIST, sizeof(TIME_LIST));
    int itemsRead  = bytesRead / sizeof(int);

    Serial.print("Items read: ");
    Serial.println(itemsRead);
    Serial.println(TIME_LIST);

    for (int i = 0; i < itemsRead; i++) {
    Serial.println(itemsRead[i]);
    }

    preferences.end();
}

}