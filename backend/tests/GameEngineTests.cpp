#include "GameEngine.hpp"

#include <cassert>
#include <iostream>

namespace {
void test_reset_changes_board_dimensions()
{
    clearbomb::GameEngine engine;
    const auto initial = engine.snapshot();

    engine.reset(clearbomb::BoardConfig{9, 9, 10});
    const auto updated = engine.snapshot();

    assert(updated.rows == 9);
    assert(updated.columns == 9);
    assert(updated.mines == 10);
    assert(updated.flags_remaining == 10);
    assert(updated.status == clearbomb::GameStatus::Playing);

    // Ensure the previous snapshot remains intact for comparison.
    assert(initial.rows != updated.rows || initial.columns != updated.columns || initial.mines != updated.mines);
}

void test_flagging_consistency()
{
    clearbomb::GameEngine engine;
    const auto snapshot = engine.snapshot();

    clearbomb::Position pos{0, 0};
    auto flag_result = engine.toggle_flag(pos);
    assert(flag_result.updated_cell.state == clearbomb::CellState::Flagged || flag_result.updated_cell.state == clearbomb::CellState::Hidden);

    if (flag_result.updated_cell.state == clearbomb::CellState::Flagged) {
        auto unflag_result = engine.toggle_flag(pos);
        assert(unflag_result.updated_cell.state == clearbomb::CellState::Hidden);
        assert(unflag_result.flags_remaining == snapshot.flags_remaining);
    }
}

}  // namespace

int main()
{
    test_reset_changes_board_dimensions();
    test_flagging_consistency();

    std::cout << "GameEngine smoke tests completed successfully." << std::endl;
    return 0;
}
