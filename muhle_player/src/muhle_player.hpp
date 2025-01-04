#pragma once

#include <string>
#include <vector>

#include <gui_base/gui_base.hpp>

#include "board.hpp"
#include "engine.hpp"

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
    void reset_position();
    void set_position();
    void about();
    void notation();
    void board();
    void controls();

    int get_player_type(board::Player player) const;

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
        NotStarted,
        NextTurn,
        HumanThinking,
        ComputerBegin,
        ComputerThinking
    } m_state {State::NotStarted};

    std::vector<std::string> m_moves;
};
