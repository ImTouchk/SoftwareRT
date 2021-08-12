#include "common.h"
#include "hittable.h"
#include "hittable_list.h"
#include "material.h"
#include "sphere.h"
#include "camera.h"
#include "config.h"
#include "image.h"

#include <format>
#include <chrono>
#include <queue>
#include <thread>
#include <mutex>
#include <vector>

struct point2 {
    int x, y;
};

static Prefs prefs;
static color* pBuffer = NULL;
color& pixelAt(size_t x, size_t y)
{
    return pBuffer[y * prefs.image_width + x];
}

color ray_color(const ray& r, const hittable& world, int depth)
{
    hit_record rec;

    // If we've exceeded the ray bounce limit, no more light is gathered.
    if (depth <= 0)
        return color(0,0,0);

    if (world.hit(r, 0.001, infinity, rec)) {
        ray   scattered;
        color attenuation;

        if (rec.mat_ptr->scatter(r, rec, attenuation, scattered))
            return attenuation * ray_color(scattered, world, depth-1);

        return color(0,0,0);
    }

    vec3 unit_direction = unit_vector(r.direction());
    auto t              = 0.5*(unit_direction.y() + 1.0);
    return (1.0-t)*color(1.0, 1.0, 1.0) + t*color(0.5, 0.7, 1.0);
}

hittable_list random_scene()
{
    hittable_list world;

    auto ground_material = make_shared<lambertian>(color(0.5, 0.5, 0.5));
    world.add(make_shared<sphere>(point3(0,-1000,0), 1000, ground_material));

    for (int a = -30; a < 30; a++) {
        for (int b = -30; b < 30; b++) {
            auto choose_mat = random_double();
            point3 center(
                a + 0.9*random_double(), 
                random_double(0.2, 0.5), /* 0.2 */
                b + 0.9*random_double()
            );

            if ((center - point3(4, 0.2, 0)).length() > 0.9) {
                shared_ptr<material> sphere_material;

                if (choose_mat < 0.60) {
                    // diffuse
                    auto albedo     = color::random() * color::random();
                    sphere_material = make_shared<lambertian>(albedo);
                    world.add(make_shared<sphere>(center, 0.2, sphere_material));
                } else if (choose_mat < 0.75) {
                    // metal
                    auto albedo     = color::random(0.5, 1);
                    auto fuzz       = random_double(0, 0.5);
                    sphere_material = make_shared<metal>(albedo, fuzz);
                    world.add(make_shared<sphere>(center, 0.2, sphere_material));
                } else {
                    // glass
                    sphere_material = make_shared<dielectric>(1.5);
                    world.add(make_shared<sphere>(center, 0.2, sphere_material));
                }
            }
        }
    }

    auto material1 = make_shared<dielectric>(1.5);
    world.add(make_shared<sphere>(point3(0, 1, 0), 1.0, material1));

    auto material2 = make_shared<lambertian>(color(0.4, 0.2, 0.1));
    world.add(make_shared<sphere>(point3(-4, 1, 0), 1.0, material2));

    auto material3 = make_shared<metal>(color(0.7, 0.6, 0.5), 0.0);
    world.add(make_shared<sphere>(point3(4, 1, 0), 1.0, material3));

    return world;
}

static std::queue<int> lineQueue;
static std::mutex      queueMutex;
static std::mutex      flagMutex;
static unsigned        threadsFinished = 0;
void WorkerThread(camera& cam, hittable& world)
{
    while(true) {
        int line;

        {
            std::unique_lock<std::mutex> lock(queueMutex);
            if(lineQueue.empty()) {
                std::unique_lock<std::mutex> flagLock(flagMutex);
                threadsFinished++;
                return;
            }
            line = lineQueue.front();
            lineQueue.pop();
        }

        for(int i = 0; i < prefs.image_width; i++) {
            point2 pixel = { 
                .x = i,
                .y = line
            };

            color pixel_color(0,0,0);
            for (int s = 0; s < prefs.samples_per_pixel; ++s) {
                auto u = (pixel.x + random_double()) / (prefs.image_width  - 1);
                auto v = (pixel.y + random_double()) / (prefs.image_height - 1);
                ray  r = cam.get_ray(u, v);
                pixel_color += ray_color(r, world, prefs.max_depth);
            }

            pixelAt(pixel.x, pixel.y) = pixel_color;
        }
    }
}

static std::uniform_real_distribution<double> distribution(0.0, 1.0);
static std::mt19937                           generator;
double random_double()
{
    return distribution(generator);
}

int main() {
    // Read config from file
    prefs = read_from_file("prefs.cfg");

#ifndef NDEBUG
    std::cerr << "SoftwareRT (Debug Build)\n";
#else
    std::cerr << "SoftwareRT (Release Build)\n";
#endif // NDEBUG


    std::cerr << "Info: Running raytracer with the following configuration:\n";
    std::cerr << std::format(" | Aspect ratio: {}\n", prefs.aspect_ratio);
    std::cerr << std::format(" | Resolution: {}x{}\n", prefs.image_width, prefs.image_height);
    std::cerr << std::format(" | Samples per pixel: {}\n", prefs.samples_per_pixel);
    std::cerr << std::format(" | Max depth: {}\n", prefs.max_depth);
    std::cerr << std::format(" | Enable multithreading: {}\n", prefs.use_threading);
    std::cerr << std::format(" | World seed: {}\n", prefs.seed);

    generator = std::mt19937(prefs.seed);
    pBuffer = new color[prefs.image_width * prefs.image_height];

    // World

    auto world = random_scene();

    // Camera

    point3 lookfrom(13,2,3);
    point3 lookat(0,0,0);
    vec3   vup(0,1,0);
    auto   dist_to_focus = 10.0;
    auto   aperture      = 0.1;

    camera cam(
        lookfrom, 
        lookat,
        vup,
        20,
        prefs.aspect_ratio,
        aperture,
        dist_to_focus
    );

    // Render

    auto start = std::chrono::system_clock::now();

    if (prefs.use_threading) {
        for(int j = prefs.image_height - 1; j >= 0; --j) {
            lineQueue.push(j);
        }

        const unsigned threadCount = std::thread::hardware_concurrency();
        std::cerr << std::format("Info: Using {} threads.\n", threadCount);

        std::vector<std::thread> threads(0);
        for(unsigned i = 0; i < threadCount; i++) {
            threads.push_back(
                std::thread(WorkerThread, std::ref(cam), std::ref(world))
            );
        }

        while(true) {
            std::unique_lock<std::mutex> flagLock(flagMutex);
            if(threadsFinished == threadCount) {
                break;
            }

            std::cerr << std::format("\rScanlines remaining: {} ", lineQueue.size());
            std::cerr << std::flush;
        }

        for(auto& thread : threads)
            thread.join();

        threads.clear();
    } else {
        std::cerr << "Info: Using one single thread.\n";

        for(int j = prefs.image_height - 1; j >= 0; --j) {
            for(int i = 0; i < prefs.image_width; ++i) {
                color pixel_color(0, 0, 0);
                for(int s = 0; s < prefs.samples_per_pixel; ++s) {
                    auto u   = (i + random_double()) / (prefs.image_width  - 1);
                    auto v   = (j + random_double()) / (prefs.image_height - 1);
                    ray  ray = cam.get_ray(u, v);
                    pixel_color += ray_color(ray, world, prefs.max_depth);
                }

                pixelAt(i, j) = pixel_color;
            }

            std::cerr << std::format("\rScanlines remaining: {} ", j);
            std::cerr << std::flush;
        }
    }


    auto end  = std::chrono::system_clock::now();
    auto time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cerr << std::format(
        "\nInfo: Finished rendering in {}ms/{}s/{}m.\n",
        time.count(),
        time.count() / 1000,
        time.count() / 1000 / 60
    );

    std::cerr << "Info: Writing output to file.\n";

    const std::time_t now = std::time(nullptr);
    const std::tm calendarTime = *std::localtime(std::addressof(now));

    // Filename: MM-DD HH:MM:SS
    std::string out = std::format(
        "{}-{} {}-{}-{}", 
        calendarTime.tm_mon,
        calendarTime.tm_mday,
        calendarTime.tm_hour,
        calendarTime.tm_min,
        calendarTime.tm_sec
    );

    std::string out_ppm = out + ".ppm";
    std::string out_bmp = out + ".bmp";

    write_as_ppm(pBuffer, prefs, out_ppm.c_str());
    //write_as_bmp(pBuffer, prefs, out_bmp.c_str());
    
    delete[] pBuffer;
    return 0;
}