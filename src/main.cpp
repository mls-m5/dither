#include "SDL2/SDL_image.h"
#include "sdlpp/events.hpp"
#include "sdlpp/surface.hpp"
#include "sdlpp/window.hpp"
#include <cstring>
#include <iostream>

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

sdl::Surface errorCorrectingDither(sdl::SurfaceView surface) {}

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

    auto ditherSurface = randDither({surface});

    IMG_SavePNG(ditherSurface, "Lenna-rand-dithered.png");

    auto screen = window.surface();
    screen.blitScaled({ditherSurface});
    window.updateSurface();

    return mainLoop();
}
