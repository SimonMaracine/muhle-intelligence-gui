#pragma once

#include <array>
#include <vector>
#include <string_view>
#include <functional>

#include <gui_base/gui_base.hpp>

namespace board {
    using Idx = int;

    inline constexpr Idx NULL_INDEX {-1};

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
                Idx place_index;
            } place;

            struct {
                Idx place_index;
                Idx take_index;
            } place_take;

            struct {
                Idx source_index;
                Idx destination_index;
            } move;

            struct {
                Idx source_index;
                Idx destination_index;
                Idx take_index;
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

    class MuhleBoard {
    public:
        MuhleBoard() = default;
        explicit MuhleBoard(std::function<void(const Move&)>&& move_callback);

        Player get_turn() const { return m_turn; }
        GameOver get_game_over() const { return m_game_over; }

        void update(bool user_input = true);
        void reset(std::string_view position_string = "");
        void debug() const;

        void place(Idx place_index);
        void place_take(Idx place_index, Idx take_index);
        void move(Idx source_index, Idx destination_index);
        void move_take(Idx source_index, Idx destination_index, Idx take_index);
    private:
        void update_user_input();
        void select(Idx index);
        void try_place(Idx place_index);
        void try_place_take(Idx place_index, Idx take_index);
        void try_move(Idx source_index, Idx destination_index);
        void try_move_take(Idx source_index, Idx destination_index, Idx take_index);
        void finish_turn(bool advancement = true);
        void check_winner_material();
        void check_winner_blocking();
        void check_fifty_move_rule();
        void check_threefold_repetition(const Position& position);

        static bool point_in_circle(ImVec2 point, ImVec2 circle, float radius);
        Idx get_index(ImVec2 position) const;

        // Move generation
        std::vector<Move> generate_moves() const;
        static void generate_moves_phase1(Board& board, std::vector<Move>& moves, Player player);
        static void generate_moves_phase2(Board& board, std::vector<Move>& moves, Player player);
        static void generate_moves_phase3(Board& board, std::vector<Move>& moves, Player player);
        static void make_place_move(Board& board, Player player, Idx place_index);
        static void unmake_place_move(Board& board, Idx place_index);
        static void make_move_move(Board& board, Idx source_index, Idx destination_index);
        static void unmake_move_move(Board& board, Idx source_index, Idx destination_index);
        static bool is_mill(const Board& board, Player player, Idx index);
        static bool all_pieces_in_mills(const Board& board, Player player);
        static std::vector<Idx> neighbor_free_positions(const Board& board, Idx index);
        static Move create_place(Idx place_index);
        static Move create_place_take(Idx place_index, Idx take_index);
        static Move create_move(Idx source_index, Idx destination_index);
        static Move create_move_take(Idx source_index, Idx destination_index, Idx take_index);
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
        ImVec2 m_board_offset {};
        Idx m_selected_index {NULL_INDEX};
        Idx m_take_action_index {NULL_INDEX};
        std::vector<Move> m_legal_moves;
        std::function<void(const Move&)> m_move_callback;
    };

    Move move_from_string(std::string_view string);
    std::string move_to_string(const Move& move);
}
