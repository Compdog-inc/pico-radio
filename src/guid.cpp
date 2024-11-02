// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==

#include <pico/stdlib.h>
#include <pico/rand.h>
#include "guid.h"
#include "sha1.hpp"

using namespace std::literals;

Guid::Guid()
{
    _a = 0;
    _b = 0;
    _c = 0;
    _d = 0;
    _e = 0;
    _f = 0;
    _g = 0;
    _h = 0;
    _i = 0;
    _j = 0;
    _k = 0;
}

Guid::Guid(const uint8_t b[16])
{
    _a = ((int32_t)b[3] << 24) | ((int32_t)b[2] << 16) | ((int32_t)b[1] << 8) | b[0];
    _b = (int16_t)(((int32_t)b[5] << 8) | b[4]);
    _c = (int16_t)(((int32_t)b[7] << 8) | b[6]);
    _d = b[8];
    _e = b[9];
    _f = b[10];
    _g = b[11];
    _h = b[12];
    _i = b[13];
    _j = b[14];
    _k = b[15];
}

Guid::Guid(uint32_t a, uint16_t b, uint16_t c, uint8_t d, uint8_t e, uint8_t f, uint8_t g, uint8_t h, uint8_t i, uint8_t j, uint8_t k)
{
    _a = (int32_t)a;
    _b = (int16_t)b;
    _c = (int16_t)c;
    _d = d;
    _e = e;
    _f = f;
    _g = g;
    _h = h;
    _i = i;
    _j = j;
    _k = k;
}

Guid::Guid(int32_t a, int16_t b, int16_t c, const uint8_t d[8])
{
    _a = a;
    _b = b;
    _c = c;
    _d = d[0];
    _e = d[1];
    _f = d[2];
    _g = d[3];
    _h = d[4];
    _i = d[5];
    _j = d[6];
    _k = d[7];
}

Guid::Guid(int32_t a, int16_t b, int16_t c, uint8_t d, uint8_t e, uint8_t f, uint8_t g, uint8_t h, uint8_t i, uint8_t j, uint8_t k)
{
    _a = a;
    _b = b;
    _c = c;
    _d = d;
    _e = e;
    _f = f;
    _g = g;
    _h = h;
    _i = i;
    _j = j;
    _k = k;
}

Guid Guid::NewGuid()
{
    Guid guid = {};
    get_rand_128((rng_128_t *)&guid); // populate 128-bit GUID with random values
    return guid;
}

std::string Guid::toString()
{
    return base64_encode((uint8_t *)this, 16);
}

bool Guid::equals(const Guid &other) const
{
    // Compare each of the elements
    if (other._a != _a)
        return false;
    if (other._b != _b)
        return false;
    if (other._c != _c)
        return false;
    if (other._d != _d)
        return false;
    if (other._e != _e)
        return false;
    if (other._f != _f)
        return false;
    if (other._g != _g)
        return false;
    if (other._h != _h)
        return false;
    if (other._i != _i)
        return false;
    if (other._j != _j)
        return false;
    if (other._k != _k)
        return false;

    return true;
}