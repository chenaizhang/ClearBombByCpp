#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

#include "MinesweeperBoard.hpp"
#include "AutoMarker.hpp"

namespace clearbomb {

struct SelectionRect {
    std::size_t row_begin;
    std::size_t col_begin;
    std::size_t row_end;
    std::size_t col_end;
};

struct RevealResult {
    // TODO: Expand result payload with cell updates, game-over state, timers, etc.
    std::vector<CellState> updated_cells;
    bool game_over;
};

class GameEngine {
public:
    GameEngine();

    // TODO: Allow seeding RNGs and customizing dimensions/mines via constructor.
    explicit GameEngine(std::unique_ptr<MinesweeperBoard> board);

    RevealResult reveal_cell(Position position);
    bool toggle_flag(Position position);
    std::optional<std::vector<Position>> auto_mark(SelectionRect selection);
    void reset();

    const MinesweeperBoard& board() const noexcept;

private:
    std::unique_ptr<MinesweeperBoard> board_;
    AutoMarker auto_marker_;

    // TODO: Track additional metadata (elapsed time, flag counts, deterministic replay info).
};

}  // namespace clearbomb
