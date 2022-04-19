#include "SDL2/SDL_image.h"
#include "sdlpp/events.hpp"
#include "sdlpp/surface.hpp"
#include "sdlpp/window.hpp"
#include <cstring>
#include <iostream>
#include <vector>

int mainLoop() {
    auto running = true;

    for (std::optional<SDL_Event> o; (o = sdl::waitEvent()) && running;) {
        auto event = *o;

        switch (event.type) {
        case SDL_QUIT:
            running = false;
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

    auto screen = window.surface();

    auto ditherSurface = randDither({surface});
    IMG_SavePNG(ditherSurface, "Lenna-rand-dithered.png");
    screen.blitScaled({ditherSurface});

    auto errorDiffused = errorDiffusingDither({surface});
    IMG_SavePNG(errorDiffused, "Lenna-error-dithered.png");
    screen.blitScaled({errorDiffused});

    window.updateSurface();

    return mainLoop();
}
