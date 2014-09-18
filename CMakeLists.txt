cmake_minimum_required(VERSION 2.6)
project(munt)
set(munt_VERSION_MAJOR 1)
set(munt_VERSION_MINOR 4)
set(munt_VERSION_PATCH 0)

option(munt_WITH_MT32EMU_SMF2WAV "Build command line standard MIDI file conversion tool" FALSE)
option(munt_WITH_MT32EMU_QT "Build Qt-based UI-enabled application" FALSE)
option(munt_WITH_INTERNAL_PORTAUDIO "Use internal version of PortAudio" FALSE)
option(munt_WITH_MT32EMU_LV2 "Build LV2 plugin" TRUE)
option(munt_WITH_MT32EMU_LV2UI "Build Qt-based LV2 plugin GUI" TRUE)

set(LV2_INSTALL_DIR lib/lv2 CACHE PATH "Specifies where the LV2 libraries should be installed")

# Set a default build type for single-configuration
# CMake generators if no build type is set.
IF(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
   SET(CMAKE_BUILD_TYPE RelWithDebInfo)
ENDIF(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)

# Common between LV2 and LV2UI
find_package (PkgConfig REQUIRED)
pkg_check_modules(LV2 lv2>=.0.1.0) # sets LV2_LIBRARIES

if(munt_WITH_MT32EMU_QT AND munt_WITH_INTERNAL_PORTAUDIO)
  add_subdirectory(portaudio)
endif()

add_subdirectory(mt32emu)

if(munt_WITH_MT32EMU_SMF2WAV)
  add_subdirectory(mt32emu_smf2wav)
  add_dependencies(mt32emu-smf2wav mt32emu)
endif()

if(munt_WITH_MT32EMU_QT)
  add_subdirectory(mt32emu_qt)
  add_dependencies(mt32emu-qt mt32emu)
endif()

if(munt_WITH_MT32EMU_QT AND munt_WITH_INTERNAL_PORTAUDIO)
  add_dependencies(mt32emu-qt portaudio)
endif()

if(munt_WITH_MT32EMU_LV2)
  add_subdirectory(mt32emu_lv2)
  add_dependencies(mt32emu-lv2 mt32emu)
endif()

if(munt_WITH_MT32EMU_LV2UI)
  add_subdirectory(mt32emu_lv2ui)
  add_dependencies(mt32emu-lv2ui mt32emu)
endif()

# build a CPack driven installer package
include(InstallRequiredSystemLibraries)
set(CPACK_PACKAGE_VERSION_MAJOR "${munt_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${munt_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${munt_VERSION_PATCH}")
set(CPACK_PACKAGE_VENDOR "muntemu.org")
set(CPACK_PACKAGE_CONTACT "sergm@muntemu.org")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/FreeBSD/usr/ports/audio/munt/pkg-descr")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Software synthesiser emulating pre-GM MIDI devices such as Roland MT-32, CM-32L, CM-64 and LAPC-I")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/mt32emu_qt/COPYING.txt")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_STRIP_FILES TRUE)
set(CPACK_SOURCE_GENERATOR TGZ)
set(CPACK_SOURCE_STRIP_FILES TRUE)
# The default source package file name has "-Source" at the end, which doesn't match our previous releases
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
set(CPACK_RPM_PACKAGE_GROUP "Audio/Emulators")
set(CPACK_RPM_PACKAGE_LICENSE "LGPL, GPLv2.1, GPLv3")
set(CPACK_RPM_PACKAGE_URL "http://munt.sourceforge.net/")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS "ON")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "http://munt.sourceforge.net/")
set(CPACK_DEBIAN_PACKAGE_SECTION "sound")
include(CPack)
