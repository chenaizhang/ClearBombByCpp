#include "MinesweeperBoard.hpp"

#include <algorithm>
#include <numeric>
#include <stdexcept>

namespace clearbomb {

namespace {
constexpr int kNeighborOffsets[8][2] = {
    {-1, -1}, {-1, 0}, {-1, 1},
    {0, -1},           {0, 1},
    {1, -1},  {1, 0},  {1, 1}
};
}

MinesweeperBoard::MinesweeperBoard(std::size_t rows, std::size_t columns, std::size_t mine_count)
    : rows_(rows)
    , columns_(columns)
    , mine_count_(mine_count)
    , cells_(rows * columns)
    , rng_(std::random_device{}())
{
    if (rows == 0 || columns == 0) {
        throw std::invalid_argument("Board dimensions must be positive.");
    }
    if (mine_count == 0 || mine_count >= rows * columns) {
        throw std::invalid_argument("Mine count must be between 1 and total cell count - 1.");
    }

    populate_board();
}

RevealOutcome MinesweeperBoard::reveal(Position position)
{
    if (!in_bounds(position)) {
        throw std::out_of_range("Reveal position outside of board bounds.");
    }

    RevealOutcome outcome{};
    Cell& cell = cells_.at(index(position));

    if (cell.state == CellState::Flagged || cell.state == CellState::Revealed) {
        return outcome;
    }

    if (cell.is_mine) {
        cell.state = CellState::Revealed;
        cell.exploded = true;
        outcome.hit_mine = true;
        outcome.revealed_cells.push_back(cell);
        return outcome;
    }

    std::vector<bool> visited(cells_.size(), false);
    std::queue<Position> frontier;
    frontier.push(position);
    visited[index(position)] = true;

    while (!frontier.empty()) {
        const Position current = frontier.front();
        frontier.pop();

        Cell& current_cell = cells_.at(index(current));
        if (current_cell.state == CellState::Flagged) {
            continue;
        }
        if (current_cell.state != CellState::Revealed) {
            current_cell.state = CellState::Revealed;
            current_cell.exploded = false;
            ++revealed_safe_cells_;
            outcome.revealed_cells.push_back(current_cell);
        }

        if (current_cell.adjacent_mines != 0) {
            continue;
        }

        for (const auto& neighbor_cell : neighbors(current)) {
            const auto neighbor_index = index(neighbor_cell.position);
            if (visited[neighbor_index]) {
                continue;
            }
            visited[neighbor_index] = true;

            Cell& mutable_neighbor = cells_.at(neighbor_index);
            if (mutable_neighbor.is_mine || mutable_neighbor.state == CellState::Flagged) {
                continue;
            }

            frontier.push(neighbor_cell.position);
        }
    }

    return outcome;
}

ToggleOutcome MinesweeperBoard::toggle_flag(Position position)
{
    if (!in_bounds(position)) {
        throw std::out_of_range("Toggle position outside of board bounds.");
    }

    Cell& cell = cells_.at(index(position));
    if (cell.state == CellState::Revealed) {
        return ToggleOutcome{cell, false};
    }

    if (cell.state == CellState::Hidden) {
        cell.state = CellState::Flagged;
        cell.exploded = false;
        return ToggleOutcome{cell, true};
    }

    cell.state = CellState::Hidden;
    cell.exploded = false;
    return ToggleOutcome{cell, false};
}

const Cell& MinesweeperBoard::cell_at(Position position) const
{
    if (!in_bounds(position)) {
        throw std::out_of_range("Cell request outside of board bounds.");
    }
    return cells_.at(index(position));
}

Cell& MinesweeperBoard::mutable_cell(Position position)
{
    if (!in_bounds(position)) {
        throw std::out_of_range("Cell request outside of board bounds.");
    }
    return cells_.at(index(position));
}

const std::vector<Cell>& MinesweeperBoard::cells() const noexcept
{
    return cells_;
}

std::vector<Cell> MinesweeperBoard::neighbors(Position position) const
{
    std::vector<Cell> result;
    result.reserve(8);

    for (const auto& offset : kNeighborOffsets) {
        const long neighbor_row = static_cast<long>(position.row) + offset[0];
        const long neighbor_col = static_cast<long>(position.column) + offset[1];
        if (neighbor_row < 0 || neighbor_col < 0) {
            continue;
        }
        const Position neighbor{
            static_cast<std::size_t>(neighbor_row),
            static_cast<std::size_t>(neighbor_col)
        };
        if (!in_bounds(neighbor)) {
            continue;
        }

        result.push_back(cells_.at(index(neighbor)));
    }

    return result;
}

void MinesweeperBoard::resize(std::size_t rows, std::size_t columns, std::size_t mine_count)
{
    if (rows == 0 || columns == 0) {
        throw std::invalid_argument("Board dimensions must be positive.");
    }
    if (mine_count == 0 || mine_count >= rows * columns) {
        throw std::invalid_argument("Mine count must be between 1 and total cell count - 1.");
    }

    rows_ = rows;
    columns_ = columns;
    mine_count_ = mine_count;
    cells_.assign(rows * columns, Cell{});
    regenerate();
}

void MinesweeperBoard::regenerate()
{
    rng_.seed(std::random_device{}());
    populate_board();
}

std::size_t MinesweeperBoard::rows() const noexcept { return rows_; }
std::size_t MinesweeperBoard::columns() const noexcept { return columns_; }
std::size_t MinesweeperBoard::mine_count() const noexcept { return mine_count_; }
std::size_t MinesweeperBoard::revealed_safe_cells() const noexcept { return revealed_safe_cells_; }
std::size_t MinesweeperBoard::total_safe_cells() const noexcept { return rows_ * columns_ - mine_count_; }

bool MinesweeperBoard::all_safe_cells_revealed() const noexcept
{
    return revealed_safe_cells_ == total_safe_cells();
}

void MinesweeperBoard::populate_board()
{
    if (rows_ == 0 || columns_ == 0) {
        throw std::logic_error("Board dimensions must be set before population.");
    }

    std::vector<std::size_t> indices(rows_ * columns_);
    std::iota(indices.begin(), indices.end(), 0);
    std::shuffle(indices.begin(), indices.end(), rng_);

    std::vector<bool> mine_mask(rows_ * columns_, false);
    for (std::size_t i = 0; i < mine_count_; ++i) {
        mine_mask[indices[i]] = true;
    }

    for (std::size_t idx = 0; idx < cells_.size(); ++idx) {
        const bool is_mine = mine_mask[idx];
        cells_[idx] = Cell{
            .position = Position{idx / columns_, idx % columns_},
            .is_mine = is_mine,
            .adjacent_mines = 0,
            .state = CellState::Hidden,
            .exploded = false
        };
    }

    for (std::size_t idx = 0; idx < cells_.size(); ++idx) {
        if (!cells_[idx].is_mine) {
            continue;
        }

        const auto position = cells_[idx].position;
        for (const auto& offset : kNeighborOffsets) {
            const long neighbor_row = static_cast<long>(position.row) + offset[0];
            const long neighbor_col = static_cast<long>(position.column) + offset[1];
            if (neighbor_row < 0 || neighbor_col < 0) {
                continue;
            }
            const Position neighbor{
                static_cast<std::size_t>(neighbor_row),
                static_cast<std::size_t>(neighbor_col)
            };
            if (!in_bounds(neighbor)) {
                continue;
            }

            Cell& neighbor_cell = cells_.at(index(neighbor));
            if (!neighbor_cell.is_mine) {
                ++neighbor_cell.adjacent_mines;
            }
        }
    }

    revealed_safe_cells_ = 0;
}

std::size_t MinesweeperBoard::index(Position position) const
{
    if (!in_bounds(position)) {
        throw std::out_of_range("Index calculation outside of board bounds.");
    }
    return position.row * columns_ + position.column;
}

bool MinesweeperBoard::in_bounds(Position position) const noexcept
{
    return position.row < rows_ && position.column < columns_;
}

}  // namespace clearbomb
