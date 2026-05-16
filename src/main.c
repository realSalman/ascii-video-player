#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "../include/image.h"
#include "../include/print_image.h"
#include "../include/argparse.h"
#include "../include/video.h"

#ifdef _WIN32
#include <windows.h>
#include <process.h>
void sleep_ms(int ms) {
    Sleep(ms);
}
#else
#include <unistd.h>
void sleep_ms(int ms) {
    usleep(ms * 1000);
}
#endif

int is_video_file(const char* filepath) {
    const char* ext = strrchr(filepath, '.');
    if (!ext) return 0;
    
    char lower_ext[16] = {0};
    for (int i = 0; ext[i] && i < 15; i++) {
        lower_ext[i] = tolower((unsigned char)ext[i]);
    }
    
    if (strcmp(lower_ext, ".mp4") == 0 ||
        strcmp(lower_ext, ".webm") == 0 ||
        strcmp(lower_ext, ".avi") == 0 ||
        strcmp(lower_ext, ".mkv") == 0 ||
        strcmp(lower_ext, ".mov") == 0) {
        return 1;
    }
    return 0;
}

int main(int argc, char* argv[]) {
    // Parses arguments
    args_t args = parse_args(argc, argv);
    if (args.file_path == NULL)
        return 1;

    if (is_video_file(args.file_path)) {
        // Optimized open: ffplay for audio + ffmpeg for scaled video pipe
        video_stream_t* vs = open_video(args.file_path, args.max_width, args.max_height, args.character_ratio);
        if (!vs) {
            fprintf(stderr, "Failed to open video. Ensure 'ffprobe.exe' is available.\n");
            return 1;
        }

        // Try to launch background audio
        char audio_cmd[2048];
#ifdef _WIN32
        char full_path[MAX_PATH];
        if (GetFullPathNameA(args.file_path, MAX_PATH, full_path, NULL)) {
            // Use PowerShell to play audio since ffplay might be missing.
            // We use PresentationCore's MediaPlayer which supports MP4/AAC/etc natively on Windows.
            snprintf(audio_cmd, sizeof(audio_cmd), 
                "start /B powershell -WindowStyle Hidden -Command \"Add-Type -AssemblyName PresentationCore; $m = New-Object System.Windows.Media.MediaPlayer; $m.Open('%s'); $m.Play(); Start-Sleep -s 3600\"", 
                full_path);
        } else {
            snprintf(audio_cmd, sizeof(audio_cmd), "echo Error: Could not get full path for audio.");
        }
#else
        snprintf(audio_cmd, sizeof(audio_cmd), "ffplay -nodisp -autoexit -v error \"%s\" &", args.file_path);
#endif
        system(audio_cmd);

        double target_frame_time = 1.0 / vs->fps;
        image_t frame = {0};

        // Clear screen and hide cursor for video playback
        printf("\x1b[2J\x1b[H\x1b[?25l");

        clock_t start_time = clock();
        int frame_count = 0;

        if (!read_video_frame(vs, &frame)) {
            fprintf(stderr, "\x1b[?25h\n[ERROR] Could not read frames from ffmpeg.\n");
            close_video(vs);
            return 1;
        }

        do {
            // Note: frame is already resized by ffmpeg pipe in video.c!
            print_image(&frame, args.edge_threshold, args.use_retro_colors, 1);
            
            frame_count++;
            
            // Calculate how long to wait to maintain sync
            double elapsed = (double)(clock() - start_time) / CLOCKS_PER_SEC;
            double next_frame_time = frame_count * target_frame_time;
            
            if (next_frame_time > elapsed) {
                sleep_ms((int)((next_frame_time - elapsed) * 1000));
            }
        } while (read_video_frame(vs, &frame));

        // Show cursor again
        printf("\x1b[?25h");
        printf("\n");

        close_video(vs);
    } else {
        // Loads image
        image_t original = load_image(args.file_path);
        if (!original.data)
            return 1;

        // Resizes image
        image_t resized = make_resized(&original, args.max_width, args.max_height, args.character_ratio);
        if (!resized.data) {
            free_image(&original);
            return 1;
        }
        
        print_image(&resized, args.edge_threshold, args.use_retro_colors, 0);
            
        free_image(&original);
        free_image(&resized);
    }

    return 0;
}
