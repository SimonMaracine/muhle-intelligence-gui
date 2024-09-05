#include "common/board.hpp"

#include <utility>
#include <unordered_map>
#include <algorithm>
#include <cmath>
#include <cstdlib>
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

    static Idx index_from_string(std::string_view string) {
        const std::unordered_map<std::string_view, Idx> map {
            { "a7", 0 },
            { "d7", 1 },
            { "g7", 2 },
            { "b6", 3 },
            { "d6", 4 },
            { "f6", 5 },
            { "c5", 6 },
            { "d5", 7 },
            { "e5", 8 },
            { "a4", 9 },
            { "b4", 10 },
            { "c4", 11 },
            { "e4", 12 },
            { "f4", 13 },
            { "g4", 14 },
            { "c3", 15 },
            { "d3", 16 },
            { "e3", 17 },
            { "b2", 18 },
            { "d2", 19 },
            { "f2", 20 },
            { "a1", 21 },
            { "d1", 22 },
            { "g1", 23 }
        };

        return map.at(string);
    }

    static std::string_view string_from_index(Idx index) {
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
            default: std::abort();
        }
    }

    MuhleBoard::MuhleBoard(std::function<void(const Move&)>&& move_callback)
        : m_move_callback(std::move(move_callback)) {
        m_legal_moves = generate_moves();
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

            const float UNIT {canvas_size.x < canvas_size.y ? (canvas_p1.x - canvas_p0.x) / 10.0f : (canvas_p1.y - canvas_p0.y) / 10.0f};
            const ImVec2 OFFSET {ImVec2(canvas_p0.x, canvas_p0.y)};

            const ImColor COLOR {ImColor(200, 200, 200)};
            const float THICKNESS {2.0f};

            m_board_unit = UNIT;
            m_board_offset = OFFSET;

            draw_list->AddRectFilled(canvas_p0, canvas_p1, ImColor(45, 45, 45));

            draw_list->AddRect(ImVec2(2.0f * UNIT + OFFSET.x, 8.0f * UNIT + OFFSET.y), ImVec2(8.0f * UNIT + OFFSET.x, 2.0f * UNIT + OFFSET.y), COLOR, 0.0f, 0, THICKNESS);
            draw_list->AddRect(ImVec2(3.0f * UNIT + OFFSET.x, 7.0f * UNIT + OFFSET.y), ImVec2(7.0f * UNIT + OFFSET.x, 3.0f * UNIT + OFFSET.y), COLOR, 0.0f, 0, THICKNESS);
            draw_list->AddRect(ImVec2(4.0f * UNIT + OFFSET.x, 6.0f * UNIT + OFFSET.y), ImVec2(6.0f * UNIT + OFFSET.x, 4.0f * UNIT + OFFSET.y), COLOR, 0.0f, 0, THICKNESS);

            draw_list->AddLine(ImVec2(5.0f * UNIT + OFFSET.x, 2.0f * UNIT + OFFSET.y), ImVec2(5.0f * UNIT + OFFSET.x, 4.0f * UNIT + OFFSET.y), COLOR, THICKNESS);
            draw_list->AddLine(ImVec2(6.0f * UNIT + OFFSET.x, 5.0f * UNIT + OFFSET.y), ImVec2(8.0f * UNIT + OFFSET.x, 5.0f * UNIT + OFFSET.y), COLOR, THICKNESS);
            draw_list->AddLine(ImVec2(5.0f * UNIT + OFFSET.x, 6.0f * UNIT + OFFSET.y), ImVec2(5.0f * UNIT + OFFSET.x, 8.0f * UNIT + OFFSET.y), COLOR, THICKNESS);
            draw_list->AddLine(ImVec2(2.0f * UNIT + OFFSET.x, 5.0f * UNIT + OFFSET.y), ImVec2(4.0f * UNIT + OFFSET.x, 5.0f * UNIT + OFFSET.y), COLOR, THICKNESS);

            draw_list->AddText(ImVec2(2.0f * UNIT + OFFSET.x, 1.0f * UNIT + OFFSET.y), COLOR, "A");
            draw_list->AddText(ImVec2(3.0f * UNIT + OFFSET.x, 1.0f * UNIT + OFFSET.y), COLOR, "B");
            draw_list->AddText(ImVec2(4.0f * UNIT + OFFSET.x, 1.0f * UNIT + OFFSET.y), COLOR, "C");
            draw_list->AddText(ImVec2(5.0f * UNIT + OFFSET.x, 1.0f * UNIT + OFFSET.y), COLOR, "D");
            draw_list->AddText(ImVec2(6.0f * UNIT + OFFSET.x, 1.0f * UNIT + OFFSET.y), COLOR, "E");
            draw_list->AddText(ImVec2(7.0f * UNIT + OFFSET.x, 1.0f * UNIT + OFFSET.y), COLOR, "F");
            draw_list->AddText(ImVec2(8.0f * UNIT + OFFSET.x, 1.0f * UNIT + OFFSET.y), COLOR, "G");

            draw_list->AddText(ImVec2(2.0f * UNIT + OFFSET.x, 9.0f * UNIT + OFFSET.y), COLOR, "A");
            draw_list->AddText(ImVec2(3.0f * UNIT + OFFSET.x, 9.0f * UNIT + OFFSET.y), COLOR, "B");
            draw_list->AddText(ImVec2(4.0f * UNIT + OFFSET.x, 9.0f * UNIT + OFFSET.y), COLOR, "C");
            draw_list->AddText(ImVec2(5.0f * UNIT + OFFSET.x, 9.0f * UNIT + OFFSET.y), COLOR, "D");
            draw_list->AddText(ImVec2(6.0f * UNIT + OFFSET.x, 9.0f * UNIT + OFFSET.y), COLOR, "E");
            draw_list->AddText(ImVec2(7.0f * UNIT + OFFSET.x, 9.0f * UNIT + OFFSET.y), COLOR, "F");
            draw_list->AddText(ImVec2(8.0f * UNIT + OFFSET.x, 9.0f * UNIT + OFFSET.y), COLOR, "G");

            draw_list->AddText(ImVec2(9.0f * UNIT + OFFSET.x, 2.0f * UNIT + OFFSET.y), COLOR, "7");
            draw_list->AddText(ImVec2(9.0f * UNIT + OFFSET.x, 3.0f * UNIT + OFFSET.y), COLOR, "6");
            draw_list->AddText(ImVec2(9.0f * UNIT + OFFSET.x, 4.0f * UNIT + OFFSET.y), COLOR, "5");
            draw_list->AddText(ImVec2(9.0f * UNIT + OFFSET.x, 5.0f * UNIT + OFFSET.y), COLOR, "4");
            draw_list->AddText(ImVec2(9.0f * UNIT + OFFSET.x, 6.0f * UNIT + OFFSET.y), COLOR, "3");
            draw_list->AddText(ImVec2(9.0f * UNIT + OFFSET.x, 7.0f * UNIT + OFFSET.y), COLOR, "2");
            draw_list->AddText(ImVec2(9.0f * UNIT + OFFSET.x, 8.0f * UNIT + OFFSET.y), COLOR, "1");

            draw_list->AddText(ImVec2(1.0f * UNIT + OFFSET.x, 2.0f * UNIT + OFFSET.y), COLOR, "7");
            draw_list->AddText(ImVec2(1.0f * UNIT + OFFSET.x, 3.0f * UNIT + OFFSET.y), COLOR, "6");
            draw_list->AddText(ImVec2(1.0f * UNIT + OFFSET.x, 4.0f * UNIT + OFFSET.y), COLOR, "5");
            draw_list->AddText(ImVec2(1.0f * UNIT + OFFSET.x, 5.0f * UNIT + OFFSET.y), COLOR, "4");
            draw_list->AddText(ImVec2(1.0f * UNIT + OFFSET.x, 6.0f * UNIT + OFFSET.y), COLOR, "3");
            draw_list->AddText(ImVec2(1.0f * UNIT + OFFSET.x, 7.0f * UNIT + OFFSET.y), COLOR, "2");
            draw_list->AddText(ImVec2(1.0f * UNIT + OFFSET.x, 8.0f * UNIT + OFFSET.y), COLOR, "1");

            for (Idx i {0}; i < 24; i++) {
                switch (m_board[i]) {
                    case Piece::None:
                        break;
                    case Piece::White:
                        draw_list->AddCircleFilled(
                            ImVec2(
                                static_cast<float>(NODE_POSITIONS[i][0]) * m_board_unit + m_board_offset.x,
                                static_cast<float>(NODE_POSITIONS[i][1]) * m_board_unit + m_board_offset.y
                            ),
                            m_board_unit / NODE_RADIUS,
                            ImColor(235, 235, 235, 255)
                        );
                        break;
                    case Piece::Black:
                        draw_list->AddCircleFilled(
                            ImVec2(
                                static_cast<float>(NODE_POSITIONS[i][0]) * m_board_unit + m_board_offset.x,
                                static_cast<float>(NODE_POSITIONS[i][1]) * m_board_unit + m_board_offset.y
                            ),
                            m_board_unit / NODE_RADIUS,
                            ImColor(15, 15, 15, 255)
                        );
                        break;
                }
            }

            const float WIDTH {m_board_unit < 55.0f ? 2.0f : 3.0f};

            if (m_user_selected_index != NULL_INDEX) {
                const ImVec2 position {
                    static_cast<float>(NODE_POSITIONS[m_user_selected_index][0]) * m_board_unit + m_board_offset.x,
                    static_cast<float>(NODE_POSITIONS[m_user_selected_index][1]) * m_board_unit + m_board_offset.y
                };

                draw_list->AddCircle(position, m_board_unit / NODE_RADIUS + 1.0f, ImColor(240, 30, 30, 255), 0, WIDTH);
            }

            if (m_user_take_action_index != NULL_INDEX) {
                const ImVec2 position {
                    static_cast<float>(NODE_POSITIONS[m_user_take_action_index][0]) * m_board_unit + m_board_offset.x,
                    static_cast<float>(NODE_POSITIONS[m_user_take_action_index][1]) * m_board_unit + m_board_offset.y
                };

                draw_list->AddCircle(position, m_board_unit / NODE_RADIUS + 1.0f, ImColor(30, 30, 240, 255), 0, WIDTH);
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

        m_user_selected_index = NULL_INDEX;
        m_user_take_action_index = NULL_INDEX;

        m_legal_moves = generate_moves();
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
            ImGui::Text("user_selected_index: %d", m_user_selected_index);
            ImGui::Text("user_take_action_index: %d", m_user_take_action_index);
            ImGui::Text("legal_moves: %lu", m_legal_moves.size());
        }

        ImGui::End();
    }

    void MuhleBoard::place(Idx place_index) {
        assert(m_board[place_index] == Piece::None);

        m_board[place_index] = static_cast<Piece>(m_turn);

        finish_turn();
        check_winner_blocking();

        m_move_callback(create_place(place_index));
    }

    void MuhleBoard::place_take(Idx place_index, Idx take_index) {
        assert(m_board[place_index] == Piece::None);
        assert(m_board[take_index] != Piece::None);

        m_board[place_index] = static_cast<Piece>(m_turn);
        m_board[take_index] = Piece::None;

        finish_turn();
        check_winner_material();
        check_winner_blocking();

        m_move_callback(create_place_take(place_index, take_index));
    }

    void MuhleBoard::move(Idx source_index, Idx destination_index) {
        assert(m_board[source_index] != Piece::None);
        assert(m_board[destination_index] == Piece::None);

        std::swap(m_board[source_index], m_board[destination_index]);

        finish_turn(false);
        check_winner_blocking();
        check_fifty_move_rule();
        check_threefold_repetition({m_board, m_turn});

        m_move_callback(create_move(source_index, destination_index));
    }

    void MuhleBoard::move_take(Idx source_index, Idx destination_index, Idx take_index) {
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

    void MuhleBoard::update_user_input() {
        if (!ImGui::IsWindowFocused()) {
            return;
        }

        if (m_game_over != GameOver::None) {
            return;
        }

        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            const ImVec2 position {ImGui::GetMousePos()};
            const Idx index {get_index(position)};

            if (index == NULL_INDEX) {
                return;
            }

            if (m_plies >= 18) {
                if (m_user_take_action_index != NULL_INDEX) {
                    try_move_take(m_user_selected_index, m_user_take_action_index, index);
                } else {
                    try_move(m_user_selected_index, index);
                    select(index);
                }
            } else {
                if (m_user_take_action_index != NULL_INDEX) {
                    try_place_take(m_user_take_action_index, index);
                } else {
                    try_place(index);
                }
            }
        }
    }

    void MuhleBoard::select(Idx index) {
        if (m_user_selected_index == NULL_INDEX) {
            if (m_board[index] == static_cast<Piece>(m_turn)) {
                m_user_selected_index = index;
            }
        } else {
            if (index == m_user_selected_index) {
                if (m_user_take_action_index == NULL_INDEX) {
                    m_user_selected_index = NULL_INDEX;
                }
            } else if (m_board[index] == static_cast<Piece>(m_turn)) {
                if (m_user_take_action_index == NULL_INDEX) {
                    m_user_selected_index = index;
                }
            }
        }
    }

    void MuhleBoard::try_place(Idx place_index) {
        auto iter {std::find_if(m_legal_moves.begin(), m_legal_moves.end(), [=](const Move& move) {
            return move.type == MoveType::Place && move.place.place_index == place_index;
        })};

        if (iter != m_legal_moves.end()) {
            place(place_index);
            return;
        }

        iter = std::find_if(m_legal_moves.begin(), m_legal_moves.end(), [=](const Move& move) {
            return move.type == MoveType::PlaceTake && move.place_take.place_index == place_index;
        });

        if (iter != m_legal_moves.end()) {
            m_user_take_action_index = place_index;
        }
    }

    void MuhleBoard::try_place_take(Idx place_index, Idx take_index) {
        const auto iter {std::find_if(m_legal_moves.begin(), m_legal_moves.end(), [=](const Move& move) {
            return (
                move.type == MoveType::PlaceTake &&
                move.place_take.place_index == place_index &&
                move.place_take.take_index == take_index
            );
        })};

        if (iter != m_legal_moves.end()) {
            place_take(place_index, take_index);
        }
    }

    void MuhleBoard::try_move(Idx source_index, Idx destination_index) {
        auto iter {std::find_if(m_legal_moves.begin(), m_legal_moves.end(), [=](const Move& move) {
            return (
                move.type == MoveType::Move &&
                move.move.source_index == source_index &&
                move.move.destination_index == destination_index
            );
        })};

        if (iter != m_legal_moves.end()) {
            move(source_index, destination_index);
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
            m_user_take_action_index = destination_index;
        }
    }

    void MuhleBoard::try_move_take(Idx source_index, Idx destination_index, Idx take_index) {
        const auto iter {std::find_if(m_legal_moves.begin(), m_legal_moves.end(), [=](const Move& move) {
            return (
                move.type == MoveType::MoveTake &&
                move.move_take.source_index == source_index &&
                move.move_take.destination_index == destination_index &&
                move.move_take.take_index == take_index
            );
        })};

        if (iter != m_legal_moves.end()) {
            move_take(source_index, destination_index, take_index);
        }
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

        m_user_selected_index = NULL_INDEX;
        m_user_take_action_index = NULL_INDEX;
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

    bool MuhleBoard::point_in_circle(ImVec2 point, ImVec2 circle, float radius) {
        const ImVec2 subtracted {circle.x - point.x, circle.y - point.y};
        const float length {std::pow(subtracted.x * subtracted.x + subtracted.y * subtracted.y, 0.5f)};

        return length < radius;
    }

    Idx MuhleBoard::get_index(ImVec2 position) const {
        for (Idx i {0}; i < 24; i++) {
            const ImVec2 node {
                static_cast<float>(NODE_POSITIONS[i][0]) * m_board_unit + m_board_offset.x,
                static_cast<float>(NODE_POSITIONS[i][1]) * m_board_unit + m_board_offset.y
            };

            if (point_in_circle(position, node, m_board_unit / NODE_RADIUS)) {
                return i;
            }
        }

        return NULL_INDEX;
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
        for (Idx i {0}; i < 24; i++) {
            if (board[i] != Piece::None) {
                continue;
            }

            make_place_move(board, player, i);

            if (is_mill(board, player, i)) {
                const Player opponent_player {opponent(player)};
                const bool all_in_mills {all_pieces_in_mills(board, opponent_player)};

                for (Idx j {0}; j < 24; j++) {
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
        for (Idx i {0}; i < 24; i++) {
            if (board[i] != static_cast<Piece>(player)) {
                continue;
            }

            const auto free_positions {neighbor_free_positions(board, i)};

            for (Idx j {0}; j < static_cast<Idx>(free_positions.size()); j++) {
                make_move_move(board, i, free_positions[j]);

                if (is_mill(board, player, free_positions[j])) {
                    const Player opponent_player {opponent(player)};
                    const bool all_in_mills {all_pieces_in_mills(board, opponent_player)};

                    for (Idx k {0}; k < 24; k++) {
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
        for (Idx i {0}; i < 24; i++) {
            if (board[i] != static_cast<Piece>(player)) {
                continue;
            }

            for (Idx j {0}; j < 24; j++) {
                if (board[j] != Piece::None) {
                    continue;
                }

                make_move_move(board, i, j);

                if (is_mill(board, player, j)) {
                    const Player opponent_player {opponent(player)};
                    const bool all_in_mills {all_pieces_in_mills(board, opponent_player)};

                    for (Idx k {0}; k < 24; k++) {
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

    void MuhleBoard::make_place_move(Board& board, Player player, Idx place_index) {
        assert(board[place_index] == Piece::None);

        board[place_index] = static_cast<Piece>(player);
    }

    void MuhleBoard::unmake_place_move(Board& board, Idx place_index) {
        assert(board[place_index] != Piece::None);

        board[place_index] = Piece::None;
    }

    void MuhleBoard::make_move_move(Board& board, Idx source_index, Idx destination_index) {
        assert(board[source_index] != Piece::None);
        assert(board[destination_index] == Piece::None);

        std::swap(board[source_index], board[destination_index]);
    }

    void MuhleBoard::unmake_move_move(Board& board, Idx source_index, Idx destination_index) {
        assert(board[source_index] == Piece::None);
        assert(board[destination_index] != Piece::None);

        std::swap(board[source_index], board[destination_index]);
    }

#ifdef __GNUG__
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wparentheses"
#endif

#define IS_PC(const_index) (board[const_index] == piece)

    bool MuhleBoard::is_mill(const Board& board, Player player, Idx index) {
        const Piece piece {static_cast<Piece>(player)};

        assert(board[index] == piece);

        switch (index) {
            case 0:
                return IS_PC(1) && IS_PC(2) || IS_PC(9) && IS_PC(21);
            case 1:
                return IS_PC(0) && IS_PC(2) || IS_PC(4) && IS_PC(7);
            case 2:
                return IS_PC(0) && IS_PC(1) || IS_PC(14) && IS_PC(23);
            case 3:
                return IS_PC(4) && IS_PC(5) || IS_PC(10) && IS_PC(18);
            case 4:
                return IS_PC(3) && IS_PC(5) || IS_PC(1) && IS_PC(7);
            case 5:
                return IS_PC(3) && IS_PC(4) || IS_PC(13) && IS_PC(20);
            case 6:
                return IS_PC(7) && IS_PC(8) || IS_PC(11) && IS_PC(15);
            case 7:
                return IS_PC(6) && IS_PC(8) || IS_PC(1) && IS_PC(4);
            case 8:
                return IS_PC(6) && IS_PC(7) || IS_PC(12) && IS_PC(17);
            case 9:
                return IS_PC(0) && IS_PC(21) || IS_PC(10) && IS_PC(11);
            case 10:
                return IS_PC(9) && IS_PC(11) || IS_PC(3) && IS_PC(18);
            case 11:
                return IS_PC(9) && IS_PC(10) || IS_PC(6) && IS_PC(15);
            case 12:
                return IS_PC(13) && IS_PC(14) || IS_PC(8) && IS_PC(17);
            case 13:
                return IS_PC(12) && IS_PC(14) || IS_PC(5) && IS_PC(20);
            case 14:
                return IS_PC(12) && IS_PC(13) || IS_PC(2) && IS_PC(23);
            case 15:
                return IS_PC(16) && IS_PC(17) || IS_PC(6) && IS_PC(11);
            case 16:
                return IS_PC(15) && IS_PC(17) || IS_PC(19) && IS_PC(22);
            case 17:
                return IS_PC(15) && IS_PC(16) || IS_PC(8) && IS_PC(12);
            case 18:
                return IS_PC(19) && IS_PC(20) || IS_PC(3) && IS_PC(10);
            case 19:
                return IS_PC(18) && IS_PC(20) || IS_PC(16) && IS_PC(22);
            case 20:
                return IS_PC(18) && IS_PC(19) || IS_PC(5) && IS_PC(13);
            case 21:
                return IS_PC(22) && IS_PC(23) || IS_PC(0) && IS_PC(9);
            case 22:
                return IS_PC(21) && IS_PC(23) || IS_PC(16) && IS_PC(19);
            case 23:
                return IS_PC(21) && IS_PC(22) || IS_PC(2) && IS_PC(14);
        }

        return {};
    }

#ifdef __GNUG__
    #pragma GCC diagnostic pop
#endif

    bool MuhleBoard::all_pieces_in_mills(const Board& board, Player player) {
        for (Idx i {0}; i < 24; i++) {
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

    std::vector<Idx> MuhleBoard::neighbor_free_positions(const Board& board, Idx index) {
        std::vector<Idx> result;
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

    Move MuhleBoard::create_place(Idx place_index) {
        Move move;
        move.type = MoveType::Place;
        move.place.place_index = place_index;

        return move;
    }

    Move MuhleBoard::create_place_take(Idx place_index, Idx take_index) {
        Move move;
        move.type = MoveType::PlaceTake;
        move.place_take.place_index = place_index;
        move.place_take.take_index = take_index;

        return move;
    }

    Move MuhleBoard::create_move(Idx source_index, Idx destination_index) {
        Move move;
        move.type = MoveType::Move;
        move.move.source_index = source_index;
        move.move.destination_index = destination_index;

        return move;
    }

    Move MuhleBoard::create_move_take(Idx source_index, Idx destination_index, Idx take_index) {
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
                const Idx place_index {index_from_string(string.substr(1, 2))};

                if (string.size() > 3) {
                    const Idx take_index {index_from_string(string.substr(4, 2))};

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
                const Idx source_index {index_from_string(string.substr(1, 2))};
                const Idx destination_index {index_from_string(string.substr(4, 2))};

                if (string.size() > 6) {
                    const Idx take_index {index_from_string(string.substr(7, 2))};

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
