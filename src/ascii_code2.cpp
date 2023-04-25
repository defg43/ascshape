#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

void print_usage() {
    printf("Usage: img_to_code [image_file] [output_width] [output_height]\n");
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        print_usage();
        return 1;
    }

    char *image_path = argv[1];
    int output_width = atoi(argv[2]);
    int output_height = atoi(argv[3]);

    int width, height, channels;
    unsigned char *img_data = stbi_load(image_path, &width, &height, &channels, 0);
    if (!img_data) {
        fprintf(stderr, "Error loading image file: %s\n", image_path);
        return 1;
    }

    printf("#include <stdio.h>\n\n");
    printf("int main() {\n");

    float h_scale = (float) height / output_height;
    float w_scale = (float) width / output_width;

    for (int y = 0; y < output_height; y++) {
        printf("    printf(\"");
        for (int x = 0; x < output_width; x++) {
            int img_x = (int) (x * w_scale);
            int img_y = (int) (y * h_scale);
            unsigned char *pixel = &img_data[(img_y * width + img_x) * channels];

            // Compute average brightness of pixel
            float brightness = 0;
            for (int i = 0; i < channels; i++) {
                brightness += pixel[i];
            }
            brightness /= channels * 255;

            // Map brightness to ASCII character
            char ascii_char;
            if (brightness < 0.8) {
                ascii_char = '#';
            } else {
                ascii_char = ' ';
            }

            printf("%c", ascii_char);
        }
        printf("\\n\");\n");
    }

    printf("    return 0;\n");
    printf("}\n");

    stbi_image_free(img_data);

    return 0;
}
