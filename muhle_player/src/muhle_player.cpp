#include "muhle_player.hpp"

#include <iostream>
#include <filesystem>
#include <stdexcept>
#include <numeric>
#include <algorithm>
#include <cassert>

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
            assert_engine_game_over();
            m_state = State::Stop;
            return;
        }

        m_state = State::NextTurn;
    });
}

void MuhlePlayer::update() {
    main_menu_bar();
    board();
    controls();
    game();
    options();
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
                    m_state = State::ComputerStartThinking;
                    break;
            }

            break;
        }
        case State::HumanThinking:
            break;
        case State::ComputerStartThinking:
            assert(m_engine);

            try {
                m_engine->start_thinking(
                    board::position_to_string(m_board.get_setup_position()),
                    m_moves,
                    m_clock.get_white_time(),
                    m_clock.get_black_time(),
                    std::nullopt,
                    std::nullopt
                );
            } catch (const engine::EngineError& e) {
                engine_error(e);
                m_state = State::Stop;
                break;
            }

            m_state = State::ComputerThinking;

            break;
        case State::ComputerThinking: {
            assert(m_engine);

            std::optional<std::string> best_move;

            try {
                best_move = m_engine->done_thinking();
            } catch (const engine::EngineError& e) {
                engine_error(e);
                m_state = State::Stop;
                break;
            }

            if (best_move) {
                if (*best_move == "none") {
                    if (m_board.get_game_over() == board::GameOver::None) {
                        throw std::runtime_error("The engine calls game over, but the GUI doesn't agree");
                    }
                } else {
                    m_board.play_move(board::move_from_string(*best_move));
                }
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
    assert(!m_engine);

    m_engine = std::make_unique<engine::Engine>();
    m_engine->set_log_output(true);
    m_engine->set_info_callback([this](const engine::Engine::Info& info) {
        if (info.score) {
            switch (info.score->index()) {
                case 0:
                    m_score = "eval " + std::to_string(std::get<0>(*info.score).value);
                    break;
                case 1:
                    m_score = "win " + std::to_string(std::get<1>(*info.score).value);
                    break;
            }
        }

        if (info.pv) {
            if (info.pv->empty()) {
                m_pv.clear();
            } else {
                m_pv = std::accumulate(++info.pv->cbegin(), info.pv->cend(), *info.pv->cbegin(), [](std::string r, const std::string& move) {
                    return std::move(r) + " " + move;
                });
            }
        }
    });

    try {
        m_engine->initialize(file_path);
        m_engine->set_debug(true);
        m_engine->new_game();
        m_engine->synchronize();
    } catch (const engine::EngineError& e) {
        engine_error(e);
    }
}

void MuhlePlayer::unload_engine() {
    if (!m_engine) {
        return;
    }

    try {
        m_engine->uninitialize();
    } catch (const engine::EngineError& e) {
        std::cerr << "Engine error: " << e.what() << '\n';
    }

    m_engine.reset();
}

void MuhlePlayer::main_menu_bar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Player")) {
            if (ImGui::MenuItem("Load Engine")) {
                load_engine();
            }
            if (ImGui::MenuItem("Reset Position")) {
                reset_position(std::nullopt);
            }
            if (ImGui::BeginMenu("Set Position")) {
                set_position();

                ImGui::EndMenu();
            }
            if (ImGui::MenuItem("Quit")) {
                quit();
            }

            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Options")) {
            if (ImGui::MenuItem("Twelve Men's Morris", nullptr, &m_twelve_mens_morris, static_cast<bool>(m_engine))) {
                set_twelve_mens_morris();
            }

            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::BeginMenu("About")) {
                about();

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
                unload_engine();  // Unload any engine first
                load_engine(file_path);
                reset_position(std::nullopt);  // The position needs to be fresh
            }
        }

        ImGuiFileDialog::Instance()->Close();
    }
}

void MuhlePlayer::reset_position(const std::optional<std::string>& position) {
    if (m_engine) {
        try {
            m_engine->stop_thinking();  // Stop the engine first
            m_engine->new_game();
            m_engine->synchronize();
        } catch (const engine::EngineError& e) {
            engine_error(e);
        }
    }

    try {
        m_board.reset(position ? board::position_from_string(*position) : board::Position());
    } catch (const board::BoardError& e) {
        std::cerr << "Invalid input: " << e.what() << '\n';
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

void MuhlePlayer::board() {
    m_board.update(m_state == State::HumanThinking);
    m_board.debug();
}

void MuhlePlayer::controls() {
    if (ImGui::Begin("Controls")) {
        ImGui::Text("Engine: %s", m_engine ? m_engine->get_name().c_str() : "");
        ImGui::Separator();

        ImGui::Spacing();

        if (m_state != State::Ready) {
            ImGui::BeginDisabled();
            ImGui::Button("Start Game");
            ImGui::EndDisabled();
        } else {
            if (ImGui::Button("Start Game")) {
                if (m_white == PlayerComputer || m_black == PlayerComputer) {
                    if (m_engine) {
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
        std::apply(ImGui::Text, std::tuple_cat(std::forward_as_tuple("%u:%02u.%02u"), clock_::Clock::split_time(m_clock.get_black_time())));

        ImGui::Text("w.");
        ImGui::SameLine();
        std::apply(ImGui::Text, std::tuple_cat(std::forward_as_tuple("%u:%02u.%02u"), clock_::Clock::split_time(m_clock.get_white_time())));

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

void MuhlePlayer::options() {
    if (ImGui::Begin("Options")) {
        if (m_engine) {
            for (const auto& option : m_engine->get_options()) {
                ImGui::Text("%s", option.name.c_str());

                // TODO
            }
        }
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

void MuhlePlayer::assert_engine_game_over() {
    assert(m_engine);

    try {
        m_engine->start_thinking(
            board::position_to_string(m_board.get_setup_position()),
            m_moves,
            std::nullopt,
            std::nullopt,
            std::nullopt,
            100
        );

        while (true) {
            const auto best_move {m_engine->done_thinking()};

            if (!best_move) {
                continue;
            }

            if (*best_move != "none") {
                throw std::runtime_error("The GUI calls game over, but the engine doesn't agree");
            }

            break;
        }
    } catch (const engine::EngineError& e) {
        engine_error(e);
        m_state = State::Stop;
    }
}

void MuhlePlayer::engine_error(const engine::EngineError& e) {
    std::cerr << "Engine error: " << e.what() << '\n';
    m_engine.reset();
}

void MuhlePlayer::set_twelve_mens_morris() {
    assert(m_engine);

    const auto iter {std::find_if(m_engine->get_options().cbegin(), m_engine->get_options().cend(), [](const auto& option) {
        return option.name == "TwelveMensMorris";
    })};

    if (iter == m_engine->get_options().cend()) {
        throw std::runtime_error("Engine doesn't support twelve men's morris");
    }

    try {
        m_engine->set_option("TwelveMensMorris", m_twelve_mens_morris ? "true" : "false");
    } catch (const engine::EngineError& e) {
        engine_error(e);
        return;
    }

    m_board.twelve_mens_morris(m_twelve_mens_morris);
}
