#pragma once

#include <chrono>
#include <tuple>

namespace clock_ {
    class Clock {
    public:
        void reset(unsigned int time = 1000 * 60 * 3);
        void start();
        void stop();
        void update();
        void switch_turn();

        unsigned int get_white_time() const { return m_white_time; }
        unsigned int get_black_time() const { return m_black_time; }

        static std::tuple<unsigned int, unsigned int, unsigned int> split_time(unsigned int time);
    private:
        static void set_time(std::chrono::steady_clock::time_point& last_time);
        static void update_time(unsigned int& time, std::chrono::steady_clock::time_point& last_time);

        bool m_running {false};
        bool m_player_white {true};
        unsigned int m_white_time {1000 * 60 * 3};
        unsigned int m_black_time {1000 * 60 * 3};
        std::chrono::steady_clock::time_point m_white_last_time {};
        std::chrono::steady_clock::time_point m_black_last_time {};
    };
}
