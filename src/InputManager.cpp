#include "InputManager.h"

void InputManager::init() {}

void InputManager::onMouseButton(int button, int action, int row, int col) {
    if (button != 0 || action != 1) return; // left press only
    if (inputBlocked) return; // block input during swap animation

    if (!inBounds(row, col)) {
        // Click outside board → deselect
        selRow = -1;
        selCol = -1;
        return;
    }

    if (selRow == -1) {
        // First selection
        selRow = row;
        selCol = col;
    } else {
        // Second selection → fire swap callback
        if (swapCb) swapCb(selRow, selCol, row, col);
        selRow = -1;
        selCol = -1;
    }
}

