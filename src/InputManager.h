#pragma once
#include "Board.h"
#include <functional>

class InputManager {
public:
    using SwapCallback = std::function<void(int,int,int,int)>;

    void init();
    void setSwapCallback(SwapCallback cb) { swapCb = cb; }

    // Call from GLFW mouse button callback — row,col already computed via ray picking
    void onMouseButton(int button, int action, int row, int col);

    int selRow = -1, selCol = -1; // currently selected tile (-1 = none)
    bool inputBlocked = false;    // set by Game during swap animations

private:
    SwapCallback swapCb;

    bool inBounds(int r, int c) const {
        return r >= 0 && r < BOARD_ROWS && c >= 0 && c < BOARD_COLS;
    }
};
