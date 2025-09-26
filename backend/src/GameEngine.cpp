#include "GameEngine.hpp"

#include <algorithm>
#include <stdexcept>

namespace clearbomb {

namespace {
BoardConfig make_config_from_board(const MinesweeperBoard& board)
{
    return BoardConfig{board.rows(), board.columns(), board.mine_count()};
}

std::vector<Cell> copy_cells(const std::vector<Cell>& cells)
{
    return cells;
}
}

GameEngine::GameEngine()
    : board_(std::make_unique<MinesweeperBoard>(16, 16, 40))
    , current_config_(make_config_from_board(*board_))
    , flags_remaining_(board_->mine_count())
{
}

GameEngine::GameEngine(std::unique_ptr<MinesweeperBoard> board)
    : board_(std::move(board))
    , current_config_(make_config_from_board(*board_))
    , flags_remaining_(board_->mine_count())
{
    if (!board_) {
        throw std::invalid_argument("GameEngine requires a valid board instance.");
    }
}

RevealResult GameEngine::reveal_cell(Position position)
{
    if (game_over_) {
        return RevealResult{{}, status_ == GameStatus::Defeat, status_ == GameStatus::Victory, flags_remaining_};
    }

    auto outcome = board_->reveal(position);
    auto updated_cells = std::move(outcome.revealed_cells);

    if (outcome.hit_mine) {
        status_ = GameStatus::Defeat;
        game_over_ = true;
        reveal_all_mines(updated_cells);
        return RevealResult{std::move(updated_cells), true, false, flags_remaining_};
    }

    if (board_->all_safe_cells_revealed()) {
        status_ = GameStatus::Victory;
        game_over_ = true;
        reveal_all_mines(updated_cells);
        return RevealResult{std::move(updated_cells), false, true, flags_remaining_};
    }

    status_ = GameStatus::Playing;
    return RevealResult{std::move(updated_cells), false, false, flags_remaining_};
}

FlagResult GameEngine::toggle_flag(Position position)
{
    if (game_over_) {
        return FlagResult{board_->cell_at(position), flags_remaining_, status_ == GameStatus::Victory};
    }

    const Cell& current_cell = board_->cell_at(position);
    const bool was_flagged = current_cell.state == CellState::Flagged;

    if (!was_flagged && current_cell.state == CellState::Hidden && flags_remaining_ == 0) {
        return FlagResult{current_cell, flags_remaining_, status_ == GameStatus::Victory};
    }

    auto outcome = board_->toggle_flag(position);

    if (outcome.flag_added) {
        if (flags_remaining_ > 0) {
            --flags_remaining_;
        }
    } else if (was_flagged && flags_remaining_ < current_config_.mines) {
        ++flags_remaining_;
    }

    return FlagResult{outcome.updated_cell, flags_remaining_, status_ == GameStatus::Victory};
}

std::optional<AutoMarkResult> GameEngine::auto_mark(SelectionRect selection)
{
    if (game_over_) {
        return std::nullopt;
    }

    const auto row_begin = std::min(selection.row_begin, selection.row_end);
    const auto row_end = std::min<std::size_t>(std::max(selection.row_begin, selection.row_end), board_->rows() - 1);
    const auto col_begin = std::min(selection.col_begin, selection.col_end);
    const auto col_end = std::min<std::size_t>(std::max(selection.col_begin, selection.col_end), board_->columns() - 1);

    std::vector<Position> selection_cells;
    selection_cells.reserve((row_end - row_begin + 1) * (col_end - col_begin + 1));
    for (std::size_t row = row_begin; row <= row_end; ++row) {
        for (std::size_t col = col_begin; col <= col_end; ++col) {
            selection_cells.push_back(Position{row, col});
        }
    }

    if (selection_cells.empty()) {
        return std::nullopt;
    }

    auto detected = auto_marker_.detect_certain_mines(*board_, std::move(selection_cells));
    if (!detected || detected->empty()) {
        return std::nullopt;
    }

    std::vector<Cell> flagged_cells;
    flagged_cells.reserve(detected->size());

    for (const auto& position : *detected) {
        const Cell& cell = board_->cell_at(position);
        if (cell.state != CellState::Hidden) {
            continue;
        }
        if (flags_remaining_ == 0) {
            break;
        }
        auto outcome = board_->toggle_flag(position);
        if (outcome.flag_added) {
            --flags_remaining_;
            flagged_cells.push_back(outcome.updated_cell);
        }
    }

    if (flagged_cells.empty()) {
        return std::nullopt;
    }

    if (board_->all_safe_cells_revealed()) {
        status_ = GameStatus::Victory;
        game_over_ = true;
        reveal_all_mines(flagged_cells);
    }

    return AutoMarkResult{std::move(flagged_cells), flags_remaining_, status_ == GameStatus::Victory};
}

BoardSnapshot GameEngine::snapshot() const
{
    BoardSnapshot snap{
        .rows = board_->rows(),
        .columns = board_->columns(),
        .mines = board_->mine_count(),
        .flags_remaining = flags_remaining_,
        .status = status_,
        .cells = copy_cells(board_->cells())
    };
    return snap;
}

void GameEngine::reset(std::optional<BoardConfig> config)
{
    const BoardConfig next_config = config.value_or(current_config_);
    board_ = std::make_unique<MinesweeperBoard>(next_config.rows, next_config.columns, next_config.mines);
    current_config_ = next_config;
    flags_remaining_ = next_config.mines;
    status_ = GameStatus::Playing;
    game_over_ = false;
}

const MinesweeperBoard& GameEngine::board() const noexcept
{
    return *board_;
}

void GameEngine::reveal_all_mines(std::vector<Cell>& accumulator)
{
    for (std::size_t row = 0; row < board_->rows(); ++row) {
        for (std::size_t col = 0; col < board_->columns(); ++col) {
            Position pos{row, col};
            Cell& cell = board_->mutable_cell(pos);
            if (!cell.is_mine) {
                continue;
            }
            if (cell.state == CellState::Revealed) {
                continue;
            }
            cell.state = CellState::Revealed;
            cell.exploded = (status_ == GameStatus::Defeat);
            accumulator.push_back(cell);
        }
    }
}

}  // namespace clearbomb
