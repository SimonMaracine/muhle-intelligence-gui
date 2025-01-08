#include "clock.hpp"

namespace clock_ {
    void Clock::reset(unsigned int time) {
        m_running = false;
        m_player_white = true;
        m_white_time = time;
        m_black_time = time;
    }

    void Clock::start() {
        m_running = true;

        if (m_player_white) {
            set_time(m_white_last_time);
        } else {
            set_time(m_black_last_time);
        }
    }

    void Clock::stop() {
        m_running = false;
    }

    void Clock::update() {
        if (!m_running) {
            return;
        }

        if (m_player_white) {
            update_time(m_white_time, m_white_last_time);
        } else {
            update_time(m_black_time, m_black_last_time);
        }
    }

    void Clock::switch_turn() {
        m_player_white = !m_player_white;

        if (m_player_white) {
            set_time(m_white_last_time);
        } else {
            set_time(m_black_last_time);
        }
    }

    void Clock::set_time(std::chrono::steady_clock::time_point& last_time) {
        last_time = std::chrono::steady_clock::now();
    }

    void Clock::update_time(unsigned int& time, std::chrono::steady_clock::time_point& last_time) {
        const auto now {std::chrono::steady_clock::now()};

        auto delta {(now - last_time).count()};

        while ((delta -= 1000 * 1000) >= 0) {
            if (time > 0) {
                time -= 1;
            }
        }

        last_time = now;
    }
}
