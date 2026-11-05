#pragma once

#include "mandelbrot_fractal_utils.hpp"
#include "types_sfml.hpp"

#include <stdexec/execution.hpp>

#include <print>

using namespace std::chrono_literals;
namespace ex = stdexec;

namespace mandelbrot {

namespace {

constexpr std::size_t kChannelsPerPixel = 4;  // R, G, B, A
constexpr std::uint8_t kAlphaOpaque = 255;

}  // namespace

static auto MakeComputeSender(RenderSettings settings, ViewPort viewport) {
    static AvrTimeCounter time_counter;
    return ex::then([settings, viewport](FrameBuffer *fb) {
               time_counter.Start();

               const std::uint32_t width = fb->width;
               const std::uint32_t height = fb->height;

               for (std::uint32_t y = 0; y < height; ++y) {
                   for (std::uint32_t x = 0; x < width; ++x) {
                       const Complex c = Pixel2DToComplex(x, y, viewport, width, height);

                       const std::uint32_t iterations =
                           CalculateIterationsForPoint(c, settings.max_iterations, settings.escape_radius);

                       const RgbColor color = IterationsToColor(iterations, settings.max_iterations);

                       const std::size_t idx = (static_cast<std::size_t>(y) * width + x) * kChannelsPerPixel;
                       fb->rgba[idx + 0] = color.r;
                       fb->rgba[idx + 1] = color.g;
                       fb->rgba[idx + 2] = color.b;
                       fb->rgba[idx + 3] = kAlphaOpaque;
                   }
               }

               return fb;
           }) |
           ex::then([](FrameBuffer *fb) {
               time_counter.End();
               if (time_counter.Count() % 10 == 0) {
                   std::println("\nAverage compute time: {} ms over {} frames", time_counter.GetAvr(),
                                time_counter.Count());
               }
               return fb;
           });
}

}  // namespace mandelbrot
