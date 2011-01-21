#include "def-helper.h"

DEF_HELPER_2(raise_exception, void, i32, i32)

DEF_HELPER_3(dbga_l, void, i32, i32, i32)
DEF_HELPER_3(dbga_h, void, i32, i32, i32)
DEF_HELPER_1(outc, void, i32)
DEF_HELPER_1(dbg, void, i32)

DEF_HELPER_0(astat_load, i32)
DEF_HELPER_1(astat_store, void, i32)

DEF_HELPER_1(ones, i32, i32)
DEF_HELPER_2(signbits, i32, i32, i32)
DEF_HELPER_2(signbits_64, i32, i64, i32)

#include "def-helper.h"
