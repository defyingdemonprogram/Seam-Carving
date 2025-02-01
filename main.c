#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <float.h>
#include <math.h>

#include "nob.h"
#include "stb_image.h"
#include "stb_image_write.h"

typedef struct {
    uint32_t *pixels;
    int width, height, stride;
} Img;

#define IMG_AT(img, row, col) (img).pixels[(row)*(img).stride + (col)]

typedef struct {
    float *items;
    int width, height, stride;
} Mat;

#define MAT_AT(mat, row, col) (mat).items[(row)*(mat).stride + (col)]

Mat mat_alloc(int width, int height)
{
    Mat mat = {0};
    mat.items = malloc(sizeof(float) * width * height);
    assert(mat.items != NULL && "Buy More RAM lo!!!");
    mat.width = width;
    mat.height = height;
    mat.stride = width;
    return mat;
}

float rgb_to_lum(uint32_t rgb)
{
    float r = ((rgb >> (8 * 0)) & 0xFF) / 255.0;
    float g = ((rgb >> (8 * 1)) & 0xFF) / 255.0;
    float b = ((rgb >> (8 * 2)) & 0xFF) / 255.0;

    return 0.2126 * r + 0.7152 * g + 0.0722 * b;
}
#if 0
void min_and_max(Mat mat, float *mn, float *mx)
{
    *mn = FLT_MAX;
    *mx = FLT_MIN;
    for (int y = 0; y < mat.height; ++y)
    {
        for (int x = 0; x < mat.width; ++x)
        {
            float value = MAT_AT(mat, y, x);
            if (value < *mn) *mn = value;
            if (value > *mx) *mx = value;
        }
    }
}

void analyze_min_and_max(const char *prompt, Mat mat)
{
    float mn, mx;
    min_and_max(mat, &mn, &mx);
    printf("%s: min = %f, max = %f\n", prompt, mn, mx);
}

bool dump_mat(const char *file_path, Mat mat)
{
    uint32_t *pixels = NULL;
    bool result = true;

    float mn, mx;
    min_and_max(mat, &mn, &mx);

    pixels = malloc(sizeof(*pixels) * mat.width * mat.height);
    assert(pixels != NULL && "Buy More RAM lol!!!");

    for (int y = 0; y < mat.height; ++y)
    {
        for (int x = 0; x < mat.width; ++x)
        {
            int i = y * mat.width + x;
            float t = (MAT_AT(mat, y, x) - mn) / (mx - mn);
            uint32_t value = 255 * t;
            pixels[i] = 0xFF000000 | (value << (8 * 2)) | (value << (8 * 1)) | (value << (8 * 0));
        }
    }

    if (!stbi_write_png(file_path, mat.width, mat.height, 4, pixels, mat.width * sizeof(*pixels)))
    {
        fprintf(stderr, "ERROR: could not save file %s", file_path);
        result = false;
        goto defer;
    }

    printf("[INFO] Image saved at file: %s\n", file_path);

defer:
    free(pixels);
    return result;
}
#else
#define analyze_min_and_max(...)
#define dump_mat(...)
#endif // 0

void luminance(Img img, Mat lum)
{
    assert(img.width == lum.width);
    assert(img.height == lum.height);

    for (int y = 0; y < lum.height; ++y)
    {
        for (int x = 0; x < lum.width; ++x)
        {
            MAT_AT(lum, y, x) = rgb_to_lum(IMG_AT(img, y, x));
        }
    }
}

void sobel_filter(Mat mat, Mat grad)
{
    assert(mat.width == grad.width);
    assert(mat.height == grad.height);

    static float gx[3][3] = {
        {1.0, 0.0, -1.0},
        {2.0, 0.0, -2.0},
        {1.0, 0.0, -1.0},
    };

    static float gy[3][3] = {
        {1.0, 2.0, 1.0},
        {0.0, 0.0, 0.0},
        {-1.0, -2.0, -1.0},
    };

    for (int cy = 0; cy < mat.height; ++cy)
    {
        for (int cx = 0; cx < mat.width; ++cx)
        {
            float sx = 0.0;
            float sy = 0.0;
            for (int dy = -1; dy <= 1; ++dy)
            {
                for (int dx = -1; dx <= 1; ++dx)
                {
                    int x = cx + dx;
                    int y = cy + dy;
                    float c = 0 <= x && x < mat.width && 0 <= y && y < mat.height ? MAT_AT(mat, y, x) : 0.0;
                    sx += c * gx[dy + 1][dx + 1];
                    sy += c * gy[dy + 1][dx + 1];
                }
            }
            MAT_AT(grad, cy, cx) = sqrtf(sx * sx + sy * sy);
        }
    }
}

void grad_to_dp(Mat grad, Mat dp)
{
    assert(grad.width == dp.width);
    assert(grad.height == dp.height);

    for (int x = 0; x < grad.width; ++x)
    {
        MAT_AT(dp, 0, x) = MAT_AT(grad, 0, x);
    }

    for (int y = 1; y < grad.height; ++y)
    {
        for (int cx = 0; cx < grad.width; ++cx)
        {
            float m = FLT_MAX;
            for (int dx = -1; dx <= 1; ++dx)
            {
                int x = cx + dx;
                float value = 0 <= x && x < grad.width ? MAT_AT(dp, y - 1, x) : FLT_MAX;
                if (value < m) m = value;
            }
            MAT_AT(dp, y, cx) = MAT_AT(grad, y, cx) + m;
        }
    }
}

int main()
{
    const char *file_path = "data/people.jpg";

    int width, height;
    uint32_t *pixels = (uint32_t *)stbi_load(file_path, &width, &height, NULL, 4);
    if (pixels == NULL)
    {
        fprintf(stderr, "ERROR: could not read %s\n", file_path);
        return 1;
    }

    Img img = {
        .pixels = pixels,
        .width = width,
        .height = height,
        .stride = width,
    };

    Mat lum = mat_alloc(width, height);
    Mat grad = mat_alloc(width, height);
    Mat dp = mat_alloc(width, height);

    int seams_to_remove = img.width*2/3;
    for (int i = 0; i < seams_to_remove; ++i)
    {
        printf("Removing seam %d\n", i);
        luminance(img, lum);
        analyze_min_and_max("lum", lum);
        dump_mat("lum.png", lum);

        sobel_filter(lum, grad);
        analyze_min_and_max("grad", grad);

        dump_mat("grad.png", grad);
        grad_to_dp(grad, dp);
        analyze_min_and_max("dp", dp);
        dump_mat("dp.png", dp);

        int y = img.height - 1;
        int seam = 0;
        for (int x = 1; x < img.width; ++x)
        {
            if (MAT_AT(dp, y, x) < MAT_AT(dp, y, seam))
            {
                seam = x;
            }
        }

        uint32_t *pixel_row = &IMG_AT(img, y, 0);
        memmove(pixel_row + seam, pixel_row + seam + 1, (img.width - seam - 1) * sizeof(uint32_t));

        for (int y = img.height - 2; y >= 0; --y)
        {
            for (int dx = -1; dx <= 1; ++dx)
            {
                int x = seam + dx;
                if (0 <= x && x < img.width && MAT_AT(dp, y, x) < MAT_AT(dp, y, seam))
                {
                    seam = x;
                }
            }
            pixel_row = &IMG_AT(img, y, 0);
            memmove(pixel_row + seam, pixel_row + seam + 1, (img.width - seam - 1) * sizeof(uint32_t));
        }

        img.width -= 1;
        lum.width -= 1;
        grad.width -= 1;
        dp.width -= 1;
    }

    const char *out_file_path = "seam.png";

    if (!stbi_write_png(out_file_path, img.width, img.height, 4, img.pixels, img.stride * sizeof(uint32_t)))
    {
        fprintf(stderr, "ERROR: could not save file %s", out_file_path);
        return 1;
    }

    free(pixels);
    return 0;
}
