#ifndef UREACTOR_INCLUDE_UREACTOR_SINGLETON_HPP
#define UREACTOR_INCLUDE_UREACTOR_SINGLETON_HPP

#include <string>
#include <iostream>

namespace ureactor{

class Singleton
{
public:
    Singleton(const Singleton&) = delete;
    Singleton(Singleton&&) = delete;
    Singleton& operator=(const Singleton&) = delete;
    Singleton& operator=(Singleton&&) = delete;

    static Singleton& Instance()
    {
        static Singleton instance;
        return instance;
    }

    void log(const std::string msg)
    {
        std::cout << "[LOG]" << msg << std::endl;
    }
    
private:
    Singleton() = default;
    ~Singleton() = default;

};

// templateM<typename T>
// class Singleton
// {
// public:
//     static inline T& Instance()
//     {
//         static T instance;
//         return instance;
//     }
//     Singleton() = delete;
//     Singleton(const Singleton&) = delete;
//     Singleton(Singleton&&) = delete;
//     Singleton& operator=(const Singleton&) = delete;
//     Singleton& operator=(Singleton&&) = delete;
// private:

// };

// template<typename T>
// class Singleton
// {
// public:
//     static Singleton& Instance()
//     {
//         static Singleton& instance;
//         return instance;
//     }
//     Singleton() = delete;
//     Singleton(const Singleton&) = delete;
//     Singleton(Singleton&&) = delete;
//     Singleton& operator=(const Singleton&) = delete;
//     Singleton& operator=(Singleton&&) = delete;

// protected:
//     Singleton() = default;
//     ~Singleton() = default;

// };


}


#endif