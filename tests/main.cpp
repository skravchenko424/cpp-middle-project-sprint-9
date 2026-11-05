#include <gtest/gtest.h>

#include <SFML/Graphics.hpp>

#include <exception>
#include <memory>
#include <optional>
#include <utility>

#include <exec/static_thread_pool.hpp>
#include <stdexec/execution.hpp>

#include "mandelbrot_sender.hpp"
#include "sfml_display_sender.hpp"
#include "sfml_events_handler.hpp"
#include "types_sfml.hpp"

namespace ex = stdexec;

struct ReceiverLog {
    bool got_value = false;
    bool got_error = false;
    bool got_stopped = false;
    std::exception_ptr error{};
    FrameBuffer *value_ptr = nullptr;
};

struct RecordingReceiver {
    using receiver_concept = stdexec::receiver_t;

    std::shared_ptr<ReceiverLog> log;

    void set_value() && noexcept { log->got_value = true; }

    void set_value(FrameBuffer *fb) && noexcept {
        log->got_value = true;
        log->value_ptr = fb;
    }

    void set_error(std::exception_ptr e) && noexcept {
        log->got_error = true;
        log->error = e;
    }

    void set_stopped() && noexcept { log->got_stopped = true; }

    auto get_env() const noexcept { return stdexec::empty_env{}; }
};

class SenderFixture : public ::testing::Test {
protected:
    void SetUp() override {
        state_ = std::make_unique<SfmlState>(
            RenderSettings{.width = 64, .height = 64, .max_iterations = 100, .escape_radius = 2.0});
    }

    std::unique_ptr<SfmlState> state_;
};

// ============================================================================
//  MakeComputeSender
// ============================================================================

TEST_F(SenderFixture, ComputeFillsAlphaChannel) {
    auto sender =
        ex::just(&state_->fb) | mandelbrot::MakeComputeSender(state_->render_settings, state_->app_state.viewport);
    auto res = ex::sync_wait(std::move(sender));
    ASSERT_TRUE(res.has_value());

    for (std::size_t i = 3; i < state_->fb.rgba.size(); i += 4) {
        EXPECT_EQ(state_->fb.rgba[i], 255u);
    }
}

TEST_F(SenderFixture, ComputeReturnsSameBufferPointer) {
    auto sender =
        ex::just(&state_->fb) | mandelbrot::MakeComputeSender(state_->render_settings, state_->app_state.viewport);
    auto res = ex::sync_wait(std::move(sender));
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(std::get<0>(*res), &state_->fb);
}

TEST_F(SenderFixture, ComputeProducesNonBlackPixels) {
    auto sender =
        ex::just(&state_->fb) | mandelbrot::MakeComputeSender(state_->render_settings, state_->app_state.viewport);
    ex::sync_wait(std::move(sender));

    bool has_non_black = false;
    for (std::size_t i = 0; i < state_->fb.rgba.size(); i += 4) {
        if (state_->fb.rgba[i] || state_->fb.rgba[i + 1] || state_->fb.rgba[i + 2]) {
            has_non_black = true;
            break;
        }
    }
    EXPECT_TRUE(has_non_black);
}

// ============================================================================
//  SfmlEventHandler
// ============================================================================

TEST_F(SenderFixture, EventHandlerEmitsValueWithoutEvents) {
    state_->app_state.should_exit = false;
    state_->app_state.need_rerender = false;
    state_->app_state.auto_zoom_enabled = false;

    auto log = std::make_shared<ReceiverLog>();
    auto op = ex::connect(SfmlEventHandler{state_->window, state_->render_settings, state_->app_state},
                          RecordingReceiver{log});
    ex::start(op);

    EXPECT_TRUE(log->got_value);
    EXPECT_FALSE(log->got_stopped);
    EXPECT_FALSE(log->got_error);
    // Без событий pollEvent ничего не отдаёт — состояние не меняется
    EXPECT_FALSE(state_->app_state.should_exit);
    EXPECT_FALSE(state_->app_state.need_rerender);
}

TEST_F(SenderFixture, EventHandlerEmitsStoppedWhenExitFlagPreset) {
    state_->app_state.should_exit = true;

    auto log = std::make_shared<ReceiverLog>();
    auto op = ex::connect(SfmlEventHandler{state_->window, state_->render_settings, state_->app_state},
                          RecordingReceiver{log});
    ex::start(op);

    EXPECT_TRUE(log->got_stopped);
    EXPECT_FALSE(log->got_value);
    EXPECT_FALSE(log->got_error);
}

TEST_F(SenderFixture, EventHandlerConnectYieldsOperationState) {
    auto op = ex::connect(SfmlEventHandler{state_->window, state_->render_settings, state_->app_state},
                          RecordingReceiver{std::make_shared<ReceiverLog>()});
    (void)op;  // just test if it compiles
}

// ============================================================================
//  SfmlDisplaySender
// ============================================================================

TEST_F(SenderFixture, DisplaySenderEmitsValue) {
    auto log = std::make_shared<ReceiverLog>();
    auto op = ex::connect(ex::just(&state_->fb) | render::MakeSfmlDisplaySender(*state_), RecordingReceiver{log});
    ex::start(op);

    EXPECT_TRUE(log->got_value);
    EXPECT_FALSE(log->got_error);
    EXPECT_FALSE(log->got_stopped);
}

struct TestReceiver {
    using receiver_concept = stdexec::receiver_t;

    bool *done;
    std::exception_ptr *err;

    void set_value(auto &&...) && noexcept { *done = true; }
    void set_error(std::exception_ptr e) && noexcept { *err = e; }
    void set_stopped() && noexcept {}
    stdexec::env<> get_env() const noexcept { return {}; }
};

bool BufferHasVisiblePixel(const FrameBuffer &buf) {
    for (std::size_t idx = 0; idx < buf.rgba.size(); idx += 4) {
        if (buf.rgba[idx] | buf.rgba[idx + 1] | buf.rgba[idx + 2]) {
            return true;
        }
    }
    return false;
}

TEST(CustomReceiver, SenderSignalsThroughCustomReceiver) {
    constexpr std::uint32_t kWidth = 4;
    constexpr std::uint32_t kHeight = 4;
    ViewPort vp{-2.0, 2.0, -2.0, 2.0};
    RenderSettings cfg{.width = kWidth, .height = kHeight, .max_iterations = 10, .escape_radius = 2.0};

    auto fb = FrameBuffer::Make(kWidth, kHeight);
    auto sender = ex::just(&fb) | mandelbrot::MakeComputeSender(cfg, vp);
    bool done = false;
    std::exception_ptr error = nullptr;

    auto op = stdexec::connect(std::move(sender), TestReceiver{&done, &error});
    stdexec::start(op);

    EXPECT_TRUE(done);
    EXPECT_EQ(error, nullptr);
}

TEST(Integration, ComputePipelineProducesPixels) {
    constexpr std::uint32_t kSize = 16;
    ViewPort vp{-2.5, 1.5, -2.0, 2.0};
    RenderSettings cfg{.width = kSize, .height = kSize, .max_iterations = 100, .escape_radius = 2.0};

    exec::static_thread_pool pool{2};
    auto sched = pool.get_scheduler();
    auto fb = FrameBuffer::Make(kSize, kSize);
    auto work = ex::on(sched, ex::just(&fb) | mandelbrot::MakeComputeSender(cfg, vp));
    auto outcome = ex::sync_wait(std::move(work));

    ASSERT_TRUE(outcome.has_value());
    EXPECT_TRUE(BufferHasVisiblePixel(fb));
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}