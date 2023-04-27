#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstring>
#include <cmath>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Function to print usage message
void print_usage() {
    std::cout << "Usage: img_to_code [image_path] [source_path] [output_width] [output_height]\n";
}

// Function to load image and convert to ASCII art
char* image_to_ascii(const char* image_path, int output_width, int output_height) {
    int width, height, channels;
    unsigned char *img_data = stbi_load(image_path, &width, &height, &channels, 0);

    if (!img_data) {
        std::cerr << "Error loading image file: " << image_path << std::endl;
        return NULL;
    }

    char *ascii_image = (char *)malloc(output_width * output_height * sizeof(char));

    if (!ascii_image) {
        std::cerr << "Error allocating memory for ASCII image\n";
        return NULL;
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

    stbi_image_free(img_data);

    return ascii_image;
}

// Function to replace '#' characters in the ASCII art with code from source file
void replace_with_code(char* ascii_image, int ascii_size, const char* source_path) {
    std::ifstream source_file(source_path);

    if (!source_file.is_open()) {
        std::cerr << "Error opening source file: " << source_path << std::endl;
        return;
    }

    std::string line;
    std::string code_buffer;
    int i = 0;

    while (std::getline(source_file, line)) {
        int j = 0;

        while (j < line.length()) {
            if (i >= ascii_size) {
                std::cerr << "Error: ASCII image size is smaller than source file size\n";
                return;
            }

            if (ascii_image[i] == '#') {
                // Find the next non-alphanumeric character in the line
                while (j < line.length() && (isalnum(line[j]) || line[j] == '_')) {
                    code_buffer.push_back(line[j]);
                    j++;
                }

                // Output the code buffer and update the index
                std::cout << code_buffer;
                i += code_buffer.length();
                code_buffer.clear();

                // Output the '#' character
                std::cout                << ascii_image[i];
            } else {
                std::cout << ascii_image[i];
            }

            i++;
            j++;
        }

        std::cout << std::endl;
    }

    source_file.close();
}

// Main function
int main(int argc, char** argv) {
    // Parse command line arguments
    if (argc != 5) {
        print_usage();
        return EXIT_FAILURE;
    }

    const char* image_path = argv[1];
    const char* source_path = argv[2];
    int output_width = std::stoi(argv[3]);
    int output_height = std::stoi(argv[4]);

    // Load image and convert to ASCII art
    char* ascii_image = image_to_ascii(image_path, output_width, output_height);

    if (!ascii_image) {
        return EXIT_FAILURE;
    }

    // Replace '#' characters with code from source file
    replace_with_code(ascii_image, output_width * output_height, source_path);

    free(ascii_image);

    return EXIT_SUCCESS;
}

