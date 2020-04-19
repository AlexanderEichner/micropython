#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "py/mpconfig.h"
#include "py/compile.h"
#include "py/runtime.h"
#include "py/repl.h"
#include "py/stackctrl.h"
#include "py/gc.h"
#include "py/mperrno.h"
#include "lib/utils/pyexec.h"
#include "lib/utils/gchelper.h"

/** Status code success indicator. */
#define INF_SUCCESS (0)

/** Operation still pending, try again later. */
#define INF_TRY_AGAIN         (1)
/** Invalid parameter passed. */
#define ERR_INVALID_PARAMETER (-1)
/** Buffer overflow. */
#define ERR_BUFFER_OVERFLOW   (-2)
/** Not implemented at the moment. */
#define ERR_NOT_IMPLEMENTED   (-3)
/** Invalid state encountered. */
#define ERR_INVALID_STATE     (-4)

#define NIL_X86PADDR 0xffffffffffffffffULL

typedef uint64_t X86PADDR;
typedef uint32_t SMNADDR;
typedef uint32_t PSPADDR;

#define _1M (1024*1024)
#define _64M (64 * _1M)

/** Flat binary load address. */
#define CM_FLAT_BINARY_LOAD_ADDR 0x10000
/** Inifinite amount of waiting time. */
#define CM_WAIT_INDEFINITE       UINT32_MAX

/** Pointer to a const code module interface callback table. */
typedef const struct CMIF *PCCMIF;

/**
 * The interface callback table for the code module to use.
 */
typedef struct CMIF
{
    /**
     * Peeks how many bytes are available for reading from the given input buffer ID.
     *
     * @returns Number of bytes available for reading from the given input buffer ID.
     * @param   pCmIf               Pointer to this interface table.
     * @param   idInBuf             Input buffer ID.
     */
    size_t (*pfnInBufPeek) (PCCMIF pCmIf, uint32_t idInBuf);

    /**
     * Polls until there is something to read from the given input buffer.
     *
     * @returns Status code.
     * @param   pCmIf               Pointer to this interface table.
     * @param   idInBuf             Input buffer ID to poll.
     * @param   cMillies            How many milliseconds to poll, CM_WAIT_INDEFINITE to wait
     *                              until there is something.
     */
    int (*pfnInBufPoll) (PCCMIF pCmIf, uint32_t idInBuf, uint32_t cMillies);

    /**
     * Reads from the given input buffer.
     *
     * @returns Status code.
     * @param   pCmIf               Pointer to this interface table.
     * @param   idInBuf             Input buffer ID to read from.
     * @param   pvBuf               Where to store the read data.
     * @param   cbRead              How much to read.
     * @param   pcbRead             Where to store the amount of bytes read, optional.
     */
    int    (*pfnInBufRead) (PCCMIF pCmIf, uint32_t idInBuf, void *pvBuf, size_t cbRead, size_t *pcbRead);

    /**
     * Writes to the given output buffer.
     *
     * @returns Status code.
     * @param   pCmIf               Pointer to this interface table.
     * @param   idOutBuf            Output buffer ID to write to.
     * @param   pvBuf               The data to write.
     * @param   cbWrite             How much to write.
     * @param   pcbWritten          Where to store the amount of data actually written.
     */
    int    (*pfnOutBufWrite) (PCCMIF pCmIf, uint32_t idOutBuf, const void *pvBuf, size_t cbWrite, size_t *pcbWritten);

    /**
     * Waits for the given amount of milliseconds.
     *
     * @returns nothing.
     * @param   pCmIf               Pointer to this interface table.
     * @param   cMillies            Number of milliseconds to wait.
     */
    void   (*pfnDelayMs) (PCCMIF pCmIf, uint32_t cMillies);

    /**
     * Returns a millisecond precision timestamp since some arbitrary point in the past (usually startup time).
     *
     * @returns Millisecond timestamp.
     * @param   pCmIf               Pointer to this interface table.
     */
    uint32_t (*pfnTsGetMilli) (PCCMIF pCmIf);

} CMIF;
/** Pointer to a code module interface callback table. */
typedef CMIF *PCMIF;

extern uint32_t _heap_start;
extern uint32_t _heap_end;
extern uint32_t _estack;
extern uint32_t _sstack;

uint32_t _old_stack;

static PCCMIF g_pCmIf = NULL;

// Receive single character
int mp_hal_stdin_rx_chr(void) {
    char bVal;
    g_pCmIf->pfnInBufPoll(g_pCmIf, 0 /*idInBuf*/, CM_WAIT_INDEFINITE);
    g_pCmIf->pfnInBufRead(g_pCmIf, 0 /*idInBuf*/, &bVal, sizeof(bVal), NULL /*pcbRead*/);
    return bVal;
}

// Send string of given length
void mp_hal_stdout_tx_strn(const char *str, mp_uint_t len) {
    g_pCmIf->pfnOutBufWrite(g_pCmIf, 0 /*idOutBuf*/, str, len, NULL /*pcbWritten*/);
}


uint32_t main(PCCMIF pCmIf, uint32_t u32Arg0, uint32_t u32Arg1, uint32_t u32Arg2, uint32_t u32Arg3)
{
    g_pCmIf = pCmIf;

    mp_stack_set_top(&_estack);
    mp_stack_set_limit((char *)&_estack - (char *)&_sstack - 1024);
    gc_init(&_heap_start, &_heap_end);

    mp_init();
//    readline_init0();

#if MICROPY_ENABLE_COMPILER
# if MICROPY_REPL_EVENT_DRIVEN
    pyexec_event_repl_init();
    for (;;) {
        int c = mp_hal_stdin_rx_chr();
        if (pyexec_event_repl_process_char(c)) {
            break;
        }
    }
# else
    pyexec_friendly_repl();
# endif
#else
    pyexec_frozen_module("frozentest.py");
#endif
    mp_deinit();
    return 0;
}

void gc_collect(void)
{
    // start the GC
    gc_collect_start();

    // get the registers and the sp
    uintptr_t regs[10];
    uintptr_t sp = gc_helper_get_regs_and_sp(regs);

    // trace the stack, including the registers (since they live on the stack in this function)
    gc_collect_root((void **)sp, (_estack - sp) / sizeof(uint32_t));

    // end the GC
    gc_collect_end();
}

mp_lexer_t *mp_lexer_new_from_file(const char *filename) {
    mp_raise_OSError(MP_ENOENT);
}

mp_import_stat_t mp_import_stat(const char *path) {
    return MP_IMPORT_STAT_NO_EXIST;
}

mp_obj_t mp_builtin_open(size_t n_args, const mp_obj_t *args, mp_map_t *kwargs) {
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(mp_builtin_open_obj, 1, mp_builtin_open);

void nlr_jump_fail(void *val) {
    g_pCmIf->pfnOutBufWrite(g_pCmIf, 0 /*idOutBuf*/, "nlr_jump_fail\n", sizeof("nlr_jump_fail\n") - 1, NULL);
    while (1) {
        ;
    }
}

void NORETURN __fatal_error(const char *msg) {
    while (1) {
        ;
    }
}

#ifndef NDEBUG
void MP_WEAK __assert_func(const char *file, int line, const char *func, const char *expr) {
    for (;;) ;
}
#endif

