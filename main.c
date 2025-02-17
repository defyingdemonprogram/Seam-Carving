#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <float.h>
#include <math.h>

#include "stb_image.h"
#include "stb_image_write.h"

#define NOB_IMPLEMENTATION
#include "nob.h"

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
#define MAT_WITHIN(mat, row, col) \
    (0 <= (col) && (col) < (mat).width && 0 <= (row) && (row) < (mat).height)

static Mat mat_alloc(int width, int height)
{
    Mat mat = {0};
    mat.items = malloc(sizeof(float) * width * height);
    assert(mat.items != NULL && "Buy More RAM lo!!!");
    mat.width = width;
    mat.height = height;
    mat.stride = width;
    return mat;
}

static float rgb_to_lum(uint32_t rgb)
{
    float r = ((rgb >> (8 * 0)) & 0xFF) / 255.0;
    float g = ((rgb >> (8 * 1)) & 0xFF) / 255.0;
    float b = ((rgb >> (8 * 2)) & 0xFF) / 255.0;

    return 0.2126 * r + 0.7152 * g + 0.0722 * b;
}
#if 0
static void min_and_max(Mat mat, float *mn, float *mx)
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

static void analyze_min_and_max(const char *prompt, Mat mat)
{
    float mn, mx;
    min_and_max(mat, &mn, &mx);
    printf("%s: min = %f, max = %f\n", prompt, mn, mx);
}

static bool dump_mat(const char *file_path, Mat mat)
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

static void luminance(Img img, Mat lum)
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

static float sobel_filter_at(Mat mat, int cx, int cy)
{
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

    float sx = 0.0;
    float sy = 0.0;
    for (int dy = -1; dy <= 1; ++dy)
    {
        for (int dx = -1; dx <= 1; ++dx)
        {
            int x = cx + dx;
            int y = cy + dy;
            float c = MAT_WITHIN(mat, y, x) ? MAT_AT(mat, y, x) : 0.0;
            sx += c * gx[dy + 1][dx + 1];
            sy += c * gy[dy + 1][dx + 1];
        }
    }
    return sqrtf(sx*sx + sy*sy);
}

static void sobel_filter(Mat mat, Mat grad)
{
    assert(mat.width == grad.width);
    assert(mat.height == grad.height);

    for (int cy = 0; cy < mat.height; ++cy)
    {
        for (int cx = 0; cx < mat.width; ++cx)
        {
            MAT_AT(grad, cy, cx) = sobel_filter_at(mat, cx, cy);
        }
    }
}

static void grad_to_dp(Mat grad, Mat dp)
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

static void usage(const char *program)
{
    fprintf(stderr, "Usage: %s <input> <output>\n", program);
}

static void img_remove_column_at_row(Img img, int row, int column)
{
    uint32_t *pixel_row = &IMG_AT(img, row, 0);
    memmove(pixel_row + column, pixel_row + column + 1, (img.width - column - 1) * sizeof(uint32_t));
}

static void mat_remove_column_at_row(Mat mat, int row, int column)
{
    float *pixel_row = &MAT_AT(mat, row, 0);
    memmove(pixel_row + column, pixel_row + column + 1, (mat.width - column - 1) * sizeof(float));
}


static void compute_seam(Mat dp, int *seam)
{
    int y = dp.height - 1;
    seam[y] = 0;
    for (int x = 1; x < dp.width; ++x)
    {
        if (MAT_AT(dp, y, x) < MAT_AT(dp, y, seam[y]))
        {
            seam[y] = x;
        }
    }

    for (int y = dp.height - 2; y >= 0; --y)
    {
        seam[y] = seam[y+1];
        for (int dx = -1; dx <= 1; ++dx)
        {
            int x = seam[y+1] + dx;
            if (0 <= x && x < dp.width && MAT_AT(dp, y, x) < MAT_AT(dp, y, seam[y]))
            {
                seam[y] = x;
            }
        }
    }
}

static void markout_sobel_pathces(Mat grad, int *seam)
{
    for (int cy = 0; cy < grad.height; ++cy)
    {
        int cx = seam[cy];
        for (int dy = -1; dy <= 1; ++dy)
        {
            for (int dx = -1; dx <= 1; ++dx)
            {
                int x = cx + dx;
                int y = cy + dy;
                if (MAT_WITHIN(grad, y, x) && y < grad.height)
                {
                    *(uint32_t*)&MAT_AT(grad, y, x) = 0xFFFFFFFF;
                }
            }
        }
    }
}

int main(int argc, char **argv)
{
    const char *program = nob_shift_args(&argc, &argv);

    if (argc <= 0)
    {
        usage(program);
        fprintf(stderr, "ERROR: no input file is provided\n");
        return 1;
    }
    const char *file_path = nob_shift_args(&argc, &argv);
    
    if (argc <= 0)
    {
        usage(program);
        fprintf(stderr, "ERROR: no output file is provided\n");
        return 1;
    }
    const char *out_file_path = nob_shift_args(&argc, &argv);

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
    int *seam = malloc(sizeof(*seam)*height);

    int seams_to_remove = img.width*2/3;

    luminance(img, lum);
    analyze_min_and_max("lum", lum);
    dump_mat("lum.png", lum);

    sobel_filter(lum, grad);
    analyze_min_and_max("grad", grad);
    dump_mat("grad.png", grad);

    
    for (int i = 0; i < seams_to_remove; ++i)
    {
        printf("Removing seam %d\n", i);
        grad_to_dp(grad, dp);
        analyze_min_and_max("dp", dp);
        dump_mat("dp.png", dp);

        compute_seam(dp, seam);
        markout_sobel_pathces(grad, seam);

        for (int cy = 0; cy < img.height; ++cy)
        {
            int cx = seam[cy];
            img_remove_column_at_row(img, cy, cx);
            mat_remove_column_at_row(lum, cy, cx);
            mat_remove_column_at_row(grad, cy, cx);
        }

        img.width -= 1;
        lum.width -= 1;
        grad.width -= 1;
        dp.width -= 1;

        for (int cy = 0; cy < grad.height; ++cy)
        {
            for (int cx = seam[cy]; cx < grad.width && *(uint32_t*)&MAT_AT(grad, cy, cx) == 0xFFFFFFFF; ++cx)
            {
                MAT_AT(grad, cy, cx) = sobel_filter_at(lum, cx, cy);
            }
            for (int cx = seam[cy] - 1; cx >= 0 && *(uint32_t*)&MAT_AT(grad, cy, cx) == 0xFFFFFFFF; --cx)
            {
                MAT_AT(grad, cy, cx) = sobel_filter_at(lum, cx, cy);
            }
        }
    }

    if (!stbi_write_png(out_file_path, img.width, img.height, 4, img.pixels, img.stride * sizeof(uint32_t)))
    {
        fprintf(stderr, "ERROR: could not save file %s\n", out_file_path);
        return 1;
    }

    free(pixels);
    return 0;
}
