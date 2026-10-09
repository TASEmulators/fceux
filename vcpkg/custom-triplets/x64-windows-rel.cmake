set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE dynamic)

# Force vcpkg manifest mode to skip debug builds entirely
set(VCPKG_BUILD_TYPE release)