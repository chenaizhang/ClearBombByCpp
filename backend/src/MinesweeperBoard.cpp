#include "MinesweeperBoard.hpp"
#include "Logger.hpp"

#include <algorithm>
#include <chrono>
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
        LOG_ERROR(
            "MinesweeperBoard",
            "Board creation failed - non-positive dimensions " << rows << 'x' << columns
        );
        throw std::invalid_argument("Board dimensions must be positive.");
    }
    if (mine_count == 0 || mine_count >= rows * columns) {
        LOG_ERROR(
            "MinesweeperBoard",
            "Board creation failed - invalid mine count " << mine_count << " for " << rows * columns
        );
        throw std::invalid_argument("Mine count must be between 1 and total cell count - 1.");
    }

    const auto start = std::chrono::steady_clock::now();
    populate_board();
    const auto end = std::chrono::steady_clock::now();
    const auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    LOG_INFO(
        "MinesweeperBoard",
        "Board populated " << rows_ << 'x' << columns_ << " with " << mine_count_ << " mines in "
                            << duration_ms << " ms"
    );
}

RevealOutcome MinesweeperBoard::reveal(Position position)
{
    if (!in_bounds(position)) {
        LOG_ERROR(
            "MinesweeperBoard",
            "Reveal request out of bounds at (" << position.row << ',' << position.column << ")"
        );
        throw std::out_of_range("Reveal position outside of board bounds.");
    }

    LOG_DEBUG("MinesweeperBoard", "Reveal processing at (" << position.row << ',' << position.column << ")");

    RevealOutcome outcome{};
    Cell& cell = cells_.at(index(position));

    if (cell.state == CellState::Flagged || cell.state == CellState::Revealed) {
        LOG_DEBUG(
            "MinesweeperBoard",
            "Reveal ignored due to cell already in state "
                << (cell.state == CellState::Flagged ? "Flagged" : "Revealed")
        );
        return outcome;
    }

    if (cell.is_mine) {
        cell.state = CellState::Revealed;
        cell.exploded = true;
        outcome.hit_mine = true;
        outcome.revealed_cells.push_back(cell);
        LOG_WARNING(
            "MinesweeperBoard",
            "Mine revealed at (" << position.row << ',' << position.column << ")"
        );
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
            LOG_DEBUG(
                "MinesweeperBoard",
                "Skipping expansion from flagged cell at (" << current.row << ',' << current.column << ")"
            );
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

    LOG_DEBUG(
        "MinesweeperBoard",
        "Reveal finished at (" << position.row << ',' << position.column << ") exposing "
                               << outcome.revealed_cells.size() << " cells"
    );
    return outcome;
}

ToggleOutcome MinesweeperBoard::toggle_flag(Position position)
{
    if (!in_bounds(position)) {
        LOG_ERROR(
            "MinesweeperBoard",
            "Flag toggle out of bounds at (" << position.row << ',' << position.column << ")"
        );
        throw std::out_of_range("Toggle position outside of board bounds.");
    }

    Cell& cell = cells_.at(index(position));
    if (cell.state == CellState::Revealed) {
        LOG_DEBUG(
            "MinesweeperBoard",
            "Flag toggle ignored - cell already revealed at (" << position.row << ',' << position.column << ")"
        );
        return ToggleOutcome{cell, false};
    }

    if (cell.state == CellState::Hidden) {
        cell.state = CellState::Flagged;
        cell.exploded = false;
        LOG_DEBUG(
            "MinesweeperBoard",
            "Flag placed at (" << position.row << ',' << position.column << ")"
        );
        return ToggleOutcome{cell, true};
    }

    cell.state = CellState::Hidden;
    cell.exploded = false;
    LOG_DEBUG(
        "MinesweeperBoard",
        "Flag removed at (" << position.row << ',' << position.column << ")"
    );
    return ToggleOutcome{cell, false};
}

const Cell& MinesweeperBoard::cell_at(Position position) const
{
    if (!in_bounds(position)) {
        LOG_ERROR(
            "MinesweeperBoard",
            "Cell access out of bounds at (" << position.row << ',' << position.column << ")"
        );
        throw std::out_of_range("Cell request outside of board bounds.");
    }
    return cells_.at(index(position));
}

Cell& MinesweeperBoard::mutable_cell(Position position)
{
    if (!in_bounds(position)) {
        LOG_ERROR(
            "MinesweeperBoard",
            "Mutable cell access out of bounds at (" << position.row << ',' << position.column << ")"
        );
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
        LOG_ERROR(
            "MinesweeperBoard",
            "Resize rejected - non-positive dimensions " << rows << 'x' << columns
        );
        throw std::invalid_argument("Board dimensions must be positive.");
    }
    if (mine_count == 0 || mine_count >= rows * columns) {
        LOG_ERROR(
            "MinesweeperBoard",
            "Resize rejected - invalid mine count " << mine_count << " for " << rows * columns
        );
        throw std::invalid_argument("Mine count must be between 1 and total cell count - 1.");
    }

    rows_ = rows;
    columns_ = columns;
    mine_count_ = mine_count;
    cells_.assign(rows * columns, Cell{});
    regenerate();
    LOG_INFO(
        "MinesweeperBoard",
        "Board resized to " << rows_ << 'x' << columns_ << " with " << mine_count_ << " mines"
    );
}

void MinesweeperBoard::regenerate()
{
    rng_.seed(std::random_device{}());
    populate_board();
    LOG_DEBUG(
        "MinesweeperBoard",
        "Board regenerated with layout shuffle - mine count " << mine_count_
    );
}

void MinesweeperBoard::ensure_safe_cell(Position position)
{
    if (!in_bounds(position)) {
        LOG_ERROR(
            "MinesweeperBoard",
            "Safe-cell request out of bounds at (" << position.row << ',' << position.column << ")"
        );
        throw std::out_of_range("Safe-cell request outside of board bounds.");
    }

    const std::size_t target_index = index(position);
    Cell& target_cell = cells_.at(target_index);
    if (!target_cell.is_mine) {
        return;
    }

    std::size_t replacement_index = cells_.size();
    for (std::size_t idx = 0; idx < cells_.size(); ++idx) {
        if (!cells_[idx].is_mine) {
            replacement_index = idx;
            break;
        }
    }

    if (replacement_index == cells_.size()) {
        LOG_CRITICAL(
            "MinesweeperBoard",
            "Unable to relocate mine from (" << position.row << ',' << position.column
                                            << ") - no safe cells available"
        );
        return;
    }

    const auto adjust_neighbors = [&](Position center, int delta) {
        for (const auto& offset : kNeighborOffsets) {
            const long neighbor_row = static_cast<long>(center.row) + offset[0];
            const long neighbor_col = static_cast<long>(center.column) + offset[1];
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
            if (neighbor_cell.is_mine) {
                continue;
            }
            neighbor_cell.adjacent_mines += delta;
            if (neighbor_cell.adjacent_mines < 0) {
                neighbor_cell.adjacent_mines = 0;
            }
        }
    };

    const auto recompute_adjacency = [&](Position center) {
        int count = 0;
        for (const auto& offset : kNeighborOffsets) {
            const long neighbor_row = static_cast<long>(center.row) + offset[0];
            const long neighbor_col = static_cast<long>(center.column) + offset[1];
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
            if (cells_.at(index(neighbor)).is_mine) {
                ++count;
            }
        }
        return count;
    };

    const Position replacement_position = cells_.at(replacement_index).position;

    adjust_neighbors(position, -1);

    target_cell.is_mine = false;
    target_cell.state = CellState::Hidden;
    target_cell.exploded = false;
    target_cell.adjacent_mines = recompute_adjacency(position);

    adjust_neighbors(replacement_position, +1);

    Cell& replacement_cell = cells_.at(replacement_index);
    replacement_cell.is_mine = true;
    replacement_cell.adjacent_mines = 0;
    replacement_cell.state = CellState::Hidden;
    replacement_cell.exploded = false;

    LOG_DEBUG(
        "MinesweeperBoard",
        "Relocated mine from (" << position.row << ',' << position.column << ") to ("
                                << replacement_position.row << ',' << replacement_position.column << ')'
    );
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
        LOG_CRITICAL("MinesweeperBoard", "Populate called without valid dimensions");
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
    LOG_DEBUG(
        "MinesweeperBoard",
        "Board population complete - " << mine_count_ << " mines distributed"
    );
}

std::size_t MinesweeperBoard::index(Position position) const
{
    if (!in_bounds(position)) {
        LOG_ERROR(
            "MinesweeperBoard",
            "Index request out of bounds at (" << position.row << ',' << position.column << ")"
        );
        throw std::out_of_range("Index calculation outside of board bounds.");
    }
    return position.row * columns_ + position.column;
}

bool MinesweeperBoard::in_bounds(Position position) const noexcept
{
    return position.row < rows_ && position.column < columns_;
}

}  // namespace clearbomb
