/*
 "MCPWM and Web test"
 made by maehara.

 Timer0 = Left
 Timer1 = Right
 
*/
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
#include <driver/mcpwm.h>
#include "soc/mcpwm_reg.h"
#include "soc/mcpwm_struct.h"

#define GPIO_PWM0A_OUT 15   //Set GPIO 15 as PWM0A
#define GPIO_PWM0B_OUT 16   //Set GPIO 16 as PWM0B
#define GPIO_PWM1A_OUT 18   //Set GPIO 18 as PWM1A
#define GPIO_PWM1B_OUT 19   //Set GPIO 19 as PWM1B


const char ssid[] = "ESP32-WiFi";  // SSID
const char pass[] = "maepu123";   // password
const IPAddress ip(192, 168, 0, 15);      // IPアドレス
const IPAddress subnet(255, 255, 255, 0); 

const char html[] =
"<!DOCTYPE html><html lang='ja'><head><meta charset='UTF-8'>\
<title>WiFi_Car Controller</title></head>\
<body><p>Motor Controller</p>\
<form method='get'>\
<input type='submit' name='le' value='Left' />\
<input type='submit' name='fo' value='Forward' />\
<input type='submit' name='ri' value='Right' /><br>\
<input type='submit' name='st' value='Stop' /><br>\
<input type='submit' name='bl' value='Backleft' />\
<input type='submit' name='ba' value='Back' />\
<input type='submit' name='br' value='Backright' /><br><br>\
</form></body></html>";

WiFiServer server(80);

static void mcpwm_example_gpio_initialize()
{
    printf("initializing mcpwm gpio...\n");
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, GPIO_PWM0A_OUT);
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0B, GPIO_PWM0B_OUT);
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM1A, GPIO_PWM1A_OUT);
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM1B, GPIO_PWM1B_OUT);
}

static void brushed_motor_forward(mcpwm_unit_t mcpwm_num, mcpwm_timer_t timer_num , float duty_cycle)
{
    mcpwm_set_signal_low(mcpwm_num, timer_num, MCPWM_OPR_B);
    mcpwm_set_duty(mcpwm_num, timer_num, MCPWM_OPR_A, duty_cycle);
    mcpwm_set_duty_type(mcpwm_num, timer_num, MCPWM_OPR_A, MCPWM_DUTY_MODE_0); //call this each time, if operator was previously in low/high state
}

static void brushed_motor_backward(mcpwm_unit_t mcpwm_num, mcpwm_timer_t timer_num , float duty_cycle)
{
    mcpwm_set_signal_low(mcpwm_num, timer_num, MCPWM_OPR_A);
    mcpwm_set_duty(mcpwm_num, timer_num, MCPWM_OPR_B, duty_cycle);
    mcpwm_set_duty_type(mcpwm_num, timer_num, MCPWM_OPR_B, MCPWM_DUTY_MODE_0);  //call this each time, if operator was previously in low/high state
}

static void brushed_motor_stop(mcpwm_unit_t mcpwm_num, mcpwm_timer_t timer_num)
{
    mcpwm_set_signal_low(mcpwm_num, timer_num, MCPWM_OPR_A);
    mcpwm_set_signal_low(mcpwm_num, timer_num, MCPWM_OPR_B);
}

void motor_init(void){
  //1. mcpwm gpio initialization
  mcpwm_example_gpio_initialize();

  //2. initial mcpwm configuration
  printf("Configuring Initial Parameters of mcpwm...\n");
  mcpwm_config_t pwm_config;
  pwm_config.frequency = 500;    //frequency = 250Hz,
  pwm_config.cmpr_a = 0;    //duty cycle of PWMxA = 0
  pwm_config.cmpr_b = 0;    //duty cycle of PWMxb = 0
  pwm_config.counter_mode = MCPWM_UP_COUNTER;
  pwm_config.duty_mode = MCPWM_DUTY_MODE_0;
  mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);    //Configure PWM0A & PWM0B with above settings  
  mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_1, &pwm_config);    //Configure PWM1A & PWM1B with above settings  
  brushed_motor_stop(MCPWM_UNIT_0, MCPWM_TIMER_0);
  brushed_motor_stop(MCPWM_UNIT_0, MCPWM_TIMER_1);
}

void setup() {
  Serial.begin(115200);

  WiFi.softAP(ssid, pass);           // SSIDとパスの設定
  delay(100);                        
  WiFi.softAPConfig(ip, ip, subnet); // IPアドレス、ゲートウェイ、サブネットマスクの設定
  IPAddress myIP = WiFi.softAPIP();  // WiFi.softAPIP()でWiFi起動
  server.begin();                    // サーバーを起動(htmlを表示させるため)
  /* 各種情報を表示 */
  Serial.print("SSID: ");
  Serial.println(ssid);
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  Serial.println("Server start!");
  
  /* Motor initiarize */ 
  motor_init();
  
}

void loop() {
  WiFiClient client = server.available();
  
  if (client) {                             // if you get a client,
    Serial.println("New Client.");           // print a message out the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
//        Serial.write(c);
        if (c == '\n') {   
          if (currentLine.length() == 0) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();
            client.print(html);
            client.println();
            break;
          } else {    // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
       if (currentLine.endsWith("GET /?fo")) {
//          Serial.println("OkFrt\n");
          brushed_motor_forward(MCPWM_UNIT_0, MCPWM_TIMER_0, 20.0);
          brushed_motor_forward(MCPWM_UNIT_0, MCPWM_TIMER_1, 20.0);
       }
       if (currentLine.endsWith("GET /?le")) {
          Serial.println("OkLe\n");
       }
       if (currentLine.endsWith("GET /?ri")) {
          Serial.println("OkRi\n");
       }
        if (currentLine.endsWith("GET /?ba")) {
//          Serial.println("OkBa\n");
          brushed_motor_backward(MCPWM_UNIT_0, MCPWM_TIMER_0, 20.0);
          brushed_motor_backward(MCPWM_UNIT_0, MCPWM_TIMER_1, 20.0);
       }
       if (currentLine.endsWith("GET /?bl")) {
          Serial.println("OKBL\n");
       }
       if (currentLine.endsWith("GET /?br")) {
          Serial.println("OKRI\n");
       }
       if (currentLine.endsWith("GET /?st")) {
//          Serial.println("OKSt\n");
          brushed_motor_stop(MCPWM_UNIT_0, MCPWM_TIMER_0);
          brushed_motor_stop(MCPWM_UNIT_0, MCPWM_TIMER_1);
       }
      }
    }

    // 接続が切れた場合
    client.stop();
    Serial.println("\nclient disonnected");
  }
}
