#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bitmap.h"


/*
 * Read in bitmap header data from stdin, and return a pointer to
 * a new Bitmap struct containing the important metadata for the image file.
 *
 * TODO: complete this function.
 *
 * Notes:
 *   1. Store header data in an array of unsigned char (essentially
 *      an array of bytes). Examples:
 *      on the stack,
 *          unsigned char data[10];
 *      on the heap,
 *          unsigned char *data = malloc(10);
 *
 *   2. Don't make any assumptions about the header size. You should read in
 *      BMP_HEADER_SIZE_OFFSET bytes first, and then the header size,
 *      and then use this to allocate enough space for the actual header.
 *
 *   3. You can use memcpy to transfer bytes to and from the Bitmap "header" field.
 *      You can even write these bytes to memory allocated for variables of other types!
 *      For example:
 *          unsigned char bytes[4];
 *          int x = 10;
 *          int y;
 *          memcpy(bytes, &x, 4);  // Copy the int x into bytes.
 *          memcpy(&y, bytes, 4);  // Copy the content of bytes into y.
 *
 *   4. You can use either fread/fwrite or read/write to perform I/O operations here.
 *
 *   5. Make good use of the provided macros in bitmap.h to index into the "header" array.
 */
Bitmap *read_header() {
    Bitmap *bitmap = malloc(sizeof(Bitmap));
    unsigned char data[BMP_HEADER_SIZE_OFFSET];
    int error_1, error_2, error_3;
    error_1 = fread(data, sizeof(unsigned char), BMP_HEADER_SIZE_OFFSET, stdin);
    if (error_1 != BMP_HEADER_SIZE_OFFSET) {
        fprintf(stderr, "Error: beginning of the header could not be read.\n");
    }
    error_2 = fread(&bitmap->headerSize, sizeof(int), 1, stdin);
    if (error_2 != 1) {
        fprintf(stderr, "Error: header size could not be read.\n");
    }
    bitmap->header = malloc(sizeof(unsigned char) * bitmap->headerSize);
    unsigned char headerSize[4];
    memcpy(headerSize, &bitmap->headerSize, 4);
    int counter = 0;
    for (int i = 0; i < sizeof(data); i++){
        bitmap->header[counter] = data[i];
        counter++;
    }
    for (int i = 0; i < sizeof(headerSize); i++){
        bitmap->header[counter] = headerSize[i];
        counter++;
    }
    int current = BMP_HEADER_SIZE_OFFSET + 4;
    unsigned char data2[bitmap->headerSize - current];
    error_3 = fread(data2, sizeof(unsigned char), bitmap->headerSize - current, stdin);
    if (error_3 != bitmap->headerSize - current) {
        fprintf(stderr, "Error: rest of the header could not be read.\n");
    }
    for (int i = 0; i < sizeof(data2); i++){
        bitmap->header[counter] = data2[i];
        counter++;
    }
    unsigned char height[4] = {data2[BMP_HEIGHT_OFFSET - current], data2[BMP_HEIGHT_OFFSET - current + 1],
                               data2[BMP_HEIGHT_OFFSET - current + 2], data2[BMP_HEIGHT_OFFSET - current + 3]};
    unsigned char width[4] = {data2[BMP_WIDTH_OFFSET - current], data2[BMP_WIDTH_OFFSET - current + 1],
                              data2[BMP_WIDTH_OFFSET - current + 2], data2[BMP_WIDTH_OFFSET - current + 3]};
    int num_height;
    int num_width;
    memcpy(&num_height, height, 4);
    memcpy(&num_width, width, 4);
    bitmap->height = num_height;
    bitmap->width = num_width;
    return bitmap;
}

/*
 * Write out bitmap metadata to stdout.
 * You may add extra fprintf calls to *stderr* here for debugging purposes.
 */
void write_header(const Bitmap *bmp) {
    fwrite(bmp->header, bmp->headerSize, 1, stdout);
}

/*
 * Free the given Bitmap struct.
 */
void free_bitmap(Bitmap *bmp) {
    free(bmp->header);
    free(bmp);
}

/*
 * Update the bitmap header to record a resizing of the image.
 *
 * TODO: complete this function when working on the "scale" filter.
 *
 * Notes:
 *   1. As with read_header, use memcpy and the provided macros in bitmap.h.
 *
 *   2. bmp->header *must* be updated, as this is what's written out
 *      in write_header.
 *
 *   3. You may choose whether or not to also update bmp->width and bmp->height.
 *      This choice may depend on how you implement the scale filter.
 */
void scale(Bitmap *bmp, int scale_factor) {
    int new_width = scale_factor * bmp->width;
    int new_height = scale_factor * bmp->height;
    int new_file_size = new_height * new_width * 3 + bmp->headerSize;
    unsigned char width_2[4];
    unsigned char height_2[4];
    unsigned char new_size[4];
    memcpy(width_2, &new_width, 4);
    memcpy(height_2, &new_height, 4);
    memcpy(new_size, &new_file_size, 4);
    int size_offset = BMP_FILE_SIZE_OFFSET;
    for (int i = 0; i < sizeof(new_size); i++) {
        bmp->header[size_offset] = new_size[i];
        size_offset += 1;
    }
    int width_offset = BMP_WIDTH_OFFSET;
    for (int i = 0; i < sizeof(width_2); i++) {
        bmp->header[width_offset] = width_2[i];
        width_offset += 1;
    }
    int height_offset = BMP_HEIGHT_OFFSET;
    for (int i = 0; i < sizeof(height_2); i++) {
        bmp->header[height_offset] = height_2[i];
        height_offset += 1;
    }
}


/*
 * The "main" function.
 *
 * Run a given filter function, and apply a scale factor if necessary.
 * You don't need to modify this function to make it work with any of
 * the filters for this assignment.
 */
void run_filter(void (*filter)(Bitmap *), int scale_factor) {
    Bitmap *bmp = read_header();

    if (scale_factor > 1) {
        scale(bmp, scale_factor);
    }

    write_header(bmp);

    // Note: here is where we call the filter function.
    filter(bmp);

    free_bitmap(bmp);
}


/******************************************************************************
 * The gaussian blur and edge detection filters.
 * You should NOT modify any of the code below.
 *****************************************************************************/
const int gaussian_kernel[3][3] = {
    {1, 2, 1},
    {2, 4, 2},
    {1, 2, 1}
};

const int kernel_dx[3][3] = {
    {1, 0, -1},
    {2, 0, -2},
    {1, 0, -1}
};

const int kernel_dy[3][3] = {
    {1, 2, 1},
    {0, 0, 0},
    {-1, -2, -1}
};

const int gaussian_normalizing_factor = 16;


Pixel apply_gaussian_kernel(Pixel *row0, Pixel *row1, Pixel *row2) {
    int b = 0, g = 0, r = 0;
    Pixel *rows[3] = {row0, row1, row2};
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            b += rows[i][j].blue * gaussian_kernel[i][j];
            g += rows[i][j].green * gaussian_kernel[i][j];
            r += rows[i][j].red * gaussian_kernel[i][j];
        }
    }

    b /= gaussian_normalizing_factor;
    g /= gaussian_normalizing_factor;
    r /= gaussian_normalizing_factor;

    Pixel new = {
        .blue = b,
        .green = g,
        .red = r
    };

    return new;
}


Pixel apply_edge_detection_kernel(Pixel *row0, Pixel *row1, Pixel *row2) {
    int b_dx = 0, b_dy = 0;
    int g_dx = 0, g_dy = 0;
    int r_dx = 0, r_dy = 0;
    Pixel *rows[3] = {row0, row1, row2};

    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            b_dx += rows[i][j].blue * kernel_dx[i][j];
            b_dy += rows[i][j].blue * kernel_dy[i][j];
            g_dx += rows[i][j].green * kernel_dx[i][j];
            g_dy += rows[i][j].green * kernel_dy[i][j];
            r_dx += rows[i][j].red * kernel_dx[i][j];
            r_dy += rows[i][j].red * kernel_dy[i][j];
        }
    }
    int b = floor(sqrt(square(b_dx) + square(b_dy)));
    int g = floor(sqrt(square(g_dx) + square(g_dy)));
    int r = floor(sqrt(square(r_dx) + square(r_dy)));

    int edge_val = max(r, max(g, b));
    Pixel new = {
        .blue = edge_val,
        .green = edge_val,
        .red = edge_val
    };

    return new;
}
