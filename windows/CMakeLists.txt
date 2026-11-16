# 빠른 한문 입력기 — Windows 11 TSF 입력기
cmake_minimum_required(VERSION 3.20)
project(hanmun VERSION 0.1.2 LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# 정적 런타임: 따로 VC++ 재배포 패키지를 깔 필요 없게
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# 웹 버전 JSON 사전 → hanmun.dict (DLL 안에 넣는다)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(DICT_PATH ${CMAKE_BINARY_DIR}/hanmun.dict)
file(GLOB DICT_JSON ${CMAKE_SOURCE_DIR}/../web/src/*.json)
add_custom_command(
  OUTPUT ${DICT_PATH}
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/make_dict.py ${CMAKE_SOURCE_DIR}/../web/src ${DICT_PATH}
  DEPENDS ${CMAKE_SOURCE_DIR}/tools/make_dict.py ${DICT_JSON}
  COMMENT "사전 만드는 중")
add_custom_target(dict DEPENDS ${DICT_PATH})

set(ICON_PATH ${CMAKE_SOURCE_DIR}/src/hanmun.ico)
set(V1 ${PROJECT_VERSION_MAJOR})
set(V2 ${PROJECT_VERSION_MINOR})
set(V3 ${PROJECT_VERSION_PATCH})
configure_file(src/hanmun.rc.in ${CMAKE_BINARY_DIR}/hanmun.rc @ONLY)
set_source_files_properties(${CMAKE_BINARY_DIR}/hanmun.rc PROPERTIES OBJECT_DEPENDS "${DICT_PATH};${ICON_PATH}")

add_library(hanmun SHARED
  src/Dll.cpp
  src/TextService.cpp
  src/CandidateWindow.cpp
  src/LangBarButton.cpp
  src/Engine.cpp
  src/Hangul.cpp
  src/hanmun.def
  ${CMAKE_BINARY_DIR}/hanmun.rc)
add_dependencies(hanmun dict)
target_compile_definitions(hanmun PRIVATE UNICODE _UNICODE NOMINMAX)
target_compile_options(hanmun PRIVATE /utf-8 /W3 /EHsc /permissive-)
target_link_libraries(hanmun PRIVATE ole32 oleaut32 uuid advapi32 user32 gdi32 shell32 dwmapi)
