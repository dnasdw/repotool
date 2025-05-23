AUTO_FILES("." "src" "\\.(cpp|h)$")
AUTO_FILES("${ROOT_SOURCE_DIR}/dep/libsundaowen" "src" "\\.(cpp|h)$")
AUTO_FILES("${ROOT_SOURCE_DIR}/dep/rapidjson/include" "src" "\\.h$")
include_directories(${DEP_INCLUDE_DIR})
link_directories(${DEP_LIBRARY_DIR})
add_definitions(-DSDW_MAIN -DCURL_STATICLIB)
if(APPLE)
  add_definitions(-DSDW_XCONVERT)
endif()
ADD_EXE(repotool "${src}")
if(WIN32)
  if(MSVC)
    if(MSVC_VERSION LESS 1800)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4005")
    endif()
    target_link_libraries(repotool libcurl libcrypto WS2_32 Wldap32 Crypt32)
    set(repotool_LINK_FLAGS_DEBUG "${repotool_LINK_FLAGS_DEBUG} /NODEFAULTLIB:LIBCMT")
    if(MSVC_VERSION GREATER 1700)
      set(repotool_LINK_FLAGS_DEBUG "${repotool_LINK_FLAGS_DEBUG} /IGNORE:4099")
      set(repotool_LINK_FLAGS_RELWITHDEBINFO "${repotool_LINK_FLAGS_RELWITHDEBINFO} /IGNORE:4099")
    endif()
    set_target_properties(repotool PROPERTIES
      LINK_FLAGS_DEBUG "${repotool_LINK_FLAGS_DEBUG}"
      LINK_FLAGS_RELWITHDEBINFO "${repotool_LINK_FLAGS_RELWITHDEBINFO}")
  else()
    target_link_libraries(repotool curl crypto)
  endif()
else()
  target_link_libraries(repotool curl ssl crypto)
  if(APPLE)
    target_link_libraries(repotool ldap)
  else()
    target_link_libraries(repotool pthread dl)
  endif()
  if(APPLE OR CYGWIN)
    target_link_libraries(repotool iconv)
  endif()
  if(APPLE)
    target_link_libraries(repotool omp)
  endif()
endif()
install(TARGETS repotool DESTINATION bin)
