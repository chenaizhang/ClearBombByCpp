#pragma once

#include <optional>
#include <vector>

#include "MinesweeperBoard.hpp"

namespace clearbomb {

class AutoMarker {
public:
    AutoMarker() = default;

    // TODO: Inject strategy options (probability thresholds, heuristics, AI difficulty).
    std::optional<std::vector<Position>> detect_certain_mines(
        const MinesweeperBoard& board,
        std::vector<Position> selection_cells
    ) const;
};

}  // namespace clearbomb
