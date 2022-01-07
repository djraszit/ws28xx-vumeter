/*
 *      Author: raszit
 */

#ifndef USER_WS28XX_ESP8266_H_
#define USER_WS28XX_ESP8266_H_

#define LEDS	150

#define GAMMA_R	0.8
#define GAMMA_G	0.6
#define GAMMA_B 0.5

#define COLOR_RED_MASK		0x01
#define COLOR_GREEN_MASK	0x02
#define COLOR_BLUE_MASK		0x04
#define COLOR_ALL_MASK		0x07
#define COLOR_MIX_OR		0x08
#define COLOR_MIX_XOR		0x10
#define COLOR_MIX_OVER		0x20


typedef struct  {
	uint8_t b;
	uint8_t r;
	uint8_t g;
}ws28xx_leds;

typedef struct  {
	uint8_t blue;
	uint8_t green;
	uint8_t red;
	uint8_t aplha;
}s_argb_color;

typedef union  {
	uint32_t color32;
	s_argb_color color;
	struct {
		uint8_t blue;
		uint8_t green;
		uint8_t red;
		uint8_t aplha;
	};
}u_argb;

void ws28xx_set_pixel(ws28xx_leds *p, uint16_t pos, uint32_t color, uint32_t color_mask,
		int brightness);
uint32_t ws28xx_get_pixel_color(ws28xx_leds *p, int16_t pos);
void ws28xx_fill_color(ws28xx_leds * pasek, int16_t start_pos, int16_t leds, uint32_t color, int brightness);
uint32_t interpolate2colors(uint32_t colorA, uint32_t colorB, int pos, int len);
void mixcolor(ws28xx_leds *leds, int16_t pos, int16_t lenght, uint32_t color1, uint32_t color2);
void shift_buf_fwd(ws28xx_leds* buf, int lenght);
void shift_buf_bwd(ws28xx_leds* buf, int lenght);
//double pow_(double x, double y);
u_argb RGB_compute(int xrgb, int yrgb, int zrgb, int max_x,
		int max_y);
void ws28xx_send_buf(void *leds, int size);
void ws28xx_asm_send_buf(void *leds, int size);
void fade_out(ws28xx_leds *buf, uint16_t lenght, uint8_t how_much);
//void fade_out_pixel(ws28xx_leds *buf, int pos, uint32_t color_mask, uint8_t b);
void fade_out_(ws28xx_leds *buf, uint16_t lenght, uint32_t color_mask, uint8_t b);
uint8_t gamma_correction(double value, double max, double gamma);

#endif /* USER_WS28XX_ESP8266_H_ */
