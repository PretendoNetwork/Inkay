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

#include <coreinit/dynload.h>

#include "utils/logger.h"

void Inkay_Initialize(bool apply_patches) {
    OSDynLoad_Module module;
    void (*moduleInitialize)(bool) = nullptr;

    if (OSDynLoad_Acquire("inkay", &module) != OS_DYNLOAD_OK) {
        DEBUG_FUNCTION_LINE("Failed to acquire module");
        return;
    }

    if (OSDynLoad_FindExport(module, OS_DYNLOAD_EXPORT_FUNC, "Inkay_Initialize", reinterpret_cast<void * *>(&moduleInitialize)) != OS_DYNLOAD_OK) {
        DEBUG_FUNCTION_LINE("Failed to find initialization function");
        return;
    }

    moduleInitialize(apply_patches);

    OSDynLoad_Release(module);
}
