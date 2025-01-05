#include "muhle_player.hpp"

#include <iostream>
#include <filesystem>
#include <chrono>

#include <gui_base/gui_base.hpp>
#include <ImGuiFileDialog.h>

void MuhlePlayer::start() {
    ImGuiIO& io {ImGui::GetIO()};
    io.ConfigWindowsMoveFromTitleBarOnly = true;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    m_board = board::Board([this](const board::Move& move) {
        m_moves.push_back(board::move_to_string(move));

        if (m_board.get_game_over() != board::GameOver::None) {
            m_state = State::NotStarted;
            return;
        }

        switch (get_player_type(m_board.get_player())) {
            case PlayerHuman:
                m_state = State::NextTurn;
                break;
            case PlayerComputer:
                m_state = State::NextTurn;
                break;
        }

        // if (!m_muhle_process.active()) {
        //     return;
        // }

        // int player {};

        // switch (m_muhle_board.get_player()) {
        //     case board::Player::White:
        //         player = m_black;
        //         break;
        //     case board::Player::Black:
        //         player = m_white;
        //         break;
        // }

        // switch (player) {
        //     case PlayerHuman:
        //         try {
        //             m_muhle_process.write("move " + board::move_to_string(move) + '\n');
        //         } catch (const subprocess::Error& e) {
        //             terminate_process(e.what());
        //         }

        //         m_state = State::NextTurn;

        //         break;
        //     case PlayerComputer:
        //         break;
        // }
    });
}

void MuhlePlayer::update() {
    main_menu_bar();
    board();
    controls();
    load_engine_dialog();

    if (!m_engine.active()) {
        return;
    }

    if (m_board.get_game_over() != board::GameOver::None) {
        return;
    }

    switch (m_state) {
        case State::NotStarted:
            break;
        case State::NextTurn: {
            switch (get_player_type(m_board.get_player())) {
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
            // try {
            //     m_muhle_process.write("go\n");
            // } catch (const subprocess::Error& e) {
            //     terminate_process(e.what());
            //     break;
            // }
            try {
                m_engine.start_thinking(
                    std::nullopt,
                    m_moves,
                    std::nullopt,
                    std::nullopt,
                    4,
                    std::nullopt
                );
            } catch (const subprocess::Error& e) {
                std::cerr << "Engine error: " << e.what() << '\n';
                m_state = State::NotStarted;
                break;
            }

            m_state = State::ComputerThinking;

            break;
        case State::ComputerThinking: {
            std::optional<std::string> best_move;

            try {
                best_move = m_engine.done_thinking();
            } catch (const subprocess::Error& e) {
                std::cerr << "Engine error: " << e.what() << '\n';
                m_state = State::NotStarted;
                break;
            }

            if (best_move) {
                if (*best_move == "none") {
                    m_state = State::NotStarted;
                    break;
                }

                m_board.play_move(board::move_from_string(*best_move));
            }

            // std::optional<std::string> message;

            // try {
            //     message = m_muhle_process.read();
            // } catch (const subprocess::Error& e) {
            //     terminate_process(e.what());
            //     break;
            // }

            // if (!message) {
            //     break;
            // }

            // const auto tokens {parse_message(message->substr(0, message->size() - 1))};

            // if (tokens.at(0) == "bestmove") {
            //     if (tokens.at(1) == "none") {
            //         break;
            //     }

            //     const auto move {board::move_from_string(tokens.at(1))};

            //     switch (move.type) {
            //         case board::MoveType::Place:
            //             m_muhle_board.place_piece(move.place.place_index);
            //             break;
            //         case board::MoveType::PlaceTake:
            //             m_muhle_board.place_take_piece(move.place_take.place_index, move.place_take.take_index);
            //             break;
            //         case board::MoveType::Move:
            //             m_muhle_board.move_piece(move.move.source_index, move.move.destination_index);
            //             break;
            //         case board::MoveType::MoveTake:
            //             m_muhle_board.move_take_piece(move.move_take.source_index, move.move_take.destination_index, move.move_take.take_index);
            //             break;
            //     }

            //     m_state = State::NextTurn;
            // }

            break;
        }
    }
}

void MuhlePlayer::stop() {
    unload_engine();
}

void MuhlePlayer::load_engine(const std::string& file_path) {
    try {
        m_engine.initialize(file_path);
    } catch (const subprocess::Error& e) {
        std::cerr << "Could not start engine: " << e.what() << '\n';
        return;
    }

    try {
        m_engine.set_debug(true);
    } catch (const subprocess::Error& e) {
        std::cerr << "Engine error: " << e.what() << '\n';
        return;
    }

    try {
        m_engine.new_game();
        m_engine.synchronize();
    } catch (const subprocess::Error& e) {
        std::cerr << "Engine error: " << e.what() << '\n';
    }

    m_engine_name = m_engine.get_name();
    // try {
    //     m_muhle_process = subprocess::Subprocess(file_path);
    // } catch (const subprocess::Error& e) {
    //     std::cerr << "Could not start engine: " << e.what() << '\n';
    //     return;
    // }

    // {
    //     const auto begin {std::chrono::steady_clock::now()};

    //     while (true) {
    //         using namespace std::chrono_literals;

    //         const auto now {std::chrono::steady_clock::now()};

    //         if (now - begin > 5s) {
    //             terminate_process("Engine did not respond in a timely manner, aborting");
    //             return;
    //         }

    //         std::optional<std::string> message;

    //         try {
    //             message = m_muhle_process.read();
    //         } catch (const subprocess::Error& e) {
    //             terminate_process(e.what());
    //             return;
    //         }

    //         if (!message) {
    //             continue;
    //         }

    //         if (*message == "ready\n") {
    //             break;
    //         }

    //         terminate_process("Engine did not respond with the ready message, aborting");
    //         return;
    //     }
    // }

    // try {
    //     m_muhle_process.write("init\n");
    // } catch (const subprocess::Error& e) {
    //     terminate_process(e.what());
    //     return;
    // }

    // m_engine_name = std::filesystem::path(file_path).filename();
}

void MuhlePlayer::unload_engine() {
    if (!m_engine.active()) {
        return;
    }

    try {
        m_engine.uninitialize();
    } catch (const subprocess::Error& e) {
        std::cerr << "Could not stop engine: " << e.what() << '\n';
    }

    // if (!m_muhle_process.active()) {
    //     return;
    // }

    // try {
    //     m_muhle_process.write("quit\n");
    // } catch (const subprocess::Error& e) {
    //     terminate_process(e.what());
    //     return;
    // }

    // try {
    //     m_muhle_process.wait();
    // } catch (const subprocess::Error& e) {
    //     std::cerr << e.what() << '\n';
    // }
}

void MuhlePlayer::main_menu_bar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Player")) {
            if (ImGui::MenuItem("Load Engine")) {
                load_engine();
            }
            if (ImGui::MenuItem("Reset Position", nullptr, nullptr, m_state != State::ComputerThinking)) {
                reset_position();
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

void MuhlePlayer::reset_position() {
    if (m_engine.active()) {
        try {
            m_engine.new_game();
            m_engine.synchronize();
        } catch (const subprocess::Error& e) {
            std::cerr << "Engine error: " << e.what() << '\n';
        }
    }

    m_board.reset(std::nullopt);
    m_state = State::NotStarted;

    // if (m_muhle_process.active()) {
    //     try {
    //         m_muhle_process.write("newgame\n");
    //     } catch (const subprocess::Error& e) {
    //         terminate_process(e.what());
    //     }
    // }

    // m_muhle_board.reset();
    // m_state = State::NextTurn;
}

void MuhlePlayer::set_position() {
    char buffer[32] {};

    if (ImGui::InputText("string", buffer, sizeof(buffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
        // TODO
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

        if (ImGui::Button("Start Game")) {
            if (m_engine.active()) {
                m_state = State::NextTurn;
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

        if (m_state == State::NotStarted) {
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

        if (m_state == State::NotStarted) {
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

int MuhlePlayer::get_player_type(board::Player player) const {
    switch (m_board.get_player()) {
        case board::Player::White:
            return m_white;
        case board::Player::Black:
            return m_black;
    }

    return {};
}
