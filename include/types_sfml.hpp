#pragma once

#include <SFML/Config.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/System/Clock.hpp>

#include <cstdint>
#include <vector>

#include "types_core.hpp"

//
// Holds the framebuffer data for rendering
//
struct FrameBuffer {
    std::uint32_t width{};
    std::uint32_t height{};
    std::vector<sf::Uint8> rgba;

    static FrameBuffer Make(std::uint32_t w, std::uint32_t h) {
        FrameBuffer fb;
        fb.width = w;
        fb.height = h;
        fb.rgba.resize(static_cast<size_t>(w) * h * 4u);
        return fb;
    }
};

//
// Must be created and accessed only from SFML thread
//
struct SfmlState {
    RenderSettings render_settings;
    AppState app_state;

    sf::RenderWindow window;
    sf::Texture texture;
    sf::Sprite sprite;
    FrameBuffer fb;

    FrameClock frame_clock;

    explicit SfmlState(const RenderSettings &s)
        : render_settings(s),
          window(sf::VideoMode{render_settings.width, render_settings.height}, "Mandelbrot Fractal"),
          fb(FrameBuffer::Make(render_settings.width, render_settings.height)) {

        window.setKeyRepeatEnabled(false);

        texture.create(render_settings.width, render_settings.height);
        sprite.setTexture(texture);
    }
};
