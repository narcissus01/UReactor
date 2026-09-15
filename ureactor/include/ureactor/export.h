#ifndef UREACTOR_EXPORT_H
#define UREACTOR_EXPORT_H

#if defined(_WIN32) && defined(UREACTOR_SHARED_LIBRARY)
#if defined(UREACTOR_BUILDING_LIBRARY)
#define UREACTOR_API __declspec(dllexport)
#else
#define UREACTOR_API __declspec(dllimport)
#endif

#elif defined(__GNUC__) && defined(UREACTOR_SHARED_LIBRARY)
#define UREACTOR_API __attribute__((visibility("default")))

#else
#define UREACTOR_API
#endif



#endif // UREACTOR_EXPORT_H