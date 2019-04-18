cmake_minimum_required(VERSION 3.0)

project("Vulkan Test")

set (CMAKE_CXX_STANDARD 14)
set(CMAKE_BUILD_TYPE Debug)

option(USE_INSTALLED_VKSDK "Use Installed Vulkan SDK" OFF)

# enables debug symbols
if(MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zi")
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -Wall -std=c++1y -lstdc++fs")
endif()

if(DEFINED ENV{VULKAN_SDK})
	set(VULKAN_SDK "$ENV{VULKAN_SDK}")
else()
	#if VULKAN_SDK isn't set, make sure you edit this!
	set(VULKAN_SDK "C:/VulkanSDK/1.1.97.0")
endif()

if(${USE_INSTALLED_VKSDK})
	if(NOT EXISTS "${VULKAN_SDK}")
		message(FATAL_ERROR "ERROR: Specified Vulkan SDK doesn't exist! (${VULKAN_SDK})'")
	endif()
endif()

#uncomment these if required libraries not installed, but have been built from source
set(VULKAN_INCLUDE "${VULKAN_SDK}/Include")
set(VULKAN_BINARIES "${VULKAN_SDK}/Lib32")

set(GLFW_INCLUDE "${CMAKE_SOURCE_DIR}/glfw/include")
set(GLFW_BINARIES "${CMAKE_SOURCE_DIR}/lib")

set(YAML_INCLUDE "${CMAKE_SOURCE_DIR}/yaml-cpp/include")
set(YAML_BINARIES "${CMAKE_SOURCE_DIR}/lib")

set(YAML_INCLUDE "${CMAKE_SOURCE_DIR}/yaml-cpp/include")
set(YAML_BINARIES "${CMAKE_SOURCE_DIR}/lib")

set(IRRKLANG_INCLUDE "${CMAKE_SOURCE_DIR}/irrklang/include")
set(IRRKLANG_BINARIES "${CMAKE_SOURCE_DIR}/lib/irrklang")

set(GLM_INCLUDE "${CMAKE_SOURCE_DIR}/glm")

set(STB_INCLUDE "${CMAKE_SOURCE_DIR}/stb")



include(sources.cmake)

add_executable(vulkan_test ${SOURCE_FILES} ${HEADER_FILES})

target_include_directories(vulkan_test PUBLIC "${CMAKE_SOURCE_DIR}/include")


include(CheckIncludeFile)
include(CheckIncludeFileCXX)

if(UNIX)
    include_directories("/usr/local/include")
endif()

###### GLFW LIBRARIES + HEADERS

list(APPEND GLFW_LIBRARY_NAMES "libglfw3" "glfw3" "libglfw" "glfw" "glfw3dll.lib")

find_library(GLFW_LIBRARY NAMES ${GLFW_LIBRARY_NAMES} HINTS ${GLFW_BINARIES})

get_filename_component(glfw_static "${GLFW_LIBRARY}" NAME)

get_filename_component(glfw_ext "${GLFW_LIBRARY}" EXT)

# when linking to the loader, copy glfw3.dll to the binary location
if("${glfw_static}" STREQUAL "glfw3dll.lib")
	add_custom_command(TARGET vulkan_test POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy "${GLFW_BINARIES}/glfw3.dll" "$<TARGET_FILE_DIR:vulkan_test>"
	)
endif()

if("${glfw_ext}" STREQUAL ".so")
	add_library(glfw3 SHARED IMPORTED)
else()
	add_library(glfw3 STATIC IMPORTED)
endif()


if(NOT GLFW_LIBRARY)
	message(FATAL_ERROR "GLFW library not installed and GLFW_BINARIES not specified!")
endif()

set_property(TARGET glfw3 PROPERTY IMPORTED_LOCATION "${GLFW_LIBRARY}")



check_include_file("GLFW/glfw3.h" GLFW_HEADER)

if(NOT GLFW_HEADER)
    if(GLFW_INCLUDE)
		target_include_directories(vulkan_test SYSTEM PUBLIC "${GLFW_INCLUDE}")
		message("Installed GLFW not found, using provided headers")
    else()
		message(FATAL_ERROR "GLFW headers not installed and GLFW_INCLUDE not specified!")
    endif()
endif()

###### VULKAN LIBRARIES + HEADERS

add_library(vulkan STATIC IMPORTED)

unset(VULKAN_LIBRARY)
find_library(VULKAN_LIBRARY NAMES vulkan libvulkan "vulkan-1" HINTS ${VULKAN_BINARIES})

if(NOT VULKAN_LIBRARY)
	message(FATAL_ERROR "Vulkan loader not installed and VULKAN_BINARIES not specified!")
endif()

set_property(TARGET vulkan PROPERTY IMPORTED_LOCATION "${VULKAN_LIBRARY}")



check_include_file("vulkan/vulkan.h" VULKAN_HEADER)

if(NOT VULKAN_HEADER)
    if(VULKAN_INCLUDE)
		target_include_directories(vulkan_test SYSTEM PUBLIC "${VULKAN_INCLUDE}")
		message("Installed Vulkan not found, using provided headers")
    else()
		message(FATAL_ERROR "Vulkan headers not installed and VULKAN_INCLUDE not specified!")
    endif()
endif()

###### YAML CPP LIBRARIES + HEADERS

add_library(yamlcpp STATIC IMPORTED)

find_library(YAML_LIBRARY NAMES "yaml-cpp" "libyaml-cpp" "yaml-cppmdd" "libyaml-cppmdd" HINTS ${YAML_BINARIES})

if(NOT YAML_LIBRARY)
    message(FATAL_ERROR "yaml-cpp static library not found - must be installed")
endif()


set_property(TARGET yamlcpp PROPERTY IMPORTED_LOCATION "${YAML_LIBRARY}")

check_include_file("yaml-cpp/yaml.h" YAML_HEADER)

if(NOT YAML_HEADER)
    if(YAML_INCLUDE)
		target_include_directories(vulkan_test SYSTEM PUBLIC "${YAML_INCLUDE}")
		message("Installed yaml-cpp not found, using provided headers")
    else()
		message(FATAL_ERROR "yaml-cpp headers not installed and YAML_INCLUDE not specified!")
    endif()
endif()

###### IRRLKANG LIBRARIES + headers

if(UNIX)

	find_library(ILK_LIBRARY "IrrKlang" HINTS "${IRRKLANG_BINARIES}")
	find_library(ILK_FLAC "ikpFlac" HINTS "${IRRKLANG_BINARIES}")
	find_library(ILK_MP3 "ikpMP3" HINTS "${IRRKLANG_BINARIES}")

	if(NOT ILK_LIBRARY OR NOT ILK_FLAC OR NOT ILK_MP3 )
		message(FATAL_ERROR "Could not find irrKlang libraries!")
	endif()

	# always copy the loaders to the binary location
	add_custom_command(TARGET vulkan_test POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy "${ILK_FLAC}" "$<TARGET_FILE_DIR:vulkan_test>"
		COMMAND ${CMAKE_COMMAND} -E copy "${ILK_MP3}" "$<TARGET_FILE_DIR:vulkan_test>"
	)

	message(${ILK_LIBRARY} ${ILK_FLAC} ${ILK_MP3})

	add_library(irrklang SHARED IMPORTED)
	set_property(TARGET irrklang PROPERTY IMPORTED_LOCATION "${ILK_LIBRARY}")

else()

	find_library(ILK_LIBRARY "irrKlang" HINTS "${IRRKLANG_BINARIES}")
	find_file(ILK_DYN "irrKlang.dll" HINTS "${IRRKLANG_BINARIES}")
	find_file(ILK_FLAC "ikpFlac.dll" HINTS "${IRRKLANG_BINARIES}")
	find_file(ILK_MP3 "ikpMP3.dll" HINTS "${IRRKLANG_BINARIES}")

	if(NOT ILK_LIBRARY OR NOT ILK_FLAC OR NOT ILK_MP3 OR NOT ILK_DYN)
		message(FATAL_ERROR "Could not find irrKlang libraries!")
	endif()

	# always copy the loaders to the binary location
	add_custom_command(TARGET vulkan_test POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy "${ILK_FLAC}" "$<TARGET_FILE_DIR:vulkan_test>"
		COMMAND ${CMAKE_COMMAND} -E copy "${ILK_MP3}" "$<TARGET_FILE_DIR:vulkan_test>"
		COMMAND ${CMAKE_COMMAND} -E copy "${ILK_DYN}" "$<TARGET_FILE_DIR:vulkan_test>"
	)

	add_library(irrklang STATIC IMPORTED)
	set_property(TARGET irrklang PROPERTY IMPORTED_LOCATION "${ILK_LIBRARY}")

endif()


check_include_file("irrklang/irrKlang.h" ILK_HEADER)

if(NOT ILK_HEADER)
    if(IRRKLANG_INCLUDE)
		target_include_directories(vulkan_test SYSTEM PUBLIC "${IRRKLANG_INCLUDE}")
		message("Installed irrKlang not found, using provided headers")
    else()
		message(FATAL_ERROR "irrKlang headers not installed and IRRKLANG_INCLUDE not specified!")
    endif()
endif()



###### GLM HEADERS

check_include_file_cxx("glm/glm.hpp" GLM_HEADER)

if(NOT GLM_HEADER)
    if(GLM_INCLUDE)
		target_include_directories(vulkan_test SYSTEM PUBLIC "${GLM_INCLUDE}")
		message("Installed GLM not found, using provided headers")
    else()
		message(FATAL_ERROR "GLM headers not installed and GLM_INCLUDE not specified!")
    endif()
endif()

###### STB HEADERS

check_include_file("stb/stb.h" STB_HEADER)

if(NOT STB_HEADER)
    if(STB_INCLUDE)
		target_include_directories(vulkan_test SYSTEM PUBLIC "${STB_INCLUDE}")
		message("Installed STB not found, using provided headers")
    else()
		message(FATAL_ERROR "STB headers not installed and STB_INCLUDE not specified!")
    endif()
endif()



#add the required libraries
target_link_libraries(vulkan_test PUBLIC vulkan)
target_link_libraries(vulkan_test PUBLIC glfw3)
target_link_libraries(vulkan_test PUBLIC yamlcpp)
target_link_libraries(vulkan_test PUBLIC irrklang)



#from https://gist.github.com/vlsh/a0d191701cb48f157b05be7f74d79396, compiles the shaders automatically

if (${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "AMD64")
    find_file(GLSL_VALIDATOR glslangValidator.exe HINTS "${VULKAN_SDK}/Bin/")
else()
    find_file(GLSL_VALIDATOR glslangValidator.exe HINTS "${VULKAN_SDK}/Bin32/")
endif()

if(NOT EXISTS "${GLSL_VALIDATOR}")
    find_file(GLSL_VALIDATOR glslangValidator)
endif()

if(NOT GLSL_VALIDATOR)
    find_file(GLSL_VALIDATOR glslangValidator.exe)
endif()

if(NOT GLSL_VALIDATOR)
    message(FATAL_ERROR "Could not find glslangValidator.exe (tried to find at ${GLSL_VALIDATOR} and in PATH)")
endif()

file(GLOB_RECURSE GLSL_SOURCE_FILES
    "shader/*.frag"
    "shader/*.vert"
)

foreach(GLSL ${GLSL_SOURCE_FILES})
  get_filename_component(FILE_NAME ${GLSL} NAME)
  set(SPIRV "${PROJECT_BINARY_DIR}/shader/${FILE_NAME}.spv")
  add_custom_command(
    OUTPUT ${SPIRV}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/shader/"
    COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV}
    DEPENDS ${GLSL})
  list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(
    Shaders 
    DEPENDS ${SPIRV_BINARY_FILES}
	SOURCES ${GLSL_SOURCE_FILES}
    )

add_dependencies(vulkan_test Shaders)

add_custom_command(TARGET vulkan_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:vulkan_test>/shader/"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_BINARY_DIR}/shader"
        "$<TARGET_FILE_DIR:vulkan_test>/shader"
)


#copies the sprites folder to the output
add_custom_command(TARGET vulkan_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:vulkan_test>/sprites/"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/sprites"
        "$<TARGET_FILE_DIR:vulkan_test>/sprites"
)

#copies the game folder to the output
add_custom_command(TARGET vulkan_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:vulkan_test>/game/"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/game"
        "$<TARGET_FILE_DIR:vulkan_test>/game"
)

#copies the sounds folder to the output
add_custom_command(TARGET vulkan_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:vulkan_test>/sounds/"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/sounds"
        "$<TARGET_FILE_DIR:vulkan_test>/sounds"
)
