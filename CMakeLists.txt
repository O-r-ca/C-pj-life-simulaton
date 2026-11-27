cmake_minimum_required(VERSION 3.20)
project(LifeSim LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 소스 파일이 UTF-8(한글)이라 MSVC에 알려준다
if(MSVC)
    add_compile_options(/utf-8)
endif()

include(FetchContent)

# D: 드라이브가 exFAT이라 git clone(소유자 검사)과 압축 해제 시 시간 복원이 실패한다.
# 그래서 zip으로 받고, 원래 시간은 복원하지 않는다.

# --- GLFW (창 + 입력) ---
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(glfw
    URL https://github.com/glfw/glfw/archive/refs/tags/3.4.zip
    DOWNLOAD_EXTRACT_TIMESTAMP FALSE)

# --- Dear ImGui ---
FetchContent_Declare(imgui
    URL https://github.com/ocornut/imgui/archive/refs/tags/v1.91.5.zip
    DOWNLOAD_EXTRACT_TIMESTAMP FALSE)

# --- nlohmann/json (스토리·시나리오 JSON 읽기) ---
FetchContent_Declare(json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
    DOWNLOAD_EXTRACT_TIMESTAMP FALSE)

# --- ImPlot (그래프) ---
FetchContent_Declare(implot
    URL https://github.com/epezent/implot/archive/refs/tags/v0.16.zip
    DOWNLOAD_EXTRACT_TIMESTAMP FALSE)

FetchContent_MakeAvailable(glfw imgui json implot)

find_package(OpenGL REQUIRED)

add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp)
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_link_libraries(imgui PUBLIC glfw OpenGL::GL)

add_library(implot STATIC
    ${implot_SOURCE_DIR}/implot.cpp
    ${implot_SOURCE_DIR}/implot_items.cpp)
target_include_directories(implot PUBLIC ${implot_SOURCE_DIR})
target_link_libraries(implot PUBLIC imgui)

# --- 엔진 (화면 의존 없음) ---
add_library(engine STATIC
    engine/Expr.cpp
    engine/FakeEngine.cpp
    engine/SimEngine.cpp
    engine/Worlds.cpp)
target_include_directories(engine PUBLIC engine)
target_link_libraries(engine PRIVATE nlohmann_json::nlohmann_json)

# --- ImGui 화면 ---
add_executable(lifesim
    ui/imgui/main.cpp
    ui/imgui/Theme.cpp
    ui/imgui/Widgets.cpp)
target_link_libraries(lifesim PRIVATE engine imgui implot)
# 개발 중엔 프로젝트 폴더의 story/, scenarios/를 바로 읽는다 (JSON 고치면 재빌드 없이 반영)
target_compile_definitions(lifesim PRIVATE LIFESIM_DATA_DIR="${CMAKE_SOURCE_DIR}")

# --- 테스트 ---
enable_testing()
add_executable(fake_engine_test tests/fake_engine_test.cpp)
target_link_libraries(fake_engine_test PRIVATE engine)
target_compile_definitions(fake_engine_test PRIVATE LIFESIM_DATA_DIR="${CMAKE_SOURCE_DIR}")
add_test(NAME fake_engine_test COMMAND fake_engine_test)

add_executable(sim_engine_test tests/sim_engine_test.cpp)
target_link_libraries(sim_engine_test PRIVATE engine)
target_compile_definitions(sim_engine_test PRIVATE LIFESIM_DATA_DIR="${CMAKE_SOURCE_DIR}")
add_test(NAME sim_engine_test COMMAND sim_engine_test)

# 수치 균형 리포트 (테스트가 아니라 조율용 도구)
add_executable(balance_report tests/balance_report.cpp)
target_link_libraries(balance_report PRIVATE engine)
target_compile_definitions(balance_report PRIVATE LIFESIM_DATA_DIR="${CMAKE_SOURCE_DIR}")
