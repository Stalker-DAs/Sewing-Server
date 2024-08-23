#pragma once

#include <string.h>
#include <assert.h>
#include "util.h"
#include "log.h"

#define ASSERT(x)                                  \
    if (!(x))                                      \
    {                                              \
        LOG_ERROR(LOG_ROOT()) << "ASSERTION: " #x; \
        assert(x);                                 \
    }

#define ASSERT2(x, w)                             \
    if (!(x))                                     \
    {                                             \
        LOG_ERROR(LOG_ROOT()) << "ASSERTION: " #x \
                              << "\n"             \
                              << w;               \
        assert(x);                                \
    }
