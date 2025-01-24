#pragma once

#include <string>
#include <vector>
#include <optional>
#include <memory>

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
    void board();
    void controls();
    void game();
    void options();

    int get_board_player_type() const;
    void assert_engine_game_over();
    void engine_error(const engine::EngineError& e);
    void set_twelve_mens_morris();

    enum PlayerType {
        PlayerHuman,
        PlayerComputer
    };

    board::Board m_board;
    std::unique_ptr<engine::Engine> m_engine;

    int m_white {PlayerHuman};
    int m_black {PlayerComputer};

    enum class State {
        Ready,
        Start,
        NextTurn,
        HumanThinking,
        ComputerStartThinking,
        ComputerThinking,
        Stop,
        Over
    } m_state {State::Ready};

    std::vector<std::string> m_moves;
    std::string m_score;
    std::string m_pv;
    clock_::Clock m_clock;

    bool m_twelve_mens_morris {false};
};
