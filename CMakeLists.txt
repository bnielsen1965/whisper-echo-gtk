cmake_minimum_required(VERSION 3.16)
project(whisper-echo-gtk C)

find_package(Git QUIET)
if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --tags --abbrev=0 --always
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_TAG
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE GIT_TAG_RESULT
    )
    if(GIT_TAG_RESULT EQUAL 0 AND GIT_TAG MATCHES "^v?[0-9]")
        string(REGEX REPLACE "^v" "" PROJECT_VERSION "${GIT_TAG}")
    else()
        set(PROJECT_VERSION "0.3.0-dev")
    endif()

    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --tags --long --always
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_DESCRIBE
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE GIT_DESCRIBE_RESULT
    )
    if(GIT_DESCRIBE_RESULT EQUAL 0)
        string(REGEX REPLACE "^v" "" GIT_DESCRIBE "${GIT_DESCRIBE}")
        string(REGEX MATCH "^[^ -]+" GIT_BASE "${GIT_DESCRIBE}")
        string(REGEX MATCH "-([0-9]+)-g" GIT_COMMIT_COUNT "${GIT_DESCRIBE}")
        if(GIT_COMMIT_COUNT)
            string(REGEX REPLACE ".*-([0-9]+)-g.*" "\\1" PROJECT_RELEASE "${GIT_COMMIT_COUNT}")
        else()
            set(PROJECT_RELEASE "1")
        endif()
        if(NOT PROJECT_RELEASE)
            set(PROJECT_RELEASE "1")
        endif()
    else()
        set(PROJECT_RELEASE "1")
    endif()
else()
    set(PROJECT_VERSION "0.3.0-dev")
    set(PROJECT_RELEASE "1")
endif()

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
install(FILES data/whisper-echo-gtk.desktop DESTINATION share/applications)
install(FILES data/icon.svg DESTINATION share/icons/hicolor/scalable/apps RENAME whisper-echo-gtk.svg)
install(FILES docs/man/whisper-echo-gtk.1.gz DESTINATION share/man/man1)
install(DIRECTORY docs/ DESTINATION share/doc/whisper-echo-gtk)

# Copy data directory to build directory for development
file(COPY data/ DESTINATION ${CMAKE_BINARY_DIR}/data)

set(CPACK_PROJECT_NAME "whisper-echo-gtk")
set(CPACK_PACKAGE_NAME "whisper-echo-gtk")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_VENDOR "whisper-echo-gtk")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "GTK4 GUI frontend for whisper-echo")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/README.md")
set(CPACK_PACKAGE_LICENSE "MIT")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_CONTACT "Bryan Nielsen <bnielsen1965@gmail.com>")
set(CPACK_PACKAGE_URL "https://github.com/bnielsen1965/whisper-echo")
set(CPACK_GENERATOR "RPM;DEB")

set(CPACK_RPM_PACKAGE_NAME "whisper-echo-gtk")
set(CPACK_RPM_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_RPM_PACKAGE_RELEASE "${PROJECT_RELEASE}")
set(CPACK_RPM_PACKAGE_LICENSE "MIT")
set(CPACK_RPM_PACKAGE_REQUIRES "whisper-echo, gtk4, libadwaita")
set(CPACK_RPM_PACKAGE_GROUP "Applications/Graphics")
set(CPACK_RPM_PACKAGE_URL "https://github.com/bnielsen1965/whisper-echo")
set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION "/usr/share")

set(CPACK_DEBIAN_PACKAGE_NAME "whisper-echo-gtk")
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "${CMAKE_SYSTEM_PROCESSOR}")
set(CPACK_DEBIAN_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_DEBIAN_PACKAGE_RELEASE "${PROJECT_RELEASE}")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Bryan Nielsen <bnielsen1965@gmail.com>")
set(CPACK_DEBIAN_PACKAGE_SECTION "multimedia")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "whisper-echo (>= 0.1.0), libgtk-4-1, libadwaita-1-0")
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_SOURCE_DIR}/packaging/deb/postinst;${CMAKE_SOURCE_DIR}/packaging/deb/prerm")

include(CPack)
