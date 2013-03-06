

#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <poll.h>
#include <assert.h>


#define TIMEOUT		0
#define LEFT 		1
#define RIGHT 		2
#define FORWARD		3
#define	FASTER		4
#define SLOWER		5
#define STOP		6
#define REVERSE		7
#define PENUP		8
#define PENDOWN		9
#define READADC		10


void bt_write(int fd, char * data) {
	fprintf(stderr, "WRITE: %s\n", data);
	write(fd, data, strlen(data));

};

void bt_read(int fd) {
	char data[100];
	read(fd, data, sizeof(data));
	fprintf(stderr, "READ: %s\n", data);
}



void bt_connect(int fd, char * remote_address) {
	char str[100];
	tcflush(fd, TCIOFLUSH);
	tcflow(fd, TCION);
	tcflow(fd,TCOON);
	sleep(2);
	bt_write(fd, "$$$");
	sleep(1);
//	bt_read(fd);
	bt_write(fd,"\r\nK,\r\n");
	sleep(3);
	bt_write(fd, "$$$");
	sleep(2);

//	bt_read(fd);
	sprintf(str, "\r\nC,%s\r\n", remote_address);
	bt_write(fd, str);
//	bt_read(fd);
	sleep(5);
}


void dbg(char * stuff, ...) {
//	fprintf(stderr, stuff);
//	fprintf(stderr, "\n\r");
}

int prepare_input(void) {
	struct termios t;

	
	fcntl(0, F_SETFL, O_NONBLOCK);
	
	tcgetattr(0, &t);

	t.c_lflag &= ~(ICANON | ECHO);

	tcsetattr(0, 0, &t);

	return 0;
}	


int get_next_operation(void) {

	char key;


	struct pollfd pfd[1];
	int e;

	pfd[0].fd = 0;
	pfd[0].events = POLLIN;
	pfd[0].revents = 0;

	e = poll(pfd, 1, 100);

	if (pfd[0].revents & POLLIN) {


		e = read(0, &key, 1);

		if (e < 1) {
			dbg("Error");
			return 0;
		}

		switch(key){
		case '4':
			dbg("Left");
			return LEFT;
			break;
		case '5':
			dbg("Stop");
			return STOP;
			break;
		case '6':
			dbg("Right");
			return RIGHT;
			break;
		case '8':
			dbg("Forward");
			return FORWARD;
			break;
		case '2':
			dbg("Reverse");
			return REVERSE;
			
		case '[':
			dbg("Faster");
			return FASTER;
			break;
		case ']':
			dbg("Slower");
			return SLOWER;
			break;
		case 'u':
			dbg("Penup");
			return PENUP;
		case 'd':
			dbg("Pendown");
			return PENDOWN;
		case 'a':
			dbg("ADC");
			return READADC;
		};
	} else {
		dbg("Timeout");
		return TIMEOUT;
	}

	return (1);
}



#define C_MOTORS 'T'

#define C_MOTOR0 'x'
#define C_MOTOR1 'y'
#define C_LINE 	 'g'
#define C_GPIO   'b'
//#define C_SET	 's'
//#define C_RESET  'r'
#define C_STOP   'q'
#define C_CONTINUE 'c'
#define C_AUX 'u'

#define C_LEFT 'X'
#define C_RIGHT 'Y'

#define GP_M1R   1
#define GP_M0R   4
#define GP_M1F   2
#define GP_M0F	 8


int check_for_data(int fd) {

	struct pollfd fds[1];

	fds[0].fd = fd;
	fds[0].events = POLLIN;
	fds[0].revents = 0;

	poll(fds, 1, 0);

	if (fds[0].revents & POLLIN) {
		char data[100];
		int n;
		n = read(fd, data, 99);
		if (n>0) {
			data[n] = 0;
			printf("ROBOT SAYS: %s\n", data);
		} else {
			printf("Wierd read error: %i\n", n);
		}
	}
}


int set_pen(int fd, int pen) {
/*
	if(pen) {
		send_command(fd, C_AUX, 16);
	} else {
		send_command(fd, C_AUX, 0);
	}
	*/
}

int go_forward(int fd, int speed) {
//	send_command(fd, C_GPIO,  GP_M1F | GP_M0F);
	send_command2(fd, C_MOTORS,'0', speed, speed);
	return 0;
}

int go_backward(int fd, int speed) {
//	send_command(fd, C_GPIO,  GP_M1R | GP_M0R);
	send_command2(fd, C_MOTORS, '0', -speed, -speed);
	return 0;
}

int turn(int fd, int speed, int dir) {
	int mc = 0;
//	send_command(fd, C_GPIO, dir);
	if (dir == GP_M0F || dir == GP_M0R) {
		send_command2(fd, C_MOTORS, '0', speed, -speed);
	} else {
		send_command2(fd, C_MOTORS, '0', -speed, speed);
	}

}

int read_adc(int fd) {
	send_command0(fd, 'a', '1');
}


int stop(int fd) {
	go_forward(fd, 0);
}

int keepatit(int fd) {
	send_command0(fd, C_CONTINUE, '0');
}



	
int send_command0(int fd, int command, int sn) {
	char string[100];
	sprintf(string, "%c%c\n", command, sn);
	write(fd, string, strlen(string));
	return 0;
}

	
int send_command1(int fd, int command, int sn, int arg1) {
	char string[100];
	sprintf(string, "%c%c %i\n", command, sn, arg1);
	write(fd, string, strlen(string));
	return 0;
}

	
int send_command2(int fd, int command, int sn, int arg1, int arg2) {
	char string[100];
	sprintf(string, "%c%c %i %i\n", command, sn, arg1, arg2);
	write(fd, string, strlen(string));
	return 0;
}

	
int send_command3(int fd, int command, int sn, int arg1, int arg2, int arg3) {
	char string[100];
	sprintf(string, "%c%c %i %i %i\n", arg1, arg2, arg3);
	write(fd, string, strlen(string));
	return 0;
}


int send_command(int fd, int command, int argument) {

	char string[10];
	sprintf(string, "%c%.3i\n", command, argument);

	write(fd, string, strlen(string));
	return 0;


}


int prepare_rs232(char * file) {
	int fd;
	struct termios t;
	fprintf(stderr, "Connecting to %s\n", file);
	fd = open(file, O_RDWR | O_NONBLOCK);
	assert(fd >= 0);

	fprintf(stderr, "Configuring serial port at fd %i\n", fd);

        memset(&t, 0, sizeof(t));

	t.c_iflag = (IGNPAR | IGNBRK);

	t.c_oflag = 0;

	t.c_cflag = (CREAD | CLOCAL | CS8);

	t.c_lflag = 0;

	t.c_cc[VMIN]=2;

   	t.c_cc[VTIME]=0;

	cfsetispeed(&t, B9600);

        cfsetospeed(&t, B9600);

	tcflush(fd, TCIOFLUSH);

	tcsetattr(fd,TCSANOW,&t);

	fprintf(stderr, "Configured");
//	fcntl(fd, F_SETFL, 0);
	
	return fd;

};

int reallyforward = 0;


int main(int argc, char ** argv) {

	int what;
	char zero = 0;
	int speed = 100;
	int rs232_fd;
	int last_state = -1;
	prepare_input();

	rs232_fd = prepare_rs232(argv[1]);
	bt_connect(rs232_fd, argv[2]);

	send_command2(rs232_fd, C_MOTORS, '0', 0, 0);

	while (1) {
		check_for_data(rs232_fd);
		what = get_next_operation();
		if (what == FASTER) {
			speed +=5;
			if (speed > 250) {
				speed = 250;
			}
		}
		if (what == READADC) {

			read_adc(rs232_fd);
		}

		if (what == PENUP) {
			set_pen(rs232_fd, 1);
		}
		if (what == PENDOWN) {
			set_pen(rs232_fd, 0);
		}
		if (what == SLOWER) {
			speed -=5;
			if (speed < 0) {
				speed = 0;
			}
		}
		if (what != TIMEOUT) {
			keepatit(rs232_fd);
		}
		if (last_state != what) {
			last_state = what;

			if (what == FORWARD) {
				if (reallyforward) {
					go_forward(rs232_fd, speed);
				} else {
					last_state = -1;
				}
				reallyforward++;
			} else {
				reallyforward = 0;
			}

			if (what == LEFT) {
				turn(rs232_fd, speed, GP_M0F);
			}
			if (what == RIGHT) {
				turn(rs232_fd, speed, GP_M1F);
			}
			if (what == STOP || what == TIMEOUT) {
				stop(rs232_fd);
			}
			if (what == REVERSE) {
				go_backward(rs232_fd, speed);
			}


		}


	}


}


