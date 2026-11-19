file(MAKE_DIRECTORY "${TEST_DIR}")
configure_file("${SOURCE_DIR}/macros/source-cs137.mac" "${TEST_DIR}/source-cs137.mac" COPYONLY)
file(WRITE "${TEST_DIR}/run-test.mac"
  "/random/setSeeds 12345 67890\n"
  "/gps/verbose 2\n"
  "/control/execute ${SOURCE_DIR}/macros/run-decay-cs137.mac\n")

execute_process(
  COMMAND "${CMAKE_COMMAND}" -E env G4RUN_MANAGER_TYPE=Serial
          "${APP}" -r on -b "${TEST_DIR}/run-test.mac"
  RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT result STREQUAL "0")
  message(FATAL_ERROR "Cs-137 run failed (${result}):\n${output}\n${error}")
endif()

string(REGEX MATCHALL "Position: \\([-0-9.eE+]+,[-0-9.eE+]+,[-0-9.eE+]+\\)" positions "${output}")
list(LENGTH positions count)
if(NOT count EQUAL 200)
  message(FATAL_ERROR "Expected 200 primary positions; found ${count}")
endif()

set(x_low 0)
set(x_high 0)
set(y_low 0)
set(y_high 0)
set(z_low 0)
set(z_high 0)
foreach(position IN LISTS positions)
  string(REGEX REPLACE "^Position: \\(([^,]+),([^,]+),([^\\)]+)\\)$" "\\1" x "${position}")
  string(REGEX REPLACE "^Position: \\(([^,]+),([^,]+),([^\\)]+)\\)$" "\\2" y "${position}")
  string(REGEX REPLACE "^Position: \\(([^,]+),([^,]+),([^\\)]+)\\)$" "\\3" z "${position}")
  if(x LESS -2.376 OR x GREATER 2.376 OR y LESS -2.376 OR y GREATER 2.376
      OR z LESS -25.001 OR z GREATER 25.001)
    message(FATAL_ERROR "Primary position outside the cylinder's bounding box: ${position}")
  endif()
  if(x LESS -1.5)
    math(EXPR x_low "${x_low} + 1")
  endif()
  if(x GREATER 1.5)
    math(EXPR x_high "${x_high} + 1")
  endif()
  if(y LESS -1.5)
    math(EXPR y_low "${y_low} + 1")
  endif()
  if(y GREATER 1.5)
    math(EXPR y_high "${y_high} + 1")
  endif()
  if(z LESS -20)
    math(EXPR z_low "${z_low} + 1")
  endif()
  if(z GREATER 20)
    math(EXPR z_high "${z_high} + 1")
  endif()
endforeach()
if(x_low LESS 5 OR x_high LESS 5 OR y_low LESS 5 OR y_high LESS 5
    OR z_low LESS 5 OR z_high LESS 5)
  message(FATAL_ERROR "Primary positions do not span the source volume")
endif()
