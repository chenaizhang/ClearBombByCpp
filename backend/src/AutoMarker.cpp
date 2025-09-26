#include "AutoMarker.hpp"

#include <unordered_set>

namespace clearbomb {

namespace {
struct PositionHash {
    std::size_t operator()(const Position& position) const noexcept
    {
        return (position.row << 16) ^ position.column;
    }
};
}

std::optional<std::vector<Position>> AutoMarker::detect_certain_mines(
    const MinesweeperBoard& board,
    std::vector<Position> selection_cells
) const
{
    std::unordered_set<std::size_t> unique_indices;
    std::vector<Position> result;

    const auto rows = board.rows();
    const auto columns = board.columns();

    for (const auto& position : selection_cells) {
        if (position.row >= rows || position.column >= columns) {
            continue;
        }

        const Cell& cell = board.cell_at(position);
        if (cell.state != CellState::Revealed || cell.adjacent_mines <= 0) {
            continue;
        }

        const auto neighbors = board.neighbors(position);
        std::vector<Position> hidden_neighbors;
        std::size_t flagged_neighbors = 0;

        for (const auto& neighbor : neighbors) {
            if (neighbor.state == CellState::Hidden) {
                hidden_neighbors.push_back(neighbor.position);
            } else if (neighbor.state == CellState::Flagged) {
                ++flagged_neighbors;
            }
        }

        if (hidden_neighbors.empty()) {
            continue;
        }

        const auto remaining_mines = cell.adjacent_mines - static_cast<int>(flagged_neighbors);
        if (remaining_mines <= 0) {
            continue;
        }

        if (remaining_mines == static_cast<int>(hidden_neighbors.size())) {
            for (const auto& hidden : hidden_neighbors) {
                const auto index = hidden.row * columns + hidden.column;
                if (unique_indices.insert(index).second) {
                    result.push_back(hidden);
                }
            }
        }
    }

    if (result.empty()) {
        return std::nullopt;
    }

    return result;
}

}  // namespace clearbomb
