

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
	bt_write(fd, "+++\n\r");
	sleep(1);
//	bt_read(fd);
	bt_write(fd,"K,\n\r");
	sleep(1);
	bt_write(fd, "+++\n\r");

//	bt_read(fd);
	sprintf(str, "C,%s\n\r", remote_address);
	bt_write(fd, str);
//	bt_read(fd);
	sleep(5);
}


void dbg(char * stuff, ...) {
	fprintf(stderr, stuff);
	fprintf(stderr, "\n\r");
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
		};
	} else {
		dbg("Timeout");
		return TIMEOUT;
	}

	return (1);
}

#define C_MOTOR0 0
#define C_MOTOR1 1
#define C_GPIO	 2

#define GPIO_MENABLE	1
#define GPIO_BACKWARDS	6


int send_command(int fd, int command, int argument) {

	char string[10];
	string[0] = '0' + command;

	string[1] = argument / 16 + 'A';
	string[2] = argument % 16 + 'A';
	string[3] = 10;
	string[4] = 0;
	

	write(fd, string, 5);
	return 0;


}



int prepare_rs232(char * file) {
	int fd;
	fd = open(file, O_RDWR);
	assert(fd >= 0);
	return fd;

};




int main(int argc, char ** argv) {

	int what;
	char zero = 0;
	int speed = 100;
	int rs232_fd;
	int last_state = -1;
	prepare_input();

	rs232_fd = prepare_rs232(argv[1]);
	bt_connect(rs232_fd, argv[2]);

	send_command(rs232_fd, C_MOTOR1, 0);
	send_command(rs232_fd, C_MOTOR0, 0);
	send_command(rs232_fd, C_GPIO, GPIO_MENABLE);
	while (1) {
		what = get_next_operation();
		if (what == FASTER) {
			speed +=5;
			if (speed > 250) {
				speed = 250;
			}
		}
		if (what == SLOWER) {
			speed -=5;
			if (speed < 0) {
				speed = 0;
			}
		}
		if (what != TIMEOUT) {
			write(rs232_fd, &zero, 1);
		}
		if (last_state != what) {
			if (what != TIMEOUT) {
				
				send_command(rs232_fd, C_GPIO, GPIO_MENABLE);
			}


			if (what == FORWARD) {
				send_command(rs232_fd, C_MOTOR0, speed * 1.5);
				send_command(rs232_fd, C_MOTOR1, speed);
			}

			if (what == LEFT) {
				send_command(rs232_fd, C_MOTOR0, speed * 1.5);
				send_command(rs232_fd, C_MOTOR1, 0);
			}
			if (what == RIGHT) {
				send_command(rs232_fd, C_MOTOR0, 0);
				send_command(rs232_fd, C_MOTOR1, speed);
			}
			if (what == STOP) {
				send_command(rs232_fd, C_MOTOR1, 0);
				send_command(rs232_fd, C_MOTOR0, 0);
			}
			if (what == TIMEOUT) {
				send_command(rs232_fd, C_MOTOR1, 0);
				send_command(rs232_fd, C_MOTOR0, 0);
			}
			if (what == REVERSE) {

				send_command(rs232_fd, C_MOTOR0, speed * 1.5);
				send_command(rs232_fd, C_MOTOR1, speed);
				send_command(rs232_fd, C_GPIO, GPIO_MENABLE | GPIO_BACKWARDS);
			}


		}
		last_state = what;


	}


}


