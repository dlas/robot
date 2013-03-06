#include <stdlib.h>

unsigned short global_counter;
char command[6];
int target_dir;
short ctrl_speed;
short ctrl_mode;
short ctrl_error;
#define MODE_STOP 0
#define MODE_GO 1

#define MOTOR2_ENA 2
#define MOTOR2_DIR 5
#define MOTOR1_DIR 7
#define MOTOR1_ENA 4
#define MOTOR1_PWM 3
#define MOTOR2_PWM 6


void go() {
        if (ctrl_mode == MODE_GO) {
              int x,y;
           
              int diff = get_heading() - target_dir;
             // diff = 0;
                            if (diff > 180) {
                      diff-=360;
              }
              if (diff < -180){
                      diff+=360;
              }
              if (diff > 40 || diff< -40) {
                      ctrl_mode = MODE_STOP;
                      ctrl_error = 1;
                      set_motor(0, 0, 0, 0);
                      
                      return;
              }

              
              diff = diff * 20;
//              Serial.println(diff);  
              x = ctrl_speed + diff;
              y = ctrl_speed - diff;
              if (x > 255) x = 255;
              if (x < 0) x = 0;
              if (y < 0) y = 0;
              if (y > 255) y = 255;
              analogWrite(MOTOR1_PWM, y);
              analogWrite(MOTOR2_PWM, x);
        }
}


void setup() {
        Serial.begin(9600);
        global_counter = 0;
//        pinMode(9, OUTPUT);
        pinMode(10, INPUT);
        pinMode(11, OUTPUT);
        pinMode(12, OUTPUT);
        pinMode(13, OUTPUT);
        pinMode(3, OUTPUT);
        pinMode(2, OUTPUT);
        pinMode(4, OUTPUT);
        pinMode(5, OUTPUT);
        pinMode(6, OUTPUT);
        pinMode(7, OUTPUT);
        set_motor(0, 0, 0, 0);        
}


int global_x, global_y;
int last_tac;
void handle_tac(void) {
  int td;
  int dx, dy;
  float p;
  td = digitalRead(10);
  digitalWrite(13, td);
  
  if (!td && last_tac) {
    p = get_headingf();
    dx = sin(p) * 10.0;
    dy = cos(p) * 10.0;
    global_x +=dx;
    global_y +=dy;
  
  }
  last_tac = td;
}


void loop() {
        handle_tac();
        go();
        command[5] = 0;
        global_counter --;
        if (global_counter == 0) {
                digitalWrite(12, LOW);
                set_motor(0, 0, 0, 0);      
                ctrl_mode = MODE_STOP;
}
        if (Serial.available()>=1) {
                int i;
                command[0] = command[1];
                command[1] = command[2];
                command[2] = command[3];
                command[3] = command[4];
                command[4] = Serial.read();
               Serial.println(command[3]);
        }
        
        if(command[4] == 10) {
                global_counter = 10000;
                parse_new_input(command);
                command[4] = 0;
                digitalWrite(12, HIGH);
           //     digitalWrite(13, HIGH);
        }
}



void parse_new_input(char * string) { 
       int i;
       char mode = string[0];
       i = atoi(string+1);
       switch (mode) {
               case('g'):
                       if(i==0) {
                               set_motor(0, 0, 0, 0);
                               
                               analogWrite(MOTOR1_PWM, 0);
                               analogWrite(MOTOR2_PWM, 0);
                               
                               ctrl_speed = 0;
                               ctrl_mode = 0;
                       } else {
                               set_motor(0, 1, 0, 1);
                               ctrl_speed = i;
                               ctrl_mode = 1;
                               target_dir = get_heading();
                               global_counter=65000;
                       }
                       break;
                
               case ('X'):
                       analogWrite(MOTOR1_PWM, i);
                       analogWrite(MOTOR2_PWM, i);
                       set_motor(1, 1, 0, 1);
                       break;
               case ('Y'):
                       analogWrite(MOTOR1_PWM, i);
                       analogWrite(MOTOR2_PWM, i);
                       set_motor(0, 1, 1, 1);
                       break;
               case ('x'):
                       analogWrite(MOTOR1_PWM, i);
                       set_motor(0, 1, 0, 0);
                       break;
               case ('y'):
                       analogWrite(MOTOR2_PWM, i);
                  
                        set_motor(0, 0, 0, 1);
                       break;
               case ('s'):
                       digitalWrite(i, HIGH);
                       break;
                       
               case('r'):
                       digitalWrite(i, LOW);
                       break;
               case('a'):
               {
                       int ax, ay;
                       ax= analogRead(A0);
                       ay = analogRead(A1);
                       Serial.println(ax);
                       Serial.println(ay);
                       break;
               }

                case ('l'):{
                 int deg = get_heading();
                 Serial.println(deg);
                 Serial.println(global_x);
                 Serial.println(global_y);
                 global_x = global_y = 0;
                 break;    
                }
               case('b'):
                       sr_write(i);
                       break;
               
               case('d'):
                {
                        int deg;
                        deg = get_heading();
                        Serial.println(deg);
                        
                 
         }
       }
       
       Serial.println("OK\n");
//       Serial.println(i);
}

float get_headingf() {
          float ax, ay, deg;
        ax = analogRead(A0) - 517;
        ay = analogRead(A1) - 510;
        deg = atan2(ax, ay);
        return deg;
}


int get_heading(){
        float ax, ay, deg;
        ax = analogRead(A0) - 517;
        ay = analogRead(A1) - 510;
        deg = atan2(ax, ay) * 180.0/3.14159;
        return deg;
}
         
#define  SR_CLR 2
#define  SR_DAT 7
#define  SR_CLK 5
#define SR_LOAD 4

int gpo_data;
void set_motor(int motor0_dir, int motor0_ena, int motor1_dir, int motor1_ena) {
  gpo_data &= 0xf0;
  
  if(motor0_dir) {
    gpo_data |= 1;
  }
  if (motor1_dir) {
    gpo_data |=4;
  }
  if (!motor0_ena) {
    gpo_data |=2;
  }
  if (!motor1_ena) {
    gpo_data |=8;
  }
  sr_write(gpo_data);
  
}



int sr_write(int x) {
    
  int l;
  digitalWrite(SR_CLR, HIGH);
  digitalWrite(SR_CLK, LOW);
  digitalWrite(SR_LOAD, LOW);
  for (l=0; l < 8; l++) {
        int b = !!((128>>l) & (x));
        digitalWrite(SR_DAT, b);
        digitalWrite(SR_CLK, HIGH);
        digitalWrite(SR_CLK, LOW);
  }
  
  digitalWrite(SR_LOAD, HIGH);
  
}

  
