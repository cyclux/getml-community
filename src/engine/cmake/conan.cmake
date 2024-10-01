function(install_conan)
  execute_process(
    COMMAND ${CONAN_EXECUTABLE} --version
    OUTPUT_STRIP_TRAILING_WHITESPACE
    OUTPUT_VARIABLE CONAN_VERSION
  )
  message(STATUS "Using Conan version: ${CONAN_VERSION}")

  if(NOT $ENV{CONAN_PROFILE} STREQUAL "")
    message(STATUS "Using Conan profile: $ENV{CONAN_PROFILE}")
    set(USE_CONAN_PROFILE --profile:all $ENV{CONAN_PROFILE} CACHE STRING "Conan profile")
  endif()

  if(EXISTS "${CMAKE_BINARY_DIR}/conan/conan_toolchain.cmake")
    message(STATUS "Conan powered build already set up to: ${CMAKE_BINARY_DIR}")
    message(STATUS "Re-running Conan to update dependencies.")
  endif()

  execute_process(
    COMMAND ${CONAN_EXECUTABLE}
    install ${CMAKE_SOURCE_DIR}
    ${USE_CONAN_PROFILE}
    --output-folder ${CMAKE_BINARY_DIR}/conan
    --build missing
    --settings build_type=${CMAKE_BUILD_TYPE}
    COMMAND_ERROR_IS_FATAL ANY
  )

  message(STATUS "Conan powered build set up to: ${CMAKE_BINARY_DIR}")
  message(STATUS "Conan toolchain file: ${CMAKE_BINARY_DIR}/conan/conan_toolchain.cmake")

  set(
    CMAKE_TOOLCHAIN_FILE
      ${CMAKE_BINARY_DIR}/conan/conan_toolchain.cmake
    CACHE
    FILEPATH
    "Path to conan toolchain file")

endfunction(install_conan)


function(install_local_conan_repository)
  execute_process(
    COMMAND ${CONAN_EXECUTABLE}
    remote list
    OUTPUT_VARIABLE CONAN_REMOTES
  )
  set(LOCAL_ENGINE_CONAN_REPOSITORY "${CMAKE_SOURCE_DIR}/conan-repository")
  set(LOCAL_ENGINE_CONAN_REPOSITORY_NAME "conan-remote-repository:${CMAKE_SOURCE_DIR}/conan-repository")
  if("${CONAN_REMOTES}" MATCHES ".*${LOCAL_ENGINE_CONAN_REPOSITORY_NAME}: ${LOCAL_ENGINE_CONAN_REPOSITORY}.*")
    message(STATUS "Conan remote already added: ${LOCAL_ENGINE_CONAN_REPOSITORY_NAME} at ${LOCAL_ENGINE_CONAN_REPOSITORY}")
  else()
    execute_process(
      COMMAND ${CONAN_EXECUTABLE}
      remote add
      ${LOCAL_ENGINE_CONAN_REPOSITORY_NAME}
      ${LOCAL_ENGINE_CONAN_REPOSITORY}
    )
    message(STATUS "Conan remote added: ${LOCAL_ENGINE_CONAN_REPOSITORY_NAME} at ${LOCAL_ENGINE_CONAN_REPOSITORY}")
  endif()

endfunction(install_local_conan_repository)

function(uninstall_local_conan_repository)
  execute_process(
    COMMAND ${CONAN_EXECUTABLE}
    remote list
    OUTPUT_VARIABLE CONAN_REMOTES
  )
  set(LOCAL_ENGINE_CONAN_REPOSITORY "${CMAKE_SOURCE_DIR}/conan-repository")
  set(LOCAL_ENGINE_CONAN_REPOSITORY_NAME "conan-remote-repository:${CMAKE_SOURCE_DIR}/conan-repository")
  if("${CONAN_REMOTES}" MATCHES ".*${LOCAL_ENGINE_CONAN_REPOSITORY_NAME}: ${LOCAL_ENGINE_CONAN_REPOSITORY}.*")
    execute_process(
      COMMAND ${CONAN_EXECUTABLE}
      remote remove
      ${LOCAL_ENGINE_CONAN_REPOSITORY_NAME}
    )
    message(STATUS "Conan remote removed: ${LOCAL_ENGINE_CONAN_REPOSITORY_NAME} at ${LOCAL_ENGINE_CONAN_REPOSITORY}")
  endif()

endfunction(uninstall_local_conan_repository)


function(prepare_conan)
  set(
    USE_CONAN
      OFF
    CACHE
    BOOL
    "Use conan to install dependencies")

  if(NOT USE_CONAN)
    message(STATUS "Not using Conan to install dependencies.")
    return()
  endif()

  message(STATUS "Using Conan to install dependencies.")

  if(DEFINED ENV{CONAN_BINARY})
    if(NOT EXISTS $ENV{CONAN_BINARY})
      message(FATAL_ERROR "Variable CONAN_BINARY must be set to the binary with its full path.")
      return()
    endif()

    message(STATUS "Using CONAN_BINARY from environment variable $ENV{CONAN_BINARY}.")
    set(
      CONAN_BINARY
        $ENV{CONAN_BINARY}
      CACHE
      FILEPATH
      "Conan binary with its full path"
      FORCE)

  else()
    set(
      CONAN_BINARY
        "CONAN_BINARY_NOT_FOUND"
      CACHE
      FILEPATH
      "Conan binary with its full path")

    if("${CONAN_BINARY}" STREQUAL "CONAN_BINARY_NOT_FOUND" OR "${CONAN_BINARY}" STREQUAL "")
      message(FATAL_ERROR "Variable CONAN_BINARY must be set when using conan via USE_CONAN=ON.")
      return()
    endif()

    message(STATUS "Using CONAN_BINARY from cache variable ${CONAN_BINARY}.")
  endif()

  find_program(CONAN_EXECUTABLE ${CONAN_BINARY} NO_CACHE REQUIRED)

  install_local_conan_repository()
  install_conan()
  uninstall_local_conan_repository()

endfunction(prepare_conan)
