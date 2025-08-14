cmake_minimum_required(VERSION 3.21)
project(ball LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# SFML 3.x из vcpkg
find_package(SFML 3 CONFIG REQUIRED COMPONENTS Graphics Window System)

add_executable(ball src/main.cpp)

target_link_libraries(ball PRIVATE
  SFML::Graphics
  SFML::Window
  SFML::System
)

# скопировать зависимости (.dll) рядом с exe после сборки
if (WIN32)
  add_custom_command(TARGET ball POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      $<TARGET_RUNTIME_DLLS:ball> $<TARGET_FILE_DIR:ball>
    COMMAND_EXPAND_LISTS)
endif()
