// evan kahn
// e155, final project

#include "easyPIO.h"
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <signal.h>
#include <math.h>

#define PI 3.14159265

#define LEDOUT 1
#define DEBUG 0
#define RPM 500

unsigned char pixels, revs, total_frames, speed;

struct pixel {
	unsigned char r, g, b;
};

//help with pixel_data structure from eric mueller; adapted from following code:
//   	https://gist.github.com/hmc-cs-emueller/c4ebbe9f3b4064fed16a

struct pixel *getPixel(struct pixel *frames, unsigned i, unsigned j, unsigned k){
	unsigned addr = i + j * pixels + k * pixels * revs;
	return frames + addr;
}

unsigned char gamma_correct(unsigned char color){
	int corrected = pow(color,3)/(255*255);
	return (unsigned char) corrected;
}


void setLED (struct pixel *p){
	if (LEDOUT){
		spiSendReceive(0xff);
		spiSendReceive(gamma_correct(p->b));
		spiSendReceive(gamma_correct(p->g));
		spiSendReceive(gamma_correct(p->r));
	}
}

void initFrame (){
	int i;
	if (LEDOUT) for (i=0;i<4;i++) spiSendReceive(0x00);
}

void endFrame (){
	if (LEDOUT){
		spiSendReceive(0xff);
		spiSendReceive(0xff);
		spiSendReceive(0xff);
		spiSendReceive(0xff);
	}
}


void lightUp(struct pixel *frames, int revnum, int framenum){
	int i;
	initFrame();
	struct pixel *p;
	if (DEBUG == 1) printf("\nbegin horiz %d\n", revnum);
	for (i=0;i<pixels;i++){	
		p = getPixel(frames, i, revnum, framenum);
		if (DEBUG){
			printf("%02d 0x%02x%02x%02x ",i,p->b,p->g,p->r);
		}
		setLED(p);
		if (DEBUG && ((i+1) % 8 == 0)) printf("\n");
	}
	endFrame();

}

void sighandler (int sig){
		if (LEDOUT){
			digitalWrite(22, 0);
			delayMicros(100000);
			int i;
			struct pixel p;
			p.r = 0x00;
			p.g = 0x00;
			p.b = 0x00;
			initFrame();
			for (i = 0; i < pixels; ++i){
				setLED(&p);
			}
			endFrame();
		}
		exit(0);
}

int main() {
	signal(SIGINT, sighandler);
	unsigned int time_1;
	if (LEDOUT){
		pioInit();
		spiInit(10000000);
		pinMode(22, OUTPUT);
		pinMode(27, OUTPUT);
		digitalWrite(27, 1);
		delayMicros(10000);
		digitalWrite(27, 0);
	}


	//	frames per revolution: revs
	// 	revolutions per second: revs/60
	// 	seconds per revolution: 60/RPM
	// 	seconds per frame: 60/revs/RPM
	// 	microseconds per frame: 1000000 * 60 / (revs * RPM)

	unsigned revolutions = 0;
	struct pixel *frames;

	char fname[64];
	int done = 0;
	FILE *fp;
	do{
		printf("Enter filename: ");
		scanf("%s",fname);
		strcat(fname, ".led");

	if( access( fname, F_OK ) != -1 ) {
		fp = fopen(fname, "r");
		fscanf(fp, "%c%c%c%c", &pixels, &revs, &speed, &total_frames);
		char junk;
		fscanf(fp, "%c%c%c%c", &junk, &junk, &junk, &junk);
		unsigned long size = pixels * revs * total_frames * sizeof(struct pixel);
		frames = malloc(size);
		if (!frames){
			printf("malloc failed");
			exit(1);
		}
		memset(frames, 0x00, size);
		fread(frames, sizeof(char), size, fp);
		done = 1;

	}else{
		printf("not found.\n");
	}
	}while (!done);

	digitalWrite(22,1);

	int revnum = revs - 1;
	int delay = 1000000 * 60 / (revs * RPM);
	int framenum = 0;
	while(1){
		time_1 = getTime();
		lightUp(frames,revnum, framenum);

		if (revnum == 0) revnum = revs - 1;
		else revnum = revnum - 1;

		if (revnum == 0){
			++revolutions;
		}
		if (revolutions == speed){
			revolutions = 0;
			framenum = ((framenum + 1)%total_frames);
		}
		while (getTime() < time_1 + delay){};
	}

	printf("\n");
	return(0);
}
