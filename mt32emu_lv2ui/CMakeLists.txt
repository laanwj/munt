cmake_minimum_required(VERSION 2.6)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../cmake/Modules/")

project(mt32emu-lv2ui)
set(mt32emu_lv2ui_VERSION_MAJOR 1)
set(mt32emu_lv2ui_VERSION_MINOR 2)
set(mt32emu_lv2ui_VERSION_PATCH 0)

add_definitions(-DPACKAGE_VERSION="${munt_VERSION_MAJOR}.${munt_VERSION_MINOR}.${munt_VERSION_PATCH}")
add_definitions(-DAPP_VERSION="${mt32emu_lv2ui_VERSION_MAJOR}.${mt32emu_lv2ui_VERSION_MINOR}.${mt32emu_lv2ui_VERSION_PATCH}")

include(CheckIncludeFiles)
include(CheckLibraryExists)

pkg_check_modules(NTK ntk REQUIRED)
pkg_check_modules(NTK_IMAGES ntk_images REQUIRED)
include_directories(${NTK_INCLUDE_DIRS} ${NTK_IMAGES_INCLUDE_DIRS})
link_directories(${NTK_LIBRARY_DIRS} ${NTK_IMAGES_LIBRARY_DIRS})
set(EXT_LIBS ${EXT_LIBS} ${NTK_LIBRARIES} ${NTK_IMAGES_LIBRARIES})

set(mt32emu_lv2ui_SOURCES
  src/mt32emu_lv2ui.cpp
  src/fl_munt_ui.cxx
  src/fl_lcd_display.cpp
)

set(mt32emu_lv2ui_HEADERS
)

find_package(MT32EMU REQUIRED)
set(EXT_LIBS ${EXT_LIBS} ${MT32EMU_LIBRARIES})
include_directories(${MT32EMU_INCLUDE_DIRS})

include_directories(${LV2_INCLUDE_DIRS})
set(EXT_LIBS ${EXT_LIBS} ${LV2_LIBRARIES})

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER MATCHES "(^|/)clang\\+\\+$")
  add_definitions(-Wall -Wextra -Wnon-virtual-dtor -ansi -fPIC -fvisibility=hidden -std=c++11)
  set ( CMAKE_SHARED_LINKER_FLAGS "-Wl,--fatal-warnings -Wl,--no-undefined -lc ${CMAKE_SHARED_LINKER_FLAGS}")
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../mt32emu_lv2/include)

add_library(mt32emu-lv2ui SHARED
  ${mt32emu_lv2ui_HEADERS}
  ${mt32emu_lv2ui_SOURCES}
)
set_target_properties(mt32emu-lv2ui PROPERTIES PREFIX "")

target_link_libraries(mt32emu-lv2ui
  ${EXT_LIBS}
)

# installation
install(TARGETS mt32emu-lv2ui
    LIBRARY
    DESTINATION ${CMAKE_INSTALL_PREFIX}/${LV2_INSTALL_DIR}/mt32emu.lv2
)
