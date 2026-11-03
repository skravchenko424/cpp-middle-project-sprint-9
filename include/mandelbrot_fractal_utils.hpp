#pragma once

#include <complex>
#include <cstdint>

#include "types_core.hpp"

namespace mandelbrot {

using Complex = std::complex<double>;

struct RgbColor {
    std::uint8_t r;
    std::uint8_t g;
    std::uint8_t b;
};

struct RgbColors {
    RgbColors() = delete;

    inline static constexpr RgbColor BLACK = RgbColor{0, 0, 0};
};

[[nodiscard]] constexpr std::uint32_t CalculateIterationsForPoint(const Complex &c, std::uint32_t max_iterations,
                                                                  double escape_radius) noexcept {

    Complex z{0.0, 0.0};
    const double escape_radius_squared = escape_radius * escape_radius;

    for (std::uint32_t i = 0; i < max_iterations; ++i) {
        if (std::norm(z) > escape_radius_squared) {
            return i;
        }
        z = z * z + c;
    }
    return max_iterations;
}

[[nodiscard]] constexpr Complex Pixel2DToComplex(std::uint32_t x, std::uint32_t y, const ViewPort &viewport,
                                                 const std::uint32_t screen_width,
                                                 const std::uint32_t screen_height) noexcept {

    const double real = viewport.x_min + (static_cast<double>(x) / screen_width) * viewport.width();
    const double imag = viewport.y_min + (static_cast<double>(y) / screen_height) * viewport.height();
    return Complex{real, imag};
}

[[nodiscard]] constexpr RgbColor IterationsToColor(std::uint32_t iterations, std::uint32_t max_iterations) noexcept {

    // Точка принадлежит множеству Мандельброта
    if (iterations == max_iterations) {
        return RgbColors::BLACK;
    }

    // Переводим количество итераций в RGB цвет
    const double hue = (360.0 * iterations) / max_iterations;
    const double saturation = 1.0;
    const double value = 1.0;

    const double c = value * saturation;
    const double x = c * (1.0 - std::abs(std::fmod(hue / 60.0, 2.0) - 1.0));
    const double m = value - c;

    double r, g, b;
    if (hue < 60) {
        r = c;
        g = x;
        b = 0;
    } else if (hue < 120) {
        r = x;
        g = c;
        b = 0;
    } else if (hue < 180) {
        r = 0;
        g = c;
        b = x;
    } else if (hue < 240) {
        r = 0;
        g = x;
        b = c;
    } else if (hue < 300) {
        r = x;
        g = 0;
        b = c;
    } else {
        r = c;
        g = 0;
        b = x;
    }

    const auto red = static_cast<std::uint8_t>((r + m) * 255);
    const auto green = static_cast<std::uint8_t>((g + m) * 255);
    const auto blue = static_cast<std::uint8_t>((b + m) * 255);

    return RgbColor{red, green, blue};
}

}  // namespace mandelbrot