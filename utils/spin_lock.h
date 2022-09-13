#ifndef SEARCH_PROJECT_SPIN_LOCK_H
#define SEARCH_PROJECT_SPIN_LOCK_H


#include <atomic>

namespace search {
    class SpinLock {
    public:
        SpinLock();

        void Lock();

        void UnLock();

    private:
        std::atomic_flag m_Lock{};
    };
}

#endif //SEARCH_PROJECT_SPIN_LOCK_H
