/* Copyright (c) 2010-2019 Sander Mertens
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <bake_util.h>
#include <io.h>

static
ut_proc ut_proc_run_intern(
    const char* exec,
    const char *argv[],
    bool redirect,
    FILE *in,
    FILE *out,
    FILE *err)
{
    char *cmdline = NULL;

    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    BOOL bSuccess = FALSE;

    char filename[MAX_PATH];
    LPSTR lpFilePart;
    if (!SearchPath(NULL, exec, UT_OS_BIN_EXT, MAX_PATH, filename, &lpFilePart)) {
        ut_throw("SearchPath: failed to locate executable '%s' in PATH", exec);
        goto error;
    }

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // redirect currently not supported

    ut_strbuf buf = UT_STRBUF_INIT;
    int count = 1;
    ut_strbuf_append(&buf, "\"%s\"", filename);
    while (argv[count]) {
        ut_strbuf_append(&buf, " %s", argv[count ++]);
    }
    cmdline = ut_strbuf_get(&buf);

    ut_trace("#[cyan]%s %s", filename, cmdline);

    bSuccess = CreateProcess(filename,
        cmdline,       // command line 
        NULL,          // process security attributes 
        NULL,          // primary thread security attributes 
        TRUE,          // handles are inherited 
        0,             // creation flags 
        NULL,          // use parent's environment 
        NULL,          // use parent's current directory 
        &si,           // STARTUPINFO pointer 
        &pi);

    if (!bSuccess) {
        ut_trace("failed to start '%s' (code %d)\n", filename, GetLastError());
        goto error;
    }

    return pi.hProcess;
error:
    return NULL;
}

ut_proc ut_proc_runRedirect(
    const char* exec,
    const char *argv[],
    FILE *in,
    FILE *out,
    FILE *err)
{
    return ut_proc_run_intern(exec, argv, true, in, out, err);
}

ut_proc ut_proc_run(
    const char* exec,
    const char *argv[])
{
    return ut_proc_run_intern(exec, argv, false, NULL, NULL, NULL);
}

int ut_proc_kill(ut_proc hProcess, ut_procsignal sig) {
    // Close process and thread handles. 
    CloseHandle(hProcess);
    return 0;
}

int ut_proc_wait(ut_proc hProcess, int8_t *rc) {
    WaitForSingleObject(hProcess, INFINITE);
    
    if (rc) {
        DWORD exit_code = 0;
        if (!GetExitCodeProcess(hProcess, &exit_code)) {
            ut_throw("failed to get exit code of process: %s", ut_last_win_error());
            return -1;
        }

        if (exit_code && (exit_code != ((int8_t)exit_code))) {
            exit_code = UT_CMD_ERR;
        }

        *rc = exit_code;
    }

    CloseHandle(hProcess);

    return 0;
}

int ut_proc_check(ut_proc pid, int8_t *rc) {
    return 0;
}

int ut_beingTraced(void) {
    return 0;
}

ut_proc _ut_proc(void) {
    HANDLE currentProcessHandle = GetCurrentProcess(); 
    return currentProcessHandle;
}
