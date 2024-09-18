#pragma once

#include <array>
#include <vector>
#include <string>
#include <functional>

#include <gui_base/gui_base.hpp>

namespace board {
    enum class Player {
        White = 1,
        Black = 2
    };

    enum class MoveType {
        Place,
        PlaceTake,
        Move,
        MoveTake
    };

    enum class Piece {
        None,
        White,
        Black
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
                int take_index;
            } place_take;

            struct {
                int source_index;
                int destination_index;
            } move;

            struct {
                int source_index;
                int destination_index;
                int take_index;
            } move_take;
        };

        MoveType type {};
    };

    using Board = std::array<Piece, 24>;

    struct Position {
        Board board {};
        Player turn {};

        bool operator==(const Position& other) const {
            return board == other.board && turn == other.turn;
        }
    };

    class PieceObj {
    public:
        PieceObj() = default;
        PieceObj(Player type)
            : m_type(type) {}

        Player get_type() const { return m_type; }

        void update();
        void render(ImDrawList* draw_list, float m_board_unit);
        void move(ImVec2 target);

        int node_index {-1};
    private:
        Player m_type {};
        ImVec2 m_position;
        ImVec2 m_target;
        bool m_moving {false};
    };

    class MuhleBoard {
    public:
        MuhleBoard() = default;
        explicit MuhleBoard(std::function<void(const Move&)>&& move_callback);

        Player get_turn() const { return m_turn; }
        GameOver get_game_over() const { return m_game_over; }

        void update(bool user_input = true);
        void reset(const std::string& position_string = "");
        void debug() const;

        void place_piece(int place_index);
        void place_take_piece(int place_index, int take_index);
        void move_piece(int source_index, int destination_index);
        void move_take_piece(int source_index, int destination_index, int take_index);
    private:
        void update_user_input();
        void select(int index);

        void user_place(int place_index);
        void user_place_take_just_place(int place_index);
        void user_place_take(int place_index, int take_index);
        void user_move(int source_index, int destination_index);
        void user_move_take_just_move(int source_index, int destination_index);
        void user_move_take(int source_index, int destination_index, int take_index);

        void try_place(int place_index);
        void try_place_take(int place_index, int take_index);
        void try_move(int source_index, int destination_index);
        void try_move_take(int source_index, int destination_index, int take_index);

        // These just change the state
        void place(int place_index);
        void place_take(int place_index, int take_index);
        void move(int source_index, int destination_index);
        void move_take(int source_index, int destination_index, int take_index);

        void finish_turn(bool advancement = true);
        void check_winner_material();
        void check_winner_blocking();
        void check_fifty_move_rule();
        void check_threefold_repetition(const Position& position);

        void initialize_pieces();
        int new_piece_to_place(Player type) const;
        int piece_on_node(int index) const;
        int get_index(ImVec2 position) const;
        ImVec2 node_position(int index) const;
        static bool point_in_circle(ImVec2 point, ImVec2 circle, float radius);

        // Move generation
        std::vector<Move> generate_moves() const;
        static void generate_moves_phase1(Board& board, std::vector<Move>& moves, Player player);
        static void generate_moves_phase2(Board& board, std::vector<Move>& moves, Player player);
        static void generate_moves_phase3(Board& board, std::vector<Move>& moves, Player player);
        static void make_place_move(Board& board, Player player, int place_index);
        static void unmake_place_move(Board& board, int place_index);
        static void make_move_move(Board& board, int source_index, int destination_index);
        static void unmake_move_move(Board& board, int source_index, int destination_index);
        static bool is_mill(const Board& board, Player player, int index);
        static bool all_pieces_in_mills(const Board& board, Player player);
        static std::vector<int> neighbor_free_positions(const Board& board, int index);
        static Move create_place(int place_index);
        static Move create_place_take(int place_index, int take_index);
        static Move create_move(int source_index, int destination_index);
        static Move create_move_take(int source_index, int destination_index, int take_index);
        static unsigned int count_pieces(const Board& board, Player player);
        static Player opponent(Player player);

        // Game data
        Board m_board {};
        Player m_turn {Player::White};
        GameOver m_game_over {GameOver::None};
        unsigned int m_plies {};
        unsigned int m_plies_without_advancement {};
        std::vector<Position> m_positions;

        // GUI data
        float m_board_unit {};
        ImVec2 m_board_offset;
        int m_selected_index {-1};
        int m_take_action_index {-1};
        std::array<PieceObj, 18> m_pieces;
        std::vector<Move> m_legal_moves;
        std::function<void(const Move&)> m_move_callback;
    };

    Move move_from_string(const std::string& string);
    std::string string_from_move(const Move& move);
}
