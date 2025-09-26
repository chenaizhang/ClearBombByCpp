#include "AutoMarker.hpp"
#include "Logger.hpp"

#include <unordered_set>

namespace clearbomb {

namespace {
struct PositionHash {
    std::size_t operator()(const Position& position) const noexcept
    {
        return (position.row << 16) ^ position.column;
    }
};

const char* cell_state_name(CellState state)
{
    switch (state) {
    case CellState::Hidden:
        return "Hidden";
    case CellState::Revealed:
        return "Revealed";
    case CellState::Flagged:
        return "Flagged";
    }
    return "Unknown";
}
}

std::optional<std::vector<Position>> AutoMarker::detect_certain_mines(
    const MinesweeperBoard& board,
    std::vector<Position> selection_cells
) const
{
    LOG_DEBUG(
        "AutoMarker",
        "Detecting certain mines within selection of " << selection_cells.size() << " cells"
    );

    std::unordered_set<std::size_t> unique_indices;
    std::vector<Position> result;

    const auto rows = board.rows();
    const auto columns = board.columns();

    for (const auto& position : selection_cells) {
        if (position.row >= rows || position.column >= columns) {
            LOG_DEBUG(
                "AutoMarker",
                "Skipping out-of-bounds cell (" << position.row << ',' << position.column << ")"
            );
            continue;
        }

        const Cell& cell = board.cell_at(position);
        if (cell.state != CellState::Revealed || cell.adjacent_mines <= 0) {
            LOG_DEBUG(
                "AutoMarker",
                "Skipping cell (" << position.row << ',' << position.column
                                   << ") state=" << cell_state_name(cell.state)
                                   << " adjacent=" << cell.adjacent_mines
            );
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
            LOG_DEBUG(
                "AutoMarker",
                "Cell (" << position.row << ',' << position.column << ") has no hidden neighbors"
            );
            continue;
        }

        const auto remaining_mines = cell.adjacent_mines - static_cast<int>(flagged_neighbors);
        if (remaining_mines <= 0) {
            LOG_DEBUG(
                "AutoMarker",
                "All mines already flagged around cell (" << position.row << ',' << position.column << ")"
            );
            continue;
        }

        if (remaining_mines == static_cast<int>(hidden_neighbors.size())) {
            for (const auto& hidden : hidden_neighbors) {
                const auto index = hidden.row * columns + hidden.column;
                if (unique_indices.insert(index).second) {
                    result.push_back(hidden);
                }
            }
            LOG_DEBUG(
                "AutoMarker",
                "Marked " << hidden_neighbors.size() << " certain mine(s) around (" << position.row << ','
                           << position.column << ")"
            );
        }
    }

    if (result.empty()) {
        LOG_DEBUG("AutoMarker", "No certain mines found in selection");
        return std::nullopt;
    }

    LOG_INFO("AutoMarker", "Detected " << result.size() << " mine(s) with certainty");
    return result;
}

}  // namespace clearbomb
