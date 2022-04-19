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

    auto s1 = surface.duplicate();

    s1.lock();

    auto p = reinterpret_cast<uint8_t *>(s1->pixels);

    auto depth = s1->format->BytesPerPixel;

    for (int y = 0; y < s1->h; ++y) {
        for (int x = 0; x < s1->w; ++x) {
            auto value = (s1.pixelData(x, y, 1, depth) > (rand() % 255)) * 255;

            s1.pixelData(x, y, 0, depth) = value;
            s1.pixelData(x, y, 1, depth) = value;
            s1.pixelData(x, y, 2, depth) = value;
        }
    }

    s1.unlock();

    auto screen = window.surface();

    screen.blitScaled({s1});
    //    screen.blit({s1});

    IMG_SavePNG(s1, "Lenna-rand-dithered.png");

    window.updateSurface();

    return mainLoop();
}
