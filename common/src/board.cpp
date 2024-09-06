#include "common/board.hpp"

#include <utility>
#include <algorithm>
#include <cstddef>
#include <cmath>
#include <cassert>

namespace board {
    static constexpr float NODE_RADIUS {2.2f};

    static const int NODE_POSITIONS[24][2] {
        { 2, 2 },
        { 5, 2 },
        { 8, 2 },
        { 3, 3 },
        { 5, 3 },
        { 7, 3 },
        { 4, 4 },
        { 5, 4 },
        { 6, 4 },
        { 2, 5 },
        { 3, 5 },
        { 4, 5 },
        { 6, 5 },
        { 7, 5 },
        { 8, 5 },
        { 4, 6 },
        { 5, 6 },
        { 6, 6 },
        { 3, 7 },
        { 5, 7 },
        { 7, 7 },
        { 2, 8 },
        { 5, 8 },
        { 8, 8 }
    };

    static int index_from_string(std::string_view string) {
        if (string == "a7") return 0;
        else if (string == "d7") return 1;
        else if (string == "g7") return 2;
        else if (string == "b6") return 3;
        else if (string == "d6") return 4;
        else if (string == "f6") return 5;
        else if (string == "c5") return 6;
        else if (string == "d5") return 7;
        else if (string == "e5") return 8;
        else if (string == "a4") return 9;
        else if (string == "b4") return 10;
        else if (string == "c4") return 11;
        else if (string == "e4") return 12;
        else if (string == "f4") return 13;
        else if (string == "g4") return 14;
        else if (string == "c3") return 15;
        else if (string == "d3") return 16;
        else if (string == "e3") return 17;
        else if (string == "b2") return 18;
        else if (string == "d2") return 19;
        else if (string == "f2") return 20;
        else if (string == "a1") return 21;
        else if (string == "d1") return 22;
        else if (string == "g1") return 23;

        assert(false);
        return {};
    }

    static std::string_view string_from_index(int index) {
        switch (index) {
            case 0: return "a7";
            case 1: return "d7";
            case 2: return "g7";
            case 3: return "b6";
            case 4: return "d6";
            case 5: return "f6";
            case 6: return "c5";
            case 7: return "d5";
            case 8: return "e5";
            case 9: return "a4";
            case 10: return "b4";
            case 11: return "c4";
            case 12: return "e4";
            case 13: return "f4";
            case 14: return "g4";
            case 15: return "c3";
            case 16: return "d3";
            case 17: return "e3";
            case 18: return "b2";
            case 19: return "d2";
            case 20: return "f2";
            case 21: return "a1";
            case 22: return "d1";
            case 23: return "g1";
        }

        assert(false);
        return {};
    }

    void PieceObj::update() {
        if (!m_moving) {
            return;
        }

        {
            const auto difference {ImVec2(m_target.x - m_position.x, m_target.y - m_position.y)};
            const float difference_mag {std::sqrt(difference.x * difference.x + difference.y * difference.y)};
            const auto velocity {ImVec2(difference.x / difference_mag * 10.0f, difference.y / difference_mag * 10.0f)};

            m_position = ImVec2(m_position.x + velocity.x, m_position.y + velocity.y);
        }

        const auto difference {ImVec2(m_target.x - m_position.x, m_target.y - m_position.y)};
        const float difference_mag {std::sqrt(difference.x * difference.x + difference.y * difference.y)};

        if (difference_mag < 10.0f + 0.1f) {
            m_position = m_target;
            m_moving = false;
        }
    }

    void PieceObj::render(ImDrawList* draw_list, float m_board_unit) {
        switch (m_type) {
            case Player::White:
                draw_list->AddCircleFilled(
                    m_position,
                    m_board_unit / NODE_RADIUS,
                    ImColor(235, 235, 235, 255)
                );
                break;
            case Player::Black:
                draw_list->AddCircleFilled(
                    m_position,
                    m_board_unit / NODE_RADIUS,
                    ImColor(15, 15, 15, 255)
                );
                break;
        }
    }

    void PieceObj::move(ImVec2 target) {
        m_target = target;
        m_moving = true;
    }

    MuhleBoard::MuhleBoard(std::function<void(const Move&)>&& move_callback)
        : m_move_callback(std::move(move_callback)) {
        m_legal_moves = generate_moves();

        initialize_pieces();
    }

    void MuhleBoard::update(bool user_input) {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(350.0f, 350.0f));

        if (ImGui::Begin("Board")) {
            const ImVec2 canvas_p0 {ImGui::GetCursorScreenPos()};
            ImVec2 canvas_size {ImGui::GetContentRegionAvail()};

            canvas_size.x = std::max(canvas_size.x, 350.0f);
            canvas_size.y = std::max(canvas_size.y, 350.0f);

            const ImVec2 canvas_p1 {ImVec2(canvas_p0.x + canvas_size.x, canvas_p0.y + canvas_size.y)};

            ImDrawList* draw_list {ImGui::GetWindowDrawList()};

            const float unit {canvas_size.x < canvas_size.y ? (canvas_p1.x - canvas_p0.x) / 10.0f : (canvas_p1.y - canvas_p0.y) / 10.0f};
            const ImVec2 offset {ImVec2(canvas_p0.x, canvas_p0.y)};

            static constexpr ImColor COLOR {ImColor(200, 200, 200)};
            static constexpr float THICKNESS {2.0f};

            m_board_unit = unit;
            m_board_offset = offset;

            draw_list->AddRectFilled(canvas_p0, canvas_p1, ImColor(45, 45, 45));

            draw_list->AddRect(ImVec2(2.0f * unit + offset.x, 8.0f * unit + offset.y), ImVec2(8.0f * unit + offset.x, 2.0f * unit + offset.y), COLOR, 0.0f, 0, THICKNESS);
            draw_list->AddRect(ImVec2(3.0f * unit + offset.x, 7.0f * unit + offset.y), ImVec2(7.0f * unit + offset.x, 3.0f * unit + offset.y), COLOR, 0.0f, 0, THICKNESS);
            draw_list->AddRect(ImVec2(4.0f * unit + offset.x, 6.0f * unit + offset.y), ImVec2(6.0f * unit + offset.x, 4.0f * unit + offset.y), COLOR, 0.0f, 0, THICKNESS);

            draw_list->AddLine(ImVec2(5.0f * unit + offset.x, 2.0f * unit + offset.y), ImVec2(5.0f * unit + offset.x, 4.0f * unit + offset.y), COLOR, THICKNESS);
            draw_list->AddLine(ImVec2(6.0f * unit + offset.x, 5.0f * unit + offset.y), ImVec2(8.0f * unit + offset.x, 5.0f * unit + offset.y), COLOR, THICKNESS);
            draw_list->AddLine(ImVec2(5.0f * unit + offset.x, 6.0f * unit + offset.y), ImVec2(5.0f * unit + offset.x, 8.0f * unit + offset.y), COLOR, THICKNESS);
            draw_list->AddLine(ImVec2(2.0f * unit + offset.x, 5.0f * unit + offset.y), ImVec2(4.0f * unit + offset.x, 5.0f * unit + offset.y), COLOR, THICKNESS);

            draw_list->AddText(ImVec2(2.0f * unit + offset.x, 1.0f * unit + offset.y), COLOR, "A");
            draw_list->AddText(ImVec2(3.0f * unit + offset.x, 1.0f * unit + offset.y), COLOR, "B");
            draw_list->AddText(ImVec2(4.0f * unit + offset.x, 1.0f * unit + offset.y), COLOR, "C");
            draw_list->AddText(ImVec2(5.0f * unit + offset.x, 1.0f * unit + offset.y), COLOR, "D");
            draw_list->AddText(ImVec2(6.0f * unit + offset.x, 1.0f * unit + offset.y), COLOR, "E");
            draw_list->AddText(ImVec2(7.0f * unit + offset.x, 1.0f * unit + offset.y), COLOR, "F");
            draw_list->AddText(ImVec2(8.0f * unit + offset.x, 1.0f * unit + offset.y), COLOR, "G");

            draw_list->AddText(ImVec2(2.0f * unit + offset.x, 9.0f * unit + offset.y), COLOR, "A");
            draw_list->AddText(ImVec2(3.0f * unit + offset.x, 9.0f * unit + offset.y), COLOR, "B");
            draw_list->AddText(ImVec2(4.0f * unit + offset.x, 9.0f * unit + offset.y), COLOR, "C");
            draw_list->AddText(ImVec2(5.0f * unit + offset.x, 9.0f * unit + offset.y), COLOR, "D");
            draw_list->AddText(ImVec2(6.0f * unit + offset.x, 9.0f * unit + offset.y), COLOR, "E");
            draw_list->AddText(ImVec2(7.0f * unit + offset.x, 9.0f * unit + offset.y), COLOR, "F");
            draw_list->AddText(ImVec2(8.0f * unit + offset.x, 9.0f * unit + offset.y), COLOR, "G");

            draw_list->AddText(ImVec2(9.0f * unit + offset.x, 2.0f * unit + offset.y), COLOR, "7");
            draw_list->AddText(ImVec2(9.0f * unit + offset.x, 3.0f * unit + offset.y), COLOR, "6");
            draw_list->AddText(ImVec2(9.0f * unit + offset.x, 4.0f * unit + offset.y), COLOR, "5");
            draw_list->AddText(ImVec2(9.0f * unit + offset.x, 5.0f * unit + offset.y), COLOR, "4");
            draw_list->AddText(ImVec2(9.0f * unit + offset.x, 6.0f * unit + offset.y), COLOR, "3");
            draw_list->AddText(ImVec2(9.0f * unit + offset.x, 7.0f * unit + offset.y), COLOR, "2");
            draw_list->AddText(ImVec2(9.0f * unit + offset.x, 8.0f * unit + offset.y), COLOR, "1");

            draw_list->AddText(ImVec2(1.0f * unit + offset.x, 2.0f * unit + offset.y), COLOR, "7");
            draw_list->AddText(ImVec2(1.0f * unit + offset.x, 3.0f * unit + offset.y), COLOR, "6");
            draw_list->AddText(ImVec2(1.0f * unit + offset.x, 4.0f * unit + offset.y), COLOR, "5");
            draw_list->AddText(ImVec2(1.0f * unit + offset.x, 5.0f * unit + offset.y), COLOR, "4");
            draw_list->AddText(ImVec2(1.0f * unit + offset.x, 6.0f * unit + offset.y), COLOR, "3");
            draw_list->AddText(ImVec2(1.0f * unit + offset.x, 7.0f * unit + offset.y), COLOR, "2");
            draw_list->AddText(ImVec2(1.0f * unit + offset.x, 8.0f * unit + offset.y), COLOR, "1");

            for (PieceObj& piece : m_pieces) {
                piece.update();
                piece.render(draw_list, m_board_unit);
            }

            const float width {m_board_unit < 55.0f ? 2.0f : 3.0f};

            if (m_selected_index != -1 && m_take_action_index == -1) {
                const ImVec2 position {
                    static_cast<float>(NODE_POSITIONS[m_selected_index][0]) * m_board_unit + m_board_offset.x,
                    static_cast<float>(NODE_POSITIONS[m_selected_index][1]) * m_board_unit + m_board_offset.y
                };

                draw_list->AddCircle(position, m_board_unit / NODE_RADIUS + 1.0f, ImColor(240, 30, 30, 255), 0, width);
            }

            if (user_input) {
                update_user_input();
            }
        }

        ImGui::End();

        ImGui::PopStyleVar(2);
    }

    void MuhleBoard::reset(std::string_view position_string) {
        m_board = {};
        m_turn = Player::White;
        m_game_over = GameOver::None;
        m_plies = 0;
        m_plies_without_advancement = 0;
        m_positions.clear();

        m_selected_index = -1;
        m_take_action_index = -1;

        m_legal_moves = generate_moves();

        initialize_pieces();
    }

    void MuhleBoard::debug() const {
        if (ImGui::Begin("Board Internal")) {
            const char* game_over_string {};

            switch (m_game_over) {
                case GameOver::None:
                    game_over_string = "None";
                    break;
                case GameOver::WinnerWhite:
                    game_over_string = "WinnerWhite";
                    break;
                case GameOver::WinnerBlack:
                    game_over_string = "WinnerBlack";
                    break;
                case GameOver::TieBetweenBothPlayers:
                    game_over_string = "TieBetweenBothPlayers";
                    break;
            }

            ImGui::Text("turn: %s", m_turn == Player::White ? "white" : "black");
            ImGui::Text("game_over: %s", game_over_string);
            ImGui::Text("plies: %u", m_plies);
            ImGui::Text("plies_without_advancement: %u", m_plies_without_advancement);
            ImGui::Text("positions: %lu", m_positions.size());
            ImGui::Text("selected_index: %d", m_selected_index);
            ImGui::Text("take_action_index: %d", m_take_action_index);
            ImGui::Text("legal_moves: %lu", m_legal_moves.size());
        }

        ImGui::End();
    }

    void MuhleBoard::place_piece(int place_index) {
        const auto iter {std::find_if(m_legal_moves.begin(), m_legal_moves.end(), [=](const Move& move) {
            return move.type == MoveType::Place && move.place.place_index == place_index;
        })};

        assert(iter != m_legal_moves.end());

        place(place_index);

        m_pieces[new_piece_to_place(opponent(m_turn))].move(node_position(place_index));
        m_pieces[new_piece_to_place(opponent(m_turn))].node_index = place_index;
    }

    void MuhleBoard::place_take_piece(int place_index, int take_index) {
        const auto iter {std::find_if(m_legal_moves.begin(), m_legal_moves.end(), [=](const Move& move) {
            return (
                move.type == MoveType::PlaceTake &&
                move.place_take.place_index == place_index &&
                move.place_take.take_index == take_index
            );
        })};

        assert(iter != m_legal_moves.end());

        place_take(place_index, take_index);

        m_pieces[new_piece_to_place(opponent(m_turn))].move(node_position(place_index));
        m_pieces[new_piece_to_place(opponent(m_turn))].node_index = place_index;
        m_pieces[piece_on_node(take_index)].move(ImVec2());
        m_pieces[piece_on_node(take_index)].node_index = -1;
    }

    void MuhleBoard::move_piece(int source_index, int destination_index) {
        const auto iter {std::find_if(m_legal_moves.begin(), m_legal_moves.end(), [=](const Move& move) {
            return (
                move.type == MoveType::Move &&
                move.move.source_index == source_index &&
                move.move.destination_index == destination_index
            );
        })};

        assert(iter != m_legal_moves.end());

        move(source_index, destination_index);

        m_pieces[piece_on_node(source_index)].move(node_position(destination_index));
        m_pieces[piece_on_node(source_index)].node_index = destination_index;
    }

    void MuhleBoard::move_take_piece(int source_index, int destination_index, int take_index) {
        const auto iter {std::find_if(m_legal_moves.begin(), m_legal_moves.end(), [=](const Move& move) {
            return (
                move.type == MoveType::MoveTake &&
                move.move_take.source_index == source_index &&
                move.move_take.destination_index == destination_index &&
                move.move_take.take_index == take_index
            );
        })};

        assert(iter != m_legal_moves.end());  // FIXME this failed once

        move_take(source_index, destination_index, take_index);

        m_pieces[piece_on_node(source_index)].move(node_position(destination_index));
        m_pieces[piece_on_node(source_index)].node_index = destination_index;
        m_pieces[piece_on_node(take_index)].move(ImVec2());
        m_pieces[piece_on_node(take_index)].node_index = -1;
    }

    void MuhleBoard::update_user_input() {
        if (!ImGui::IsWindowFocused()) {
            return;
        }

        if (m_game_over != GameOver::None) {
            return;
        }

        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            const ImVec2 position {ImGui::GetMousePos()};
            const int index {get_index(position)};

            if (index == -1) {
                return;
            }

            if (m_plies >= 18) {
                if (m_take_action_index != -1) {
                    try_move_take(m_selected_index, m_take_action_index, index);
                } else {
                    try_move(m_selected_index, index);
                    select(index);
                }
            } else {
                if (m_take_action_index != -1) {
                    try_place_take(m_take_action_index, index);
                } else {
                    try_place(index);
                }
            }
        }
    }

    void MuhleBoard::select(int index) {
        if (m_selected_index == -1) {
            if (m_board[index] == static_cast<Piece>(m_turn)) {
                m_selected_index = index;
            }
        } else {
            if (index == m_selected_index) {
                if (m_take_action_index == -1) {
                    m_selected_index = -1;
                }
            } else if (m_board[index] == static_cast<Piece>(m_turn)) {
                if (m_take_action_index == -1) {
                    m_selected_index = index;
                }
            }
        }
    }

    void MuhleBoard::user_place(int place_index) {
        place(place_index);

        m_pieces[new_piece_to_place(opponent(m_turn))].move(node_position(place_index));
        m_pieces[new_piece_to_place(opponent(m_turn))].node_index = place_index;
    }

    void MuhleBoard::user_place_take_just_place(int place_index) {
        m_take_action_index = place_index;

        m_pieces[new_piece_to_place(opponent(m_turn))].move(node_position(place_index));
        m_pieces[new_piece_to_place(opponent(m_turn))].node_index = place_index;
    }

    void MuhleBoard::user_place_take(int place_index, int take_index) {
        place_take(place_index, take_index);

        m_pieces[piece_on_node(take_index)].move(ImVec2());
        m_pieces[piece_on_node(take_index)].node_index = -1;
    }

    void MuhleBoard::user_move(int source_index, int destination_index) {
        move(source_index, destination_index);

        m_pieces[piece_on_node(source_index)].move(node_position(destination_index));
        m_pieces[piece_on_node(source_index)].node_index = destination_index;
    }

    void MuhleBoard::user_move_take_just_move(int source_index, int destination_index) {
        m_take_action_index = destination_index;

        m_pieces[piece_on_node(source_index)].move(node_position(destination_index));
        m_pieces[piece_on_node(source_index)].node_index = destination_index;
    }

    void MuhleBoard::user_move_take(int source_index, int destination_index, int take_index) {
        move_take(source_index, destination_index, take_index);

        m_pieces[piece_on_node(take_index)].move(ImVec2());
        m_pieces[piece_on_node(take_index)].node_index = -1;
    }

    void MuhleBoard::try_place(int place_index) {
        auto iter {std::find_if(m_legal_moves.begin(), m_legal_moves.end(), [=](const Move& move) {
            return move.type == MoveType::Place && move.place.place_index == place_index;
        })};

        if (iter != m_legal_moves.end()) {
            user_place(place_index);
            return;
        }

        iter = std::find_if(m_legal_moves.begin(), m_legal_moves.end(), [=](const Move& move) {
            return move.type == MoveType::PlaceTake && move.place_take.place_index == place_index;
        });

        if (iter != m_legal_moves.end()) {
            user_place_take_just_place(place_index);
        }
    }

    void MuhleBoard::try_place_take(int place_index, int take_index) {
        const auto iter {std::find_if(m_legal_moves.begin(), m_legal_moves.end(), [=](const Move& move) {
            return (
                move.type == MoveType::PlaceTake &&
                move.place_take.place_index == place_index &&
                move.place_take.take_index == take_index
            );
        })};

        if (iter != m_legal_moves.end()) {
            user_place_take(place_index, take_index);
        }
    }

    void MuhleBoard::try_move(int source_index, int destination_index) {
        auto iter {std::find_if(m_legal_moves.begin(), m_legal_moves.end(), [=](const Move& move) {
            return (
                move.type == MoveType::Move &&
                move.move.source_index == source_index &&
                move.move.destination_index == destination_index
            );
        })};

        if (iter != m_legal_moves.end()) {
            user_move(source_index, destination_index);
            return;
        }

        iter = std::find_if(m_legal_moves.begin(), m_legal_moves.end(), [=](const Move& move) {
            return (
                move.type == MoveType::MoveTake &&
                move.move_take.source_index == source_index &&
                move.move_take.destination_index == destination_index
            );
        });

        if (iter != m_legal_moves.end()) {
            user_move_take_just_move(source_index, destination_index);
        }
    }

    void MuhleBoard::try_move_take(int source_index, int destination_index, int take_index) {
        const auto iter {std::find_if(m_legal_moves.begin(), m_legal_moves.end(), [=](const Move& move) {
            return (
                move.type == MoveType::MoveTake &&
                move.move_take.source_index == source_index &&
                move.move_take.destination_index == destination_index &&
                move.move_take.take_index == take_index
            );
        })};

        if (iter != m_legal_moves.end()) {
            user_move_take(source_index, destination_index, take_index);
        }
    }

    void MuhleBoard::place(int place_index) {
        assert(m_board[place_index] == Piece::None);

        m_board[place_index] = static_cast<Piece>(m_turn);

        finish_turn();
        check_winner_blocking();

        m_move_callback(create_place(place_index));
    }

    void MuhleBoard::place_take(int place_index, int take_index) {
        assert(m_board[place_index] == Piece::None);
        assert(m_board[take_index] != Piece::None);

        m_board[place_index] = static_cast<Piece>(m_turn);
        m_board[take_index] = Piece::None;

        finish_turn();
        check_winner_material();
        check_winner_blocking();

        m_move_callback(create_place_take(place_index, take_index));
    }

    void MuhleBoard::move(int source_index, int destination_index) {
        assert(m_board[source_index] != Piece::None);
        assert(m_board[destination_index] == Piece::None);

        std::swap(m_board[source_index], m_board[destination_index]);

        finish_turn(false);
        check_winner_blocking();
        check_fifty_move_rule();
        check_threefold_repetition({m_board, m_turn});

        m_move_callback(create_move(source_index, destination_index));
    }

    void MuhleBoard::move_take(int source_index, int destination_index, int take_index) {
        assert(m_board[source_index] != Piece::None);
        assert(m_board[destination_index] == Piece::None);
        assert(m_board[take_index] != Piece::None);

        std::swap(m_board[source_index], m_board[destination_index]);
        m_board[take_index] = Piece::None;

        finish_turn();
        check_winner_material();
        check_winner_blocking();

        m_move_callback(create_move_take(source_index, destination_index, take_index));
    }

    void MuhleBoard::finish_turn(bool advancement) {
        if (m_turn == Player::White) {
            m_turn = Player::Black;
        } else {
            m_turn = Player::White;
        }

        m_plies++;
        m_legal_moves = generate_moves();

        if (advancement) {
            m_plies_without_advancement = 0;
            m_positions.clear();
        } else {
            m_plies_without_advancement++;
        }

        m_positions.push_back({m_board, m_turn});

        m_selected_index = -1;
        m_take_action_index = -1;
    }

    void MuhleBoard::check_winner_material() {
        if (m_game_over != GameOver::None) {
            return;
        }

        if (m_plies < 18) {
            return;
        }

        if (count_pieces(m_board, m_turn) < 3) {
            m_game_over = static_cast<GameOver>(opponent(m_turn));
        }
    }

    void MuhleBoard::check_winner_blocking() {
        if (m_game_over != GameOver::None) {
            return;
        }

        if (m_legal_moves.empty()) {
            m_game_over = static_cast<GameOver>(opponent(m_turn));
        }
    }

    void MuhleBoard::check_fifty_move_rule() {
        if (m_game_over != GameOver::None) {
            return;
        }

        if (m_plies_without_advancement == 100) {
            m_game_over = GameOver::TieBetweenBothPlayers;
        }
    }

    void MuhleBoard::check_threefold_repetition(const Position& position) {
        if (m_game_over != GameOver::None) {
            return;
        }

        unsigned int repetitions {1};

        for (auto iter {m_positions.begin()}; iter != std::prev(m_positions.end()); iter++) {
            if (*iter == position) {
                if (++repetitions == 3) {
                    m_game_over = GameOver::TieBetweenBothPlayers;
                    return;
                }
            }
        }
    }

    void MuhleBoard::initialize_pieces() {
        for (std::size_t i {0}; i < 9; i++) {
            m_pieces[i] = PieceObj(Player::White);
        }

        for (std::size_t i {9}; i < 18; i++) {
            m_pieces[i] = PieceObj(Player::Black);
        }
    }

    int MuhleBoard::new_piece_to_place(Player type) const {
        for (std::size_t i {0}; i < m_pieces.size(); i++) {
            if (m_pieces[i].get_type() == type && m_pieces[i].node_index == -1) {
                return static_cast<int>(i);
            }
        }

        assert(false);
        return {};
    }

    int MuhleBoard::piece_on_node(int index) const {
        for (std::size_t i {0}; i < m_pieces.size(); i++) {
            if (m_pieces[i].node_index == index) {
                return static_cast<int>(i);
            }
        }

        assert(false);
        return {};
    }

    int MuhleBoard::get_index(ImVec2 position) const {
        for (int i {0}; i < 24; i++) {
            if (point_in_circle(position, node_position(i), m_board_unit / NODE_RADIUS)) {
                return i;
            }
        }

        return -1;
    }

    ImVec2 MuhleBoard::node_position(int index) const {
        return ImVec2(
            static_cast<float>(NODE_POSITIONS[index][0]) * m_board_unit + m_board_offset.x,
            static_cast<float>(NODE_POSITIONS[index][1]) * m_board_unit + m_board_offset.y
        );
    }

    bool MuhleBoard::point_in_circle(ImVec2 point, ImVec2 circle, float radius) {
        const ImVec2 subtracted {circle.x - point.x, circle.y - point.y};
        const float length {std::pow(subtracted.x * subtracted.x + subtracted.y * subtracted.y, 0.5f)};

        return length < radius;
    }

    std::vector<Move> MuhleBoard::generate_moves() const {
        std::vector<Move> moves;
        Board local_board {m_board};

        if (m_plies < 18) {
            generate_moves_phase1(local_board, moves, m_turn);
        } else {
            if (count_pieces(local_board, m_turn) == 3) {
                generate_moves_phase3(local_board, moves, m_turn);
            } else {
                generate_moves_phase2(local_board, moves, m_turn);
            }
        }

        return moves;
    }

    void MuhleBoard::generate_moves_phase1(Board& board, std::vector<Move>& moves, Player player) {
        for (int i {0}; i < 24; i++) {
            if (board[i] != Piece::None) {
                continue;
            }

            make_place_move(board, player, i);

            if (is_mill(board, player, i)) {
                const Player opponent_player {opponent(player)};
                const bool all_in_mills {all_pieces_in_mills(board, opponent_player)};

                for (int j {0}; j < 24; j++) {
                    if (board[j] != static_cast<Piece>(opponent_player)) {
                        continue;
                    }

                    if (is_mill(board, opponent_player, j) && !all_in_mills) {
                        continue;
                    }

                    moves.push_back(create_place_take(i, j));
                }
            } else {
                moves.push_back(create_place(i));
            }

            unmake_place_move(board, i);
        }
    }

    void MuhleBoard::generate_moves_phase2(Board& board, std::vector<Move>& moves, Player player) {
        for (int i {0}; i < 24; i++) {
            if (board[i] != static_cast<Piece>(player)) {
                continue;
            }

            const auto free_positions {neighbor_free_positions(board, i)};

            for (int j {0}; j < static_cast<int>(free_positions.size()); j++) {
                make_move_move(board, i, free_positions[j]);

                if (is_mill(board, player, free_positions[j])) {
                    const Player opponent_player {opponent(player)};
                    const bool all_in_mills {all_pieces_in_mills(board, opponent_player)};

                    for (int k {0}; k < 24; k++) {
                        if (board[k] != static_cast<Piece>(opponent_player)) {
                            continue;
                        }

                        if (is_mill(board, opponent_player, k) && !all_in_mills) {
                            continue;
                        }

                        moves.push_back(create_move_take(i, free_positions[j], k));
                    }
                } else {
                    moves.push_back(create_move(i, free_positions[j]));
                }

                unmake_move_move(board, i, free_positions[j]);
            }
        }
    }

    void MuhleBoard::generate_moves_phase3(Board& board, std::vector<Move>& moves, Player player) {
        for (int i {0}; i < 24; i++) {
            if (board[i] != static_cast<Piece>(player)) {
                continue;
            }

            for (int j {0}; j < 24; j++) {
                if (board[j] != Piece::None) {
                    continue;
                }

                make_move_move(board, i, j);

                if (is_mill(board, player, j)) {
                    const Player opponent_player {opponent(player)};
                    const bool all_in_mills {all_pieces_in_mills(board, opponent_player)};

                    for (int k {0}; k < 24; k++) {
                        if (board[k] != static_cast<Piece>(opponent_player)) {
                            continue;
                        }

                        if (is_mill(board, opponent_player, k) && !all_in_mills) {
                            continue;
                        }

                        moves.push_back(create_move_take(i, j, k));
                    }
                } else {
                    moves.push_back(create_move(i, j));
                }

                unmake_move_move(board, i, j);
            }
        }
    }

    void MuhleBoard::make_place_move(Board& board, Player player, int place_index) {
        assert(board[place_index] == Piece::None);

        board[place_index] = static_cast<Piece>(player);
    }

    void MuhleBoard::unmake_place_move(Board& board, int place_index) {
        assert(board[place_index] != Piece::None);

        board[place_index] = Piece::None;
    }

    void MuhleBoard::make_move_move(Board& board, int source_index, int destination_index) {
        assert(board[source_index] != Piece::None);
        assert(board[destination_index] == Piece::None);

        std::swap(board[source_index], board[destination_index]);
    }

    void MuhleBoard::unmake_move_move(Board& board, int source_index, int destination_index) {
        assert(board[source_index] == Piece::None);
        assert(board[destination_index] != Piece::None);

        std::swap(board[source_index], board[destination_index]);
    }

#ifdef __GNUG__
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wparentheses"
#endif

    bool MuhleBoard::is_mill(const Board& board, Player player, int index) {
        const Piece piece {static_cast<Piece>(player)};

        assert(board[index] == piece);

        switch (index) {
            case 0: return board[1] == piece && board[2] == piece || board[9] == piece && board[21] == piece;
            case 1: return board[0] == piece && board[2] == piece || board[4] == piece && board[7] == piece;
            case 2: return board[0] == piece && board[1] == piece || board[14] == piece && board[23] == piece;
            case 3: return board[4] == piece && board[5] == piece || board[10] == piece && board[18] == piece;
            case 4: return board[3] == piece && board[5] == piece || board[1] == piece && board[7] == piece;
            case 5: return board[3] == piece && board[4] == piece || board[13] == piece && board[20] == piece;
            case 6: return board[7] == piece && board[8] == piece || board[11] == piece && board[15] == piece;
            case 7: return board[6] == piece && board[8] == piece || board[1] == piece && board[4] == piece;
            case 8: return board[6] == piece && board[7] == piece || board[12] == piece && board[17] == piece;
            case 9: return board[0] == piece && board[21] == piece || board[10] == piece && board[11] == piece;
            case 10: return board[9] == piece && board[11] == piece || board[3] == piece && board[18] == piece;
            case 11: return board[9] == piece && board[10] == piece || board[6] == piece && board[15] == piece;
            case 12: return board[13] == piece && board[14] == piece || board[8] == piece && board[17] == piece;
            case 13: return board[12] == piece && board[14] == piece || board[5] == piece && board[20] == piece;
            case 14: return board[12] == piece && board[13] == piece || board[2] == piece && board[23] == piece;
            case 15: return board[16] == piece && board[17] == piece || board[6] == piece && board[11] == piece;
            case 16: return board[15] == piece && board[17] == piece || board[19] == piece && board[22] == piece;
            case 17: return board[15] == piece && board[16] == piece || board[8] == piece && board[12] == piece;
            case 18: return board[19] == piece && board[20] == piece || board[3] == piece && board[10] == piece;
            case 19: return board[18] == piece && board[20] == piece || board[16] == piece && board[22] == piece;
            case 20: return board[18] == piece && board[19] == piece || board[5] == piece && board[13] == piece;
            case 21: return board[22] == piece && board[23] == piece || board[0] == piece && board[9] == piece;
            case 22: return board[21] == piece && board[23] == piece || board[16] == piece && board[19] == piece;
            case 23: return board[21] == piece && board[22] == piece || board[2] == piece && board[14] == piece;
        }

        return {};
    }

#ifdef __GNUG__
    #pragma GCC diagnostic pop
#endif

    bool MuhleBoard::all_pieces_in_mills(const Board& board, Player player) {
        for (int i {0}; i < 24; i++) {
            if (board[i] != static_cast<Piece>(player)) {
                continue;
            }

            if (!is_mill(board, player, i)) {
                return false;
            }
        }

        return true;
    }

#define IS_FREE_CHECK(const_index) \
    if (board[const_index] == Piece::None) { \
        result.push_back(const_index); \
    }

    std::vector<int> MuhleBoard::neighbor_free_positions(const Board& board, int index) {
        std::vector<int> result;
        result.reserve(4);

        switch (index) {
            case 0:
                IS_FREE_CHECK(1)
                IS_FREE_CHECK(9)
                break;
            case 1:
                IS_FREE_CHECK(0)
                IS_FREE_CHECK(2)
                IS_FREE_CHECK(4)
                break;
            case 2:
                IS_FREE_CHECK(1)
                IS_FREE_CHECK(14)
                break;
            case 3:
                IS_FREE_CHECK(4)
                IS_FREE_CHECK(10)
                break;
            case 4:
                IS_FREE_CHECK(1)
                IS_FREE_CHECK(3)
                IS_FREE_CHECK(5)
                IS_FREE_CHECK(7)
                break;
            case 5:
                IS_FREE_CHECK(4)
                IS_FREE_CHECK(13)
                break;
            case 6:
                IS_FREE_CHECK(7)
                IS_FREE_CHECK(11)
                break;
            case 7:
                IS_FREE_CHECK(4)
                IS_FREE_CHECK(6)
                IS_FREE_CHECK(8)
                break;
            case 8:
                IS_FREE_CHECK(7)
                IS_FREE_CHECK(12)
                break;
            case 9:
                IS_FREE_CHECK(0)
                IS_FREE_CHECK(10)
                IS_FREE_CHECK(21)
                break;
            case 10:
                IS_FREE_CHECK(3)
                IS_FREE_CHECK(9)
                IS_FREE_CHECK(11)
                IS_FREE_CHECK(18)
                break;
            case 11:
                IS_FREE_CHECK(6)
                IS_FREE_CHECK(10)
                IS_FREE_CHECK(15)
                break;
            case 12:
                IS_FREE_CHECK(8)
                IS_FREE_CHECK(13)
                IS_FREE_CHECK(17)
                break;
            case 13:
                IS_FREE_CHECK(5)
                IS_FREE_CHECK(12)
                IS_FREE_CHECK(14)
                IS_FREE_CHECK(20)
                break;
            case 14:
                IS_FREE_CHECK(2)
                IS_FREE_CHECK(13)
                IS_FREE_CHECK(23)
                break;
            case 15:
                IS_FREE_CHECK(11)
                IS_FREE_CHECK(16)
                break;
            case 16:
                IS_FREE_CHECK(15)
                IS_FREE_CHECK(17)
                IS_FREE_CHECK(19)
                break;
            case 17:
                IS_FREE_CHECK(12)
                IS_FREE_CHECK(16)
                break;
            case 18:
                IS_FREE_CHECK(10)
                IS_FREE_CHECK(19)
                break;
            case 19:
                IS_FREE_CHECK(16)
                IS_FREE_CHECK(18)
                IS_FREE_CHECK(20)
                IS_FREE_CHECK(22)
                break;
            case 20:
                IS_FREE_CHECK(13)
                IS_FREE_CHECK(19)
                break;
            case 21:
                IS_FREE_CHECK(9)
                IS_FREE_CHECK(22)
                break;
            case 22:
                IS_FREE_CHECK(19)
                IS_FREE_CHECK(21)
                IS_FREE_CHECK(23)
                break;
            case 23:
                IS_FREE_CHECK(14)
                IS_FREE_CHECK(22)
                break;
        }

        return result;
    }

    Move MuhleBoard::create_place(int place_index) {
        Move move;
        move.type = MoveType::Place;
        move.place.place_index = place_index;

        return move;
    }

    Move MuhleBoard::create_place_take(int place_index, int take_index) {
        Move move;
        move.type = MoveType::PlaceTake;
        move.place_take.place_index = place_index;
        move.place_take.take_index = take_index;

        return move;
    }

    Move MuhleBoard::create_move(int source_index, int destination_index) {
        Move move;
        move.type = MoveType::Move;
        move.move.source_index = source_index;
        move.move.destination_index = destination_index;

        return move;
    }

    Move MuhleBoard::create_move_take(int source_index, int destination_index, int take_index) {
        Move move;
        move.type = MoveType::MoveTake;
        move.move_take.source_index = source_index;
        move.move_take.destination_index = destination_index;
        move.move_take.take_index = take_index;

        return move;
    }

    unsigned int MuhleBoard::count_pieces(const Board& board, Player player) {
        unsigned int result {0};

        for (const Piece piece : board) {
            result += static_cast<unsigned int>(piece == static_cast<Piece>(player));
        }

        return result;
    }

    Player MuhleBoard::opponent(Player player) {
        if (player == Player::White) {
            return Player::Black;
        } else {
            return Player::White;
        }
    }

    Move move_from_string(std::string_view string) {
        Move result {};

        switch (string[0]) {
            case 'P': {
                const int place_index {index_from_string(string.substr(1, 2))};

                if (string.size() > 3) {
                    const int take_index {index_from_string(string.substr(4, 2))};

                    result.type = MoveType::PlaceTake;
                    result.place_take.place_index = place_index;
                    result.place_take.take_index = take_index;
                } else {
                    result.type = MoveType::Place;
                    result.place.place_index = place_index;
                }

                return result;
            }
            case 'M': {
                const int source_index {index_from_string(string.substr(1, 2))};
                const int destination_index {index_from_string(string.substr(4, 2))};

                if (string.size() > 6) {
                    const int take_index {index_from_string(string.substr(7, 2))};

                    result.type = MoveType::MoveTake;
                    result.move_take.source_index = source_index;
                    result.move_take.destination_index = destination_index;
                    result.move_take.take_index = take_index;
                } else {
                    result.type = MoveType::Move;
                    result.move.source_index = source_index;
                    result.move.destination_index = destination_index;
                }

                return result;
            }
        }

        assert(false);
        return {};
    }

    std::string move_to_string(const Move& move) {
        std::string result;

        switch (move.type) {
            case MoveType::Place:
                result += 'P';
                result += string_from_index(move.place.place_index);
                break;
            case MoveType::PlaceTake:
                result += 'P';
                result += string_from_index(move.place_take.place_index);
                result += 'T';
                result += string_from_index(move.place_take.take_index);
                break;
            case MoveType::Move:
                result += 'M';
                result += string_from_index(move.move.source_index);
                result += '-';
                result += string_from_index(move.move.destination_index);
                break;
            case MoveType::MoveTake:
                result += 'M';
                result += string_from_index(move.move_take.source_index);
                result += '-';
                result += string_from_index(move.move_take.destination_index);
                result += 'T';
                result += string_from_index(move.move_take.take_index);
                break;
        }

        return result;
    }
}
