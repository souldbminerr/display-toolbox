/*
 * Copyright (c) Souldbminer and Horizon OC Contributors
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/* --------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <p-sam@d3vs.net>, <natinusala@gmail.com>, <m4x@m4xw.net>
 * wrote this file. As long as you retain this notice you can do whatever you
 * want with this stuff. If you meet any of us some day, and you think this
 * stuff is worth it, you can buy us a beer in return.  - The sys-clk authors
 * --------------------------------------------------------------------------
 */

#include "file_utils.hpp"
#include <mutex>

extern "C" void __libnx_init_time(void);

namespace fileUtils {

    namespace {

        u64 bootTimeS;
        std::mutex g_log_mutex;
        std::atomic_bool g_has_initialized = false;
        bool g_log_enabled = false;
        std::uint64_t g_last_flag_check = 0;

        void RefreshFlags(bool force) {
            std::uint64_t now = armTicksToNs(armGetSystemTick());
            if (!force && (now - g_last_flag_check) < FILE_FLAG_CHECK_INTERVAL_NS) {
                return;
            }

            FILE *file = fopen(FILE_LOG_FLAG_PATH, "r");
            if (file) {
                g_log_enabled = true;
                fclose(file);
            } else {
                g_log_enabled = false;
            }

            g_last_flag_check = now;
        }

        void InitializeThreadFunc(void *args) {
            Initialize();
        }

    }  // namespace

    bool IsInitialized() {
        return g_has_initialized;
    }

    bool IsLogEnabled() {
        return g_log_enabled;
    }

    void LogLine(const char *format, ...) {
        std::scoped_lock lock{ g_log_mutex };

        va_list args;
        va_start(args, format);
        if (g_has_initialized) {
            RefreshFlags(false);

            if (g_log_enabled) {
                FILE *file = fopen(FILE_LOG_FILE_PATH, "a");

                if (file) {
                    timespec now = {};
                    clock_gettime(CLOCK_REALTIME, &now);

                    fprintf(file, "[%luls] ", now.tv_sec - bootTimeS);
                    vfprintf(file, format, args);
                    fprintf(file, "\n");
                    fclose(file);
                }
            }
        }
        va_end(args);
    }

    void SetBootTime() {
        timespec bootTime = {};
        clock_gettime(CLOCK_REALTIME, &bootTime);
        bootTimeS = bootTime.tv_sec;
    }

    void InitializeAsync() {
        Thread initThread = { 0 };
        threadCreate(&initThread, InitializeThreadFunc, NULL, NULL, 0x4000, 0x15, 0);
        threadStart(&initThread);
    }

    Result Initialize() {
        Result rc = 0;

        if (R_SUCCEEDED(rc)) {
            rc = timeInitialize();
        }

        __libnx_init_time();
        timeExit();
        SetBootTime();

        if (R_SUCCEEDED(rc)) {
            rc = fsInitialize();
        }

        if (R_SUCCEEDED(rc)) {
            rc = fsdevMountSdmc();
        }

        if (R_SUCCEEDED(rc)) {
            RefreshFlags(true);
            g_has_initialized = true;
            LogLine("=== " TARGET " " TARGET_VERSION " ===");
        }

        return rc;
    }

    void Exit() {
        if (!g_has_initialized) {
            return;
        }

        g_has_initialized = false;
        g_log_enabled = false;

        fsdevUnmountAll();
        fsExit();
    }

}  // namespace fileUtils
