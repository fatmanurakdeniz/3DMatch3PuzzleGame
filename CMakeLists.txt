cmake_minimum_required(VERSION 3.15)
project(OceanMatch3 CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew" "/usr/local")

find_package(OpenGL REQUIRED)
find_package(glfw3  REQUIRED)
find_package(JPEG REQUIRED)

set(OCEAN_BACKGROUND_DIR "${CMAKE_SOURCE_DIR}/../OceanMatchBackground")
set(OCEAN_BACKGROUND_DEPS "${OCEAN_BACKGROUND_DIR}/build/_deps")
set(OCEAN_BACKGROUND_ASSIMP_LIB
    "${OCEAN_BACKGROUND_DEPS}/assimp-build/bin/libassimp.5.4.3.dylib")

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    src
    ${OCEAN_BACKGROUND_DEPS}/glm-src
    ${OCEAN_BACKGROUND_DEPS}/assimp-src/include
    ${OCEAN_BACKGROUND_DEPS}/assimp-build/include
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    glfw
    OpenGL::GL
    JPEG::JPEG
    ${OCEAN_BACKGROUND_ASSIMP_LIB}
)

if(APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        BUILD_RPATH "@executable_path"
    )
endif()

# Copy shaders & assets next to the binary so relative paths work
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/shaders
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders
    COMMAND ${CMAKE_COMMAND} -E make_directory
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets/textures
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets/textures
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets/textures
    COMMAND ${CMAKE_COMMAND} -E make_directory
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets/background/textures
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets/background/textures
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets/background/textures
    COMMAND ${CMAKE_COMMAND} -E make_directory
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets/background/models
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets/background/models
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets/background/models
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${OCEAN_BACKGROUND_ASSIMP_LIB}
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/libassimp.5.dylib
    COMMAND /usr/bin/xattr -cr
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/libassimp.5.dylib
    COMMAND /usr/bin/codesign --force --sign -
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/libassimp.5.dylib
    COMMAND /usr/bin/xattr -cr
        $<TARGET_FILE:${PROJECT_NAME}>
    COMMAND /usr/bin/codesign --force --sign -
        $<TARGET_FILE:${PROJECT_NAME}>
)
