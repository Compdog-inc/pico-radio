// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

#ifndef _GUID_H_
#define _GUID_H_

#include <stdint.h>
#include <stdlib.h>
#include <string>

struct Guid
{
public:
    Guid();
    // Creates a new guid from an array of bytes.
    //
    Guid(const uint8_t b[16]);
    Guid(uint32_t a, uint16_t b, uint16_t c, uint8_t d, uint8_t e, uint8_t f, uint8_t g, uint8_t h, uint8_t i, uint8_t j, uint8_t k);
    Guid(int32_t a, int16_t b, int16_t c, const uint8_t d[8]);
    Guid(int32_t a, int16_t b, int16_t c, uint8_t d, uint8_t e, uint8_t f, uint8_t g, uint8_t h, uint8_t i, uint8_t j, uint8_t k);

    Guid(std::string str);

    static bool TryParse(std::string str, Guid *guid);

    std::string toString();

    bool equals(const Guid &other);

    inline bool operator==(const Guid &other) { return equals(other); }
    inline bool operator!=(const Guid &other) { return !equals(other); }

private:
    int32_t _a;
    int16_t _b;
    int16_t _c;
    uint8_t _d;
    uint8_t _e;
    uint8_t _f;
    uint8_t _g;
    uint8_t _h;
    uint8_t _i;
    uint8_t _j;
    uint8_t _k;
};

#endif