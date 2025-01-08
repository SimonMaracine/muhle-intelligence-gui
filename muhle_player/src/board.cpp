#include "board.hpp"

#include <utility>
#include <algorithm>
#include <iterator>
#include <regex>
#include <cstddef>
#include <cmath>
#include <cstring>
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

    static int index_from_string(const std::string& string) {
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

        throw BoardError("Invalid string");
    }

    static const char* index_to_string(int index) {
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

        throw BoardError("Invalid index");
    }

    static std::vector<std::string> split(const std::string& message, const char* separator) {
        std::vector<std::string> tokens;
        std::string buffer {message};

        char* token {std::strtok(buffer.data(), separator)};

        while (token != nullptr) {
            tokens.emplace_back(token);
            token = std::strtok(nullptr, separator);
        }

        return tokens;
    }

    static Player parse_player(const std::string& string) {
        if (string == "w") {
            return Player::White;
        } else if (string == "b") {
            return Player::Black;
        } else {
            throw BoardError("Invalid string");
        }
    }

    static std::pair<std::vector<int>, Player> parse_pieces(const std::string& string) {
        const auto player {parse_player(string.substr(0, 1))};

        const auto tokens {split(string.substr(1), ",")};
        std::vector<int> pieces;

        for (const auto& token : tokens) {
            if (token.empty()) {
                continue;
            }

            pieces.push_back(index_from_string(token));
        }

        return std::make_pair(pieces, player);
    }

    bool Move::operator==(const Move& other) const {
        if (type != other.type) {
            return false;
        }

        switch (type) {
            case MoveType::Place:
                return place.place_index == other.place.place_index;
            case MoveType::PlaceCapture:
                return (
                    place_capture.place_index == other.place_capture.place_index &&
                    place_capture.capture_index == other.place_capture.capture_index
                );
            case MoveType::Move:
                return (
                    move.source_index == other.move.source_index &&
                    move.destination_index == other.move.destination_index
                );
            case MoveType::MoveCapture:
                return (
                    move_capture.source_index == other.move_capture.source_index &&
                    move_capture.destination_index == other.move_capture.destination_index &&
                    move_capture.capture_index == other.move_capture.capture_index
                );
        }

        return {};
    }

    Move Move::create_place(int place_index) {
        Move move;
        move.type = MoveType::Place;
        move.place.place_index = place_index;

        return move;
    }

    Move Move::create_place_capture(int place_index, int capture_index) {
        Move move;
        move.type = MoveType::PlaceCapture;
        move.place_capture.place_index = place_index;
        move.place_capture.capture_index = capture_index;

        return move;
    }

    Move Move::create_move(int source_index, int destination_index) {
        Move move;
        move.type = MoveType::Move;
        move.move.source_index = source_index;
        move.move.destination_index = destination_index;

        return move;
    }

    Move Move::create_move_capture(int source_index, int destination_index, int capture_index) {
        Move move;
        move.type = MoveType::MoveCapture;
        move.move_capture.source_index = source_index;
        move.move_capture.destination_index = destination_index;
        move.move_capture.capture_index = capture_index;

        return move;
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

    void PieceObj::render(ImDrawList* draw_list, float board_unit) {
        switch (m_type) {
            case Player::White:
                draw_list->AddCircleFilled(
                    m_position,
                    board_unit / NODE_RADIUS,
                    ImColor(235, 235, 235, 255)
                );
                break;
            case Player::Black:
                draw_list->AddCircleFilled(
                    m_position,
                    board_unit / NODE_RADIUS,
                    ImColor(15, 15, 15, 255)
                );
                break;
        }
    }

    void PieceObj::move(ImVec2 target) {
        m_target = target;
        m_moving = true;
    }

    Board::Board(std::function<void(const Move&)>&& move_callback)
        : m_move_callback(std::move(move_callback)) {
        m_legal_moves = generate_moves();

        initialize_pieces();
    }

    void Board::update(bool user_input) {
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

            if (m_select_index != -1 && !m_capture_piece) {
                const ImVec2 position {
                    static_cast<float>(NODE_POSITIONS[m_select_index][0]) * m_board_unit + m_board_offset.x,
                    static_cast<float>(NODE_POSITIONS[m_select_index][1]) * m_board_unit + m_board_offset.y
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

    void Board::reset(const std::optional<Position>& position) {
        m_position = position ? *position : Position {};
        m_plies_no_advancement = 0;
        m_positions.clear();

        m_capture_piece = false;
        m_select_index = -1;
        m_game_over = GameOver::None;
        m_setup_position = m_position;

        m_legal_moves = generate_moves();

        initialize_pieces();
    }

    void Board::debug() const {
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

            ImGui::Text("player: %s", m_position.player == Player::White ? "white" : "black");
            ImGui::Text("game_over: %s", game_over_string);
            ImGui::Text("plies: %u", m_position.plies);
            ImGui::Text("plies_no_advancement: %u", m_plies_no_advancement);
            ImGui::Text("positions: %lu", m_positions.size());
            ImGui::Text("capture_piece: %s", m_capture_piece ? "true" : "false");
            ImGui::Text("select_index: %d", m_select_index);
            ImGui::Text("legal_moves: %lu", m_legal_moves.size());
        }

        ImGui::End();
    }

    const Position& Board::get_setup_position() const {
        return m_setup_position;
    }

    void Board::play_move(const Move& move) {
        const auto iter {std::find(m_legal_moves.cbegin(), m_legal_moves.cend(), move)};

        if (iter == m_legal_moves.cend()) {
            throw BoardError("Illegal move");
        }

        switch (move.type) {
            case MoveType::Place:
                m_pieces[new_piece_to_place(m_position.player)].move(node_position(move.place.place_index));
                m_pieces[new_piece_to_place(m_position.player)].node_index = move.place.place_index;

                play_place_move(move);

                break;
            case MoveType::PlaceCapture:
                m_pieces[new_piece_to_place(m_position.player)].move(node_position(move.place_capture.place_index));
                m_pieces[new_piece_to_place(m_position.player)].node_index = move.place_capture.place_index;
                m_pieces[piece_on_node(iter->place_capture.capture_index)].move(ImVec2());
                m_pieces[piece_on_node(iter->place_capture.capture_index)].node_index = -1;

                play_place_capture_move(move);

                break;
            case MoveType::Move:
                m_pieces[piece_on_node(move.move.source_index)].move(node_position(move.move.destination_index));
                m_pieces[piece_on_node(move.move.source_index)].node_index = move.move.destination_index;

                play_move_move(move);

                break;
            case MoveType::MoveCapture:
                m_pieces[piece_on_node(move.move_capture.source_index)].move(node_position(move.move_capture.destination_index));
                m_pieces[piece_on_node(move.move_capture.source_index)].node_index = move.move_capture.destination_index;
                m_pieces[piece_on_node(iter->move_capture.capture_index)].move(ImVec2());
                m_pieces[piece_on_node(iter->move_capture.capture_index)].node_index = -1;

                play_move_capture_move(move);

                break;
        }
    }

    void Board::timeout(Player player) {
        switch (player) {
            case Player::White:
                m_game_over = GameOver::WinnerBlack;
                break;
            case Player::Black:
                m_game_over = GameOver::WinnerWhite;
                break;
        }
    }

    void Board::update_user_input() {
        if (!ImGui::IsWindowFocused()) {
            return;
        }

        if (m_game_over != GameOver::None) {
            return;
        }

        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            const int index {get_index(ImGui::GetMousePos())};

            if (index == -1) {
                return;
            }

            if (m_position.plies >= 18) {
                if (m_capture_piece) {
                    try_capture(index);
                } else {
                    try_move(m_select_index, index);
                    select(index);
                }
            } else {
                if (m_capture_piece) {
                    try_capture(index);
                } else {
                    try_place(index);
                }
            }
        }
    }

    void Board::select(int index) {
        if (m_select_index == -1) {
            if (m_position.board[index] == static_cast<Node>(m_position.player)) {
                m_select_index = index;
            }
        } else {
            if (index == m_select_index) {
                m_select_index = -1;
            } else if (m_position.board[index] == static_cast<Node>(m_position.player)) {
                m_select_index = index;
            }
        }
    }

    void Board::try_place(int place_index) {
        {
            const auto iter {std::find_if(m_legal_moves.cbegin(), m_legal_moves.cend(), [=](const Move& move) {
                return move.type == MoveType::Place && move.place.place_index == place_index;
            })};

            if (iter != m_legal_moves.cend()) {
                m_pieces[new_piece_to_place(m_position.player)].move(node_position(iter->place.place_index));
                m_pieces[new_piece_to_place(m_position.player)].node_index = iter->place.place_index;

                const Move move {*iter};
                play_place_move(move);

                return;
            }
        }

        m_candidate_moves.clear();

        std::copy_if(m_legal_moves.cbegin(), m_legal_moves.cend(), std::back_inserter(m_candidate_moves), [=](const Move& move) {
            return move.type == MoveType::PlaceCapture && move.place_capture.place_index == place_index;
        });

        if (!m_candidate_moves.empty()) {
            m_pieces[new_piece_to_place(m_position.player)].move(node_position(m_candidate_moves[0].place_capture.place_index));
            m_pieces[new_piece_to_place(m_position.player)].node_index = m_candidate_moves[0].place_capture.place_index;

            m_capture_piece = true;
        }
    }

    void Board::try_move(int source_index, int destination_index) {
        {
            const auto iter {std::find_if(m_legal_moves.cbegin(), m_legal_moves.cend(), [=](const Move& move) {
                return (
                    move.type == MoveType::Move &&
                    move.move.source_index == source_index &&
                    move.move.destination_index == destination_index
                );
            })};

            if (iter != m_legal_moves.cend()) {
                m_pieces[piece_on_node(iter->move.source_index)].move(node_position(iter->move.destination_index));
                m_pieces[piece_on_node(iter->move.source_index)].node_index = iter->move.destination_index;

                const Move move {*iter};
                play_move_move(move);

                return;
            }
        }

        m_candidate_moves.clear();

        std::copy_if(m_legal_moves.cbegin(), m_legal_moves.cend(), std::back_inserter(m_candidate_moves), [=](const Move& move) {
            return (
                move.type == MoveType::MoveCapture &&
                move.move_capture.source_index == source_index &&
                move.move_capture.destination_index == destination_index
            );
        });

        if (!m_candidate_moves.empty()) {
            m_pieces[piece_on_node(m_candidate_moves[0].move.source_index)].move(node_position(m_candidate_moves[0].move.destination_index));
            m_pieces[piece_on_node(m_candidate_moves[0].move.source_index)].node_index = m_candidate_moves[0].move.destination_index;

            m_capture_piece = true;
        }
    }

    void Board::try_capture(int capture_index) {
        const auto iter {std::find_if(m_candidate_moves.cbegin(), m_candidate_moves.cend(), [=](const Move& move) {
            switch (move.type) {
                case MoveType::PlaceCapture:
                    return move.place_capture.capture_index == capture_index;
                case MoveType::MoveCapture:
                    return move.move_capture.capture_index == capture_index;
                default:
                    assert(false);
                    break;
            }
        })};

        if (iter == m_candidate_moves.cend()) {
            return;
        }

        switch (iter->type) {
            case MoveType::PlaceCapture: {
                m_pieces[piece_on_node(iter->place_capture.capture_index)].move(ImVec2());
                m_pieces[piece_on_node(iter->place_capture.capture_index)].node_index = -1;

                const Move move {*iter};
                play_place_capture_move(move);

                break;
            }
            case MoveType::MoveCapture: {
                m_pieces[piece_on_node(iter->move_capture.capture_index)].move(ImVec2());
                m_pieces[piece_on_node(iter->move_capture.capture_index)].node_index = -1;

                const Move move {*iter};
                play_move_capture_move(move);

                break;
            }
            default:
                assert(false);
                break;
        }
    }

    void Board::play_place_move(const Move& move) {
        assert(move.type == MoveType::Place);
        assert(m_position.board[move.place.place_index] == Node::None);

        m_position.board[move.place.place_index] = static_cast<Node>(m_position.player);

        finish_turn();
        check_legal_moves();

        m_move_callback(move);
    }

    void Board::play_place_capture_move(const Move& move) {
        assert(move.type == MoveType::PlaceCapture);
        assert(m_position.board[move.place_capture.place_index] == Node::None);
        assert(m_position.board[move.place_capture.capture_index] != Node::None);

        m_position.board[move.place_capture.place_index] = static_cast<Node>(m_position.player);
        m_position.board[move.place_capture.capture_index] = Node::None;

        finish_turn();
        check_material();
        check_legal_moves();

        m_move_callback(move);
    }

    void Board::play_move_move(const Move& move) {
        assert(move.type == MoveType::Move);
        assert(m_position.board[move.move.source_index] != Node::None);
        assert(m_position.board[move.move.destination_index] == Node::None);

        std::swap(m_position.board[move.move.source_index], m_position.board[move.move.destination_index]);

        finish_turn(false);
        check_legal_moves();
        check_threefold_repetition();
        check_fifty_move_rule();

        m_move_callback(move);
    }

    void Board::play_move_capture_move(const Move& move) {
        assert(move.type == MoveType::MoveCapture);
        assert(m_position.board[move.move_capture.source_index] != Node::None);
        assert(m_position.board[move.move_capture.destination_index] == Node::None);
        assert(m_position.board[move.move_capture.capture_index] != Node::None);

        std::swap(m_position.board[move.move_capture.source_index], m_position.board[move.move_capture.destination_index]);
        m_position.board[move.move_capture.capture_index] = Node::None;

        finish_turn();
        check_material();
        check_legal_moves();

        m_move_callback(move);
    }

    void Board::finish_turn(bool advancement) {
        m_position.player = opponent(m_position.player);
        m_position.plies++;
        m_legal_moves = generate_moves();

        if (advancement) {
            m_plies_no_advancement = 0;
            m_positions.clear();
        } else {
            m_plies_no_advancement++;
        }

        // Store the current position anyway
        m_positions.push_back(m_position);

        m_capture_piece = false;
        m_select_index = -1;
    }

    void Board::check_material() {
        if (m_game_over != GameOver::None) {
            return;
        }

        if (m_position.plies < 18) {
            return;
        }

        if (count_pieces(m_position.board, m_position.player) < 3) {
            m_game_over = static_cast<GameOver>(opponent(m_position.player));
        }
    }

    void Board::check_legal_moves() {
        if (m_game_over != GameOver::None) {
            return;
        }

        if (m_legal_moves.empty()) {
            m_game_over = static_cast<GameOver>(opponent(m_position.player));
        }
    }

    void Board::check_fifty_move_rule() {
        if (m_game_over != GameOver::None) {
            return;
        }

        if (m_plies_no_advancement == 100) {
            m_game_over = GameOver::TieBetweenBothPlayers;
        }
    }

    void Board::check_threefold_repetition() {
        if (m_game_over != GameOver::None) {
            return;
        }

        const auto count {std::count(m_positions.cbegin(), m_positions.cend(), m_position)};

        assert(count >= 1);

        if (count == 3) {
            m_game_over = GameOver::TieBetweenBothPlayers;
        }
    }

    void Board::initialize_pieces() {
        for (int i {0}; i < 9; i++) {
            m_pieces[i] = PieceObj(Player::White);
        }

        for (int i {9}; i < 18; i++) {
            m_pieces[i] = PieceObj(Player::Black);
        }

        for (int i {0}; i < 24; i++) {
            switch (m_position.board[i]) {
                case Node::None:
                    break;
                case Node::White:
                    m_pieces[new_piece_to_place(Player::White)].move(node_position(i));
                    m_pieces[new_piece_to_place(Player::White)].node_index = i;
                    break;
                case Node::Black:
                    m_pieces[new_piece_to_place(Player::Black)].move(node_position(i));
                    m_pieces[new_piece_to_place(Player::Black)].node_index = i;
                    break;
            }
        }
    }

    int Board::new_piece_to_place(Player type) const {
        for (std::size_t i {0}; i < m_pieces.size(); i++) {
            if (m_pieces[i].get_type() == type && m_pieces[i].node_index == -1) {
                return static_cast<int>(i);
            }
        }

        assert(false);
        return {};
    }

    int Board::piece_on_node(int index) const {
        for (std::size_t i {0}; i < m_pieces.size(); i++) {
            if (m_pieces[i].node_index == index) {
                return static_cast<int>(i);
            }
        }

        assert(false);
        return {};
    }

    int Board::get_index(ImVec2 position) const {
        for (int i {0}; i < 24; i++) {
            if (point_in_circle(position, node_position(i), m_board_unit / NODE_RADIUS)) {
                return i;
            }
        }

        return -1;
    }

    ImVec2 Board::node_position(int index) const {
        return ImVec2(
            static_cast<float>(NODE_POSITIONS[index][0]) * m_board_unit + m_board_offset.x,
            static_cast<float>(NODE_POSITIONS[index][1]) * m_board_unit + m_board_offset.y
        );
    }

    bool Board::point_in_circle(ImVec2 point, ImVec2 circle, float radius) {
        const ImVec2 subtracted {circle.x - point.x, circle.y - point.y};
        const float length {std::pow(subtracted.x * subtracted.x + subtracted.y * subtracted.y, 0.5f)};

        return length < radius;
    }

    std::vector<Move> Board::generate_moves() const {
        std::vector<Move> moves;
        Board_ local_board {m_position.board};

        if (m_position.plies < 18) {
            generate_moves_phase1(local_board, moves, m_position.player);
        } else {
            if (count_pieces(local_board, m_position.player) == 3) {
                generate_moves_phase3(local_board, moves, m_position.player);
            } else {
                generate_moves_phase2(local_board, moves, m_position.player);
            }
        }

        return moves;
    }

    void Board::generate_moves_phase1(Board_& board, std::vector<Move>& moves, Player player) {
        for (int i {0}; i < 24; i++) {
            if (board[i] != Node::None) {
                continue;
            }

            make_place_move(board, player, i);

            if (is_mill(board, player, i)) {
                const Player opponent_player {opponent(player)};
                const bool all_in_mills {all_pieces_in_mills(board, opponent_player)};

                for (int j {0}; j < 24; j++) {
                    if (board[j] != static_cast<Node>(opponent_player)) {
                        continue;
                    }

                    if (is_mill(board, opponent_player, j) && !all_in_mills) {
                        continue;
                    }

                    moves.push_back(Move::create_place_capture(i, j));
                }
            } else {
                moves.push_back(Move::create_place(i));
            }

            unmake_place_move(board, i);
        }
    }

    void Board::generate_moves_phase2(Board_& board, std::vector<Move>& moves, Player player) {
        for (int i {0}; i < 24; i++) {
            if (board[i] != static_cast<Node>(player)) {
                continue;
            }

            const auto free_positions {neighbor_free_positions(board, i)};

            for (int j {0}; j < static_cast<int>(free_positions.size()); j++) {
                make_move_move(board, i, free_positions[j]);

                if (is_mill(board, player, free_positions[j])) {
                    const Player opponent_player {opponent(player)};
                    const bool all_in_mills {all_pieces_in_mills(board, opponent_player)};

                    for (int k {0}; k < 24; k++) {
                        if (board[k] != static_cast<Node>(opponent_player)) {
                            continue;
                        }

                        if (is_mill(board, opponent_player, k) && !all_in_mills) {
                            continue;
                        }

                        moves.push_back(Move::create_move_capture(i, free_positions[j], k));
                    }
                } else {
                    moves.push_back(Move::create_move(i, free_positions[j]));
                }

                unmake_move_move(board, i, free_positions[j]);
            }
        }
    }

    void Board::generate_moves_phase3(Board_& board, std::vector<Move>& moves, Player player) {
        for (int i {0}; i < 24; i++) {
            if (board[i] != static_cast<Node>(player)) {
                continue;
            }

            for (int j {0}; j < 24; j++) {
                if (board[j] != Node::None) {
                    continue;
                }

                make_move_move(board, i, j);

                if (is_mill(board, player, j)) {
                    const Player opponent_player {opponent(player)};
                    const bool all_in_mills {all_pieces_in_mills(board, opponent_player)};

                    for (int k {0}; k < 24; k++) {
                        if (board[k] != static_cast<Node>(opponent_player)) {
                            continue;
                        }

                        if (is_mill(board, opponent_player, k) && !all_in_mills) {
                            continue;
                        }

                        moves.push_back(Move::create_move_capture(i, j, k));
                    }
                } else {
                    moves.push_back(Move::create_move(i, j));
                }

                unmake_move_move(board, i, j);
            }
        }
    }

    void Board::make_place_move(Board_& board, Player player, int place_index) {
        assert(board[place_index] == Node::None);

        board[place_index] = static_cast<Node>(player);
    }

    void Board::unmake_place_move(Board_& board, int place_index) {
        assert(board[place_index] != Node::None);

        board[place_index] = Node::None;
    }

    void Board::make_move_move(Board_& board, int source_index, int destination_index) {
        assert(board[source_index] != Node::None);
        assert(board[destination_index] == Node::None);

        std::swap(board[source_index], board[destination_index]);
    }

    void Board::unmake_move_move(Board_& board, int source_index, int destination_index) {
        assert(board[source_index] == Node::None);
        assert(board[destination_index] != Node::None);

        std::swap(board[source_index], board[destination_index]);
    }

#ifdef __GNUG__
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wparentheses"
#endif

    bool Board::is_mill(const Board_& board, Player player, int index) {
        const Node node {static_cast<Node>(player)};

        assert(board[index] == node);

        switch (index) {
            case 0: return board[1] == node && board[2] == node || board[9] == node && board[21] == node;
            case 1: return board[0] == node && board[2] == node || board[4] == node && board[7] == node;
            case 2: return board[0] == node && board[1] == node || board[14] == node && board[23] == node;
            case 3: return board[4] == node && board[5] == node || board[10] == node && board[18] == node;
            case 4: return board[3] == node && board[5] == node || board[1] == node && board[7] == node;
            case 5: return board[3] == node && board[4] == node || board[13] == node && board[20] == node;
            case 6: return board[7] == node && board[8] == node || board[11] == node && board[15] == node;
            case 7: return board[6] == node && board[8] == node || board[1] == node && board[4] == node;
            case 8: return board[6] == node && board[7] == node || board[12] == node && board[17] == node;
            case 9: return board[0] == node && board[21] == node || board[10] == node && board[11] == node;
            case 10: return board[9] == node && board[11] == node || board[3] == node && board[18] == node;
            case 11: return board[9] == node && board[10] == node || board[6] == node && board[15] == node;
            case 12: return board[13] == node && board[14] == node || board[8] == node && board[17] == node;
            case 13: return board[12] == node && board[14] == node || board[5] == node && board[20] == node;
            case 14: return board[12] == node && board[13] == node || board[2] == node && board[23] == node;
            case 15: return board[16] == node && board[17] == node || board[6] == node && board[11] == node;
            case 16: return board[15] == node && board[17] == node || board[19] == node && board[22] == node;
            case 17: return board[15] == node && board[16] == node || board[8] == node && board[12] == node;
            case 18: return board[19] == node && board[20] == node || board[3] == node && board[10] == node;
            case 19: return board[18] == node && board[20] == node || board[16] == node && board[22] == node;
            case 20: return board[18] == node && board[19] == node || board[5] == node && board[13] == node;
            case 21: return board[22] == node && board[23] == node || board[0] == node && board[9] == node;
            case 22: return board[21] == node && board[23] == node || board[16] == node && board[19] == node;
            case 23: return board[21] == node && board[22] == node || board[2] == node && board[14] == node;
        }

        return {};
    }

#ifdef __GNUG__
    #pragma GCC diagnostic pop
#endif

    bool Board::all_pieces_in_mills(const Board_& board, Player player) {
        for (int i {0}; i < 24; i++) {
            if (board[i] != static_cast<Node>(player)) {
                continue;
            }

            if (!is_mill(board, player, i)) {
                return false;
            }
        }

        return true;
    }

#define IS_FREE_CHECK(const_index) \
    if (board[const_index] == Node::None) { \
        result.push_back(const_index); \
    }

    std::vector<int> Board::neighbor_free_positions(const Board_& board, int index) {
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

    unsigned int Board::count_pieces(const Board_& board, Player player) {
        int result {0};

        for (const Node node : board) {
            result += static_cast<int>(node == static_cast<Node>(player));
        }

        return result;
    }

    Player Board::opponent(Player player) {
        if (player == Player::White) {
            return Player::Black;
        } else {
            return Player::White;
        }
    }

    Move move_from_string(const std::string& string) {
        const auto tokens {split(string, "-x")};

        switch (tokens.size()) {
            case 1: {
                const auto place_index {index_from_string(tokens[0])};

                return Move::create_place(place_index);
            }
            case 2: {
                if (string.find('-') == string.npos) {
                    const auto place_index {index_from_string(tokens[0])};
                    const auto capture_index {index_from_string(tokens[1])};

                    return Move::create_place_capture(place_index, capture_index);
                } else {
                    const auto source_index {index_from_string(tokens[0])};
                    const auto destination_index {index_from_string(tokens[1])};

                    return Move::create_move(source_index, destination_index);
                }
            }
            case 3: {
                const auto source_index {index_from_string(tokens[0])};
                const auto destination_index {index_from_string(tokens[1])};
                const auto capture_index {index_from_string(tokens[2])};

                return Move::create_move_capture(source_index, destination_index, capture_index);
            }
        }

        throw BoardError("Invalid move string");
    }

    std::string move_to_string(const Move& move) {
        std::string result;

        switch (move.type) {
            case MoveType::Place:
                result += index_to_string(move.place.place_index);
                break;
            case MoveType::PlaceCapture:
                result += index_to_string(move.place_capture.place_index);
                result += 'x';
                result += index_to_string(move.place_capture.capture_index);
                break;
            case MoveType::Move:
                result += index_to_string(move.move.source_index);
                result += '-';
                result += index_to_string(move.move.destination_index);
                break;
            case MoveType::MoveCapture:
                result += index_to_string(move.move_capture.source_index);
                result += '-';
                result += index_to_string(move.move_capture.destination_index);
                result += 'x';
                result += index_to_string(move.move_capture.capture_index);
                break;
        }

        return result;
    }

    Position position_from_string(const std::string& string) {
        const std::regex re {R"(^(w|b):(w|b)([a-g][1-7])?(,[a-g][1-7])*:(w|b)([a-g][1-7])?(,[a-g][1-7])*:[0-9]{1,3}$)"};

        if (!std::regex_match(string, re)) {
            throw BoardError("Invalid position string");
        }

        const auto tokens {split(string, ":")};

        assert(tokens.size() == 4);

        const auto player {parse_player(tokens[0])};
        const auto pieces1 {parse_pieces(tokens[1])};
        const auto pieces2 {parse_pieces(tokens[2])};
        int turns {};

        try {
            turns = std::stoi(tokens[3]);
        } catch (...) {
            throw BoardError("Invalid position string");
        }

        if (pieces1.second == pieces2.second) {
            throw BoardError("Invalid position string");
        }

        if (turns < 1) {
            throw BoardError("Invalid position string");
        }

        Position position;

        position.player = player;

        for (const int index : pieces1.first) {
            assert(index >= 0 && index < 24);

            position.board[index] = static_cast<Node>(pieces1.second);
        }

        for (const int index : pieces2.first) {
            assert(index >= 0 && index < 24);

            position.board[index] = static_cast<Node>(pieces2.second);
        }

        position.plies = (turns - 1) * 2 + static_cast<unsigned int>(player == Player::Black);

        return position;
    }

    std::string position_to_string(const Position& position) {
        std::string result;

        switch (position.player) {
            case Player::White:
                result += 'w';
                break;
            case Player::Black:
                result += 'b';
                break;
        }

        result += ":w";
        for (int i {0}; i < 24; i++) {
            if (position.board[i] != Node::White) {
                continue;
            }

            result += index_to_string(i);
            result += ',';
        }

        if (result.back() == ',') {
            result.pop_back();
        }

        result += ":b";
        for (int i {0}; i < 24; i++) {
            if (position.board[i] != Node::Black) {
                continue;
            }

            result += index_to_string(i);
            result += ',';
        }

        if (result.back() == ',') {
            result.pop_back();
        }

        result += ':';
        result += std::to_string(position.plies / 2 + 1);

        return result;
    }
}
