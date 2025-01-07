#pragma once

#include <chrono>

namespace clock_ {
    class Clock {
    public:
        void reset(unsigned int time = 1000 * 60 * 10);
        void start();
        void stop();
        void update();
        void switch_turn();

        unsigned int get_white_time() const { return m_white_time; }
        unsigned int get_black_time() const { return m_black_time; }
    private:
        static void start_time(std::chrono::steady_clock::time_point& last_time);
        static void update_time(unsigned int& time, std::chrono::steady_clock::time_point& last_time);

        bool m_running {false};
        bool m_player_white {true};
        unsigned int m_white_time {1000 * 60 * 10};
        unsigned int m_black_time {1000 * 60 * 10};
        std::chrono::steady_clock::time_point m_white_last_time {};
        std::chrono::steady_clock::time_point m_black_last_time {};
    };
}
