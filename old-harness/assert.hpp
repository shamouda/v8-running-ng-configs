#ifndef ASSERT_HPP
#define ASSERT_HPP

#include <err.h>
#include <errno.h>

#define ASSERT(condition, ...)   \
    if (!(condition)) {          \
        errx(1, __VA_ARGS__);    \
        exit(-1);                \
    }
#endif