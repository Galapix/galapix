# Copyright Ingo Ruhnke <grumbel@gmail.com>
#
# Version source of truth is the top-level VERSION file (e.g. "0.2.0-dev").
# Git describe is only a fallback when VERSION is missing (standalone git trees).

function(get_project_version _outputvar)
  if(EXISTS "${CMAKE_SOURCE_DIR}/VERSION")
    file(STRINGS "${CMAKE_SOURCE_DIR}/VERSION" _file_version LIMIT_COUNT 1)
    string(STRIP "${_file_version}" _file_version)
    if(_file_version MATCHES "^\\$Format:")
      # Unexpanded export-subst placeholder — treat as missing.
      set(_file_version "")
    endif()
    if(NOT _file_version STREQUAL "")
      # Strip optional leading 'v'
      string(REGEX REPLACE "^v(.*)" "\\1" _file_version "${_file_version}")
      set(${_outputvar} "${_file_version}" PARENT_SCOPE)
      return()
    endif()
  endif()

  if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
    include(GetGitRevisionDescription)
    git_describe(GIT_REPO_VERSION "--tags" "--match" "v[0-9]*.[0-9]*.[0-9]*")
    string(REGEX REPLACE "^v([0-9].*)" "\\1" CLEANED_GIT_REPO_VERSION "${GIT_REPO_VERSION}")
    if(CLEANED_GIT_REPO_VERSION)
      set(${_outputvar} "${CLEANED_GIT_REPO_VERSION}" PARENT_SCOPE)
    else()
      set(${_outputvar} "${GIT_REPO_VERSION}" PARENT_SCOPE)
    endif()
    return()
  endif()

  get_filename_component(BASENAME "${CMAKE_SOURCE_DIR}" NAME)
  string(REGEX REPLACE "^${PROJECT_NAME}[-_]v?(.*)" "\\1" DIRECTORY_VERSION "${BASENAME}")
  if(NOT "${DIRECTORY_VERSION}" STREQUAL "${BASENAME}")
    set(${_outputvar} "${DIRECTORY_VERSION}" PARENT_SCOPE)
  else()
    set(${_outputvar} "0.0.0-dev" PARENT_SCOPE)
  endif()
endfunction()

get_project_version(PROJECT_VERSION)
message(STATUS "Project Version: ${PROJECT_VERSION}")
