#pragma once
#include "stb_image.h"

static bool imageCompare(const char *path1, const char *path2, int pixel_threshold)
{
    bool result = true;

    int w1, h1, n1;
    unsigned char *data1 = stbi_load(path1, &w1, &h1, &n1, 0);

    int w2, h2, n2;
    unsigned char *data2 = stbi_load(path2, &w2, &h2, &n2, 0);

    if (w1 != w2 || h1 != h2 || n1 != n2)
    {
        result = false;
        goto end;
    }

    int y = 0;
    for (; y < h1; ++y)
    {
        int x = 0;
        for (; x < w1; ++x)
        {
            int diff = 0;
            int c = 0;
            for (; c < n1; ++c)
            {
                int pos = (x + y * w1) * n1 + c;
                const unsigned char *c1 = data1 + pos;
                const unsigned char *c2 = data2 + pos;
                diff += (int)(*c1) - (*c2);
            }
            if (diff > pixel_threshold)
            {
                result = false;
                goto end;
            }
        }
    }
end:
    stbi_image_free(data1);
    stbi_image_free(data2);
    return result;
}
