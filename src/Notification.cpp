#include "Notification.h"
#include <coreinit/cache.h>
#include <coreinit/thread.h>
#include <coreinit/title.h>
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
    uint64_t titleID = OSGetTitleID();
    if (titleID == 0x0005001010040000L || titleID == 0x0005001010040100L || titleID == 0x0005001010040200L) {
        sShutdownHintThread = false;
        sShowHintThread = std::make_unique<std::thread>(ShowNotification, value);
    }
}

void StopNotificationThread() {
    if (sShowHintThread != nullptr) {
        sShutdownHintThread = true;
        OSMemoryBarrier();
        sShowHintThread->join();
        sShowHintThread.reset();
    }
}
