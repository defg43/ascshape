#include <iostream>
#include <fstream>
#include <cstdlib>
#include <string>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

void print_usage() {
    std::cout << "Usage: img_to_ascii [image_file] [output_width] [output_height] [output_file]\n";
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        print_usage();
        return 1;
    }

    std::string image_path = argv[1];
    int output_width = std::atoi(argv[2]);
    int output_height = std::atoi(argv[3]);
    std::string output_file = argv[4];

    int width, height, channels;
    unsigned char *img_data = stbi_load(image_path.c_str(), &width, &height, &channels, 0);
    if (!img_data) {
        std::cerr << "Error loading image file: " << image_path << std::endl;
        return 1;
    }

    char *ascii_image = new char[output_width * output_height];
    if (!ascii_image) {
        std::cerr << "Error allocating memory for ASCII image" << std::endl;
        return 1;
    }

    float h_scale = (float) height / output_height;
    float w_scale = (float) width / output_width;

    for (int y = 0; y < output_height; y++) {
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

            ascii_image[y * output_width + x] = ascii_char;
        }
    }

    std::ofstream output(output_file);
    if (!output.is_open()) {
        std::cerr << "Error opening output file: " << output_file << std::endl;
        return 1;
    }

    for (int y = 0; y < output_height; y++) {
        for (int x = 0; x < output_width; x++) {
            output << ascii_image[y * output_width + x];
        }
        output << std::endl;
    }

    output.close();

    delete[] ascii_image;
    stbi_image_free(img_data);

    return 0;
}

