#include "Notification.h"
#include <coreinit/cache.h>
#include <coreinit/thread.h>
#include <coreinit/title.h>
#include <coreinit/atomic.h>
#include <notifications/notification_defines.h>
#include <notifications/notifications.h>
#include <thread>

std::unique_ptr<std::thread> sShowHintThread;
static uint32_t sShutdownNotificationThread = 0;
static char * notification;

void ShowNotification() {
    // Wait for notification module to be ready
    bool isOverlayReady = false;
    while (!OSCompareAndSwapAtomic(&sShutdownNotificationThread, 1, 0) && NotificationModule_IsOverlayReady(&isOverlayReady) == NOTIFICATION_MODULE_RESULT_SUCCESS && !isOverlayReady)
        OSSleepTicks(OSMillisecondsToTicks(16));
    if (OSCompareAndSwapAtomic(&sShutdownNotificationThread, 1, 0) || !isOverlayReady) return;
    NotificationModuleStatus err = NotificationModule_SetDefaultValue(NOTIFICATION_MODULE_NOTIFICATION_TYPE_INFO, NOTIFICATION_MODULE_DEFAULT_OPTION_DURATION_BEFORE_FADE_OUT, 15.0f);
    if(err != NOTIFICATION_MODULE_RESULT_SUCCESS) return;

    NotificationModule_AddInfoNotification(notification);
}

void StartNotificationThread(char * value) {
    uint64_t titleID = OSGetTitleID();
    if (titleID == 0x0005001010040000L || titleID == 0x0005001010040100L || titleID == 0x0005001010040200L) {
        OSCompareAndSwapAtomic(&sShutdownNotificationThread, 0, 0);
        notification = value;
        sShowHintThread = std::make_unique<std::thread>(ShowNotification);
    }
}

void StopNotificationThread() {
    if (sShowHintThread != nullptr) {
        OSCompareAndSwapAtomic(&sShutdownNotificationThread, 0, 1);
        sShowHintThread->join();
        sShowHintThread.reset();
    }
}
