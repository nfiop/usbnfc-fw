/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Liav A
 */

#ifndef __COMMON__TYPES_H_
#define __COMMON__TYPES_H_

/* File that is common to all supported platforms (so it resides in their
 * directory).
 *
 * IMPORTANT NOTE:
 * All platforms should provide the basic set of headers below through their
 * SDK, or provide **equivalent** headers if their SDK doesn't supply such
 * headers.
 *
 * For malloc-like operations, see platform/platform.h - each platform might
 * implement it in a different way (even a simple bump allocator is valid
 * in some cases).
 * Because of that we don't include stdlib.h, because it might have such
 * functions - the generic API in platform.h should provide what's needed
 * instead.
 */
#include "platform_types.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define UNUSED(__x) (void)__x

#define TO_STRING(x) #x

#endif
