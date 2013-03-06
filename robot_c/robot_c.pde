#include <stdlib.h>

unsigned short global_counter;
//char command[6];
int target_dir;
short ctrl_speed;
short ctrl_mode;
short ctrl_error;
#define MODE_STOP 0
#define MODE_GO 1

/*
#define MOTOR2_ENA 2
#define MOTOR2_DIR 5
#define MOTOR1_DIR 7
#define MOTOR1_ENA 4
#define MOTOR1_PWM 3
#define MOTOR2_PWM 6
*/


#define MOTORA_DAC 0xB00
#define MOTORB_DAC 0xD00
#define MOTOR_FREEQ 0x900
#define MOTORA_FWD 1
#define MOTORA_REV 2
#define MOTORB_FWD 4
#define MOTORB_REV  8

#define SENSORA_DAC 0x100
#define SENSORB_DAC 0x300


#define SPI_CLK 5
#define SPI_DATA 7
#define SPI_CS_DAC 3
#define SPI_CS_IO 4
void simple_delay(int d) {
  int x;
  for (x = 0; x < d; x++) {
    
  }
}

    
void spi_write(short data, int bits, int cs, int invert) {

  int i;
  digitalWrite(cs, invert);
  digitalWrite(SPI_CLK, invert);
  
  for (i = bits; i >= 0; i--) {
      digitalWrite(SPI_DATA, !!(data & (1lu << i)));
      simple_delay(10);
      digitalWrite(SPI_CLK, !invert);
      simple_delay(10);
      digitalWrite(SPI_CLK, invert);
      simple_delay(10);
  }
  
  digitalWrite(cs, !invert);
  simple_delay(10);
  digitalWrite(cs, invert);
  simple_delay(10);
  
}
void set_motors(int a, int b) {
  int bits;
  
  spi_write(MOTORA_DAC | (255 - abs(a)), 12, SPI_CS_DAC, 1);
  spi_write(MOTORB_DAC | (255 - abs(b)), 12, SPI_CS_DAC, 1);
  spi_write(0x100 | 0x800 | 64, 12, SPI_CS_DAC, 1);
  bits = 0;
  if (a > 0) {
    bits |= MOTORA_FWD;
  } else if (a < 0) {
    bits |= MOTORA_REV;
  } 
  
  if (b > 0) {
    bits |= MOTORB_FWD;
  } else if (b < 0){
    bits |= MOTORB_REV;
  }
  
  spi_write(bits, 8, SPI_CS_IO, 0);
}


void go() {
        if (ctrl_mode == MODE_GO) {
              int x,y;
           
              int diff = get_heading() - target_dir;
            diff = 0;
                            if (diff > 180) {
                      diff-=360;
              }
              if (diff < -180){
                      diff+=360;
              }
              if (diff > 40 || diff< -40) {
                      ctrl_mode = MODE_STOP;
                      ctrl_error = 1;
                      set_motors(0, 0);
                      
                      return;
              }

              
              diff = diff;
//              Serial.println(diff);  
              x = ctrl_speed + diff;
              y = ctrl_speed - diff;
              if (x > 255) x = 255;
              if (x < 0) x = 0;
              if (y < 0) y = 0;
              if (y > 255) y = 255;
              set_motors(x, y);
              
        }
}


void setup() {
        Serial.begin(9600);
        global_counter = 0;
//        pinMode(9, OUTPUT);
          pinMode(SPI_CLK, OUTPUT);
      pinMode(SPI_DATA, OUTPUT);
  pinMode(SPI_CS_DAC, OUTPUT);
  pinMode(SPI_CS_IO, OUTPUT);
  pinMode(13, OUTPUT);
  pinMode(12, OUTPUT);
   spi_write(SENSORA_DAC | 133, 12, SPI_CS_DAC, 1);
  spi_write(SENSORB_DAC | 134, 12, SPI_CS_DAC, 1);  
  spi_write(0x100 | 0x800 | 64, 12, SPI_CS_DAC, 1);
        set_motors(0, 0);    
        build_info_table();    
}


int global_x, global_y;
int last_tac;

int last_left, last_right;
void handle_tac(void) {
  
  int tl, tr;
  tl = digitalRead(8);
  tr = digitalRead(9);
  
  if (tl != last_left) {
    Serial.write("a\n");
    last_left = tl;  
  }
  
  if (tr != last_right) {
     Serial.write("b\n");
    last_right = tr;
  }
}




typedef void (*command_handler)(char cmd, int arg1, int arg2, int arg3, char * output);

struct cmd_info {
  char ci_args;
  command_handler ci_handler;
};

struct cmd_info cmd_info_table[255];


void build_info_table(void) {
  
    // manually set motors: T0 M1 M2
    // Make M1, M2 negative to turn backwards  
    cmd_info_table['T'].ci_args = 2;
    cmd_info_table['T'].ci_handler = handler_T;
    
    // Ping system
    cmd_info_table['c'].ci_args = 0;
    cmd_info_table['c'].ci_handler = handler_c;
    
    // Loopback test
    cmd_info_table['l'].ci_args = 3;
    cmd_info_table['l'].ci_handler = handler_l;
    
    cmd_info_table['g'].ci_args = 1;
    cmd_info_table['g'].ci_handler = handler_g;
    
    cmd_info_table['a'].ci_args = 0;
    cmd_info_table['a'].ci_handler = handler_a;
  
}

void set_default_output(char * o) {
  o[0] = 'o';
  o[1] = 'k';
  o[2] = 0;
}

void handler_a(char command, int arg1, int arg2, int arg3, char * output) {
  
  int r = get_heading();
  
  render_int(output, r);
  
  
}


void handler_g(char command, int arg1, int arg2, int arg3, char * output) {
  
  if (arg1 == 0) {
    set_motors(0,0);
    ctrl_speed = ctrl_mode = 0;
  } else {
    ctrl_speed = arg1;
    ctrl_mode = 1;
    target_dir = get_heading();
  }
  
  set_default_output(output);  
}

int render_int(char * output, int x) {
  int buffer[7];
  int i = 0;
  int used = 0;
  if (x < 0) {
    x = -x;
    *output = '-';
    used = 1;
  }
  
  do {
    int mod;
    mod = x%10;
    x/=10;
    buffer[i] = mod + '0';
    i++;
  } while(x);
  for(i--; i >=0; i--) {
    output[used] = buffer[i];
    used++;
  }
  output[used] = 0;
  return used;
      
}
void handler_T(char cmd, int arg1, int arg2, int arg3, char * output) {
     set_motors(arg1, arg2);
     set_default_output(output);
     
}

void handler_c(char cmd, int arg1, int arg2, int arg3, char * output) {
  set_default_output(output);
}

void handler_l(char cmd, int arg1, int arg2, int arg3, char * output) {
    int i;
    output[0] = 'o';
    output[1] = 'k';
    output[2] = ' ';
    
    i = 3;
    i += render_int(output + i, arg1);
    i += render_int(output + i, arg2);
    i += render_int(output + i, arg3);
    output[i] = 0;
    
}

void render_output(int sn, char * data) {
  
  Serial.print(sn);
  Serial.print(" ");
  Serial.print(data);
  Serial.print("\n");  
}

char input_buffer[20];
int input_buffer_l = 0;
  
void do_error(char * why) {
  Serial.print("Error: ");
  Serial.println(why);
  input_buffer_l = 0;
}


void parse_input(char c) {
 
    if (input_buffer_l >=18) {
      input_buffer_l = 0;
    }
    input_buffer[input_buffer_l] = c;
    input_buffer_l++;
    

 
   if (c == '\n')
  {
    
    int i, pos;
    int command;
    int serialno;
    int data[3];
    int datacount;
    input_buffer[input_buffer_l] = 0;
    command = input_buffer[0];
    if (command == '\n') {
      do_error("Empty command");
      return;
    }
    
    serialno = input_buffer[1];
    datacount = cmd_info_table[command].ci_args;
    i = 2;
    pos = 0;
    for(pos = 0; pos < datacount; pos++ ){
      int r;
      r = parse_int(input_buffer + i , data + pos);
      if (r == -1) {
        do_error("Cannot parse integer");
        return;  
      } else {
        i+=r;
      }
    }
    if (input_buffer[i] != '\n') {
      do_error("trailing garbage");
      return;
    }
     dispatch_command(command, serialno, data);
    input_buffer_l = 0;
  }
      
}    

void dispatch_command(int command, int serialno, int * data) {
    if (cmd_info_table[command].ci_handler) {
      
       char outputbuffer[20];
       global_counter = 10000;
       digitalWrite(12, HIGH);
       cmd_info_table[command].ci_handler(command, data[0], data[1], data[2], outputbuffer);
       if (serialno != '0') {
         render_output(serialno, outputbuffer);
       }
    } else {
       do_error("Unknown command");
       return;
    }
  
}


int parse_int(char * str, int * output) {

  int i = 0;
  int res = 0;
  int state = 0;
  int minus = 0;
  do {
    char c;
    c = str[i];
    if(c >='0' && c <='9') {
      state = 1;
      res = res * 10 + c - '0';
    } else if (state == 0 && (c== '-')) {
      minus = 1;
    } else if (state == 1 && (c == ' ' || c == '\n')) {
      *output = minus ? -res : res;
      return i;         
    } else if (state == 0 && c == ' ') {
    } else if (state ==0 && c == '\n') {
      return -1;
    } else {
      return -1;
    }
    i++;
  } while (1);

  }
void loop() {
  handle_tac();
//  go()
  
  while (Serial.available() > 0) {
    char d = Serial.read();
    parse_input(d);
  }
  global_counter--;
  if (global_counter == 0) {
    digitalWrite(12, LOW);
    set_motors(0, 0);
    ctrl_mode = MODE_STOP;
  }
  

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
         
 
