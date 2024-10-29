/*  Copyright 2024 Pretendo Network contributors <pretendo.network>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "module.h"
#include <coreinit/dynload.h>

#include "Notification.h"
#include "utils/logger.h"

static OSDynLoad_Module module;
static void (*moduleInitialize)(bool) = nullptr;
static InkayStatus (*moduleGetStatus)() = nullptr;

void Inkay_Initialize(bool apply_patches) {
    if (module) {
        return;
    }

    if (OSDynLoad_Acquire("inkay", &module) != OS_DYNLOAD_OK) {
        DEBUG_FUNCTION_LINE("Failed to acquire module");
        ShowNotification("Cannot find Inkay module. Ensure the Inkay module is properly installed");
        return;
    }

    if (OSDynLoad_FindExport(module, OS_DYNLOAD_EXPORT_FUNC, "Inkay_Initialize", reinterpret_cast<void * *>(&moduleInitialize)) != OS_DYNLOAD_OK) {
        DEBUG_FUNCTION_LINE("Failed to find initialization function");
        ShowNotification("Cannot find Inkay module initialization function. Ensure the Inkay module is properly installed");
        OSDynLoad_Release(module);
        return;
    }

    moduleInitialize(apply_patches);
}

void Inkay_Finalize() {
    if (module) {
        OSDynLoad_Release(module);
        moduleInitialize = nullptr;
        moduleGetStatus = nullptr;
    }
}

InkayStatus Inkay_GetStatus() {
    if (!module) {
        return InkayStatus::Error;
    }

    if (!moduleGetStatus && OSDynLoad_FindExport(module, OS_DYNLOAD_EXPORT_FUNC, "Inkay_GetStatus", reinterpret_cast<void * *>(&moduleGetStatus)) != OS_DYNLOAD_OK) {
        DEBUG_FUNCTION_LINE("Failed to find status function");
        return InkayStatus::Error;
    }

    return moduleGetStatus();
}
