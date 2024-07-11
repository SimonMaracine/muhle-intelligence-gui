#include "muhle_player.hpp"

#include <iostream>
#include <cstring>
#include <filesystem>

#include <gui_base/gui_base.hpp>
#include <ImGuiFileDialog.h>

void MuhlePlayer::start() {
    ImGuiIO& io {ImGui::GetIO()};
    io.ConfigWindowsMoveFromTitleBarOnly = true;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    muhle_board = board::MuhleBoard([this](const board::Move& move, board::Player turn) {
        if (!muhle_process.active()) {
            return;
        }

        int player {};

        switch (turn) {
            case board::Player::White:
                player = white;
                break;
            case board::Player::Black:
                player = black;
                break;
        }

        switch (player) {
            case PlayerHuman:
                try {
                    muhle_process.write("move " + board::move_to_string(move) + '\n');
                } catch (const subprocess::Error& e) {
                    terminate_process(e.what());
                }

                state = State::NextTurn;

                break;
            case PlayerComputer:
                break;
        }
    });
}

void MuhlePlayer::update() {
    main_menu_bar();
    board();
    controls();
    load_engine_dialog();

    if (!muhle_process.active()) {
        return;
    }

    if (muhle_board.get_game_over() != board::GameOver::None) {
        return;
    }

    switch (state) {
        case State::NextTurn: {
            int player {};

            switch (muhle_board.get_turn()) {
                case board::Player::White:
                    player = white;
                    break;
                case board::Player::Black:
                    player = black;
                    break;
            }

            switch (player) {
                case PlayerHuman:
                    state = State::HumanThinking;
                    break;
                case PlayerComputer:
                    state = State::ComputerBegin;
                    break;
            }

            break;
        }
        case State::HumanThinking:
            break;
        case State::ComputerBegin:
            try {
                muhle_process.write("go\n");
            } catch (const subprocess::Error& e) {
                terminate_process(e.what());
                break;
            }

            state = State::ComputerThinking;

            break;
        case State::ComputerThinking: {
            std::optional<std::string> data;

            try {
                data = muhle_process.read();
            } catch (const subprocess::Error& e) {
                terminate_process(e.what());
                break;
            }

            if (!data) {
                break;
            }

            const auto tokens {parse_message(data->substr(0, data->size() - 1))};

            if (tokens.at(0) == "bestmove") {
                if (tokens.at(1) == "none") {
                    break;
                }

                const auto move {board::move_from_string(tokens.at(1))};

                switch (move.type) {
                    case board::MoveType::Place:
                        muhle_board.place(move.place.place_index);
                        break;
                    case board::MoveType::PlaceTake:
                        muhle_board.place_take(move.place_take.place_index, move.place_take.take_index);
                        break;
                    case board::MoveType::Move:
                        muhle_board.move(move.move.source_index, move.move.destination_index);
                        break;
                    case board::MoveType::MoveTake:
                        muhle_board.move_take(move.move_take.source_index, move.move_take.destination_index, move.move_take.take_index);
                        break;
                }

                state = State::NextTurn;
            }

            break;
        }
    }
}

void MuhlePlayer::stop() {
    unload_engine();
}

void MuhlePlayer::load_engine(const std::string& file_path) {
    try {
        muhle_process = subprocess::Subprocess(file_path);
    } catch (const subprocess::Error& e) {
        std::cerr << "Could not start engine: " << e.what() << '\n';
        return;
    }

    try {
        muhle_process.write("init\n");
    } catch (const subprocess::Error& e) {
        terminate_process(e.what());
        return;
    }

    engine_filename = std::filesystem::path(file_path).filename();
}

void MuhlePlayer::unload_engine() {
    if (!muhle_process.active()) {
        return;
    }

    try {
        muhle_process.write("quit\n");
    } catch (const subprocess::Error& e) {
        terminate_process(e.what());
        return;
    }

    try {
        muhle_process.wait();
    } catch (const subprocess::Error& e) {
        std::cerr << e.what() << '\n';
    }
}

void MuhlePlayer::main_menu_bar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Player")) {
            if (ImGui::MenuItem("Load Engine")) {
                load_engine();
            }
            if (ImGui::MenuItem("Reset Board", nullptr, nullptr, state != State::ComputerThinking)) {
                reset();
            }
            if (ImGui::BeginMenu("Import Position")) {
                import_position();

                ImGui::EndMenu();
            }
            if (ImGui::MenuItem("Export Position")) {

            }
            if (ImGui::MenuItem("Exit")) {
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

void MuhlePlayer::import_position() {
    char buffer[32] {};

    if (ImGui::InputText("string", buffer, sizeof(buffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
        // TODO
    }
}

void MuhlePlayer::reset() {
    if (muhle_process.active()) {
        try {
            muhle_process.write("newgame\n");
        } catch (const subprocess::Error& e) {
            terminate_process(e.what());
        }
    }

    muhle_board.reset();
    state = State::NextTurn;
}

void MuhlePlayer::about() {
    ImGui::Text("%s", u8"Mühle Player - for testing and developing Mühle Intelligence");
}

void MuhlePlayer::notation() {
    ImGui::Text("A player's turn is described as:");
    ImGui::TextColored(ImVec4(0.8f, 0.3f, 0.4f, 1.0f), "player move_type node[-node] [move_type node]");
    ImGui::Text("1. Player: W for white, B for black");
    ImGui::Text("2. Main move: P for place, M for move, first and second phase respectively");
    ImGui::Text("3. Node: one node - place location, two nodes - move source and destination");
    ImGui::Text("4. Take move: T preceeded by a node - which piece was taken");
}

void MuhlePlayer::board() {
    muhle_board.update(state == State::HumanThinking);
    muhle_board.debug();
}

void MuhlePlayer::controls() {
    if (ImGui::Begin("Controls")) {
        ImGui::Text("Engine: %s", engine_filename.c_str());
        ImGui::Separator();

        ImGui::Spacing();

        if (ImGui::Button("Stop Engine")) {

        }

        if (state == State::ComputerThinking) {
            ImGui::SameLine();
            ImGui::Text("Thinking...");
        }

        ImGui::Spacing();

        ImGui::Text("White");
        ImGui::SameLine();

        // FIXME don't change state when user made half move

        if (state != State::ComputerThinking) {
            if (ImGui::RadioButton("Human##w", &white, 0)) {
                state = State::NextTurn;
            }

            ImGui::SameLine();

            if (ImGui::RadioButton("Computer##w", &white, 1)) {
                state = State::NextTurn;
            }
        } else {
            ImGui::RadioButton("Human##w", false);
            ImGui::SameLine();
            ImGui::RadioButton("Computer##w", false);
        }

        ImGui::Text("Black");
        ImGui::SameLine();

        if (state != State::ComputerThinking) {
            if (ImGui::RadioButton("Human##b", &black, 0)) {
                state = State::NextTurn;
            }

            ImGui::SameLine();

            if (ImGui::RadioButton("Computer##b", &black, 1)) {
                state = State::NextTurn;
            }
        } else {
            ImGui::RadioButton("Human##b", false);
            ImGui::SameLine();
            ImGui::RadioButton("Computer##b", false);
        }
    }

    ImGui::End();
}

void MuhlePlayer::terminate_process(const char* message) {
    std::cerr << message << '\n';

    try {
        muhle_process.terminate();
    } catch (const subprocess::Error& e) {
        std::cerr << e.what() << '\n';
    }
}

std::vector<std::string> MuhlePlayer::parse_message(const std::string& message) {
    std::vector<std::string> tokens;

    std::string mutable_buffer {message};

    char* token {std::strtok(mutable_buffer.data(), " \t")};

    while (token != nullptr) {
        tokens.emplace_back(token);

        token = std::strtok(nullptr, " \t");
    }

    return tokens;
}
