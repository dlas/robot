

#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <poll.h>
#include <assert.h>
#include <sys/time.h>
#include <errno.h>

#include "nav.c"
#include "Xgraphics.c"

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
#define AUTOPILOT	11

#define RS_IDLE 	1
#define RS_MANUAL	2
#define RS_MOVING	3
#define RS_STARMOVE	4
#define RS_MOVEDONE 5


typedef struct robot_status {
	int rs_rs232_fd;
	struct timeval rs_basetime;
	struct input_buffer * rs_input;
} robot_status;


void display_data(navdata * d, int now) {
	printf("AT: %i. X: %3g Y:%3g pheta: %3g    ticks: %i,%i                \r", now, d->nv_x,
	    d->nv_y, d->nv_pheta * 180.0/3.1415, d->nv_left_speed.ve_count, d->nv_right_speed.ve_count);
	fflush(stderr);
	fflush(stdout);

	/*
	printf("LEFT: %i %i. RIGHT: %i %i\n\n",
	    d->nv_left_speed.ve_pointer, d->nv_left_speed.ve_average,
	    d->nv_right_speed.ve_pointer, d->nv_right_speed.ve_average);
	    */
}

int get_rel_time(struct timeval * base) {
	struct timeval now, diff;

	gettimeofday(&now, NULL);
	timersub(&now, base, &diff);
	return(diff.tv_sec * 1000 + diff.tv_usec / 1000);

}



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

void process_data(navdata * n, char * buffer, int now) {


	if (*buffer >='0' && *buffer <='9') {
		int n1, n2;
		sscanf(buffer, "%i %i", &n1, &n2);
		if (n1 == '1') {
			printf("ADC RESULT: %i\n", n2);
			set_direction(n, (float)((270 - n2))*3.1415/180.0);

		} else {
			fprintf(stderr, "Wierd data: %s\n", buffer);
		}
	} else {
		switch(*buffer) {
		case ('A'):
			update_speed_tick(&(n->nv_left_speed), 0, now);
			break;
		case ('a'):
			update_speed_tick(&(n->nv_left_speed), 1, now);
			break;
		case ('B'):
			update_speed_tick(&(n->nv_right_speed), 0, now);
			break;
		case ('b'):
			update_speed_tick(&(n->nv_right_speed), 1, now);
			break;
		case (0): 
			break;
		case ('\n'):
			break;
		default:
			fprintf(stderr, "Wierd data: %c", *buffer);
		}
	}
}


typedef struct input_buffer {
	char ib_buffer[100];
	int ib_position;
	int ib_state;
} input_buffer;

void process_robot_answer(int robot_fd, int now, navdata * nav, input_buffer * ib) {

	int rlen;
	char newchr;

	do {
		rlen = read(robot_fd, &newchr, 1);
		
		if (rlen == 1) {
			if (ib->ib_position >98) {
				fprintf(stderr, "Unterminated read!\n");
				ib->ib_position = 0;
			}
			if (newchr == '\n') {
				ib->ib_buffer[ib->ib_position] = 0;
				process_data(nav, ib->ib_buffer, now);
				ib->ib_position = 0;
			} else {
				ib->ib_buffer[ib->ib_position] = newchr;
				ib->ib_position++;
			}
		} else if (rlen < 0) {
			fprintf(stderr, "READ IS SAD!!!: %i %i %s\n", robot_fd, rlen, strerror(errno));
		}

	} while (rlen > 0);

}

void process_robot_answer2(int robot_fd, int now, navdata * nav) {

	char buffer[2];
	ssize_t rlen;

	rlen = read(robot_fd, buffer, 1);

	if (rlen == 1) {
		process_data(nav, buffer, now);

//		fprintf(stderr, "GOT %c\n", *buffer);
	} else {
		fprintf(stderr, "READ IS SAD!!!: %i %i %s", robot_fd, rlen, strerror(errno));

	}
}


int event_loop(int robot_fd, navdata * nav, input_buffer * ib, int now, int timeout) {
	struct pollfd pfd[2];

	char key;
	int e;
	int err;
	pfd[0].fd = 0;
	pfd[0].events = POLLIN;
	pfd[0].revents = 0;

	pfd[1].fd = robot_fd;
	pfd[1].events = POLLIN;
	pfd[1].revents = 0;

	err = poll(pfd, 2, timeout);


	if (pfd[1].revents & POLLIN) {
		process_robot_answer(robot_fd, now, nav, ib);
	}



	if (pfd[0].revents & POLLIN) {


		e = read(0, &key, 1);

		if (e < 1) {
			dbg("Error");
			return 0;
		}

		switch(key){
		case 'a':
		case '4':
			dbg("Left");
			return LEFT;
			break;
		case '5':
			dbg("Stop");
			return STOP;
			break;
		case 'd':
		case '6':
			dbg("Right");
			return RIGHT;
			break;
		case'w':
		case '8':
			dbg("Forward");
			return FORWARD;
			break;
		case 's':
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
		case 'U':
			dbg("Pendown");
			return PENDOWN;
		case 'C':
			dbg("ADC");
			return READADC;
		case 'P':
			dbg("Autopilot");
			return AUTOPILOT;

		};


	} else {
//		dbg("Timeout");
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
		send_command2(fd, C_MOTORS, '0', speed, 0);
	} else {
		send_command2(fd, C_MOTORS, '0', 0, speed);
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
	fd = open(file, O_RDWR| O_NONBLOCK);
	assert(fd >= 0);

	fprintf(stderr, "Configuring serial port at fd %i\n", fd);

        memset(&t, 0, sizeof(t));

	t.c_iflag = (IGNPAR | IGNBRK);

	t.c_oflag = 0;

	t.c_cflag = (CREAD | CLOCAL | CS8);

	t.c_lflag = 0;

	t.c_cc[VMIN]=1;

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


void do_gfx_nav_update(navdata * nav_info, int now, Window robotwin, int motor_l_speed, int motor_r_speed) {

	int oldx, oldy;
	oldx = nav_info->nv_x;
	oldy = nav_info->nv_y;
	update_position(nav_info, now, motor_l_speed, motor_r_speed);
	display_data(nav_info, now);
	FillCircle(robotwin, oldx + 256, oldy + 256, 4, 3);
	FillCircle(robotwin, nav_info->nv_x + 256, nav_info->nv_y + 256, 2, 2);

	{
		int x, y;
		x = cos(nav_info->nv_pheta) * 30;
		y = sin(nav_info->nv_pheta) * 30;

		FillCircle(robotwin, 40, 40, 30, 0);
		DrawLine(robotwin, 40, 40, 40+x, 40+y, 2);
		DrawCircle(robotwin, 40, 40, 30, 4);
	}

	XSync(mydisplay, True);
	

}


void drive(robot_status * rs, navdata * nv, Window robotwin, int init_m0_speed, int init_m1_speed,
    int (*update)(robot_status * rs, navdata * nv, int * m0_speed, int * m1_speed)) {
	int what;
	int next;
	int now;
	int verynext;
	int res;
	now = get_rel_time(&rs->rs_basetime);
	verynext = now + 100;
	next = now + 50;
	read_adc(rs->rs_rs232_fd);
	do {
		now = get_rel_time(&(rs->rs_basetime));
		dbg("DING");
		what = event_loop(rs->rs_rs232_fd, nv, rs->rs_input, now, next - now);

		if (now >= next) {
			do_gfx_nav_update(nv, now, robotwin, init_m0_speed, init_m1_speed);
			next = now + 50;
		}
		if (now >= verynext) {
			send_command2(rs->rs_rs232_fd, C_MOTORS,'0', init_m0_speed, init_m1_speed);
			verynext = now + 100;
		}
		if (update) {
			res =  update(rs, nv, &init_m0_speed, &init_m1_speed);
		} else {
			res = 1;
		}

		if (what != TIMEOUT) {
			dbg("USER ABORT USER ABORT USER ABORT");
		}
	} while((what == TIMEOUT) &&  res);
	send_command2(rs->rs_rs232_fd, C_MOTORS,'0', -init_m0_speed, -init_m1_speed);
	usleep(50000);
	send_command2(rs->rs_rs232_fd, C_MOTORS,'0', 0, 0);
	vestimator_reset(&(nv->nv_left_speed), now);
	vestimator_reset(&(nv->nv_right_speed), now);
	read_adc(rs->rs_rs232_fd);

}

// Its shitty cuz I'm lazy
float pi_reduce(float  x) {
	if (isfinite(x)) {
		while (x > M_PI) {
			x-= 2*M_PI;
		}
		while (x < -M_PI) {
			x+=2*M_PI;
		}
	} 

	return x;

}

void autoturn(robot_status * rs, navdata * nv, Window robotwin, float direction, int speed) {

	float mult;
	int m0_count, m1_count;
	int needed_count_diff;
	float delta_angle;

	m0_count = nv->nv_left_speed.ve_count;
	m1_count = nv->nv_right_speed.ve_count;


	delta_angle = pi_reduce(nv->nv_pheta - direction);

	if (pi_reduce(nv->nv_pheta - direction) < 0) {
		mult = -1;
	} else {
		mult = 1;
	}
	needed_count_diff = abs(delta_angle * ROBOT_WIDTH / SPEED_CONSTANT)*1.5;
	fprintf(stderr, "*** DELTA ANGLE: %f count diff: %i\n", delta_angle * 180/M_PI, needed_count_diff);

	int finished(robot_status * rs, navdata * nv, int * m0_speed, int * m1_speed) {

		/*
		float diff;
		
		diff = pi_reduce(direction - nv->nv_pheta);
		fprintf(stderr, "Dir: %f. Cur: %f Delta: %f\n", (double)direction, (double)nv->nv_pheta, (double) diff);
		if (fabs(diff) < 0.3) {
			fprintf(stderr, "LOOK! IM DONE\n");
			return 0;
		} else {
			return 1;
		}
		*/
		int diffticks;

		diffticks = nv->nv_left_speed.ve_count + nv->nv_right_speed.ve_count - (m0_count + m1_count);

		if (diffticks >= needed_count_diff) {
			fprintf(stderr, "STOP STOP STOP STOP STOP got %i ticks\n", diffticks);
			return 0;
		} else {
			return 1;
		}
	}

	drive(rs, nv, robotwin, speed * mult, -speed*mult, finished);


}


int main(int argc, char ** argv) {

	int what;
	char zero = 0;
	int speed = 150;
	int rs232_fd;
	int last_state = -1;
	int timeouttime = 0;
	robot_status rbstatus;
//	struct timeval base;
	navdata nav_info;
	input_buffer input;


	input.ib_position = 0;
	input.ib_state = 0;

	int now, next;
	int direction = 1;
	Window robotwin;
	InitX();
	robotwin = CreateWindow(512, 512, "robotmatic!");
	ShowWindow(robotwin);

	prepare_input();

	rs232_fd = prepare_rs232(argv[1]);
	if (argv[2]) {
		bt_connect(rs232_fd, argv[2]);
	}

	send_command2(rs232_fd, C_MOTORS, '0', 0, 0);

	rbstatus.rs_rs232_fd = rs232_fd;
	gettimeofday(&(rbstatus.rs_basetime), NULL);
	rbstatus.rs_input = &input;
	navdata_reset(&nav_info, 0);
	next = 100;
	now = 0;

	fprintf(stderr, "FILE IS %i\n", rs232_fd);
	while (1) {
	//	check_for_data(rs232_fd);
		what = event_loop(rs232_fd, &nav_info, &input, now, next-now);

		now = get_rel_time(&(rbstatus.rs_basetime));
		if (now >= next) {
			do_gfx_nav_update(&nav_info, now, robotwin, direction, direction);
			   
			next = now + 50;
			/*
			update_position(&nav_info, now);
			display_data(&nav_info, now);
			next = now + 50;
			FillCircle(robotwin, nav_info.nv_x + 256, nav_info.nv_y + 256, 6, 3);
			XSync(mydisplay, True);
			*/
		}

		if (what == AUTOPILOT) {
	//		drive(&rbstatus, &nav_info, robotwin, 0, 0, 0, 0);
			autoturn(&rbstatus, &nav_info, robotwin, 0, speed);
		}
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
		if (what == TIMEOUT) {
			timeouttime++;
			if (timeouttime == 10) {
				dbg("***TIMEOUT");
				update_speed_stop(&nav_info, now);
				stop(rs232_fd);
				read_adc(rs232_fd);
			}
		} else {
			timeouttime = 0;
		}


		if (last_state != what) {
			last_state = what;

			if (what == FORWARD) {
			//	if (reallyforward) {
		//			update_speed_stop(&nav_info, now);
					read_adc(rs232_fd);
					go_forward(rs232_fd, speed);
					direction = 1;
			//	} else {
			//		last_state = -1;
			//	}
				reallyforward++;
			} else {
				reallyforward = 0;
			}

			if (what == LEFT) {
		//		update_speed_stop(&nav_info, now);
				read_adc(rs232_fd);
				turn(rs232_fd, speed, GP_M0F);
				direction = 1;
			}
			if (what == RIGHT) {
		//		update_speed_stop(&nav_info, now);
				read_adc(rs232_fd);
				turn(rs232_fd, speed, GP_M1F);
				direction = 1;
			}
			if (what == STOP || what == TIMEOUT) {
		//		stop(rs232_fd);
			}
			if (what == REVERSE) {
		//		update_speed_stop(&nav_info, now);
				read_adc(rs232_fd);
				go_backward(rs232_fd, speed);
				direction = -1;
			}


		}


	}


}


