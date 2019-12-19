/*
 * Copyright (c) 2015, SICS Swedish ICT
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

#include "tools-utils.h"

#include <stdio.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>


/*---------------------------------------------------------------------------*/
#define BAUDRATE B115200
#define BAUDRATE_S "115200"

speed_t b_rate = BAUDRATE;
/*---------------------------------------------------------------------------*/
#ifdef linux
#define MODEMDEVICE (char *)"/dev/ttyUSB0"
#else
#define MODEMDEVICE "/dev/com1"
#endif /* linux */
/*---------------------------------------------------------------------------*/
#define SLIP_END      0300
#define SLIP_ESC      0333
#define SLIP_ESC_END  0334
#define SLIP_ESC_ESC  0335

#define CSNA_INIT     0x01

#define BUFSIZE         40
#define HCOLS           20
#define ICOLS           18

#define MODE_START_DATE  0
#define MODE_DATE        1
#define MODE_START_TEXT  2
#define MODE_TEXT        3
#define MODE_INT         4
#define MODE_HEX         5
#define MODE_SLIP_AUTO   6
#define MODE_SLIP        7
#define MODE_SLIP_HIDE   8
/*---------------------------------------------------------------------------*/
#ifndef O_SYNC
#define O_SYNC 0
#endif

#define OPEN_FLAGS (O_RDWR | O_NOCTTY | O_NDELAY | O_SYNC)
static unsigned char rxbuf[2048];
/*---------------------------------------------------------------------------*/
	static void
intHandler(int sig)
{
	exit(0);
}
/*---------------------------------------------------------------------------*/

fd_set mask, smask;
int fd;

unsigned char serialbuf[BUFSIZE];
int nfound;






speed_t
select_baudrate(int baudrate) {
  switch(baudrate) {
#ifdef B50
  case 50:
    return B50;
#endif
#ifdef B75
  case 75:
    return B75;
#endif
#ifdef B110
  case 110:
    return B110;
#endif
#ifdef B134
  case 134:
    return B134;
#endif
#ifdef B150
  case 150:
    return B150;
#endif
#ifdef B200
  case 200:
    return B200;
#endif
#ifdef B300
  case 300:
    return B300;
#endif
#ifdef B600
  case 600:
    return B600;
#endif
#ifdef B1200
  case 1200:
    return B1200;
#endif
#ifdef B1800
  case 1800:
    return B1800;
#endif
#ifdef B2400
  case 2400:
    return B2400;
#endif
#ifdef B4800
  case 4800:
    return B4800;
#endif
#ifdef B9600
  case 9600:
    return B9600;
#endif
#ifdef B19200
  case 19200:
    return B19200;
#endif
#ifdef B38400
  case 38400:
    return B38400;
#endif
#ifdef B57600
  case 57600:
    return B57600;
#endif
#ifdef B115200
  case 115200:
    return B115200;
#endif
#ifdef B230400
  case 230400:
    return B230400;
#endif
#ifdef B460800
  case 460800:
    return B460800;
#endif
#ifdef B500000
  case 500000:
    return B500000;
#endif
#ifdef B576000
  case 576000:
    return B576000;
#endif
#ifdef B921600
  case 921600:
    return B921600;
#endif
#ifdef B1000000
  case 1000000:
    return B1000000;
#endif
#ifdef B1152000
  case 1152000:
    return B1152000;
#endif
#ifdef B1500000
  case 1500000:
    return B1500000;
#endif
#ifdef B2000000
  case 2000000:
    return B2000000;
#endif
#ifdef B2500000
  case 2500000:
    return B2500000;
#endif
#ifdef B3000000
  case 3000000:
    return B3000000;
#endif
#ifdef B3500000
  case 3500000:
    return B3500000;
#endif
#ifdef B4000000
  case 4000000:
    return B4000000;
#endif
  default:
    return 0;
  }
}


bool InitSerialPort(){
	signal(SIGINT, intHandler);

	struct termios options;
	//fd_set mask, smask;
	//int fd;
	int baudrate = BUNKNOWN;
	char *device = MODEMDEVICE;
	char *timeformat = NULL;
	//unsigned char buf[BUFSIZE];
	char outbuf[HCOLS];
	unsigned char mode = MODE_START_TEXT;
	int flags = 0;
	unsigned char lastc = '\0';

	int index = 1;

	if(baudrate != BUNKNOWN) {
		b_rate = select_baudrate(baudrate);
		if(b_rate == 0) {
			fprintf(stderr, "unknown baudrate %d\n", baudrate);
			return false;//exit(-1);
		}
	}

	fprintf(stderr, "connecting to %s", device);

	fd = open(device, OPEN_FLAGS);

	if(fd < 0) {
		fprintf(stderr, "\n");
		perror("open");
		//exit(-1);
		return false;
	}
	fprintf(stderr, " [OK]\n");

	if(fcntl(fd, F_SETFL, 0) < 0) {
		perror("could not set fcntl");
		return false;//exit(-1);
	}

	if(tcgetattr(fd, &options) < 0) {
		perror("could not get options");
		return false;//exit(-1);
	}

	cfsetispeed(&options, b_rate);
	cfsetospeed(&options, b_rate);

	/* Enable the receiver and set local mode */
	options.c_cflag |= (CLOCAL | CREAD);
	/* Mask the character size bits and turn off (odd) parity */
	options.c_cflag &= ~(CSIZE | PARENB | PARODD);
	/* Select 8 data bits */
	options.c_cflag |= CS8;

	/* Raw input */
	options.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP
			| INLCR | IGNCR | ICRNL | IXON);
	options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	/* Raw output */
	options.c_oflag &= ~OPOST;

	if(tcsetattr(fd, TCSANOW, &options) < 0) {
		perror("could not set options");
		exit(-1);
	}

	FD_ZERO(&mask);
	FD_SET(fd, &mask);
	FD_SET(fileno(stdin), &mask);

	index = 0;
	return true;
}

int SerialCommand(char * command, char * reply){
	if(InitSerialPort()){
		//smask = mask;

		if(strlen(command) > 0) {

			int i;
			char c;
			memset(serialbuf,0,sizeof(BUFSIZE));
			strcpy((char*)serialbuf,(char*)(command));
			/*    fprintf(stderr, "SEND %d bytes\n", n);*/
			/* write slowly */
			for(i = 0; i < strlen(command); i++) {
				c=serialbuf[i];
				if(write(fd, &c, 1) <= 0) {
					perror("write");
					//exit(1);
				} else {
					fflush(NULL);
					usleep(6000);
				}
			}          
			// for of line
			c='\n';
			if(write(fd, &c, 1) <= 0) {
				perror("write");
				//exit(1);
			} else {
				fflush(NULL);
				usleep(6000);
			}
		}

		memset(serialbuf,0,sizeof(BUFSIZE));
		if(1/*FD_ISSET(fd, &smask)*/) {
			int i, n = read(fd, serialbuf, sizeof(serialbuf));
			if(n < 0) {
				perror("could not read");
				//exit(-1);
			}
			if(n == 0) {
				errno = EBADF;
				perror("serial device disconnected");
				//exit(-1);
			}

			strncpy(reply,(char *)serialbuf,n);
			/*printf("output: %s\n",serialbuf);
			messagePublish("/iot/sensor/reply",(char*)serialbuf);*/
		}
		close(fd);
	}
}