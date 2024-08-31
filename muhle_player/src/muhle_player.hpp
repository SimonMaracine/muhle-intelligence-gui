#pragma once

#include <string>
#include <vector>

#include <gui_base/gui_base.hpp>
#include <common/board.hpp>
#include <common/subprocess.hpp>

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
    void import_position();
    void reset();
    void about();
    void notation();
    void board();
    void controls();

    void terminate_process(const char* message);
    std::vector<std::string> parse_message(const std::string& message);

    enum PlayerType {
        PlayerHuman,
        PlayerComputer
    };

    board::MuhleBoard m_muhle_board;
    subprocess::Subprocess m_muhle_process;

    std::string m_engine_filename {""};

    int m_white {PlayerHuman};
    int m_black {PlayerComputer};

    enum class State {
        NextTurn,
        HumanThinking,
        ComputerBegin,
        ComputerThinking
    } m_state {State::NextTurn};
};
