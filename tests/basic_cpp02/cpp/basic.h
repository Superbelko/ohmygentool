#pragma once

// ISSUE #4
inline int getIdx(int i)
{
    static const int idx[4] = { 0, 1, 0, -1 };
    return idx[i&0x03];
}