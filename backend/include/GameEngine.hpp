#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

#include "AutoMarker.hpp"
#include "MinesweeperBoard.hpp"

namespace clearbomb {

struct SelectionRect {
    std::size_t row_begin;
    std::size_t col_begin;
    std::size_t row_end;
    std::size_t col_end;
};

enum class GameStatus {
    Playing,
    Victory,
    Defeat
};

struct RevealResult {
    std::vector<Cell> updated_cells;
    bool hit_mine;
    bool victory;
    std::size_t flags_remaining;
};

struct FlagResult {
    Cell updated_cell;
    std::size_t flags_remaining;
    bool victory;
};

struct AutoMarkResult {
    std::vector<Cell> flagged_cells;
    std::size_t flags_remaining;
    bool victory;
};

struct BoardSnapshot {
    std::size_t rows;
    std::size_t columns;
    std::size_t mines;
    std::size_t flags_remaining;
    GameStatus status;
    std::vector<Cell> cells;
};

struct BoardConfig {
    std::size_t rows;
    std::size_t columns;
    std::size_t mines;
};

class GameEngine {
public:
    GameEngine();
    explicit GameEngine(std::unique_ptr<MinesweeperBoard> board);

    RevealResult reveal_cell(Position position);
    FlagResult toggle_flag(Position position);
    std::optional<AutoMarkResult> auto_mark(SelectionRect selection);
    BoardSnapshot snapshot() const;

    void reset(std::optional<BoardConfig> config = std::nullopt);
    const MinesweeperBoard& board() const noexcept;

private:
    std::unique_ptr<MinesweeperBoard> board_;
    AutoMarker auto_marker_;
    BoardConfig current_config_;
    std::size_t flags_remaining_ {0};
    bool game_over_ {false};
    GameStatus status_ {GameStatus::Playing};

    void reveal_all_mines(std::vector<Cell>& accumulator);
};
;

}  // namespace clearbomb
