#include "SDL2/SDL_image.h"
#include "sdlpp/events.hpp"
#include "sdlpp/surface.hpp"
#include "sdlpp/window.hpp"
#include <cstring>
#include <iostream>
#include <vector>

int mainLoop() {
    auto running = true;

    for (std::optional<SDL_Event> o; (o = sdl::waitEvent());) {
        auto event = *o;

        switch (event.type) {
        case SDL_QUIT:
            running = false;
            break;
        }

        if (!running) {
            break;
        }
    }

    return 0;
}

sdl::Surface randDither(sdl::SurfaceView surface) {
    auto ditherSurface = surface.duplicate();

    ditherSurface.lock();

    auto p = reinterpret_cast<uint8_t *>(ditherSurface->pixels);

    auto depth = ditherSurface->format->BytesPerPixel;

    for (int y = 0; y < ditherSurface->h; ++y) {
        for (int x = 0; x < ditherSurface->w; ++x) {
            auto value =
                (ditherSurface.pixelData(x, y, 1, depth) > (rand() % 255)) *
                255;

            ditherSurface.pixelData(x, y, 0, depth) = value;
            ditherSurface.pixelData(x, y, 1, depth) = value;
            ditherSurface.pixelData(x, y, 2, depth) = value;
        }
    }

    ditherSurface.unlock();

    return ditherSurface;
}

sdl::Surface errorDiffusingDither(sdl::SurfaceView surface) {
    auto ns = surface.duplicate();

    auto errorData = std::vector<int>(surface->w * surface->h, 0);
    auto error = [&errorData, w = surface->w](int x, int y) -> int & {
        return errorData.at(y * w + x);
    };

    int depth = surface->format->BytesPerPixel;
    ns.lock();
    for (int y = 0; y < ns->h; ++y) {
        for (int x = 0; x < ns->w; ++x) {
            auto intensity =
                (ns.pixelData(x, y, 0, depth) + ns.pixelData(x, y, 1, depth) +
                 ns.pixelData(x, y, 2, depth)) /
                3;
            error(x, y) = intensity;
        }
    }
    ns.unlock();

    for (int y = 0; y < ns->h - 1; ++y) {
        for (int x = 1; x < ns->w - 1; ++x) {
            auto &e = error(x, y);
            auto newError = (e > (256 / 2)) ? 255 : 0;
            auto quantError = e - newError;
            e = newError;

            error(x + 1, y) = error(x + 1, y) + quantError * 7 / 16;
            error(x - 1, y + 1) = error(x - 1, y + 1) + quantError * 3 / 16;
            error(x, y + 1) = error(x, y + 1) + quantError * (5 / 16);
            error(x + 1, y + 1) = error(x + 1, y + 1) + quantError * 1 / 16;
        }
    }

    ns.lock();
    for (int y = 0; y < ns->h; ++y) {
        for (int x = 0; x < ns->w; ++x) {
            auto &e = error(x, y);
            e = std::max(0, e);
            e = std::min(255, e);

            ns.pixelData(x, y, 0, depth) = e;
            ns.pixelData(x, y, 1, depth) = e;
            ns.pixelData(x, y, 2, depth) = e;
        }
    }
    ns.unlock();

    return ns;
}

struct Matrix {
    std::vector<uint8_t> data;
    int width;
    int height;
};

Matrix createBayerMatrix() {
    auto matrix = Matrix{
        .data = std::vector<uint8_t>(6 * 6),
        .width = 6,
        .height = 6,
    };

    auto area = matrix.width * matrix.height;

    for (int y = 0; y < matrix.height; ++y) {
        for (int x = 0; x < matrix.width; ++x) {
            auto data =
                (x * matrix.width - y * (matrix.height - 1) + area) % area;

            // Implementation not finished. Do this... some time
            matrix.data.at(y * matrix.width + x) = data * 255 / area;
            std::cout << data << " ";
        }
        std::cout << "\n";
    }
    std::cout << std::endl;
    return matrix;
}

sdl::Surface orderedDither(sdl::SurfaceView surface, const Matrix &matrix) {
    auto ns = surface.duplicate();

    int depth = surface->format->BytesPerPixel;

    auto matrixArea = matrix.width * matrix.height;

    ns.lock();
    for (int y = 0; y < ns->h; ++y) {
        for (int x = 0; x < ns->w; ++x) {
            auto bx = x % matrix.width;
            auto by = y % matrix.height;
            auto bp =
                matrix.data.at(by * matrix.width + bx) * (255 / matrixArea);

            auto intensity =
                (ns.pixelData(x, y, 0, depth) + ns.pixelData(x, y, 1, depth) +
                 ns.pixelData(x, y, 2, depth)) /
                3;
            ns.pixelData(x, y, 0, depth) = ns.pixelData(x, y, 1, depth) =
                ns.pixelData(x, y, 2, depth) = (intensity > bp) * 255;
        }
    }
    ns.unlock();

    return ns;
}

int main(int argc, char *argv[]) {
    auto window = sdl::Window("dither",
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              512,
                              512,
                              SDL_WINDOW_SHOWN);

    IMG_Init(IMG_INIT_PNG);

    auto surface = sdl::Surface{IMG_Load("data/Lenna.png")};

    if (!surface) {
        std::cerr << "could not load data" << std::endl;
        std::terminate();
    }

    auto dotsMatrix = Matrix{
        .data =
            {
                // clang-format off
        34, 29, 17, 21, 30, 35,
        28, 14,  9, 16, 20, 31,
        13,  8,  4,  5, 15, 19,
        12,  3,  0,  1, 10, 18,
        27,  7,  2,  6, 23, 24,
        33, 26, 11, 22, 25, 32,
                // clang-format on
            },
        .width = 6,
        .height = 6,
    };

    // https://en.wikipedia.org/wiki/Ordered_dithering
    auto bayerMatrix = Matrix{
        .data =
            {
                // clang-format off
                0, 32,  8, 40,  2, 34, 10, 42,
                48, 16, 56, 24, 50, 18, 58, 26,
                12, 44,  4, 36, 14, 46,  6, 38,
                60, 28, 52, 20, 62, 30, 54, 22,
                3, 35, 11, 43,  1, 33,  9, 41,
                51, 19, 59, 27, 49, 17, 57, 25,
                15, 47,  7, 39, 13, 45,  5, 37,
                63, 31, 55, 23, 61, 29, 53, 21,
                // clang-format on
            },
        .width = 8,
        .height = 8,
    };

    //    auto bayerMatrix = createBayerMatrix();

    auto screen = window.surface();

    auto ditherSurface = randDither({surface});
    IMG_SavePNG(ditherSurface, "Lenna-rand-dithered.png");
    screen.blitScaled({ditherSurface});

    auto errorDiffused = errorDiffusingDither({surface});
    IMG_SavePNG(errorDiffused, "Lenna-error-dithered.png");
    screen.blitScaled({errorDiffused});

    auto ordered = orderedDither({surface}, bayerMatrix);
    IMG_SavePNG(ordered, "Lenna-ordered-dither.png");
    screen.blitScaled({ordered});

    window.updateSurface();

    return mainLoop();
}
