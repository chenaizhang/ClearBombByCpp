#pragma once

#include <optional>
#include <vector>

#include "MinesweeperBoard.hpp"

namespace clearbomb {

class AutoMarker {
public:
    AutoMarker() = default;

    std::optional<std::vector<Position>> detect_certain_mines(
        const MinesweeperBoard& board,
        std::vector<Position> selection_cells
    ) const;
};

}  // namespace clearbomb
