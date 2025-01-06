#pragma once

#include <array>
#include <vector>
#include <string>
#include <functional>
#include <stdexcept>
#include <optional>

#include <gui_base/gui_base.hpp>

namespace board {
    enum class Player {
        White = 1,
        Black = 2
    };

    enum class MoveType {
        Place,
        PlaceCapture,
        Move,
        MoveCapture
    };

    enum class Node {
        None = 0,
        White = 1,
        Black = 2
    };

    enum class GameOver {
        None,
        WinnerWhite,
        WinnerBlack,
        TieBetweenBothPlayers
    };

    struct Move {
        union {
            struct {
                int place_index;
            } place;

            struct {
                int place_index;
                int capture_index;
            } place_capture;

            struct {
                int source_index;
                int destination_index;
            } move;

            struct {
                int source_index;
                int destination_index;
                int capture_index;
            } move_capture;
        };

        MoveType type {};

        bool operator==(const Move& other) const;

        static Move create_place(int place_index);
        static Move create_place_capture(int place_index, int capture_index);
        static Move create_move(int source_index, int destination_index);
        static Move create_move_capture(int source_index, int destination_index, int capture_index);
    };

    using Board_ = std::array<Node, 24>;

    struct Position {
        Board_ board {};
        Player player {};
        unsigned int plies {};

        bool operator==(const Position& other) const {
            return board == other.board && player == other.player && plies >= 18 && other.plies >= 18;
        }
    };

    class PieceObj {
    public:
        PieceObj() = default;
        explicit PieceObj(Player type)
            : m_type(type) {}

        Player get_type() const { return m_type; }

        void update();
        void render(ImDrawList* draw_list, float board_unit);
        void move(ImVec2 target);

        int node_index {-1};
    private:
        Player m_type {};
        ImVec2 m_position;
        ImVec2 m_target;
        bool m_moving {false};
    };

    class Board {
    public:
        Board() = default;
        explicit Board(std::function<void(const Move&)>&& move_callback);

        Player get_player() const { return m_player; }
        GameOver get_game_over() const { return m_game_over; }

        void update(bool user_input = false);
        void reset(const std::optional<Position>& position);
        void debug() const;

        Position get_position() const;
        void play_move(const Move& move);
    private:
        void update_user_input();
        void select(int index);

        void try_place(int place_index);
        void try_move(int source_index, int destination_index);
        void try_capture(int capture_index);

        // These just change the state
        void play_place_move(int place_index);
        void play_place_capture_move(int place_index, int capture_index);
        void play_move_move(int source_index, int destination_index);
        void play_move_capture_move(int source_index, int destination_index, int capture_index);

        void finish_turn(bool advancement = true);
        void check_material();
        void check_legal_moves();
        void check_fifty_move_rule();
        void check_threefold_repetition();

        void initialize_pieces();
        int new_piece_to_place(Player type) const;
        int piece_on_node(int index) const;
        int get_index(ImVec2 position) const;
        ImVec2 node_position(int index) const;
        static bool point_in_circle(ImVec2 point, ImVec2 circle, float radius);

        // Move generation
        std::vector<Move> generate_moves() const;
        static void generate_moves_phase1(Board_& board, std::vector<Move>& moves, Player player);
        static void generate_moves_phase2(Board_& board, std::vector<Move>& moves, Player player);
        static void generate_moves_phase3(Board_& board, std::vector<Move>& moves, Player player);
        static void make_place_move(Board_& board, Player player, int place_index);
        static void unmake_place_move(Board_& board, int place_index);
        static void make_move_move(Board_& board, int source_index, int destination_index);
        static void unmake_move_move(Board_& board, int source_index, int destination_index);
        static bool is_mill(const Board_& board, Player player, int index);
        static bool all_pieces_in_mills(const Board_& board, Player player);
        static std::vector<int> neighbor_free_positions(const Board_& board, int index);
        static unsigned int count_pieces(const Board_& board, Player player);
        static Player opponent(Player player);

        // Game data
        Board_ m_board {};
        Player m_player {Player::White};
        unsigned int m_plies {};
        unsigned int m_plies_no_advancement {};
        GameOver m_game_over {GameOver::None};
        std::vector<Position> m_positions;

        // GUI data
        bool m_capture_piece {false};
        int m_select_index {-1};
        float m_board_unit {};
        ImVec2 m_board_offset;
        std::array<PieceObj, 18> m_pieces;
        std::vector<Move> m_legal_moves;
        std::vector<Move> m_candidate_moves;
        std::function<void(const Move&)> m_move_callback;
    };

    struct BoardError : std::runtime_error {
        explicit BoardError(const char* message)
            : std::runtime_error(message) {}
        explicit BoardError(const std::string& message)
            : std::runtime_error(message) {}
    };

    Move move_from_string(const std::string& string);
    std::string move_to_string(const Move& move);
    Position position_from_string(const std::string& string);
    std::string position_to_string(const Position& position);
}
