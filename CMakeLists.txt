cmake_minimum_required(VERSION 3.16...3.30)
project(sourceLab LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Geant4 REQUIRED ui_all vis_all)

file(GLOB SOURCE_LAB_SOURCES CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/src/*.cc)
file(GLOB SOURCE_LAB_HEADERS ${PROJECT_SOURCE_DIR}/include/*.hh)

add_executable(sourceLab sourceLab.cc ${SOURCE_LAB_SOURCES} ${SOURCE_LAB_HEADERS})
target_include_directories(sourceLab PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(sourceLab PRIVATE ${Geant4_LIBRARIES})
target_compile_definitions(sourceLab PRIVATE SOURCELAB_SOURCE_DIR="${PROJECT_SOURCE_DIR}")

include(CTest)
if(BUILD_TESTING)
  add_executable(score_statistics_test tests/score_statistics.cc src/SourceLabScoreStatistics.cc)
  target_include_directories(score_statistics_test PRIVATE ${PROJECT_SOURCE_DIR}/include)
  target_link_libraries(score_statistics_test PRIVATE ${Geant4_LIBRARIES})
  add_test(NAME score_statistics COMMAND score_statistics_test)
  add_executable(water_geometry_test
    tests/water_geometry.cc
    src/SourceLabDetectorConstruction.cc
    src/SourceLabDetectorMessenger.cc)
  target_include_directories(water_geometry_test PRIVATE ${PROJECT_SOURCE_DIR}/include)
  target_link_libraries(water_geometry_test PRIVATE ${Geant4_LIBRARIES})
  add_test(NAME water_geometry_default COMMAND water_geometry_test)
  add_test(NAME water_geometry_custom COMMAND water_geometry_test custom)
  add_test(NAME tissue_geometry COMMAND water_geometry_test tissue)
  add_test(NAME cs137_source_distribution
    COMMAND ${CMAKE_COMMAND}
      "-DAPP=$<TARGET_FILE:sourceLab>"
      "-DSOURCE_DIR=${PROJECT_SOURCE_DIR}"
      "-DTEST_DIR=${PROJECT_BINARY_DIR}/cs137-source-test"
      "-DMT_ENABLED=${Geant4_multithreaded_FOUND}"
      -P "${PROJECT_SOURCE_DIR}/tests/cs137_source_distribution.cmake")
endif()

set(SOURCELAB_REQUIRED_MACROS
  gui.mac
  init_vis.mac
  vis.mac
  run.mac
  run-water-annulus.mac
  run-tissue-annulus.mac
  run-annulus.mac
  source-cs137.mac
  position-cs137-cylinder.mac
)

foreach(_macro ${SOURCELAB_REQUIRED_MACROS})
  if(NOT EXISTS "${PROJECT_SOURCE_DIR}/macros/${_macro}")
    message(FATAL_ERROR "Missing required macro: macros/${_macro}")
  endif()
  configure_file(
    ${PROJECT_SOURCE_DIR}/macros/${_macro}
    ${PROJECT_BINARY_DIR}/${_macro}
    COPYONLY
  )
endforeach()

install(TARGETS sourceLab RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
