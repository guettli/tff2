#ifndef TFF_VERSION_H
#define TFF_VERSION_H

// Single source of the TFF version string. The numeric components are injected
// by CMake from the project() VERSION (see CMakeLists.txt); the defaults below
// only keep standalone / static-analysis builds compiling. Bump the version in
// exactly one place: the project() line in CMakeLists.txt.
#ifndef TFF_VERSION_MAJOR
#define TFF_VERSION_MAJOR 0
#endif
#ifndef TFF_VERSION_MINOR
#define TFF_VERSION_MINOR 0
#endif
#ifndef TFF_VERSION_PATCH
#define TFF_VERSION_PATCH 0
#endif

#define TFF_STR_HELPER(x) #x
#define TFF_STR(x) TFF_STR_HELPER(x)
#define TFF_VERSION_STRING                                                                         \
    TFF_STR(TFF_VERSION_MAJOR) "." TFF_STR(TFF_VERSION_MINOR) "." TFF_STR(TFF_VERSION_PATCH)

#endif  // TFF_VERSION_H
