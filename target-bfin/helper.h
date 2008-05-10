#include "def-helper.h"

DEF_HELPER_2(raise_exception, void, i32, i32)
DEF_HELPER_4(memalign, void, i32, i32, i32, i32)

DEF_HELPER_3(dbga_l, void, i32, i32, i32)
DEF_HELPER_3(dbga_h, void, i32, i32, i32)
DEF_HELPER_FLAGS_1(outc, TCG_CALL_CONST, void, i32)
DEF_HELPER_FLAGS_3(dbg, TCG_CALL_CONST, void, i32, i32, i32)
DEF_HELPER_FLAGS_2(dbg_areg, TCG_CALL_CONST, void, i64, i32)

DEF_HELPER_0(astat_load, i32)
DEF_HELPER_1(astat_store, void, i32)

DEF_HELPER_FLAGS_1(ones, TCG_CALL_CONST|TCG_CALL_PURE, i32, i32)
DEF_HELPER_FLAGS_2(signbits, TCG_CALL_CONST|TCG_CALL_PURE, i32, i32, i32)
DEF_HELPER_FLAGS_2(signbits_64, TCG_CALL_CONST|TCG_CALL_PURE, i32, i64, i32)

DEF_HELPER_FLAGS_4(dagadd, TCG_CALL_CONST|TCG_CALL_PURE, i32, i32, i32, i32, i32)
DEF_HELPER_FLAGS_4(dagsub, TCG_CALL_CONST|TCG_CALL_PURE, i32, i32, i32, i32, i32)
DEF_HELPER_FLAGS_2(add_brev, TCG_CALL_CONST|TCG_CALL_PURE, i32, i32, i32)

#include "def-helper.h"
