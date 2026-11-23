file(MAKE_DIRECTORY "${TEST_DIR}")
foreach(macro source-cs137.mac source.mac position-cs137-cylinder.mac
              run-water-annulus.mac run-tissue-annulus.mac run-annulus.mac)
  configure_file("${SOURCE_DIR}/macros/${macro}" "${TEST_DIR}/${macro}" COPYONLY)
endforeach()

foreach(macro run.mac run-water-annulus.mac run-tissue-annulus.mac run-simple.mac run-decay-cs137.mac run-cs137-statistics.mac vis-simple.mac init_vis.mac)
  file(READ "${SOURCE_DIR}/macros/${macro}" content)
  # Exercise the visualization macros' source setup without requiring a display.
  string(REPLACE "/vis/open OGL\n" "" content "${content}")
  string(REPLACE "/control/execute vis.mac\n" "" content "${content}")
  string(REPLACE "/run/beamOn 5\n" "/run/beamOn 100\n" content "${content}")
  string(REPLACE "/run/beamOn 10000000\n" "/run/beamOn 100\n" content "${content}")
  if(macro STREQUAL "init_vis.mac")
    string(APPEND content "\n/run/beamOn 100\n")
  endif()
  file(WRITE "${TEST_DIR}/run-test.mac"
    "/random/setSeeds 12345 67890\n/gps/verbose 2\n${content}")

  execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env G4RUN_MANAGER_TYPE=Serial "LAB_OUTPUT_DIR=${TEST_DIR}/output"
            "${APP}" -b "${TEST_DIR}/run-test.mac"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
  if(NOT result STREQUAL "0")
    message(FATAL_ERROR "${macro} failed (${result}):\n${output}\n${error}")
  endif()
  string(REGEX MATCH "Output path: ([^\r\n]+)" path_line "${output}")
  set(output_file "${CMAKE_MATCH_1}")
  string(FIND "${output_file}" "${TEST_DIR}/output/" prefix)
  list(FIND previous_outputs "${output_file}" duplicate)
  if(NOT path_line OR NOT EXISTS "${output_file}" OR NOT prefix EQUAL 0
      OR NOT duplicate EQUAL -1)
    message(FATAL_ERROR "${macro}: output is missing or not unique under the configured output root")
  endif()
  list(APPEND previous_outputs "${output_file}")
  if(NOT output MATCHES "Run complete:.*source=cs137.*decay=on")
    message(FATAL_ERROR "${macro}: expected Cs-137 metadata and radioactive decay enabled")
  endif()
  if(macro STREQUAL "run-tissue-annulus.mac" AND NOT output MATCHES "material=G4_TISSUE_SOFT_ICRP")
    message(FATAL_ERROR "${macro}: expected ICRP soft-tissue material")
  endif()
  if((macro STREQUAL "run.mac" OR macro STREQUAL "run-water-annulus.mac")
      AND NOT output MATCHES "file=sourceLab-run-simple.root.*material=G4_WATER.*geometry=water-annulus")
    message(FATAL_ERROR "${macro}: expected the default water output and metadata")
  endif()
  if(macro STREQUAL "run-cs137-statistics.mac"
      AND NOT output MATCHES "file=sourceLab-cs137-statistics-10m.root")
    message(FATAL_ERROR "${macro}: expected its dedicated output file")
  endif()

  set(expected 100)
  if(macro STREQUAL "run-decay-cs137.mac")
    set(expected 200)
  endif()
  string(REGEX MATCHALL "Particle name: Cs137[\r\n ]+Energy: 0[\r\n ]+Position: \\([-0-9.eE+]+,[-0-9.eE+]+,[-0-9.eE+]+\\)" primaries "${output}")
  list(LENGTH primaries count)
  if(NOT count EQUAL expected)
    message(FATAL_ERROR "${macro}: expected ${expected} Cs-137 ions at rest; found ${count}")
  endif()

  set(x_low 0)
  set(x_high 0)
  set(y_low 0)
  set(y_high 0)
  set(z_low 0)
  set(z_high 0)
  foreach(primary IN LISTS primaries)
    string(REGEX REPLACE ".*Position: \\(([^,]+),([^,]+),([^\\)]+)\\)$" "\\1" x "${primary}")
    string(REGEX REPLACE ".*Position: \\(([^,]+),([^,]+),([^\\)]+)\\)$" "\\2" y "${primary}")
    string(REGEX REPLACE ".*Position: \\(([^,]+),([^,]+),([^\\)]+)\\)$" "\\3" z "${primary}")
    if(x LESS -2.376 OR x GREATER 2.376 OR y LESS -2.376 OR y GREATER 2.376
        OR z LESS -25.001 OR z GREATER 25.001)
      message(FATAL_ERROR "${macro}: primary position outside the cylinder's bounding box: ${primary}")
    endif()
    foreach(axis x y)
      if(${axis} LESS -1.5)
        math(EXPR ${axis}_low "${${axis}_low} + 1")
      endif()
      if(${axis} GREATER 1.5)
        math(EXPR ${axis}_high "${${axis}_high} + 1")
      endif()
    endforeach()
    if(z LESS -20)
      math(EXPR z_low "${z_low} + 1")
    endif()
    if(z GREATER 20)
      math(EXPR z_high "${z_high} + 1")
    endif()
  endforeach()
  if(x_low LESS 3 OR x_high LESS 3 OR y_low LESS 3 OR y_high LESS 3
      OR z_low LESS 3 OR z_high LESS 3)
    message(FATAL_ERROR "${macro}: primary positions do not span the source volume")
  endif()
endforeach()

file(WRITE "${TEST_DIR}/late-material.mac"
  "/run/initialize\n/sourceLab/geometry/phantomMaterial G4_TISSUE_SOFT_ICRP\n")
execute_process(
  COMMAND "${CMAKE_COMMAND}" -E env G4RUN_MANAGER_TYPE=Serial "LAB_OUTPUT_DIR=${TEST_DIR}/output"
          "${APP}" -b "${TEST_DIR}/late-material.mac"
  RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(result STREQUAL "0" OR NOT "${output}${error}" MATCHES "phantomMaterial")
  message(FATAL_ERROR "Material changes after initialization must be rejected:\n${output}\n${error}")
endif()

file(WRITE "${TEST_DIR}/multiple-runs.mac"
  "/run/initialize\n/gps/particle gamma\n/gps/energy 1 MeV\n/run/beamOn 2\n/run/beamOn 2\n")
set(modes Serial)
if(MT_ENABLED)
  list(APPEND modes MT)
endif()
foreach(mode IN LISTS modes)
  set(thread_args)
  if(mode STREQUAL "MT")
    set(thread_args -t 2)
  endif()
  execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env "G4RUN_MANAGER_TYPE=${mode}" "LAB_OUTPUT_DIR=${TEST_DIR}/output"
            "${APP}" ${thread_args} -b "${TEST_DIR}/multiple-runs.mac"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
  string(REGEX MATCHALL "Output path: [^\r\n]+" paths "${output}")
  list(LENGTH paths path_count)
  list(REMOVE_DUPLICATES paths)
  list(LENGTH paths unique_path_count)
  if(NOT result STREQUAL "0" OR NOT path_count EQUAL 2 OR NOT unique_path_count EQUAL 2)
    message(FATAL_ERROR "${mode}: repeated runs did not create distinct output files:\n${output}\n${error}")
  endif()
  foreach(path IN LISTS paths)
    string(REPLACE "Output path: " "" path "${path}")
    if(NOT EXISTS "${path}")
      message(FATAL_ERROR "${mode}: repeated run output is missing: ${path}")
    endif()
  endforeach()
endforeach()

execute_process(
  COMMAND "${CMAKE_COMMAND}" -E env G4RUN_MANAGER_TYPE=Serial "LAB_OUTPUT_DIR=${TEST_DIR}/output"
          "${APP}" -r off -b "${TEST_DIR}/run-test.mac"
  RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT result STREQUAL "0" OR NOT output MATCHES "Run complete:.*source=cs137.*decay=off")
  message(FATAL_ERROR "Explicit -r off failed (${result}):\n${output}\n${error}")
endif()
