/*
 *      Author: raszit
 */


#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <time.h>
#include <signal.h>
#include <getopt.h>
#include <ncurses.h>
#include "ws28xx-esp8266.h"
#include <math.h>
#include <alsa/asoundlib.h>
#include <fftw3.h>

snd_pcm_t *pcm_handle;

#define AUDIO_DEVICE	"pulse"
#define SAMPLE_RATE		44100

char audio_source[32];
unsigned int rate = SAMPLE_RATE;

#define BUF_BYTES		2048

/***** zmienna leds musi być zdefiniowana, korzysta z niej biblioteka *****/
int leds = LEDS;

volatile uint32_t millis = 0;

typedef struct {
	int16_t left;
	int16_t right;
}audio_channels;

fftw_complex *l_in, *r_in;

fftw_complex *l_out;
fftw_complex *r_out;
fftw_plan plan_l, plan_r;

double absdoouble(double d){
	if (d >= 0){
		return d;
	}
	return -d;
}


const char *ip_addr;
uint16_t port = 8000;


ws28xx_leds *ws28xx_l1_to_send;

ws28xx_leds *ws28xx_layer1;


#define WS28xx_SIDE_SIZE		64

#define WS28xx_FRONT_BACK_SIZE	32

ws28xx_leds **ws28xx_left_side, **ws28xx_right_side;
ws28xx_leds **ws28xx_front_side, **ws28xx_back_side;


void alarm_callback(int sig) {
	millis++;
}

u_argb black = { .red = 0, .green = 0, .blue = 0 };
u_argb blue = { .red = 0, .green = 0, .blue = 255 };
u_argb green = { .red = 0, .green = 255, .blue = 0 };
u_argb red = { .red = 255, .green = 0, .blue = 0 };

u_argb yellow = { .red = 255, .green = 255, .blue = 0 };
u_argb cyan = { .red = 0, .green = 255, .blue = 255 };
u_argb magenta = { .red = 255, .green = 0, .blue = 255 };
u_argb white = { .red = 255, .green = 255, .blue = 255 };

int main(int argc, char **argv) {

	int next_option;
	const struct option long_options[] = {
		{ "target_ip",			1,	NULL,	'i' },//adres ip urządzenia
		{ "target_port",		1,	NULL,	'p' },//port urządzenia
		{ "leds",				1,	NULL,	'l' },//długość paska led
		{ "audio_source",		1,	NULL,	'A' },//źródło dźwięku
		{ "sample_rate",		1,	NULL,	'S' },//częstotliwość próbkowania
		{ "help",				0,	NULL,	'h' },//pomoc
		{ NULL, 		0,	NULL,	0	}
	};
	strcpy(audio_source, AUDIO_DEVICE);

	do {
		next_option = getopt_long(argc, argv, "i:p:l:hA:S:", long_options, NULL);
		switch(next_option){
			case 'A':
				memset(audio_source, 0, sizeof(char));
				strcpy(audio_source, optarg);
				break;
			case 'S':
				rate = atoi(optarg);
				printf("sample_rate: %d\n\r", atoi(optarg));
				break;
			case 'i':
				ip_addr = optarg;
				printf("adres ip:%s\n\r", optarg);
				break;
			case 'p':
				port = atoi(optarg);
				printf("port: %d\n\r", atoi(optarg));
				break;
			case 'l':
				leds = atoi(optarg);
				printf("długość paska: %d\n\r", atoi(optarg));
				break;
			case 'h':
				printf("Help:\n"
						"--target_ip		adres ip urządzenia\n"
						"--target_port		port urządzenia\n"
						"--leds				długość paska led\n"
						"--audio_source		źródło dźwięku\n"
						"--help				pomoc\n");
				break;
			case '?':
				abort();
				break;
			case -1:
				break;
			default:
				abort();
		}
	}while(next_option != -1);

	if (ip_addr == 0 || strlen(ip_addr) < 5){
		printf("Podaj adres ip urządzenia\n\r");
		return EXIT_FAILURE;
	}
	initscr();
	keypad(stdscr, TRUE);
	noecho();
	halfdelay(1);

	unsigned char pcm_buf[BUF_BYTES];
	audio_channels *channels = (audio_channels*) pcm_buf;
	unsigned int nchannels = 2;
	snd_pcm_sframes_t buf_frames = BUF_BYTES / nchannels / 2;

	int sockfd;
	int len;
	struct sockaddr_in address;
	int result;
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	address.sin_family = AF_INET;
	address.sin_port = htons(port);
	address.sin_addr.s_addr = inet_addr(ip_addr);
	len = sizeof(address);
	result = connect(sockfd, (struct sockaddr *) &address, len);
	if (result == -1) {
		perror("oops: client");
		exit(EXIT_FAILURE);
	}

	write(sockfd, "fx 128", 6);

/******* inicjalizacja timera ********/
	struct itimerval timer1;
	timer1.it_value.tv_sec = 0;
	timer1.it_value.tv_usec = 1000;
	timer1.it_interval.tv_sec = 0;
	timer1.it_interval.tv_usec = 1000;
	signal(SIGALRM, alarm_callback);

	setitimer(ITIMER_REAL, &timer1, NULL);
/*************************************/

	uint8_t *ws28xx;
/******** alokacja pamięci dla całej ramki do wysłania ********/
	ws28xx_l1_to_send = malloc((sizeof(ws28xx_leds) * leds) + 8);

/******** rzutowanie do uint8_t* ********/
	ws28xx = (uint8_t*) ws28xx_l1_to_send;

/******** wskaźnik na dziewiąty element (tutaj będą dane rgb) ********/
	ws28xx_layer1 = (ws28xx_leds*) &ws28xx[8];

/******** to musi być na początku ramki, aby moduł esp8266 odebrał ramkę ********/
	sprintf((char*) ws28xx_l1_to_send, "RAWDATA:");

	int16_t ws28xx_lenght = (leds * sizeof(ws28xx_leds)) + 8;

	write(sockfd, ws28xx_l1_to_send, ws28xx_lenght);

	uint8_t init = 0;

	uint32_t delay1 = 5;
	uint32_t delay2 = 45;
	uint32_t delay3 = 10;
	uint32_t delay4 = 8;

	/*********INICJALIZACJA PASKA LED W POKOJU*****************/

	ws28xx_left_side = malloc(WS28xx_SIDE_SIZE * sizeof(ws28xx_leds*));
	ws28xx_right_side = malloc(WS28xx_SIDE_SIZE * sizeof(ws28xx_leds*));
	ws28xx_front_side = malloc(WS28xx_FRONT_BACK_SIZE * sizeof(ws28xx_leds*));
	ws28xx_back_side = malloc(WS28xx_FRONT_BACK_SIZE * sizeof(ws28xx_leds*));

	for (int i = 0; i < 16; i++){
		ws28xx_left_side[i] = &ws28xx_layer1[i];
	}
	for (int i = 0; i < 48; i++){
		ws28xx_left_side[16 + i] = &ws28xx_layer1[191 - i];
	}
	for (int i = 0; i < 48; i++){
		ws28xx_right_side[i] = &ws28xx_layer1[i + 48];
	}
	for (int i = 0; i < 16; i++){
		ws28xx_right_side[48 + i] = &ws28xx_layer1[111 - i];
	}
	for (int i = 0; i < 32; i++){
		ws28xx_front_side[i] = &ws28xx_layer1[16 + i];
		ws28xx_back_side[i] = &ws28xx_layer1[112 + i];
	}

	uint8_t temp = 0;
	int16_t brightness = 50;

	int ch;

	uint32_t ms_timer1 = 0;
	uint32_t ms_timer2 = 0;
	uint32_t ms_timer3 = 0;
	uint32_t ms_timer4 = 0;

	l_in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * buf_frames * 2);
	fftw_complex *l_IN = &l_in[buf_frames];
	l_out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * buf_frames * 2);
	r_in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * buf_frames * 2);
	fftw_complex *r_IN = &r_in[buf_frames];
	r_out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * buf_frames * 2);
	plan_l = fftw_plan_dft_1d(buf_frames * 2, l_in, l_out, FFTW_FORWARD, FFTW_ESTIMATE);
	plan_r = fftw_plan_dft_1d(buf_frames * 2, r_in, r_out, FFTW_FORWARD, FFTW_ESTIMATE);

	int band = 1;

	while (1) {
		ch = getch();
		switch(ch){
			case ',':
				delay1--;
				break;
			case '.':
				delay1++;
				break;
			case '[':
				delay2--;
				break;
			case ']':
				delay2++;
				break;
			case '{':
				brightness--;
				break;
			case '}':
				brightness++;
				break;
			case '-':
				delay3--;
				break;
			case '+':
				delay3++;
				break;
			case '<':
				delay4--;
				break;
			case '>':
				delay4++;
				break;
			case 'B':
				if (band < buf_frames){
					band++;
				}
				break;
			case 'b':
				if (band > 1){
					band--;
				}
				break;
			case 'q':
				ws28xx_fill_color(ws28xx_layer1, 0, leds, black.color32, 100);
				write(sockfd, ws28xx_l1_to_send, ws28xx_lenght);
				if (pcm_handle){
					snd_pcm_close(pcm_handle);
				}
				refresh();
				getch();
				endwin();
				exit(EXIT_SUCCESS);
			default:
				{}
		}

		if (init == 0 || ch != ERR) {
			if (delay1 < 1){
				delay1 = 1;
			}
			if (delay2 < 1){
				delay2 = 1;
			}
			if (delay3 < 1){
				delay3 = 1;
			}
			if (delay4 < 1){
				delay4 = 1;
			}
			if (brightness < 0){
				brightness = 0;
			}
			if (brightness > 100){
				brightness = 100;
			}
			move(0, 0);
			printw(",/.                delay1 = %2d\n\r", delay1);
			printw("[/]                delay2 = %2d\n\r", delay2);
			printw("-/+                delay3 = %2d\n\r", delay3);
			printw("</>                delay4 = %2d\n\r", delay4);
			printw("{/}                brightness = %3d\n\r", brightness);
			printw("b/B                band = %3d\n\r", band);
		}

		if (init == 0) {
			ws28xx_fill_color(ws28xx_layer1, 0, leds, black.color32, 100);
			init = 1;
			band = 1;

/***************otwórz strumień PCM**********/
			int err = snd_pcm_open(&pcm_handle, audio_source, SND_PCM_STREAM_CAPTURE, 0);
			if (err != 0){
				refresh();
				getch();
				endwin();
				printf("pcm open error\n\r");
				exit(EXIT_SUCCESS);
			}
			
			err = snd_pcm_set_params(pcm_handle, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED, nchannels, rate, 0, 30000);
			if (err != 0){
				snd_pcm_close(pcm_handle);
				refresh();
				getch();
				endwin();
				printf("pcm set params error\n\r");
				exit(EXIT_SUCCESS);
			}
			snd_pcm_start(pcm_handle);

			snd_pcm_hw_params_t *params;
			snd_pcm_hw_params_malloc(&params);
			snd_pcm_hw_params_current(pcm_handle, params);
			
			unsigned int chan;
			snd_pcm_hw_params_get_channels(params, &chan);
			move(12, 10);
			printw("channels:%d\n\r", chan);
			
			unsigned int _rate;
			snd_pcm_hw_params_get_rate(params, &_rate, NULL);
			move(13, 10);
			printw("rate:%d\n\r", _rate);
			
			snd_pcm_format_t format;
			snd_pcm_hw_params_get_format(params, &format);
			if (format == SND_PCM_FORMAT_S16_LE){
				move(14, 10);
				printw("format:%d\n\r", format);
			}
			
			snd_pcm_uframes_t _frames;
			snd_pcm_hw_params_get_buffer_size(params, &_frames);
			move(15, 10);
			printw("frames:%d\n\r", _frames);

		}else{

/***************pobieraj próbki ze strumienia********/
			snd_pcm_sframes_t frames = snd_pcm_readi(pcm_handle, pcm_buf, buf_frames);
			move(16, 10);
			printw("buf_frames = %d, frames = %d", buf_frames, frames);

			if (frames < 0){
				move(17, 10);
				printw("snd pcm readi error");
			}

			static int p_left = 0;
			static int p_right = 0;
			static int p_b1 = 0;
			static int p_b2 = 0;
			static int p_b3 = 0;
			static int p_b4 = 0;
			static int p_b5 = 0;
			static int p_b6 = 0;
			double b = 0;

/**********załaduj próbki do fft*****************/
			for (int i = 0; i < buf_frames; i++){
				//l_in[i][0] = cos(4 * 2 * M_PI * i / buf_frames);
				l_IN[i][0] = channels[i].left / (32768.0);
				l_IN[i][1] = 0;
				r_IN[i][0] = channels[i].right / (32768.0);
				r_IN[i][1] = 0;
			}

			fftw_execute(plan_l);
			fftw_execute(plan_r);
			memcpy(l_in, l_IN, sizeof(fftw_complex) * buf_frames);
			memcpy(r_in, r_IN, sizeof(fftw_complex) * buf_frames);

			ws28xx_fill_color(ws28xx_layer1, 0, leds, black.color32, 100);
			//uint16_t pos_left = (peak_left / 32768.0) * WS28xx_SIDE_SIZE;
			//uint16_t pos_right = (peak_right / 32768.0) * WS28xx_SIDE_SIZE;
			int index1 = band;
			int index2 = (buf_frames * 2) - index1;
//					uint16_t l = (uint16_t) (l_out[band][0] * l_out[band][0] + l_out[band][1] * l_out[band][1]);
//					uint16_t r = (uint16_t) (r_out[band][0] * r_out[band][0] + r_out[band][1] * r_out[band][1]);
			double l = absdoouble(l_out[index1][0] + l_out[index1][1]) + absdoouble(l_out[index2][0] + l_out[index2][1]);
			double r = absdoouble(r_out[index1][0] + r_out[index1][1]) + absdoouble(r_out[index2][0] + r_out[index2][1]);
			
			index1 = band + 3;
			index2 = (buf_frames * 2) - index1;
			b = absdoouble(l_out[index1][0] + l_out[index1][1]) + absdoouble(l_out[index2][0] + l_out[index2][1]);
			b += absdoouble(r_out[index1][0] + r_out[index1][1]) + absdoouble(r_out[index2][0] + r_out[index2][1]);
			uint16_t pos_b1 = (b / 512.0) * WS28xx_FRONT_BACK_SIZE / 2;
			
			index1 = band + 4;
			index2 = (buf_frames * 2) - index1;
			b = absdoouble(l_out[index1][0] + l_out[index1][1]) + absdoouble(l_out[index2][0] + l_out[index2][1]);
			b += absdoouble(r_out[index1][0] + r_out[index1][1]) + absdoouble(r_out[index2][0] + r_out[index2][1]);
			uint16_t pos_b2 = (b / 512.0) * WS28xx_FRONT_BACK_SIZE / 2;

			index1 = band + 5;
			index2 = (buf_frames * 2) - index1;
			b = absdoouble(l_out[index1][0] + l_out[index1][1]) + absdoouble(l_out[index2][0] + l_out[index2][1]);
			b += absdoouble(r_out[index1][0] + r_out[index1][1]) + absdoouble(r_out[index2][0] + r_out[index2][1]);
			uint16_t pos_b3 = (b / 512.0) * WS28xx_FRONT_BACK_SIZE / 2;
			
			index1 = band + 6;
			index2 = (buf_frames * 2) - index1;
			b = absdoouble(l_out[index1][0] + l_out[index1][1]) + absdoouble(l_out[index2][0] + l_out[index2][1]);
			b += absdoouble(r_out[index1][0] + r_out[index1][1]) + absdoouble(r_out[index2][0] + r_out[index2][1]);
			uint16_t pos_b4 = (b / 512.0) * WS28xx_FRONT_BACK_SIZE / 2;
			
			index1 = band + 7;
			index2 = (buf_frames * 2) - index1;
			b = absdoouble(l_out[index1][0] + l_out[index1][1]) + absdoouble(l_out[index2][0] + l_out[index2][1]);
			b += absdoouble(r_out[index1][0] + r_out[index1][1]) + absdoouble(r_out[index2][0] + r_out[index2][1]);
			uint16_t pos_b5 = (b / 512.0) * WS28xx_FRONT_BACK_SIZE / 2;

			index1 = band + 8;
			index2 = (buf_frames * 2) - index1;
			b = absdoouble(l_out[index1][0] + l_out[index1][1]) + absdoouble(l_out[index2][0] + l_out[index2][1]);
			b += absdoouble(r_out[index1][0] + r_out[index1][1]) + absdoouble(r_out[index2][0] + r_out[index2][1]);
			uint16_t pos_b6 = (b / 512.0) * WS28xx_FRONT_BACK_SIZE / 2;

			uint16_t pos_left = (l / 512.0) * WS28xx_SIDE_SIZE;
			uint16_t pos_right = (r / 512.0) * WS28xx_SIDE_SIZE;

			static uint16_t left = 0, right = 0;

			if (pos_left > left){
				left = pos_left;
				temp = 1;
			}
			if (pos_right > right){
				right = pos_right;
				temp = 1;
			}
			if (p_left < pos_left){
				p_left = pos_left;
				temp = 1;
			}
			if (p_right < pos_right){
				p_right = pos_right;
				temp = 1;
			}
			if (pos_b1 > p_b1){
				p_b1 = pos_b1;
				temp = 1;
			}
			if (pos_b2 > p_b2){
				p_b2 = pos_b2;
				temp = 1;
			}
			if (pos_b3 > p_b3){
				p_b3 = pos_b3;
				temp = 1;
			}
			if (pos_b4 > p_b4){
				p_b4 = pos_b4;
				temp = 1;
			}
			if (pos_b5 > p_b5){
				p_b5 = pos_b5;
				temp = 1;
			}
			if (pos_b6 > p_b6){
				p_b6 = pos_b6;
				temp = 1;
			}

			if (millis >= ms_timer1 + delay1 || temp == 1){
				ms_timer1 = millis;
				temp = 0;
				for (int i = 0; i < WS28xx_SIDE_SIZE; i++){
					if (i <= left){
						ws28xx_set_pixel(ws28xx_left_side[i], 0, red.color32, COLOR_RED_MASK | COLOR_MIX_OVER, brightness);
					}else{
						ws28xx_set_pixel(ws28xx_left_side[i], 0, black.color32, COLOR_RED_MASK | COLOR_MIX_OVER, 0);
					}
					if (i <= right){
						ws28xx_set_pixel(ws28xx_right_side[i], 0, red.color32, COLOR_RED_MASK | COLOR_MIX_OVER, brightness);
					}else{
						ws28xx_set_pixel(ws28xx_right_side[i], 0, black.color32, COLOR_RED_MASK | COLOR_MIX_OVER, 0);
					}
					if (i == p_left){
						ws28xx_set_pixel(ws28xx_left_side[i], 0, blue.color32, COLOR_BLUE_MASK | COLOR_MIX_OVER, brightness);
					}else{
						ws28xx_set_pixel(ws28xx_left_side[i], 0, blue.color32, COLOR_BLUE_MASK | COLOR_MIX_OVER, 0);
					}
					if (i == p_right){
						ws28xx_set_pixel(ws28xx_right_side[i], 0, blue.color32, COLOR_BLUE_MASK | COLOR_MIX_OVER, brightness);
					}else{
						ws28xx_set_pixel(ws28xx_right_side[i], 0, blue.color32, COLOR_BLUE_MASK | COLOR_MIX_OVER, 0);
					}
					if (i == left){
						ws28xx_set_pixel(ws28xx_left_side[i], 0, green.color32, COLOR_GREEN_MASK | COLOR_MIX_OVER, brightness);
					}else{
						ws28xx_set_pixel(ws28xx_left_side[i], 0, green.color32, COLOR_GREEN_MASK | COLOR_MIX_OVER, 0);
					}
					if (i == right){
						ws28xx_set_pixel(ws28xx_right_side[i], 0, green.color32, COLOR_GREEN_MASK | COLOR_MIX_OVER, brightness);
					}else{
						ws28xx_set_pixel(ws28xx_right_side[i], 0, green.color32, COLOR_GREEN_MASK | COLOR_MIX_OVER, 0);
					}
				}
				for (int i = 0; i < WS28xx_FRONT_BACK_SIZE / 2; i++){
					int p = (WS28xx_FRONT_BACK_SIZE / 2) + i;
					int n = (WS28xx_FRONT_BACK_SIZE / 2) - i - 1;
					if (i == p_b1){
						ws28xx_set_pixel(ws28xx_front_side[p], 0, white.color32, COLOR_RED_MASK | COLOR_MIX_OVER, brightness);
						ws28xx_set_pixel(ws28xx_front_side[n], 0, white.color32, COLOR_RED_MASK | COLOR_MIX_OVER, brightness);
					}else{
						ws28xx_set_pixel(ws28xx_front_side[p], 0, white.color32, COLOR_RED_MASK | COLOR_MIX_OVER, 0);
						ws28xx_set_pixel(ws28xx_front_side[n], 0, white.color32, COLOR_RED_MASK | COLOR_MIX_OVER, 0);
					}
					if (i == p_b2){
						ws28xx_set_pixel(ws28xx_front_side[p], 0, white.color32, COLOR_GREEN_MASK | COLOR_MIX_OVER, brightness);
						ws28xx_set_pixel(ws28xx_front_side[n], 0, white.color32, COLOR_GREEN_MASK | COLOR_MIX_OVER, brightness);
					}else{
						ws28xx_set_pixel(ws28xx_front_side[p], 0, white.color32, COLOR_GREEN_MASK | COLOR_MIX_OVER, 0);
						ws28xx_set_pixel(ws28xx_front_side[n], 0, white.color32, COLOR_GREEN_MASK | COLOR_MIX_OVER, 0);
					}
					if (i == p_b3){
						ws28xx_set_pixel(ws28xx_front_side[p], 0, white.color32, COLOR_BLUE_MASK | COLOR_MIX_OVER, brightness);
						ws28xx_set_pixel(ws28xx_front_side[n], 0, white.color32, COLOR_BLUE_MASK | COLOR_MIX_OVER, brightness);
					}else{
						ws28xx_set_pixel(ws28xx_front_side[p], 0, white.color32, COLOR_BLUE_MASK | COLOR_MIX_OVER, 0);
						ws28xx_set_pixel(ws28xx_front_side[n], 0, white.color32, COLOR_BLUE_MASK | COLOR_MIX_OVER, 0);
					}
				}
				for (int i = 0; i < WS28xx_FRONT_BACK_SIZE / 2; i++){
					int p = (WS28xx_FRONT_BACK_SIZE / 2) + i;
					int n = (WS28xx_FRONT_BACK_SIZE / 2) - i - 1;
					if (i == p_b4){
						ws28xx_set_pixel(ws28xx_back_side[p], 0, white.color32, COLOR_RED_MASK | COLOR_MIX_OVER, brightness);
						ws28xx_set_pixel(ws28xx_back_side[n], 0, white.color32, COLOR_RED_MASK | COLOR_MIX_OVER, brightness);
					}else{
						ws28xx_set_pixel(ws28xx_back_side[p], 0, white.color32, COLOR_RED_MASK | COLOR_MIX_OVER, 0);
						ws28xx_set_pixel(ws28xx_back_side[n], 0, white.color32, COLOR_RED_MASK | COLOR_MIX_OVER, 0);
					}
					if (i == p_b5){
						ws28xx_set_pixel(ws28xx_back_side[p], 0, white.color32, COLOR_GREEN_MASK | COLOR_MIX_OVER, brightness);
						ws28xx_set_pixel(ws28xx_back_side[n], 0, white.color32, COLOR_GREEN_MASK | COLOR_MIX_OVER, brightness);
					}else{
						ws28xx_set_pixel(ws28xx_back_side[p], 0, white.color32, COLOR_GREEN_MASK | COLOR_MIX_OVER, 0);
						ws28xx_set_pixel(ws28xx_back_side[n], 0, white.color32, COLOR_GREEN_MASK | COLOR_MIX_OVER, 0);
					}
					if (i == p_b6){
						ws28xx_set_pixel(ws28xx_back_side[p], 0, white.color32, COLOR_BLUE_MASK | COLOR_MIX_OVER, brightness);
						ws28xx_set_pixel(ws28xx_back_side[n], 0, white.color32, COLOR_BLUE_MASK | COLOR_MIX_OVER, brightness);
					}else{
						ws28xx_set_pixel(ws28xx_back_side[p], 0, red.color32, COLOR_BLUE_MASK | COLOR_MIX_OVER, 0);
						ws28xx_set_pixel(ws28xx_back_side[n], 0, red.color32, COLOR_BLUE_MASK | COLOR_MIX_OVER, 0);
					}
				}

				write(sockfd, ws28xx_l1_to_send, ws28xx_lenght);
			}
			if (millis >= ms_timer2 + delay2){//opadanie peak'ów
				ms_timer2 = millis;
				if (p_left > 0){
					p_left--;
					temp = 1;
				}
				if (p_right > 0){
					p_right--;
					temp = 1;
				}
			}
			if (millis >= ms_timer3 + delay3){
				ms_timer3 = millis;
				if (p_b1 > 0){
					p_b1--;
					temp = 1;
				}
				if (p_b2 > 0){
					p_b2--;
					temp = 1;
				}
				if (p_b3 > 0){
					p_b3--;
					temp = 1;
				}
				if (p_b4 > 0){
					p_b4--;
					temp = 1;
				}
				if (p_b5 > 0){
					p_b5--;
					temp = 1;
				}
				if (p_b6 > 0){
					p_b6--;
					temp = 1;
				}
			}
			if (millis >= ms_timer4 + delay4){
				ms_timer4 = millis;
				if (left > 0){
					left--;
					temp = 1;
				}
				if (right > 0){
					right--;
					temp = 1;
				}
			}

		}
		usleep(20);
	}
	return 0;
}
