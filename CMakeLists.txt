cmake_minimum_required(VERSION 3.16...3.30)
project(sourceLab LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Geant4 REQUIRED ui_all vis_all)

file(GLOB SOURCE_LAB_SOURCES ${PROJECT_SOURCE_DIR}/src/*.cc)
file(GLOB SOURCE_LAB_HEADERS ${PROJECT_SOURCE_DIR}/include/*.hh)

add_executable(sourceLab sourceLab.cc ${SOURCE_LAB_SOURCES} ${SOURCE_LAB_HEADERS})
target_include_directories(sourceLab PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(sourceLab PRIVATE ${Geant4_LIBRARIES})

set(SOURCE_LAB_MACROS
  gui.mac
  init_vis.mac
  run.mac
)

foreach(_macro ${SOURCE_LAB_MACROS})
  if(EXISTS "${PROJECT_SOURCE_DIR}/macros/${_macro}")
    configure_file(
      ${PROJECT_SOURCE_DIR}/macros/${_macro}
      ${PROJECT_BINARY_DIR}/${_macro}
      COPYONLY
    )
  endif()
endforeach()

install(TARGETS sourceLab RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
