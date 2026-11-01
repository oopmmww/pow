#pragma once
#include <cstddef>

struct font_blob
{
    const unsigned char* data = nullptr;
    size_t size = 0;

    bool operator==(const font_blob& other) const
    {
        return data == other.data && size == other.size;
    }
};

extern const font_blob icons;
extern const font_blob syne;
extern const font_blob sf_pro;
extern const font_blob lineseed_th;
