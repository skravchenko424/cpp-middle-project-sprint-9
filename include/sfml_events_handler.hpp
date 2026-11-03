#pragma once

#include <SFML/Graphics.hpp>
#include <stdexec/execution.hpp>

#include "types_core.hpp"

class SfmlEventHandler {
public:

    template <typename Receiver>
    struct OperationState {
        Receiver receiver_;
        sf::RenderWindow &window_;
        RenderSettings render_settings_;
        AppState &state_;

        static constexpr float ZOOM_INTERVAL_MS = 100.0f;

        template <typename R>
        explicit OperationState(R &&r, sf::RenderWindow &window, RenderSettings render_settings, AppState &state)
            : receiver_{std::forward<R>(r)}, window_{window}, render_settings_{render_settings}, state_{state} {}

        /* Ваш код метода start() здесь */

    private:
        void HandleEvents() {
            sf::Event event;
            while (window_.pollEvent(event)) {
                switch (event.type) {
                case sf::Event::Closed:
                state_.should_exit = true;
                break;

                /* Ваш код здесь  */

                default:
                    break;
                }
            }
        }

        void HandleKeyPress(const sf::Event::KeyEvent &key) {
            /* Ваш код здесь */
        }

        void HandleMousePress(const sf::Event::MouseButtonEvent &mouse) {
            /* Ваш код здесь */
        }

        void HandleMouseRelease(const sf::Event::MouseButtonEvent &mouse) {
            if (mouse.button == sf::Mouse::Left) {
                state_.left_mouse_pressed = false;
            } else if (mouse.button == sf::Mouse::Right) {
                state_.right_mouse_pressed = false;
            }
        }

        void HandleAutoZoom() {
            /* Ваш код здесь */
        }

        void ZoomToPoint(int pixel_x, int pixel_y, bool zoom_in, double factor = 0.8) {
            const double target_x = state_.viewport.x_min +
                                    (static_cast<double>(pixel_x) / render_settings_.width) * state_.viewport.width();
            const double target_y = state_.viewport.y_min +
                                    (static_cast<double>(pixel_y) / render_settings_.height) * state_.viewport.height();

            const double zoom_factor = zoom_in ? factor : (1.0 / factor);
            const double new_width = state_.viewport.width() * zoom_factor;
            const double new_height = state_.viewport.height() * zoom_factor;
            
            /* Ваш код обновления state_ здесь  */
        }
    };

    sf::RenderWindow &window_;
    RenderSettings render_settings_;
    AppState &state_;

    SfmlEventHandler(sf::RenderWindow &window, RenderSettings render_settings, AppState &state)
        : window_{window}, render_settings_{render_settings}, state_{state} {}

    /* Ваш код методов connect() и get_completion_signatures() здесь */

};
