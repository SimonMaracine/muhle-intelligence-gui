#include "muhle_player.hpp"

#include <iostream>
#include <filesystem>
#include <stdexcept>
#include <numeric>
#include <cstdlib>

#include <gui_base/gui_base.hpp>
#include <ImGuiFileDialog.h>

void MuhlePlayer::start() {
    ImGuiIO& io {ImGui::GetIO()};
    io.ConfigWindowsMoveFromTitleBarOnly = true;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    m_board = board::Board([this](const board::Move& move) {
        m_moves.push_back(board::move_to_string(move));

        m_clock.switch_turn();

        if (m_board.get_game_over() != board::GameOver::None) {
            // TODO check engine

            m_state = State::Stop;
            return;
        }

        switch (get_board_player_type()) {
            case PlayerHuman:
                m_state = State::NextTurn;
                break;
            case PlayerComputer:
                m_state = State::NextTurn;
                break;
        }
    });

    m_engine.set_info_callback([](const engine::Engine::Info& info, void* pointer) {
        MuhlePlayer* self {static_cast<MuhlePlayer*>(pointer)};

        if (info.score) {
            switch (info.score->index()) {
                case 0:
                    self->m_score = "eval " + std::to_string(std::get<0>(*info.score).value);
                    break;
                case 1:
                    self->m_score = "win " + std::to_string(std::get<1>(*info.score).value);
                    break;
            }
        }

        if (info.pv) {
            self->m_pv = std::accumulate(++info.pv->cbegin(), info.pv->cend(), *info.pv->cbegin(), [](std::string r, const std::string& move) {
                return std::move(r) + " " + move;
            });
        }
    }, this);
}

void MuhlePlayer::update() {
    main_menu_bar();
    board();
    controls();
    game();
    load_engine_dialog();

    m_clock.update();

    if (m_clock.get_white_time() == 0) {
        m_board.timeout(board::Player::White);
    }

    if (m_clock.get_black_time() == 0) {
        m_board.timeout(board::Player::Black);
    }

    switch (m_state) {
        case State::Ready:
            break;
        case State::Start:
            m_clock.start();
            m_state = State::NextTurn;

            break;
        case State::NextTurn: {
            switch (get_board_player_type()) {
                case PlayerHuman:
                    m_state = State::HumanThinking;
                    break;
                case PlayerComputer:
                    m_state = State::ComputerBegin;
                    break;
            }

            break;
        }
        case State::HumanThinking:
            break;
        case State::ComputerBegin:
            try {
                m_engine.start_thinking(
                    board::position_to_string(m_board.get_setup_position()),
                    m_moves,
                    std::nullopt,
                    std::nullopt,
                    std::nullopt,
                    1000
                );
            } catch (const engine::EngineError& e) {
                std::cerr << "Engine error: " << e.what() << '\n';
                m_state = State::Stop;
                break;
            }

            m_state = State::ComputerThinking;

            break;
        case State::ComputerThinking: {
            std::optional<std::string> best_move;

            try {
                best_move = m_engine.done_thinking();
            } catch (const engine::EngineError& e) {
                std::cerr << "Engine error: " << e.what() << '\n';
                m_state = State::Stop;
                break;
            }

            if (best_move) {
                if (*best_move == "none") {
                    if (m_board.get_game_over() == board::GameOver::None) {
                        throw std::runtime_error("The engine calls game over, but the GUI doesn't agree");
                    }

                    break;
                }

                std::cout << "Playing move on the board " << *best_move << '\n';

                m_board.play_move(board::move_from_string(*best_move));
            }

            break;
        }
        case State::Stop:
            m_clock.stop();
            m_state = State::Over;

            break;
        case State::Over:
            break;
    }
}

void MuhlePlayer::stop() {
    unload_engine();
}

void MuhlePlayer::load_engine(const std::string& file_path) {
    try {
        m_engine.initialize(file_path);
    } catch (const engine::EngineError& e) {
        std::cerr << "Could not start engine: " << e.what() << '\n';
        return;
    }

    try {
        m_engine.set_debug(true);
    } catch (const engine::EngineError& e) {
        std::cerr << "Engine error: " << e.what() << '\n';
        return;
    }

    try {
        m_engine.new_game();
        m_engine.synchronize();
    } catch (const engine::EngineError& e) {
        std::cerr << "Engine error: " << e.what() << '\n';
        return;
    }

    m_engine_name = m_engine.get_name();
}

void MuhlePlayer::unload_engine() {
    if (!m_engine.active()) {
        return;
    }

    try {
        m_engine.uninitialize();
    } catch (const engine::EngineError& e) {
        std::cerr << "Could not stop engine: " << e.what() << '\n';
    }
}

void MuhlePlayer::main_menu_bar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Player")) {
            const bool active {m_state == State::Ready || m_state == State::HumanThinking || m_state == State::Over};

            if (ImGui::MenuItem("Load Engine", nullptr, nullptr, active)) {
                load_engine();
            }
            if (ImGui::MenuItem("Reset Position", nullptr, nullptr, active)) {
                reset_position(std::nullopt);
            }
            if (ImGui::BeginMenu("Set Position", active)) {
                set_position();

                ImGui::EndMenu();
            }
            if (ImGui::MenuItem("Quit")) {
                quit();
            }

            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Options")) {
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::BeginMenu("About")) {
                about();

                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Notation")) {
                notation();

                ImGui::EndMenu();
            }

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}

void MuhlePlayer::load_engine() {
    IGFD::FileDialogConfig config;
    config.flags |= ImGuiFileDialogFlags_Modal;

    ImGuiFileDialog::Instance()->OpenDialog(
        "FileDialog",
        "Choose File",
        "((.))",
        config
    );
}

void MuhlePlayer::load_engine_dialog() {
    if (ImGuiFileDialog::Instance()->Display("FileDialog", 32, ImVec2(768, 432))) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            const std::string file_path {ImGuiFileDialog::Instance()->GetFilePathName()};

            if (!file_path.empty()) {
                load_engine(file_path);
            }
        }

        ImGuiFileDialog::Instance()->Close();
    }
}

void MuhlePlayer::reset_position(const std::optional<std::string>& position) {
    if (m_engine.active()) {
        try {
            m_engine.new_game();
            m_engine.synchronize();
        } catch (const engine::EngineError& e) {
            std::cerr << "Engine error: " << e.what() << '\n';
            return;
        }
    }

    try {
        m_board.reset(position ? std::make_optional(board::position_from_string(*position)) : std::nullopt);
    } catch (const board::BoardError& e) {
        std::cerr << "Invalid input: " << e.what() << '\n';
        return;
    }

    m_state = State::Ready;
    m_moves.clear();
    m_score.clear();
    m_pv.clear();
    m_clock.reset();

    if (m_board.get_setup_position().player == board::Player::Black) {
        m_clock.switch_turn();
    }
}

void MuhlePlayer::set_position() {
    char buffer[32] {};

    if (ImGui::InputText("string", buffer, sizeof(buffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
        reset_position(buffer);
    }
}

void MuhlePlayer::about() {
    ImGui::Text("%s", u8"Mühle Player - for testing and developing Mühle Intelligence");
}

void MuhlePlayer::notation() {

}

void MuhlePlayer::board() {
    m_board.update(m_state == State::HumanThinking);
    m_board.debug();
}

void MuhlePlayer::controls() {
    if (ImGui::Begin("Controls")) {
        ImGui::Text("Engine: %s", m_engine_name.c_str());
        ImGui::Separator();

        ImGui::Spacing();

        if (m_state != State::Ready) {
            ImGui::BeginDisabled();
            ImGui::Button("Start Game");
            ImGui::EndDisabled();
        } else {
            if (ImGui::Button("Start Game")) {
                if (m_white == PlayerComputer || m_black == PlayerComputer) {
                    if (m_engine.active()) {
                        m_state = State::Start;
                    }
                } else {
                    m_state = State::Start;
                }
            }
        }

        ImGui::SameLine();

        if (m_state == State::ComputerThinking) {
            ImGui::Text("Thinking...");
        } else {
            ImGui::Text("Passive");
        }

        ImGui::Spacing();

        ImGui::Text("White");
        ImGui::SameLine();

        if (m_state == State::Ready) {
            ImGui::RadioButton("Human##w", &m_white, PlayerHuman);
            ImGui::SameLine();
            ImGui::RadioButton("Computer##w", &m_white, PlayerComputer);
        } else {
            ImGui::RadioButton("Human##w", false);
            ImGui::SameLine();
            ImGui::RadioButton("Computer##w", false);
        }

        ImGui::Text("Black");
        ImGui::SameLine();

        if (m_state == State::Ready) {
            ImGui::RadioButton("Human##b", &m_black, PlayerHuman);
            ImGui::SameLine();
            ImGui::RadioButton("Computer##b", &m_black, PlayerComputer);
        } else {
            ImGui::RadioButton("Human##b", false);
            ImGui::SameLine();
            ImGui::RadioButton("Computer##b", false);
        }
    }

    ImGui::End();
}

void MuhlePlayer::game() {
    if (ImGui::Begin("Game")) {
        ImGui::Text("b.");
        ImGui::SameLine();
        std::apply(ImGui::Text, std::tuple_cat(std::forward_as_tuple("%u:%02u.%02u"), split_time(m_clock.get_black_time())));

        ImGui::Text("w.");
        ImGui::SameLine();
        std::apply(ImGui::Text, std::tuple_cat(std::forward_as_tuple("%u:%02u.%02u"), split_time(m_clock.get_white_time())));

        ImGui::Separator();

        ImGui::Text("%s", m_score.c_str());
        ImGui::TextWrapped("%s", m_pv.c_str());

        ImGui::Separator();

        if (ImGui::BeginChild("Moves")) {
            if (ImGui::BeginTable("Moves Table", 3)) {
                if (m_board.get_setup_position().player == board::Player::White) {
                    for (std::size_t i {0}; i < m_moves.size(); i++) {
                        if (i % 2 == 0) {
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::Text("%lu.", i / 2 + 1);
                            ImGui::TableSetColumnIndex(1);
                            ImGui::Text("%s", m_moves[i].c_str());
                        } else {
                            ImGui::TableSetColumnIndex(2);
                            ImGui::Text("%s", m_moves[i].c_str());
                        }
                    }
                } else {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("%d.", 1);
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("--/--");

                    for (std::size_t i {0}; i < m_moves.size(); i++) {
                        if (i % 2 == 0) {
                            ImGui::TableSetColumnIndex(2);
                            ImGui::Text("%s", m_moves[i].c_str());
                        } else {
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::Text("%lu.", i / 2 + 2);
                            ImGui::TableSetColumnIndex(1);
                            ImGui::Text("%s", m_moves[i].c_str());
                        }
                    }
                }

                ImGui::EndTable();
            }

            if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 6.0f) {
                ImGui::SetScrollHereY(1.0f);
            }
        }

        ImGui::EndChild();
    }

    ImGui::End();
}

int MuhlePlayer::get_board_player_type() const {
    switch (m_board.get_player()) {
        case board::Player::White:
            return m_white;
        case board::Player::Black:
            return m_black;
    }

    return {};
}

std::tuple<unsigned int, unsigned int, unsigned int> MuhlePlayer::split_time(unsigned int time_milliseconds) {
    const auto result1 {std::div(static_cast<long long>(time_milliseconds), (1000ll * 60ll))};
    const auto result2 {std::div(result1.rem, 1000ll)};

    const auto minutes {static_cast<unsigned int>(result1.quot)};
    const auto seconds {static_cast<unsigned int>(result2.quot)};
    const auto centiseconds {static_cast<unsigned int>(result2.rem / 10)};

    return std::make_tuple(minutes, seconds, centiseconds);
}
