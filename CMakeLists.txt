cmake_minimum_required(VERSION 3.16)
project(whisper-echo-gtk C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Find GTK4, GLib and libadwaita
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED gtk4>=4.0)
pkg_check_modules(GLIB REQUIRED glib-2.0 gobject-2.0 gio-2.0)
pkg_check_modules(LIBADWAITA REQUIRED libadwaita-1)

# Add executable
add_executable(whisper-echo-gtk
    src/main.c
    src/window.c
    src/config.c
    src/process.c
)

# Include directories
target_include_directories(whisper-echo-gtk PRIVATE
    ${GTK4_INCLUDE_DIRS}
    ${GLIB_INCLUDE_DIRS}
    ${LIBADWAITA_INCLUDE_DIRS}
    src/
)

# Link libraries
target_link_libraries(whisper-echo-gtk PRIVATE
    ${GTK4_LIBRARIES}
    ${GLIB_LIBRARIES}
    ${LIBADWAITA_LIBRARIES}
)

# Compile options (flags from pkg-config)
target_compile_options(whisper-echo-gtk PRIVATE
    ${GTK4_CFLAGS_OTHER}
    ${GLIB_CFLAGS_OTHER}
    ${LIBADWAITA_CFLAGS_OTHER}
)

# Debugging symbols for gdb
target_compile_options(whisper-echo-gtk PRIVATE
    -g
    -O0
    -ggdb
)

# Install rules
install(TARGETS whisper-echo-gtk DESTINATION bin)
install(DIRECTORY data/ DESTINATION share/whisper-echo-gtk)

# Copy data directory to build directory for development
file(COPY data/ DESTINATION ${CMAKE_BINARY_DIR}/data)
