#include <stdio.h>
#include <string.h>
#include <math.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

static int mini(int a, int b) { return a < b ? a : b; }

static void unpremultiplyAlpha(unsigned char *image, int w, int h, int stride)
{
	int x, y;

	// Unpremultiply
	for (y = 0; y < h; y++)
	{
		unsigned char *row = &image[y * stride];
		for (x = 0; x < w; x++)
		{
			int r = row[0], g = row[1], b = row[2], a = row[3];
			if (a != 0)
			{
				row[0] = (int)mini(r * 255 / a, 255);
				row[1] = (int)mini(g * 255 / a, 255);
				row[2] = (int)mini(b * 255 / a, 255);
			}
			row += 4;
		}
	}

	// Defringe
	for (y = 0; y < h; y++)
	{
		unsigned char *row = &image[y * stride];
		for (x = 0; x < w; x++)
		{
			int r = 0, g = 0, b = 0, a = row[3], n = 0;
			if (a == 0)
			{
				if (x - 1 > 0 && row[-1] != 0)
				{
					r += row[-4];
					g += row[-3];
					b += row[-2];
					n++;
				}
				if (x + 1 < w && row[7] != 0)
				{
					r += row[4];
					g += row[5];
					b += row[6];
					n++;
				}
				if (y - 1 > 0 && row[-stride + 3] != 0)
				{
					r += row[-stride];
					g += row[-stride + 1];
					b += row[-stride + 2];
					n++;
				}
				if (y + 1 < h && row[stride + 3] != 0)
				{
					r += row[stride];
					g += row[stride + 1];
					b += row[stride + 2];
					n++;
				}
				if (n > 0)
				{
					row[0] = r / n;
					row[1] = g / n;
					row[2] = b / n;
				}
			}
			row += 4;
		}
	}
}

static void setAlpha(unsigned char *image, int w, int h, int stride, unsigned char a)
{
	int x, y;
	for (y = 0; y < h; y++)
	{
		unsigned char *row = &image[y * stride];
		for (x = 0; x < w; x++)
			row[x * 4 + 3] = a;
	}
}

static void flipHorizontal(unsigned char *image, int w, int h, int stride)
{
	int i = 0, j = h - 1, k;
	while (i < j)
	{
		unsigned char *ri = &image[i * stride];
		unsigned char *rj = &image[j * stride];
		for (k = 0; k < w * 4; k++)
		{
			unsigned char t = ri[k];
			ri[k] = rj[k];
			rj[k] = t;
		}
		i++;
		j--;
	}
}

void saveScreenShot(int w, int h, int premult, const char *name)
{
	unsigned char *image = (unsigned char *)malloc(w * h * 4);
	if (image == NULL)
		return;
	glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, image);
	if (premult)
		unpremultiplyAlpha(image, w, h, w * 4);
	else
		setAlpha(image, w, h, w * 4, 255);
	flipHorizontal(image, w, h, w * 4);
	stbi_write_png(name, w, h, 4, image, w * 4);
	free(image);
}
