#include "GameEngine.hpp"
#include "Logger.hpp"

#include <algorithm>
#include <stdexcept>

namespace clearbomb {

namespace {
constexpr std::size_t kMinDimension = 2;
constexpr std::size_t kMaxDimension = 50;

const char* status_to_string(GameStatus status)
{
    switch (status) {
    case GameStatus::Playing:
        return "Playing";
    case GameStatus::Victory:
        return "Victory";
    case GameStatus::Defeat:
        return "Defeat";
    }
    return "Unknown";
}

BoardConfig make_config_from_board(const MinesweeperBoard& board)
{
    return BoardConfig{board.rows(), board.columns(), board.mine_count()};
}

std::vector<Cell> copy_cells(const std::vector<Cell>& cells)
{
    return cells;
}

std::size_t max_allowed_mines(std::size_t rows, std::size_t columns)
{
    if (rows < kMinDimension || rows > kMaxDimension || columns < kMinDimension || columns > kMaxDimension) {
        return 0;
    }

    const std::size_t total_cells = rows * columns;
    if (total_cells < 3) {
        return 0;
    }

    return total_cells - 2;
}

void validate_config(const BoardConfig& config)
{
    if (config.rows < kMinDimension || config.rows > kMaxDimension ||
        config.columns < kMinDimension || config.columns > kMaxDimension) {
        throw std::invalid_argument("Board dimensions must be between 2 and 50.");
    }

    if (config.mines == 0) {
        throw std::invalid_argument("Mine count must be at least 1.");
    }

    const std::size_t max_mines = max_allowed_mines(config.rows, config.columns);
    if (max_mines == 0 || config.mines > max_mines) {
        throw std::invalid_argument("Mine count must be at most rows * columns - 2.");
    }
}
}

GameEngine::GameEngine()
    : board_(std::make_unique<MinesweeperBoard>(16, 16, 40))
    , current_config_(make_config_from_board(*board_))
    , flags_remaining_(board_->mine_count())
{
    validate_config(current_config_);
    LOG_INFO(
        "GameEngine",
        "Initialized with default board " << current_config_.rows << 'x' << current_config_.columns << " ("
                                          << current_config_.mines << " mines)"
    );
}

GameEngine::GameEngine(std::unique_ptr<MinesweeperBoard> board)
    : board_(std::move(board))
    , current_config_(make_config_from_board(*board_))
    , flags_remaining_(board_->mine_count())
{
    if (!board_) {
        throw std::invalid_argument("GameEngine requires a valid board instance.");
    }
    LOG_INFO(
        "GameEngine",
        "Initialized with injected board " << current_config_.rows << 'x' << current_config_.columns << " ("
                                            << current_config_.mines << " mines)"
    );
}

RevealResult GameEngine::reveal_cell(Position position)
{
    LOG_DEBUG("GameEngine", "Reveal requested at (" << position.row << ',' << position.column << ")");

    if (game_over_) {
        LOG_WARNING(
            "GameEngine",
            "Reveal ignored because game already finished with status " << status_to_string(status_)
        );
        return RevealResult{{}, status_ == GameStatus::Defeat, status_ == GameStatus::Victory, flags_remaining_};
    }

    auto outcome = board_->reveal(position);
    auto updated_cells = std::move(outcome.revealed_cells);

    if (outcome.hit_mine) {
        status_ = GameStatus::Defeat;
        game_over_ = true;
        reveal_all_mines(updated_cells);
        LOG_WARNING(
            "GameEngine",
            "Mine detonated at (" << position.row << ',' << position.column
                                   << ") - game over with " << flags_remaining_ << " flags remaining"
        );
        return RevealResult{std::move(updated_cells), true, false, flags_remaining_};
    }

    if (board_->all_safe_cells_revealed()) {
        status_ = GameStatus::Victory;
        game_over_ = true;
        reveal_all_mines(updated_cells);
        LOG_INFO(
            "GameEngine",
            "All safe cells revealed - victory with " << flags_remaining_ << " flags remaining"
        );
        return RevealResult{std::move(updated_cells), false, true, flags_remaining_};
    }

    status_ = GameStatus::Playing;
    LOG_DEBUG(
        "GameEngine",
        "Reveal completed at (" << position.row << ',' << position.column << ") revealing "
                                 << updated_cells.size() << " cells"
    );
    return RevealResult{std::move(updated_cells), false, false, flags_remaining_};
}

FlagResult GameEngine::toggle_flag(Position position)
{
    LOG_DEBUG("GameEngine", "Toggle flag at (" << position.row << ',' << position.column << ")");

    if (game_over_) {
        LOG_WARNING(
            "GameEngine",
            "Flag toggle ignored because game already finished with status " << status_to_string(status_)
        );
        return FlagResult{board_->cell_at(position), flags_remaining_, status_ == GameStatus::Victory};
    }

    const Cell& current_cell = board_->cell_at(position);
    const bool was_flagged = current_cell.state == CellState::Flagged;

    if (!was_flagged && current_cell.state == CellState::Hidden && flags_remaining_ == 0) {
        LOG_WARNING(
            "GameEngine",
            "Flag toggle denied at (" << position.row << ',' << position.column << ") - no flags remaining"
        );
        return FlagResult{current_cell, flags_remaining_, status_ == GameStatus::Victory};
    }

    auto outcome = board_->toggle_flag(position);

    if (outcome.flag_added) {
        if (flags_remaining_ > 0) {
            --flags_remaining_;
        }
        LOG_INFO(
            "GameEngine",
            "Flag placed at (" << position.row << ',' << position.column << ") - flags remaining: "
                               << flags_remaining_
        );
    } else if (was_flagged && flags_remaining_ < current_config_.mines) {
        ++flags_remaining_;
        LOG_INFO(
            "GameEngine",
            "Flag removed at (" << position.row << ',' << position.column << ") - flags remaining: "
                               << flags_remaining_
        );
    }

    return FlagResult{outcome.updated_cell, flags_remaining_, status_ == GameStatus::Victory};
}

std::optional<AutoMarkResult> GameEngine::auto_mark(SelectionRect selection)
{
    LOG_DEBUG(
        "GameEngine",
        "Auto-mark requested for rect [" << selection.row_begin << ',' << selection.col_begin << "] -> ["
                                          << selection.row_end << ',' << selection.col_end << "]"
    );

    if (game_over_) {
        LOG_DEBUG(
            "GameEngine",
            "Auto-mark ignored because game already finished with status " << status_to_string(status_)
        );
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
        LOG_DEBUG("GameEngine", "Auto-mark selection contained no valid cells");
        return std::nullopt;
    }

    auto detected = auto_marker_.detect_certain_mines(*board_, std::move(selection_cells));
    if (!detected || detected->empty()) {
        LOG_DEBUG("GameEngine", "Auto-mark found no certain mines");
        return std::nullopt;
    }

    std::vector<Cell> flagged_cells;
    flagged_cells.reserve(detected->size());

    for (const auto& position : *detected) {
        const Cell& cell = board_->cell_at(position);
        if (cell.state != CellState::Hidden) {
            LOG_DEBUG(
                "GameEngine",
                "Auto-mark skipped non-hidden cell at (" << position.row << ',' << position.column << ")"
            );
            continue;
        }
        if (flags_remaining_ == 0) {
            LOG_WARNING("GameEngine", "Auto-mark stopped - no flags remaining");
            break;
        }
        auto outcome = board_->toggle_flag(position);
        if (outcome.flag_added) {
            --flags_remaining_;
            flagged_cells.push_back(outcome.updated_cell);
        }
    }

    if (flagged_cells.empty()) {
        LOG_DEBUG("GameEngine", "Auto-mark placed no new flags");
        return std::nullopt;
    }

    if (board_->all_safe_cells_revealed()) {
        status_ = GameStatus::Victory;
        game_over_ = true;
        reveal_all_mines(flagged_cells);
        LOG_INFO("GameEngine", "Victory achieved via auto-mark - all safe cells cleared");
    }

    LOG_INFO(
        "GameEngine",
        "Auto-mark placed " << flagged_cells.size() << " flag(s) - flags remaining: " << flags_remaining_
    );

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
    validate_config(next_config);
    board_ = std::make_unique<MinesweeperBoard>(next_config.rows, next_config.columns, next_config.mines);
    current_config_ = next_config;
    flags_remaining_ = next_config.mines;
    status_ = GameStatus::Playing;
    game_over_ = false;
    LOG_INFO(
        "GameEngine",
        "Board reset to " << next_config.rows << 'x' << next_config.columns << " with " << next_config.mines
                           << " mines"
    );
}

const MinesweeperBoard& GameEngine::board() const noexcept
{
    return *board_;
}

void GameEngine::reveal_all_mines(std::vector<Cell>& accumulator)
{
    std::size_t revealed_mines = 0;
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
            ++revealed_mines;
        }
    }
    LOG_DEBUG("GameEngine", "Revealed " << revealed_mines << " mine cells for end-of-game state");
}

}  // namespace clearbomb
