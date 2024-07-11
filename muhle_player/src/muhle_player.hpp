#pragma once

#include <string>
#include <vector>

#include <gui_base/gui_base.hpp>
#include <common/board.hpp>
#include <common/subprocess.hpp>

struct MuhlePlayer : public gui_base::GuiApplication {
    explicit MuhlePlayer(const gui_base::WindowProperties& properties)
        : gui_base::GuiApplication(properties) {}

    void start() override;
    void update() override;
    void stop() override;

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

    board::MuhleBoard muhle_board;
    subprocess::Subprocess muhle_process;

    std::string engine_filename {""};

    int white {PlayerHuman};
    int black {PlayerComputer};

    enum class State {
        NextTurn,
        HumanThinking,
        ComputerBegin,
        ComputerThinking
    } state {State::NextTurn};

    static constexpr int PlayerHuman {0};
    static constexpr int PlayerComputer {1};
};
