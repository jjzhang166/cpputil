#ifndef __base_singleton_h__
#define __base_singleton_h__

#include "base/locker.h"

namespace base
{
    template<typename T>
    struct DefaultSingletonTraits
    {
        static T* Create()
        {
            return new T();
        }
        static void Destroy(T* obj)
        {
            delete obj;
        }

        const static bool kRegisterAtExit = true;
    };


    template<typename T>
    struct LeakySingletonTraits: public DefaultSingletonTraits<T>
    {
        static const bool kRegisterAtExit = false;
    };


    template<typename T,
             typename SingletonTraits = DefaultSingletonTraits<T>,
             template<typename Locker> class Guard = MultiThreadGuard>
    class Singleton
    {
    public:
        static T& Instance()
        {
            if (!instance_)
            {
                AutoLocker<CSLocker> guard(&locker_);
                if (!instance_)
                {
                    instance_ = SingletonTraits::Create();
                    if (instance_ && SingletonTraits::kRegisterAtExit)
                        atexit(DestroySingleton);
                }
            }

            return *instance_;
        }

    private:
        static void DestroySingleton()
        {
            SingletonTraits::Destroy(instance_);
            instance_ = 0;
        }

    private:
        static T* instance_;
        static Guard<CSLocker> locker_;

    private:
        Singleton(){}
        ~Singleton(){}

        Singleton(const Singleton&);
        Singleton& operator=(const Singleton&);
    };

    template<typename T, typename SingletonTraits, template<typename Locker> class Guard>
    T* Singleton<T, SingletonTraits, Guard>::instance_ = 0;

    template<typename T, typename SingletonTraits, template<typename Locker> class Guard>
    Guard<CSLocker> Singleton<T, SingletonTraits, Guard>::locker_;
}

#endif