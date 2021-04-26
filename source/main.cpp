#include "common.h"
#include "hittable.h"
#include "hittable_list.h"
#include "material.h"
#include "sphere.h"
#include "camera.h"

#include <chrono>
#include <queue>
#include <thread>
#include <mutex>
#include <vector>

struct point2 {
    int x, y;
};

constexpr auto aspect_ratio     = 3.0 / 2.0;
constexpr int image_width       = 400;
constexpr int image_height      = static_cast<int>(image_width / aspect_ratio);
constexpr int samples_per_pixel = 10;
constexpr int max_depth         = 50;

static color buffer[image_width][image_height];
static color ray_color(const ray& r, const hittable& world, int depth)
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

static hittable_list random_scene()
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

                if (choose_mat < 0.8) {
                    // diffuse
                    auto albedo     = color::random() * color::random();
                    sphere_material = make_shared<lambertian>(albedo);
                    world.add(make_shared<sphere>(center, 0.2, sphere_material));
                } else if (choose_mat < 0.95) {
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

static std::queue<int>    lineQueue;
static std::mutex         queueMutex;
static void WorkerThread(camera& cam, hittable& world)
{
    while(true) {
        int line;

        {
            std::unique_lock<std::mutex> lock(queueMutex);
            if(lineQueue.empty()) {
                return;
            }
            line = lineQueue.front();
            lineQueue.pop();
        }

        for(int i = 0; i < image_width; i++) {
            point2 pixel = { 
                .x = i,
                .y = line
            };

            color pixel_color(0,0,0);
            for (int s = 0; s < samples_per_pixel; ++s) {
                auto u = (pixel.x + random_double()) / (image_width  - 1);
                auto v = (pixel.y + random_double()) / (image_height - 1);
                ray  r = cam.get_ray(u, v);
                pixel_color += ray_color(r, world, max_depth);
            }

            auto r = pixel_color.x();
            auto g = pixel_color.y();
            auto b = pixel_color.z();

            /* divide the coler by the number of samples */
            auto scale = 1.0 / samples_per_pixel;
            r = sqrt(scale * r);
            g = sqrt(scale * g);
            b = sqrt(scale * b);

            buffer[pixel.x][pixel.y] = color(
                256 * clamp(r, 0.0, 0.999),
                256 * clamp(g, 0.0, 0.999),
                256 * clamp(b, 0.0, 0.999)
            );
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
    // Get a random seed

    std::string str;
    std::cerr << "World seed: ";
    std::getline(std::cin, str);
    std::seed_seq seed(str.begin(), str.end());
    generator = std::mt19937(seed);

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
        aspect_ratio,
        aperture,
        dist_to_focus
    );

    // Render

    auto start = std::chrono::system_clock::now();

#   ifndef NO_THREADING
    for(int j = image_height - 1; j >= 0; --j) {
        lineQueue.push(j);
    }

    const unsigned threadCount       = std::thread::hardware_concurrency();
    std::vector<std::thread> threads(0);
    for(unsigned i = 0; i < threadCount; i++) {
        threads.push_back(
            std::thread(WorkerThread, std::ref(cam), std::ref(world))
        );
    }

    while(true) {
        std::unique_lock<std::mutex> lock(queueMutex);
        if(lineQueue.empty()) {
            std::cerr << "\r\nImage created. Writing to file...\r\n";
            break;
        }

        std::cerr << "\rScanlines remaining: " << lineQueue.size() << ' ' << std::flush;
    }

    std::cout << "P3\n" << image_width << ' ' << image_height << "\n255\n";

    for (int j = image_height-1; j >= 0; --j) {
        for (int i = 0; i < image_width; ++i) {
            std::cout << static_cast<int>(buffer[i][j].x()) << ' '
                      << static_cast<int>(buffer[i][j].y()) << ' '
                      << static_cast<int>(buffer[i][j].z()) << '\n';
        }
    }
#   else
    std::cout << "P3\n" << image_width << ' ' << image_height << "\n255\n";
    for(int j = image_height - 1; j >= 0; --j) {
        for(int i = 0; i < image_width; ++i) {
            color pixel_color(0, 0, 0);
            for(int s = 0; s < samples_per_pixel; ++s) {
                auto u = (i + random_double()) / (image_width  - 1);
                auto v = (j + random_double()) / (image_height - 1);
                ray  r = cam.get_ray(u, v);
                pixel_color += ray_color(r, world, max_depth);
            }
            write_color(std::cout, pixel_color, samples_per_pixel);
        }

        std::cerr << "\rScanlines remaining: " 
                  << j 
                  << ' ' 
                  << std::flush;
    }
#   endif

    auto end  = std::chrono::system_clock::now();
    auto time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cerr << "\nDone in " << time.count() << "ms.\n";
    exit(1);
}