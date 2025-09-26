#include "MinesweeperBoard.hpp"

#include <algorithm>
#include <random>
#include <stdexcept>

namespace clearbomb {

MinesweeperBoard::MinesweeperBoard(std::size_t rows, std::size_t columns, std::size_t mine_count)
    : rows_(rows)
    , columns_(columns)
    , mine_count_(mine_count)
{
    if (rows == 0 || columns == 0) {
        throw std::invalid_argument("Board dimensions must be positive.");
    }
    if (mine_count >= rows * columns) {
        throw std::invalid_argument("Mine count must be less than total cell count.");
    }
    cells_.resize(rows * columns);
    populate_board();
}

void MinesweeperBoard::regenerate()
{
    // TODO: Preserve difficulty settings while regenerating the board.
    populate_board();
}

bool MinesweeperBoard::reveal(Position position)
{
    // TODO: Implement flood fill reveal behavior.
    (void)position;
    return false;
}

bool MinesweeperBoard::toggle_flag(Position position)
{
    // TODO: Update cell state and validate flag limits.
    (void)position;
    return false;
}

const Cell& MinesweeperBoard::cell_at(Position position) const
{
    return cells_.at(index(position));
}

std::vector<Cell> MinesweeperBoard::neighbors(Position position) const
{
    // TODO: Return list of neighboring cells within board bounds.
    (void)position;
    return {};
}

std::size_t MinesweeperBoard::rows() const noexcept { return rows_; }
std::size_t MinesweeperBoard::columns() const noexcept { return columns_; }
std::size_t MinesweeperBoard::mine_count() const noexcept { return mine_count_; }

void MinesweeperBoard::populate_board()
{
    // TODO: Generate mines deterministically when a fixed seed is provided.
    std::vector<std::size_t> indices(rows_ * columns_);
    std::iota(indices.begin(), indices.end(), 0);
    std::shuffle(indices.begin(), indices.end(), std::mt19937{std::random_device{}()});

    for (std::size_t idx = 0; idx < cells_.size(); ++idx) {
        const bool is_mine = idx < mine_count_;
        cells_[indices[idx]] = Cell{
            .position = Position{indices[idx] / columns_, indices[idx] % columns_},
            .is_mine = is_mine,
            .adjacent_mines = 0,
            .state = CellState::Hidden
        };
    }

    // TODO: Compute adjacent mine counts after placement.
}

std::size_t MinesweeperBoard::index(Position position) const
{
    if (position.row >= rows_ || position.column >= columns_) {
        throw std::out_of_range("Position outside of board bounds.");
    }
    return position.row * columns_ + position.column;
}

}  // namespace clearbomb
