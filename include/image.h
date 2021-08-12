#pragma once
#include <cmath>
#include <format>
#include <fstream>
#include <iostream>
#include "common.h"
#include "config.h"

inline void write_as_ppm(color* pBuf, Prefs& prefs, const char* path)
{
    std::ofstream file(path);
    if (!file.is_open()) {
        std::cerr << std::format("Error: Couldn't write to '{}'.\n", path);
        return;
    }

    file << std::format("P3\n{} {}\n255\n", prefs.image_width, prefs.image_height);

    for (int j = prefs.image_height - 1; j >= 0; --j) {
        for (int i = 0; i < prefs.image_width; ++i) {
            auto& pixel = pBuf[j * prefs.image_width + i];

            auto r = pixel.x();
            auto g = pixel.y();
            auto b = pixel.z();

            /* divide the coler by the number of samples */
            auto scale = 1.0 / prefs.samples_per_pixel;
            r = sqrt(scale * r);
            g = sqrt(scale * g);
            b = sqrt(scale * b);

            auto finalCol = color(
                256 * clamp(r, 0.0, 0.999),
                256 * clamp(g, 0.0, 0.999),
                256 * clamp(b, 0.0, 0.999)
            );

            file << static_cast<int>(finalCol.x()) << ' '
                 << static_cast<int>(finalCol.y()) << ' '
                 << static_cast<int>(finalCol.z()) << '\n';
        }
    }

    file.close();

    std::cerr << std::format("Info: Saved output to '{}'.\n", path);
}
