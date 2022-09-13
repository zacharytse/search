#ifndef SEARCH_PROJECT_SINGLETON_H
#define SEARCH_PROJECT_SINGLETON_H

#include <memory>
#include <mutex>
/**
 * 单例基类，继承该类可实现单例,子类构造函数需要私有
 */
namespace search {
    template<typename T>
    class Singleton {
    public:
        static T *GetInstance() {
            static std::once_flag s_flag;
            std::call_once(s_flag, [&]() {
                m_pSington.reset(new T());
            });

            return m_pSington.get();
        }

    protected:
        Singleton() {};

        ~Singleton() {};

        static std::shared_ptr<T> m_pSington;

    private:
        Singleton(const Singleton &) = delete;

        Singleton &operator=(const Singleton &) = delete;

    private:

    };

    template<typename T> std::shared_ptr<T> Singleton<T>::m_pSington;

}

#endif //SEARCH_PROJECT_SINGLETON_H
