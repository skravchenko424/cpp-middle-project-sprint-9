#pragma once

#include <SFML/System/Clock.hpp>
#include <chrono>
#include <cstdint>

// ===================== Compile-time config =====================

inline constexpr std::uint32_t THREAD_POOL_SIZE = 8;

// ===================== Render domain =====================

struct RenderSettings {
    std::uint32_t width{800};
    std::uint32_t height{600};
    std::uint32_t max_iterations{100};
    double escape_radius{2.0};
};

struct PixelRegion {
    std::uint32_t start_row{};
    std::uint32_t end_row{};
    std::uint32_t start_col{};
    std::uint32_t end_col{};
};

struct ViewPort {
    double x_min{-2.5};
    double x_max{1.5};
    double y_min{-2.0};
    double y_max{2.0};

    [[nodiscard]] constexpr double width() const noexcept { return x_max - x_min; }
    [[nodiscard]] constexpr double height() const noexcept { return y_max - y_min; }
};

struct AppState {
    ViewPort viewport;
    bool need_rerender{true};
    bool should_exit{false};

    sf::Clock zoom_clock;
    bool left_mouse_pressed{false};
    bool right_mouse_pressed{false};
    bool auto_zoom_enabled{false};

    // Seahorse Valley point for auto-zoom
    static constexpr double AUTO_ZOOM_TARGET_X = -0.7436438870371587;
    static constexpr double AUTO_ZOOM_TARGET_Y = 0.1318259043124;

    static constexpr ViewPort INITIAL_VIEWPORT{-2.5, 1.5, -2.0, 2.0};
};

class FrameClock {
public:
    FrameClock() { Reset(); }

    void Reset() noexcept { frame_start_ = std::chrono::steady_clock::now(); }

    auto GetFrameTime() const noexcept { return std::chrono::steady_clock::now() - frame_start_; }

private:
    std::chrono::time_point<std::chrono::steady_clock> frame_start_;
};

class AvrTimeCounter {
public:
    long GetAvr() const { return n_ > 0 ? total_ / n_ : 0; }
    long Count() const { return n_; }

    void Reset() {
        total_ = 0;
        n_ = 0;
    }

    void Start() { start_time_ = std::chrono::high_resolution_clock::now(); }

    long End() {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time_);
        total_ += duration.count();
        ++n_;
        return duration.count();
    }

private:
    long total_{0};
    long n_{0};
    std::chrono::high_resolution_clock::time_point start_time_{};
};
