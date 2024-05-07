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

#include "Notification.h"
#include <coreinit/cache.h>
#include <coreinit/thread.h>
#include <notifications/notification_defines.h>
#include <notifications/notifications.h>
#include <thread>

static std::unique_ptr<std::thread> sShowHintThread;
static bool sShutdownHintThread = false;

void ShowNotification(const char * notification) {
    // Wait for notification module to be ready
    bool isOverlayReady = false;
    while (!sShutdownHintThread && NotificationModule_IsOverlayReady(&isOverlayReady) == NOTIFICATION_MODULE_RESULT_SUCCESS && !isOverlayReady)
        OSSleepTicks(OSMillisecondsToTicks(16));
    if (sShutdownHintThread || !isOverlayReady) return;
    NotificationModuleStatus err = NotificationModule_SetDefaultValue(NOTIFICATION_MODULE_NOTIFICATION_TYPE_INFO, NOTIFICATION_MODULE_DEFAULT_OPTION_DURATION_BEFORE_FADE_OUT, 15.0f);
    if(err != NOTIFICATION_MODULE_RESULT_SUCCESS) return;

    NotificationModule_AddInfoNotification(notification);
}

void StartNotificationThread(const char * value) {
	sShutdownHintThread = false;
	sShowHintThread = std::make_unique<std::thread>(ShowNotification, value);
}

void StopNotificationThread() {
    if (sShowHintThread != nullptr) {
        sShutdownHintThread = true;
        OSMemoryBarrier();
        sShowHintThread->join();
        sShowHintThread.reset();
    }
}
