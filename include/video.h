#ifndef ASCII_VIEW_VIDEO_H
#define ASCII_VIEW_VIDEO_H

#include <stdio.h>
#include <stdbool.h>
#include "image.h"

typedef struct {
    FILE* pipe;
    size_t width;
    size_t height;
    double fps;
    double* frame_buffer; // double array [0, 1]
    unsigned char* raw_buffer; // raw rgb24 bytes
} video_stream_t;

video_stream_t* open_video(const char* filepath, size_t max_width, size_t max_height, double character_ratio);
bool read_video_frame(video_stream_t* vs, image_t* out_image);
void close_video(video_stream_t* vs);

#endif
