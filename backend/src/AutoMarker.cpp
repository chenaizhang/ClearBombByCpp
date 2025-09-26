#include "AutoMarker.hpp"

namespace clearbomb {

std::optional<std::vector<Position>> AutoMarker::detect_certain_mines(
    const MinesweeperBoard& board,
    std::vector<Position> selection_cells
) const
{
    // TODO: Analyze selection_cells and board state to find deterministic mine placements.
    (void)board;
    (void)selection_cells;
    return std::nullopt;
}

}  // namespace clearbomb
