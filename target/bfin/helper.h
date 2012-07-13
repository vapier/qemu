/*
 * Blackfin helpers
 *
 * Copyright 2007-2016 Mike Frysinger
 * Copyright 2007-2011 Analog Devices, Inc.
 *
 * Licensed under the Lesser GPL 2 or later.
 */

DEF_HELPER_FLAGS_3(raise_exception, TCG_CALL_NO_WG, void, env, i32, i32)
DEF_HELPER_FLAGS_5(memalign, TCG_CALL_NO_WG, void, env, i32, i32, i32, i32)
DEF_HELPER_FLAGS_2(require_supervisor, TCG_CALL_NO_WG, void, env, i32)

DEF_HELPER_FLAGS_4(dbga_l, TCG_CALL_NO_WG, void, env, i32, i32, i32)
DEF_HELPER_FLAGS_4(dbga_h, TCG_CALL_NO_WG, void, env, i32, i32, i32)
DEF_HELPER_FLAGS_1(outc, TCG_CALL_NO_RWG, void, i32)
DEF_HELPER_FLAGS_3(dbg, TCG_CALL_NO_RWG, void, i32, i32, i32)
DEF_HELPER_FLAGS_2(dbg_areg, TCG_CALL_NO_RWG, void, i64, i32)

DEF_HELPER_FLAGS_1(astat_load, TCG_CALL_NO_WG_SE, i32, env)
DEF_HELPER_2(astat_store, void, env, i32)

DEF_HELPER_FLAGS_1(cycles_read, TCG_CALL_NO_SE, i32, env)

DEF_HELPER_FLAGS_1(ones, TCG_CALL_NO_RWG_SE, i32, i32)
DEF_HELPER_FLAGS_1(signbits_16, TCG_CALL_NO_RWG_SE, i32, i32)
DEF_HELPER_FLAGS_1(signbits_32, TCG_CALL_NO_RWG_SE, i32, i32)
DEF_HELPER_FLAGS_1(signbits_40, TCG_CALL_NO_RWG_SE, i32, i64)

DEF_HELPER_FLAGS_4(dagadd, TCG_CALL_NO_RWG_SE, i32, i32, i32, i32, i32)
DEF_HELPER_FLAGS_4(dagsub, TCG_CALL_NO_RWG_SE, i32, i32, i32, i32, i32)
DEF_HELPER_FLAGS_2(add_brev, TCG_CALL_NO_RWG_SE, i32, i32, i32)
