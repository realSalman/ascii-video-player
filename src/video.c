#include "../include/video.h"
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define POPEN _popen
#define PCLOSE _pclose
#else
#define POPEN popen
#define PCLOSE pclose
#endif

video_stream_t* open_video(const char* filepath, size_t max_width, size_t max_height, double character_ratio) {
    char cmd[1024];
    
    // 1. Get video info using ffprobe
    snprintf(cmd, sizeof(cmd), "ffprobe -v error -select_streams v:0 -show_entries stream=width,height,r_frame_rate -of default=nw=1:nk=1 \"%s\"", filepath);
    
    FILE* probe = POPEN(cmd, "r");
    if (!probe) {
        fprintf(stderr, "Error: Failed to start ffprobe. Ensure FFmpeg is installed and in your PATH.\n");
        return NULL;
    }

    size_t orig_width = 0, orig_height = 0;
    int fps_num = 0, fps_den = 0;
    
    // Check if we can read anything
    if (fscanf(probe, "%zu\n%zu\n%d/%d", &orig_width, &orig_height, &fps_num, &fps_den) != 4) {
        rewind(probe);
        if (fscanf(probe, "%zu\n%zu\n%d", &orig_width, &orig_height, &fps_num) != 3) {
            fprintf(stderr, "Error: Could not parse video info. Is 'ffprobe' installed and in your PATH?\n");
            PCLOSE(probe);
            return NULL;
        }
        fps_den = 1;
    }
    PCLOSE(probe);

    if (orig_width == 0 || orig_height == 0 || fps_num == 0 || fps_den == 0) return NULL;

    double fps = (double)fps_num / fps_den;

    // 2. Calculate target dimensions (matching logic from image.c)
    size_t target_width, target_height;
    size_t proposed_height = (orig_height * max_width) / (character_ratio * orig_width);
    if (proposed_height <= max_height) {
        target_width = max_width;
        target_height = proposed_height;
    } else {
        target_width = (character_ratio * orig_width * max_height) / (orig_height);
        target_height = max_height;
    }

    // 3. Open ffmpeg pipe with SCALING enabled
    // We use -vf scale=W:H to let ffmpeg do the heavy lifting
    snprintf(cmd, sizeof(cmd), "ffmpeg -v error -i \"%s\" -vf scale=%zu:%zu -f image2pipe -vcodec rawvideo -pix_fmt rgb24 -", 
             filepath, target_width, target_height);
    
    FILE* pipe = POPEN(cmd, "rb");
    if (!pipe) {
        fprintf(stderr, "Error: Could not start ffmpeg pipe.\n");
        return NULL;
    }

    video_stream_t* vs = malloc(sizeof(video_stream_t));
    vs->pipe = pipe;
    vs->width = target_width;
    vs->height = target_height;
    vs->fps = fps;
    
    size_t total_pixels = target_width * target_height;
    vs->raw_buffer = malloc(total_pixels * 3);
    vs->frame_buffer = calloc(total_pixels * 3, sizeof(double));

    if (!vs->raw_buffer || !vs->frame_buffer) {
        close_video(vs);
        return NULL;
    }

    return vs;
}

bool read_video_frame(video_stream_t* vs, image_t* out_image) {
    if (!vs || !vs->pipe) return false;

    size_t expected_bytes = vs->width * vs->height * 3;
    size_t bytes_read = fread(vs->raw_buffer, 1, expected_bytes, vs->pipe);

    if (bytes_read < expected_bytes) return false;

    for (size_t i = 0; i < expected_bytes; i++) {
        vs->frame_buffer[i] = vs->raw_buffer[i] / 255.0;
    }

    out_image->width = vs->width;
    out_image->height = vs->height;
    out_image->channels = 3;
    out_image->data = vs->frame_buffer;

    return true;
}

void close_video(video_stream_t* vs) {
    if (!vs) return;
    if (vs->pipe) PCLOSE(vs->pipe);
    if (vs->raw_buffer) free(vs->raw_buffer);
    if (vs->frame_buffer) free(vs->frame_buffer);
    free(vs);
}
