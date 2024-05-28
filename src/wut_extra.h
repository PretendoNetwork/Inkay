#pragma once

#include <assert.h>
#include <cstdint>

#include <nn/act.h>

#ifdef __cplusplus
extern "C" {
#endif

namespace nn::boss {
    class Task {
    public:
        nn::act::PersistentId persistentId;
        uint32_t unk1;
        char task_name[8];
        uint64_t title_id;
        uint32_t unk2;
        uint32_t unk3;
    };

    static_assert(sizeof(Task) == 32, "nn::boss::Task must be 32 bytes.");
}

extern "C" uint32_t Initialize__Q2_2nn4bossFv();
extern "C" uint32_t Finalize__Q2_2nn4bossFv();

extern "C" void __ct__Q3_2nn4boss4TaskFv(nn::boss::Task *task);
extern "C" uint32_t Initialize__Q3_2nn4boss4TaskFPCcUi(nn::boss::Task *task, char const *, unsigned int);
extern "C" uint32_t Unregister__Q3_2nn4boss4TaskFv(nn::boss::Task *task);
extern "C" void __dt__Q3_2nn4boss4TaskFv(nn::boss::Task *task);
extern "C" uint32_t StartScheduling__Q3_2nn4boss4TaskFb(nn::boss::Task *task, bool queueTaskOnCall);
extern "C" uint32_t GetState__Q3_2nn4boss4TaskCFPUi(nn::boss::Task *task, uint32_t *outExecCount);
extern "C" uint32_t Run__Q3_2nn4boss4TaskFb(nn::boss::Task *task, bool unk);
extern "C" uint32_t UpdateIntervalSec__Q3_2nn4boss4TaskFUi(nn::boss::Task *task, uint32_t seconds);
extern "C" bool IsRegistered__Q3_2nn4boss4TaskCFv(nn::boss::Task *task);

#ifdef __cplusplus
}
#endif
