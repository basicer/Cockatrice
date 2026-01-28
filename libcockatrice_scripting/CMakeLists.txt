set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(HEADERS
  libcockatrice/scripting/script_manager.h
)

if(Qt6_FOUND)
  qt6_wrap_cpp(MOC_SOURCES ${HEADERS})
elseif(Qt5_FOUND)
  qt5_wrap_cpp(MOC_SOURCES ${HEADERS})
endif()

add_library(
  libcockatrice_scripting STATIC
  ${MOC_SOURCES}
  libcockatrice/scripting/script_manager.cpp
)


find_file(SOL2_INCLUDE_DIRS "sol/abort.hpp")
if(NOT SOL2_INCLUDE_DIRS)
  find_path(SOL2_INCLUDE_DIRS "sol/sol.hpp" PATHS ${CMAKE_SOURCE_DIR}/libcockatrice_scripting/vendor)
endif()


target_include_directories(
  libcockatrice_scripting
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
  PUBLIC ${SOL2_INCLUDE_DIRS}
)

target_link_libraries(
  libcockatrice_scripting
  PUBLIC libcockatrice_interfaces
  PUBLIC libcockatrice_utility
  PUBLIC ${QT_CORE_MODULE}
  PUBLIC ${LUA_LIBRARIES}
)
