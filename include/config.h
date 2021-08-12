#pragma once
#include <fstream> // ifstream
#include <sstream> // stringstream
#include <string_view> // string_view
#include <iostream> // cerr
#include <format> // format

struct Prefs
{
    double aspect_ratio;
    int image_width;
    int image_height;
    int samples_per_pixel;
    int max_depth;
    bool use_threading;
    int seed;
};

inline Prefs read_from_file(const char* path)
{
    std::ifstream file(path);
    if (!file.is_open()) {
        Prefs defaultVals = {
            .aspect_ratio      = 16.0 / 9.0,
            .image_width       = 400,
            .image_height      = static_cast<int>(400 / (16.0 / 9.0)),
            .samples_per_pixel = 10,
            .max_depth         = 50,
            .use_threading     = true,
            .seed              = 1234
        };

        std::cerr << std::format("Info: Couldn't get preferences from file '{}'. Using default values.\n", path);

        std::ofstream save(path);
        if (save.is_open()) {
            save << std::format("{}\n", defaultVals.aspect_ratio);
            save << std::format("{}\n", defaultVals.image_width);
            save << std::format("{}\n", defaultVals.samples_per_pixel);
            save << std::format("{}\n", defaultVals.max_depth);
            save << std::format("{}\n", (int)defaultVals.use_threading);
            save << std::format("{}\n", defaultVals.seed);

            save << "|--- What the values are:\n";
            save << "1. aspect ratio (default is 16:9)\n2. image width\n3. samples per pixel\n";
            save << "4. max depth\n5. use threading\n6. world seed\n";

            std::cerr << std::format("Info: Created file '{}' with default settings.\n", path);
        } else {
            std::cerr << std::format("Error: Couldn't write to file {}.\n", path);
        }

        return defaultVals;
    }

    Prefs prefs = {};
    file >> prefs.aspect_ratio;
    file >> prefs.image_width;
    file >> prefs.samples_per_pixel;
    file >> prefs.max_depth;
    file >> prefs.use_threading;
    file >> prefs.seed;
    file.close();

    prefs.image_height = static_cast<int>(prefs.image_width / prefs.aspect_ratio);
    return prefs;
}
