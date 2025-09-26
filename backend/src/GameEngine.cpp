#include "GameEngine.hpp"

#include <stdexcept>

namespace clearbomb {

GameEngine::GameEngine()
    : board_(std::make_unique<MinesweeperBoard>(16, 16, 40)) {}

GameEngine::GameEngine(std::unique_ptr<MinesweeperBoard> board)
    : board_(std::move(board))
{
    if (!board_) {
        throw std::invalid_argument("GameEngine requires a valid board instance.");
    }
}

RevealResult GameEngine::reveal_cell(Position position)
{
    // TODO: Forward reveal request to board_ and assemble RevealResult with diff of cells.
    (void)position;
    return RevealResult{.updated_cells = {}, .game_over = false};
}

bool GameEngine::toggle_flag(Position position)
{
    // TODO: Forward flag toggle to board_ and update metadata accordingly.
    (void)position;
    return false;
}

std::optional<std::vector<Position>> GameEngine::auto_mark(SelectionRect selection)
{
    // TODO: Translate selection into board cells and call auto_marker_.
    (void)selection;
    return std::nullopt;
}

void GameEngine::reset()
{
    board_->regenerate();
    // TODO: Reset timers, moves, and user statistics.
}

const MinesweeperBoard& GameEngine::board() const noexcept
{
    return *board_;
}

}  // namespace clearbomb
