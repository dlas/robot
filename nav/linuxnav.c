

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/poll.h>

#include "nav.c"
#include "Xgraphics.c"

int get_rel_time(struct timeval * base) {
	struct timeval now, diff;

	gettimeofday(&now, NULL);
	timersub(&now, base, &diff);
	return(diff.tv_sec * 1000 + diff.tv_usec / 1000);

}



int timeout_read(int fd, char * buffer, int len, int timeout) {

	int err;
	struct pollfd pf[1];

	pf[0].fd = fd;
	pf[0].events = POLLIN;
	pf[0].revents = 0;

	err = poll(pf, 1, timeout);

	if (err == 0) {
		return 0;
	}
	else if (err == 1) {
		ssize_t rlen;
		rlen = read(fd, buffer, len);

		if (rlen <1) {
			fprintf(stderr, 
			    "huh? no data to read after poll: %i %s",
			    (int)rlen, strerror(errno));
			return 0;
		}

		return (rlen);
	}
	else {
		fprintf(stderr, "poll on fd %i failed. sorry: %s",
		    fd, strerror(errno));
		return 0;
	}
	return 0;
}


void display_data(navdata * d, int now) {
	printf("AT: %i. X: %3g Y:%3g pheta: %3g\n",
	    now,
	    d->nv_x,
	    d->nv_y,
	    d->nv_pheta * 180.0/3.1415);

	printf("LEFT: %i %i. RIGHT: %i %i\n\n",
	    d->nv_left_speed.ve_pointer, d->nv_left_speed.ve_average,
	    d->nv_right_speed.ve_pointer, d->nv_right_speed.ve_average);
}

void process_data(navdata * n, char * buffer, int now) {


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
	case ('\n'):
		break;
	default:
		fprintf(stderr, "Wierd data: %c", *buffer);
	}
}


int main(int argc, char ** argv) {

	struct timeval base;
	int fd;
	int err;
	char buffer[2];
	int now;
	int next;
	Window robotwin;

	InitX();
	robotwin = CreateWindow(512, 512, "robotomatic!");
	ShowWindow(robotwin);
	

	navdata nav_info;
	fd = open(argv[1], O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "Cannot open %s: %s",
		    argv[1],
		    strerror(errno));
		return 1;
	}

	gettimeofday(&base, NULL);
	navdata_reset(&nav_info, 0);
	get_rel_time(&base);
	next = 100;
	now = 0;

	while (1) {
		err = timeout_read(fd, buffer, 1, next - now);
		now = get_rel_time(&base);
		if (err > 0) {
			process_data(&nav_info, buffer, now);
		}
		if (now >= next) {
			update_position(&nav_info, now);
			display_data(&nav_info, now);
		     	next = now + 100;

			FillCircle(robotwin, nav_info.nv_x + 256, nav_info.nv_y + 256, 6, 3);
			XSync(mydisplay, True);


		}

	}

}

