/*
 *      Author: raszit
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "ws28xx-esp8266.h"

#include "fixed-float.h"

extern int leds;


uint32_t interpolate2colors(uint32_t colorA, uint32_t colorB, int pos, int len) {
	u_argb A, B, C;
	A.color32 = colorA;
	B.color32 = colorB;

	C.red = (uint8_t) (((int) (B.red - A.red) * pos) / len) + (int) A.red;
	C.green = (uint8_t) (((int) (B.green - A.green) * pos) / len)
			+ (int) A.green;
	C.blue = (uint8_t) (((int) (B.blue - A.blue) * pos) / len) + (int) A.blue;

	return C.color32;
}

void mixcolor(ws28xx_leds *_leds, int16_t pos, int16_t lenght, uint32_t color1,
		uint32_t color2) {
	u_argb c, temp;
	int16_t p = 0;
	for (p = pos; p < (lenght + pos); p++) {
		if (p < 0 || p >= leds)
			continue;
		c.color32 = interpolate2colors(color1, color2, p - pos, lenght);
		temp.red = gamma_correction(c.red, 255,
		GAMMA_R);
		temp.green = gamma_correction(c.green, 255,
		GAMMA_G);
		temp.blue = gamma_correction(c.blue, 255,
		GAMMA_B);
		ws28xx_set_pixel(_leds, p, temp.color32, 0, 100);
	}
}

//
void ws28xx_set_pixel(ws28xx_leds *p, uint16_t pos, uint32_t color,
		uint32_t color_mask, int brightness) {
	if (p == 0) {
		return;
	}
	if (pos >= leds) {
		return;
	}
	if (color_mask == 0) {
		color_mask = COLOR_ALL_MASK | COLOR_MIX_OVER;
	}
	int temp;
//	struct rgb_color *c = (struct rgb_color*) color;
	u_argb c;
	c.color32 = color;
	if (color_mask & COLOR_MIX_OR) {
		if (color_mask & COLOR_RED_MASK) {
			temp = (int) (c.red * brightness);
			p[pos].r |= (uint8_t) (temp / 100);
		}
		if (color_mask & COLOR_GREEN_MASK) {
			temp = (int) (c.green * brightness);
			p[pos].g |= (uint8_t) (temp / 100);
		}
		if (color_mask & COLOR_BLUE_MASK) {
			temp = (int) (c.blue * brightness);
			p[pos].b |= (uint8_t) (temp / 100);
		}
		return;
	}
	if (color_mask & COLOR_MIX_XOR) {
		if (color_mask & COLOR_RED_MASK) {
			temp = (int) (c.red * brightness);
			p[pos].r ^= (uint8_t) (temp / 100);
		}
		if (color_mask & COLOR_GREEN_MASK) {
			temp = (int) (c.green * brightness);
			p[pos].g ^= (uint8_t) (temp / 100);
		}
		if (color_mask & COLOR_BLUE_MASK) {
			temp = (int) (c.blue * brightness);
			p[pos].b ^= (uint8_t) (temp / 100);
		}
		return;
	}
	if (color_mask & COLOR_MIX_OVER) {
		if (color_mask & COLOR_RED_MASK) {
			temp = (int) (c.red * brightness);
			p[pos].r = (uint8_t) (temp / 100);
		}
		if (color_mask & COLOR_GREEN_MASK) {
			temp = (int) (c.green * brightness);
			p[pos].g = (uint8_t) (temp / 100);
		}
		if (color_mask & COLOR_BLUE_MASK) {
			temp = (int) (c.blue * brightness);
			p[pos].b = (uint8_t) (temp / 100);
		}
		return;
	}
}

//ta funkcja chyba nie działa prawidłowo
uint32_t ws28xx_get_pixel_color(ws28xx_leds *p, int16_t pos) {
	u_argb color;
	color.red = p[pos].r;
	color.green = p[pos].g;
	color.blue = p[pos].b;
	color.aplha = 0;
	return color.color32;
}

void ws28xx_fill_color(ws28xx_leds * pasek, int16_t start_pos, int16_t leds,
		uint32_t color, int brightness) {
	int16_t x;
	int16_t end_pos = start_pos + leds;
	for (x = start_pos; x < end_pos; x++) {
		if (x < 0)
			continue;
		ws28xx_set_pixel(pasek, x, color, COLOR_ALL_MASK | COLOR_MIX_OVER,
				brightness);
	}
}


void shift_buf_fwd(ws28xx_leds* buf, int lenght) {
	ws28xx_leds temp;
	int x;
	temp.r = buf[lenght - 1].r;
	temp.g = buf[lenght - 1].g;
	temp.b = buf[lenght - 1].b;
//	temp = buf[lenght-1];
	for (x = (lenght - 1); x > 0; x--) {
		buf[x].r = buf[x - 1].r;
		buf[x].g = buf[x - 1].g;
		buf[x].b = buf[x - 1].b;
	}
	buf[0].r = temp.r;
	buf[0].g = temp.g;
	buf[0].b = temp.b;
//	buf[0] = temp;
}

void shift_buf_bwd(ws28xx_leds* buf, int lenght) {
	ws28xx_leds temp;
	int x;
	temp.r = buf[0].r;
	temp.g = buf[0].g;
	temp.b = buf[0].b;
	for (x = 0; x < (lenght - 1); x++) {
		buf[x].r = buf[x + 1].r;
		buf[x].g = buf[x + 1].g;
		buf[x].b = buf[x + 1].b;
	}
	buf[lenght - 1].r = temp.r;
	buf[lenght - 1].g = temp.g;
	buf[lenght - 1].b = temp.b;
}

u_argb RGB_compute(int xrgb, int yrgb, int zrgb, int max_x, int max_y) {
	int x1, x2, x3, x4, x5;
	x1 = max_x / 6;
	x2 = x1 * 2;
	x3 = x1 * 3;
	x4 = x1 * 4;
	x5 = x1 * 5;
//	float y_temp = (float) ((float) yrgb / (float) max_y * 1.0);
//	yrgb = (int) (y_temp * 255);

	int y_tmp = yrgb * 1000 / max_y;
	yrgb = (y_tmp * 255) / 1000;

	u_argb c;
	if (xrgb >= 0 && xrgb < x1) {
		c.red = yrgb;
//		c.green = (uint8_t) (((xrgb / (x1 - 1.0))) * yrgb * (1.0 - zrgb)
//				+ (zrgb * yrgb));
		c.green = (uint8_t) (((xrgb * 1000) / (x1 - 1) * yrgb
				* ((1000 - zrgb) / 1000)) / 1000);
		c.green += ((zrgb * yrgb) / 1000);
		c.blue = (uint8_t) (zrgb * yrgb) / 1000;
	}
	if (xrgb >= x1 && xrgb < x2) {
//		c.red = (uint8_t) (yrgb - ((((xrgb - x1) / (x1 - 1.0)) * yrgb) * (1.0 - zrgb)));
		c.red = (uint8_t) (yrgb
				- (((((xrgb - x1) * 1000) / (x1 - 1)) * yrgb)
						* ((1000 - zrgb) / 1000)) / 1000);
		c.green = yrgb;
		c.blue = (uint8_t) (zrgb * yrgb) / 1000;
	}
	if (xrgb >= x2 && xrgb < x3) {
		c.red = (uint8_t) (zrgb * yrgb) / 1000;
		c.green = yrgb;
//		c.blue = (uint8_t) (((xrgb - x2) / (x1 - 1.0)) * yrgb * (1.0 - zrgb) + (zrgb * yrgb));
		c.blue = (uint8_t) (((((xrgb - x2) * 1000) / (x1 - 1)) * yrgb
				* ((1000 - zrgb) / 1000)) / 1000);
		c.blue += (uint8_t) (zrgb * yrgb) / 1000;
	}
	if (xrgb >= x3 && xrgb < x4) {
		c.red = (uint8_t) (zrgb * yrgb) / 1000;
//		c.green = (uint8_t) (yrgb
//				- ((((xrgb - x3) / (x1 - 1.0)) * yrgb) * (1.0 - zrgb)));
		c.green = (uint8_t) (yrgb
				- (((((xrgb - x3) * 1000) / (x1 - 1)) * yrgb)
						* ((1000 - zrgb) / 1000)) / 1000);
		c.blue = yrgb;
	}
	if (xrgb >= x4 && xrgb < x5) {
//		c.red = (uint8_t) (((xrgb - x4) / (x1 - 1.0)) * yrgb * (1.0 - zrgb)
//				+ (zrgb * yrgb));
		c.red = (uint8_t) (((((xrgb - x4) * 1000) / (x1 - 1)) * yrgb
				* ((1000 - zrgb) / 1000)) / 1000);
		c.red += (uint8_t) (zrgb * yrgb) / 1000;
		c.green = (uint8_t) (zrgb * yrgb) / 1000;
		c.blue = yrgb;
	}
	if (xrgb >= x5 && xrgb <= max_x) {
		c.red = yrgb;
		c.green = (uint8_t) (zrgb * yrgb) / 1000;
//		c.blue = (uint8_t) (yrgb
//				- ((((xrgb - x5) / (x1 * 1.0)) * yrgb) * (1.0 - zrgb)));
		c.blue = (uint8_t) (yrgb
				- (((((xrgb - x5) * 1000) / (x1 - 1)) * yrgb)
						* ((1000 - zrgb) / 1000)) / 1000);

	}
	return c;
}

static inline double fabs(double x) {
	return (x < 0 ? -x : x);
}

static inline double pow_(double x, double y) {
	int q = 14;
	int64_t fixed_x = TOFIX(x, q);
	int64_t fixed_y = TOFIX(y, q);
	int64_t fixed_z;
	int one = TOFIX(1, q);

	double z;
	int64_t p = one;
	int i;

	if (y < 0) {
		z = fabs(y);
		fixed_z = fabs(fixed_y);
	} else {
		z = y;
		fixed_z = fixed_y;
	}

	for (i = 0; i < z; ++i) {
		int64_t tmp = p;
		p = FMUL(tmp, fixed_x, q);
	}

	if (fixed_y < 0) {
		int64_t tmp = FDIV(one, p, q);
		return TOFLT(tmp, q);
	} else {
		return TOFLT(p, q);
	}
	/*
	 //y<0 ? z=-y : z=y ;
	 if (y < 0)
	 z = fabs(y);
	 else
	 z = y;
	 for (i = 0; i < z; ++i) {
	 p *= x;
	 }
	 if (y < 0)
	 return 1 / p;
	 else
	 return p;*/
}

//nie pamiętam jak te funkcje działają
static inline void fade_out_pixel(ws28xx_leds *buf, int pos,
		uint32_t color_mask, uint8_t b) {
	if (pos >= leds) {
		return;
	}
	u_argb c = { .red = buf[pos].r, .green = buf[pos].g, .blue = buf[pos].b };
	ws28xx_set_pixel(buf, pos, c.color32, color_mask, b);
}

void fade_out_(ws28xx_leds *buf, uint16_t lenght, uint32_t color_mask,
		uint8_t b) {
	for (uint16_t l = 0; l < lenght; l++) {
		fade_out_pixel(buf, l, color_mask, b);
	}
}

uint8_t gamma_correction(double value, double max, double gamma) {
	return max * pow_((double) (value / max), 1.0 / gamma);
}

