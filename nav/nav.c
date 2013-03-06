
#include <math.h>
#include <string.h>

#define TIMESTAMP_MULTIPLIER 10.0
#define ROBOT_WIDTH (10.35/2)
#define HISTORY_LEN 3 
#define SPEED_CONSTANT (WHEEL_CIRCUMFERENCE  / TICKS_PER_WHEEL)
#define WHEEL_CIRCUMFERENCE 6.384
#define TICKS_PER_WHEEL 8.0
#define UPDATES_PER_SECOND 10.0



typedef struct vestimator{
	int ve_prevtimes[HISTORY_LEN];
	int ve_pointer;
	int ve_average;
	int ve_lasttime;
	int ve_count;
} vestimator;

typedef struct navdata {
        float nv_x;
        float nv_y;
        float nv_pheta;

        int nv_last_update;

        vestimator nv_left_speed;
        vestimator nv_right_speed;
} navdata;


void update_speed_tick(vestimator * v, int level, int now) {
	v->ve_count++;
	v->ve_average -=v->ve_prevtimes[v->ve_pointer];
    	v->ve_average += now - v->ve_lasttime;
	v->ve_prevtimes[v->ve_pointer] = now - v->ve_lasttime;
	v->ve_lasttime = now;
	
	v->ve_pointer++;
	if (v->ve_pointer >= HISTORY_LEN) {
		v->ve_pointer = 0;
	}



}
float get_vel(vestimator * v, int now) {
	float speed;
	int use;

	if ((now - v->ve_lasttime) > (v->ve_average / HISTORY_LEN)) {
	//	use = 2 * (now - v->ve_lasttime);
		use = 0;
	} else {
		use = v->ve_average;
	}
	
	if (use == 0) {
		return 0;
	}

	speed = SPEED_CONSTANT / (float)use;
	return speed;


}



void vestimator_reset(vestimator * v, int now) {
	memset(v, 0, sizeof(*v));

	v->ve_lasttime = now;
}

void update_speed_stop(navdata * nv, int now) {
	vestimator_reset(&(nv->nv_left_speed), now);
	vestimator_reset(&(nv->nv_right_speed), now);
}


void navdata_reset(navdata * d, int now) {
	d->nv_x = d->nv_y = d->nv_pheta = 0;
	d->nv_last_update = now;
	vestimator_reset(&(d->nv_left_speed), now);
	vestimator_reset(&(d->nv_right_speed), now);
}


void set_direction(navdata * data, float pheta) {
	data->nv_pheta = pheta;
}

int signof(int x) {
	if (x<0) {
		return -1;
	}
	if (x>0) {
		return 1;
	}
	return 0;
}




void update_position(navdata * data, int now, int r_speed, int l_speed) {
        float left_speed;
        float right_speed;
        float speed;
        float delta_t;
        float phi;

        left_speed = get_vel(&(data->nv_left_speed), now) * signof(l_speed);
        right_speed = get_vel(&(data->nv_right_speed), now) * signof(r_speed);

        delta_t = (now - data->nv_last_update) * TIMESTAMP_MULTIPLIER;

        phi = (left_speed - right_speed) / (2*ROBOT_WIDTH);

        speed = (left_speed + right_speed) / 2;

        data->nv_x += speed * delta_t * cos(data->nv_pheta - delta_t * phi/2);
        data->nv_y += speed * delta_t * sin(data->nv_pheta - delta_t * phi/2);

        data->nv_pheta -= phi * delta_t;

        data->nv_last_update = now;
}


