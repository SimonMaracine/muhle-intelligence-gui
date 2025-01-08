#pragma once

#include <string>
#include <vector>
#include <tuple>
#include <optional>

#include <gui_base/gui_base.hpp>

#include "board.hpp"
#include "engine.hpp"
#include "clock.hpp"

class MuhlePlayer : public gui_base::GuiApplication {
public:
    explicit MuhlePlayer(const gui_base::WindowProperties& properties)
        : gui_base::GuiApplication(properties) {}

    void start() override;
    void update() override;
    void stop() override;
private:
    void load_engine(const std::string& file_path);
    void unload_engine();

    void main_menu_bar();
    void load_engine();
    void load_engine_dialog();
    void reset_position(const std::optional<std::string>& position);
    void set_position();
    void about();
    void notation();
    void board();
    void controls();
    void game();

    int get_board_player_type() const;
    static std::tuple<unsigned int, unsigned int, unsigned int> split_time(unsigned int time_milliseconds);

    enum PlayerType {
        PlayerHuman,
        PlayerComputer
    };

    board::Board m_board;
    engine::Engine m_engine;
    std::string m_engine_name {""};

    int m_white {PlayerHuman};
    int m_black {PlayerComputer};

    enum class State {
        Ready,
        Start,
        NextTurn,
        HumanThinking,
        ComputerBegin,
        ComputerThinking,
        Stop,
        Over
    } m_state {State::Ready};

    std::vector<std::string> m_moves;

    clock_::Clock m_clock;
};
