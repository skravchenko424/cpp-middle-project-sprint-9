#include <chrono>
#include <memory>
#include <print>
#include <thread>
#include <utility>

#include <SFML/Graphics.hpp>

#include <exec/any_sender_of.hpp>
#include <exec/repeat_until.hpp>
#include <exec/static_thread_pool.hpp>
#include <stdexec/execution.hpp>

#include "mandelbrot_sender.hpp"
#include "sfml_display_sender.hpp"
#include "sfml_events_handler.hpp"
#include "types_sfml.hpp"

using namespace std::chrono_literals;
namespace ex = stdexec;

class WaitForFPS {
public:
    static constexpr unsigned int TARGET_FPS = 60;

    explicit WaitForFPS(FrameClock &frame_clock, unsigned int target_fps = TARGET_FPS)
        : frame_clock_(frame_clock), frame_time_(std::chrono::duration_cast<std::chrono::milliseconds>(
                                         std::chrono::duration<double>(1.0 / static_cast<double>(target_fps)))) {}

    void operator()() {
        auto cur_frame_duration = frame_clock_.GetFrameTime();

        if (cur_frame_duration < frame_time_) {
            std::this_thread::sleep_for(frame_time_ - cur_frame_duration);
        }
        frame_clock_.Reset();
    }

private:
    FrameClock &frame_clock_;
    const std::chrono::milliseconds frame_time_ = 1ms;
};

class MandelbrotApp {
public:
    MandelbrotApp() : compute_pool_{std::max(1u, std::thread::hardware_concurrency())}, sfml_thread_{1} {
        std::println("hardware_concurrency: {}\n", std::thread::hardware_concurrency());
    }

    void Run() {
        auto compute_sched = compute_pool_.get_scheduler();
        auto sfml_sched = sfml_thread_.get_scheduler();

        auto initialize =
            ex::on(sfml_sched,
                   ex::just() | ex::then([this]() {
                       state_ = std::make_unique<SfmlState>(  //
                           RenderSettings{.width = 800, .height = 600, .max_iterations = 100, .escape_radius = 2.0});
                   }));
        ex::sync_wait(std::move(initialize));

        using process_frame_sender_type = exec::any_sender<exec::any_receiver<
            ex::completion_signatures<ex::set_value_t(), ex::set_error_t(std::exception_ptr), ex::set_stopped_t()>>>;

        auto process_frame =
            ex::starts_on(sfml_sched, SfmlEventHandler{state_->window, state_->render_settings, state_->app_state}) |
            ex::let_value([this, compute_sched, sfml_sched]() -> process_frame_sender_type {
                if (!state_->app_state.need_rerender) {
                    return ex::just();
                }

                return ex::starts_on(compute_sched, ex::just(&state_->fb) |
                                                        mandelbrot::MakeComputeSender(state_->render_settings,
                                                                                      state_->app_state.viewport)) |
                       ex::continues_on(sfml_sched) | render::MakeSfmlDisplaySender(*state_);
            }) |
            ex::then([this]() { state_->app_state.need_rerender = false; }) |
            ex::then(WaitForFPS{state_->frame_clock, WaitForFPS::TARGET_FPS});

        auto repeated_pipeline = std::move(process_frame) | ex::then([this] { return state_->app_state.should_exit; }) |
                                 exec::repeat_until();
        ex::sync_wait(std::move(repeated_pipeline));
    }

private:
    std::unique_ptr<SfmlState> state_;

    exec::static_thread_pool compute_pool_;
    exec::static_thread_pool sfml_thread_;
};

int main() {
    std::println("=== Mandelbrot Fractal Renderer ===\n");
    std::println("Controls:");
    std::println("  Left Mouse Button  - Zoom In");
    std::println("  Right Mouse Button - Zoom Out");
    std::println("  X                  - Toggle Auto Zoom (infinite zoom to 'Seahorse Valley' point)");
    std::println("  C                  - Reset to Initial View");
    std::println("  Close Window       - Exit\n");

    try {
        MandelbrotApp app;
        app.Run();
    } catch (const std::exception &e) {
        std::println("Error: {}", e.what());
        return 1;
    }
    return 0;
}