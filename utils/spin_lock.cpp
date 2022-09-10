//
// Created by 24810 on 2022/09/10.
//

#include "spin_lock.h"
namespace search {
    SpinLock::SpinLock() {
        m_Lock.clear();
    }

    void SpinLock::Lock() {
        while (m_Lock.test_and_set(std::memory_order_acquire));
    }

    void SpinLock::UnLock() {
        m_Lock.clear(std::memory_order_release);
    }
}