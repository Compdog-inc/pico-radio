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
    friend struct std::hash<Guid>;

    Guid();
    // Creates a new guid from an array of bytes.
    //
    Guid(const uint8_t b[16]);
    Guid(uint32_t a, uint16_t b, uint16_t c, uint8_t d, uint8_t e, uint8_t f, uint8_t g, uint8_t h, uint8_t i, uint8_t j, uint8_t k);
    Guid(int32_t a, int16_t b, int16_t c, const uint8_t d[8]);
    Guid(int32_t a, int16_t b, int16_t c, uint8_t d, uint8_t e, uint8_t f, uint8_t g, uint8_t h, uint8_t i, uint8_t j, uint8_t k);

    static Guid NewGuid();

    std::string toString();

    bool equals(const Guid &other) const;

    inline bool operator==(const Guid &other) const { return equals(other); }
    inline bool operator!=(const Guid &other) const { return !equals(other); }

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

// https://stackoverflow.com/a/17017281
template <>
struct std::hash<Guid>
{
    static inline void hash_combine(size_t &val, size_t add)
    {
        val = (val ^ (add << 1)) >> 1;
    };

    std::size_t operator()(const Guid &g) const
    {
        using std::hash;
        using std::size_t;
        using std::string;

        size_t seed = 0;
        hash_combine(seed, hash<int32_t>()(g._a));
        hash_combine(seed, hash<int16_t>()(g._b));
        hash_combine(seed, hash<int16_t>()(g._c));
        hash_combine(seed, hash<uint8_t>()(g._d));
        hash_combine(seed, hash<uint8_t>()(g._e));
        hash_combine(seed, hash<uint8_t>()(g._f));
        hash_combine(seed, hash<uint8_t>()(g._g));
        hash_combine(seed, hash<uint8_t>()(g._h));
        hash_combine(seed, hash<uint8_t>()(g._i));
        hash_combine(seed, hash<uint8_t>()(g._j));
        hash_combine(seed, hash<uint8_t>()(g._k));

        return seed;
    }
};

#endif