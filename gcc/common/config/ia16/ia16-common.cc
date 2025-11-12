/* Common hooks for IA-16 (Intel 16-bit x86).
   Copyright (C) 2007-2025 Free Software Foundation, Inc.
   Contributed by Rask Ingemann Lambertsen <rask@sygehus.dk>
   Changes by Andrew Jenner <andrew@codesourcery.com>
   Very preliminary IA-16 far pointer support and other changes by TK Chia

   This file is part of GCC.

   GCC is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GCC is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with GCC; see the file COPYING3.  If not see
   <http://www.gnu.org/licenses/>.  */

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tm.h"
#include "common/common-target.h"
#include "common/common-target-def.h"
#include "opts.h"
#include "flags.h"

/* Implement TARGET_OPTION_OPTIMIZATION_TABLE.  */
static const struct default_options ia16_option_optimization_table[] =
  {
    /* For 16-bit targets, optimize for size by default at all levels.
       The small address space makes code size critical.  */
    { OPT_LEVELS_1_PLUS, OPT_fomit_frame_pointer, NULL, 1 },
    /* Inline small functions to reduce call overhead, but not too
       aggressively as code size is important.  */
    { OPT_LEVELS_2_PLUS, OPT_finline_small_functions, NULL, 1 },
    /* Allow optimizer to introduce store data races. This helps reduce
       code size and register pressure on a very constrained architecture.  */
    { OPT_LEVELS_ALL, OPT_fallow_store_data_races, NULL, 1 },
    { OPT_LEVELS_NONE, 0, NULL, 0 }
  };

#undef TARGET_OPTION_OPTIMIZATION_TABLE
#define TARGET_OPTION_OPTIMIZATION_TABLE ia16_option_optimization_table

/* Implement TARGET_HANDLE_OPTION.  */
static bool
ia16_handle_option (struct gcc_options *opts ATTRIBUTE_UNUSED,
                     struct gcc_options *opts_set ATTRIBUTE_UNUSED,
                     const struct cl_decoded_option *decoded ATTRIBUTE_UNUSED,
                     location_t loc ATTRIBUTE_UNUSED)
{
  /* All options are accepted by default. */
  return true;
}

#undef TARGET_HANDLE_OPTION
#define TARGET_HANDLE_OPTION ia16_handle_option

struct gcc_targetm_common targetm_common = TARGETM_COMMON_INITIALIZER;
