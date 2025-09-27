#pragma once

#include <cstddef>
#include <queue>
#include <random>
#include <vector>

namespace clearbomb {

struct Position {
    std::size_t row;
    std::size_t column;
};

enum class CellState {
    Hidden,
    Revealed,
    Flagged
};

struct Cell {
    Position position;
    bool is_mine;
    int adjacent_mines;
    CellState state;
    bool exploded;
};

struct RevealOutcome {
    std::vector<Cell> revealed_cells;
    bool hit_mine;
};

struct ToggleOutcome {
    Cell updated_cell;
    bool flag_added;
};

class MinesweeperBoard {
public:
    MinesweeperBoard(std::size_t rows, std::size_t columns, std::size_t mine_count);
    virtual ~MinesweeperBoard() = default;

    virtual RevealOutcome reveal(Position position);
    virtual ToggleOutcome toggle_flag(Position position);
    virtual const Cell& cell_at(Position position) const;
    virtual Cell& mutable_cell(Position position);
    virtual const std::vector<Cell>& cells() const noexcept;
    virtual std::vector<Cell> neighbors(Position position) const;

    virtual void resize(std::size_t rows, std::size_t columns, std::size_t mine_count);
    virtual void regenerate();
    virtual void ensure_safe_cell(Position position);

    std::size_t rows() const noexcept;
    std::size_t columns() const noexcept;
    std::size_t mine_count() const noexcept;
    std::size_t revealed_safe_cells() const noexcept;
    std::size_t total_safe_cells() const noexcept;
    bool all_safe_cells_revealed() const noexcept;

protected:
    std::size_t rows_;
    std::size_t columns_;
    std::size_t mine_count_;
    std::vector<Cell> cells_;
    std::mt19937 rng_;
    std::size_t revealed_safe_cells_ {0};

    void populate_board();
    std::size_t index(Position position) const;
    bool in_bounds(Position position) const noexcept;
};

}  // namespace clearbomb
