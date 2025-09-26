#pragma once

#include <cstddef>
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
};

class MinesweeperBoard {
public:
    MinesweeperBoard(std::size_t rows, std::size_t columns, std::size_t mine_count);
    virtual ~MinesweeperBoard() = default;

    // TODO: Provide factory helpers for deterministic board generation (seeded RNG).
    virtual void regenerate();
    virtual bool reveal(Position position);
    virtual bool toggle_flag(Position position);
    virtual const Cell& cell_at(Position position) const;
    virtual std::vector<Cell> neighbors(Position position) const;

    std::size_t rows() const noexcept;
    std::size_t columns() const noexcept;
    std::size_t mine_count() const noexcept;

protected:
    std::size_t rows_;
    std::size_t columns_;
    std::size_t mine_count_;
    std::vector<Cell> cells_;

    // TODO: Extract to dedicated generator once multiple difficulty rules are needed.
    void populate_board();
    std::size_t index(Position position) const;
};

}  // namespace clearbomb
