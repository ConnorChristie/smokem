message(STATUS "Using imgui from git")
if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/imgui")

execute_process(COMMAND "${GIT}" clone "https://github.com/ocornut/imgui"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty"
)
else()
execute_process(COMMAND "${GIT}" pull
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/imgui"
)
endif()

set(IMGUI_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/3rdparty/imgui)
file(GLOB IMGUI_SOURCES ${IMGUI_INCLUDE_DIR}/*.cpp)
file(GLOB IMGUI_HEADERS ${IMGUI_INCLUDE_DIR}/*.h)

add_library(imgui STATIC ${IMGUI_SOURCES} ${IMGUI_SOURCES})

add_definitions(-DIMGUI_IMPL_OPENGL_LOADER_GLAD)

include_directories(
    ${IMGUI_INCLUDE_DIR}
    ${OPENGL_INCLUDE_DIR}
    ${GLFW_INCLUDE_DIR}
    ${GLAD_INCLUDE_DIR})

target_link_libraries(imgui
    ${OPENGL_LIBRARIES}
    ${GLFW_LIBRARIES}
    ${GLAD_LIBRARIES})

set_target_properties(imgui PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(imgui PROPERTIES FOLDER 3rdparty)

set(IMGUI_LIBRARIES imgui)
