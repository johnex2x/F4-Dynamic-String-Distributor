cmake_minimum_required(VERSION 3.21)
project(DynamicStringDistributor VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if("${PROJECT_SOURCE_DIR}" STREQUAL "${PROJECT_BINARY_DIR}")
	message(FATAL_ERROR "in-source builds are not allowed")
endif()

# CommonLibF4 (alandtse fork): build only the library, with OG and NG/AE support
set(F4SE_SUPPORT_XBYAK ON CACHE BOOL "" FORCE)
set(ENABLE_FALLOUT_F4 ON CACHE BOOL "" FORCE)
set(ENABLE_FALLOUT_NG ON CACHE BOOL "" FORCE)
set(ENABLE_FALLOUT_VR OFF CACHE BOOL "" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
# Keep the upstream submodule pinned; apply our fixes before configuring it.
# - runtime-detection: OG / AE runtime detection and strict Address Library lookup
# - scaleform-addref-atomic: RefCountImpl::AddRef incremented the plain count, not the atomic_ref
#   (same fix as libxse/commonlibsf#50; see plugins/MenuRefCountFix for plugins built without it)
find_package(Git REQUIRED)
set(DSD_COMMONLIB_PATCHES
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake/patches/commonlibf4-runtime-detection.patch"
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake/patches/commonlibf4-scaleform-addref-atomic.patch"
)
foreach(DSD_COMMONLIB_PATCH IN LISTS DSD_COMMONLIB_PATCHES)
	set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${DSD_COMMONLIB_PATCH}")
	execute_process(
		COMMAND "${GIT_EXECUTABLE}" apply --check "${DSD_COMMONLIB_PATCH}"
		WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/extern/CommonLibF4"
		RESULT_VARIABLE DSD_PATCH_CHECK
		ERROR_VARIABLE DSD_PATCH_ERROR
	)
	if(DSD_PATCH_CHECK STREQUAL "0")
		execute_process(
			COMMAND "${GIT_EXECUTABLE}" apply "${DSD_COMMONLIB_PATCH}"
			WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/extern/CommonLibF4"
			RESULT_VARIABLE DSD_PATCH_APPLY
			ERROR_VARIABLE DSD_PATCH_APPLY_ERROR
		)
		if(NOT DSD_PATCH_APPLY STREQUAL "0")
			message(FATAL_ERROR "Failed to apply ${DSD_COMMONLIB_PATCH}: ${DSD_PATCH_APPLY_ERROR}")
		endif()
	else()
		execute_process(
			COMMAND "${GIT_EXECUTABLE}" apply --reverse --check "${DSD_COMMONLIB_PATCH}"
			WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/extern/CommonLibF4"
			RESULT_VARIABLE DSD_PATCH_REVERSE_CHECK
			ERROR_VARIABLE DSD_PATCH_REVERSE_ERROR
		)
		if(NOT DSD_PATCH_REVERSE_CHECK STREQUAL "0")
			message(FATAL_ERROR "${DSD_COMMONLIB_PATCH} neither applies nor is already applied.\n${DSD_PATCH_ERROR}\n${DSD_PATCH_REVERSE_ERROR}")
		endif()
	endif()
endforeach()
add_subdirectory(extern/CommonLibF4/CommonLibF4 EXCLUDE_FROM_ALL)

find_package(xbyak CONFIG REQUIRED)
find_path(SIMPLEINI_INCLUDE_DIRS "SimpleIni.h")

find_package(glaze CONFIG REQUIRED)
find_package(boost_unordered CONFIG REQUIRED)
find_package(boost_container_hash CONFIG REQUIRED)
find_package(ZLIB REQUIRED)

set(DSD_MSVC_OPTIONS /utf-8 /permissive- /Zc:preprocessor /EHsc /MP /W4 /external:anglebrackets /external:W0 /Zi)

# Game-independent core (translation store); linked by the plugin and the tests
add_library(dsd_core STATIC
	src/core/ChargenLayout.cpp
	src/core/DialogueOptions.cpp
	src/core/NameSlots.cpp
	src/core/QuestLogNumbers.cpp
	src/core/TranslationStore.cpp
)
target_include_directories(dsd_core PUBLIC include)
target_compile_features(dsd_core PUBLIC cxx_std_23)
target_compile_options(dsd_core PRIVATE ${DSD_MSVC_OPTIONS})
target_link_libraries(dsd_core PUBLIC glaze::glaze Boost::unordered Boost::container_hash ZLIB::ZLIB)

configure_file(cmake/Plugin.h.in "${CMAKE_CURRENT_BINARY_DIR}/cmake/Plugin.h" @ONLY)

add_library("${PROJECT_NAME}" SHARED
	src/main.cpp
	src/ActivateText.cpp
	src/BodyPartsChargen.cpp
	src/ConstTranslation.cpp
	src/Dialogue.cpp
	src/DialogueOptions.cpp
	src/HookUtil.cpp
	src/LeveledListNames.cpp
	src/Manager.cpp
	src/MiscTypes.cpp
	src/NameDiag.cpp
	src/NameRefresh.cpp
	src/PortedConstFields.cpp
	src/RuntimeText.cpp
	src/Settings.cpp
	src/TemplateNames.cpp
)

target_include_directories("${PROJECT_NAME}" PRIVATE
	include
	"${CMAKE_CURRENT_BINARY_DIR}/cmake"
	"${SIMPLEINI_INCLUDE_DIRS}"
)

target_precompile_headers("${PROJECT_NAME}" PRIVATE include/PCH.h)

target_compile_options("${PROJECT_NAME}" PRIVATE ${DSD_MSVC_OPTIONS})
target_compile_definitions("${PROJECT_NAME}" PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_options("${PROJECT_NAME}" PRIVATE
	"$<$<CONFIG:Release>:/DEBUG:FULL;/OPT:REF;/OPT:ICF;/INCREMENTAL:NO;/PDBALTPATH:$<TARGET_PDB_FILE_NAME:${PROJECT_NAME}>>"
)

target_link_libraries("${PROJECT_NAME}" PRIVATE
	CommonLibF4::CommonLibF4
	xbyak::xbyak
	dsd_core
)

# Tests (no game needed)
option(DSD_BUILD_TESTS "Build the translation store tests" ON)
if(DSD_BUILD_TESTS)
	find_package(Catch2 3 CONFIG REQUIRED)
	include(CTest)
	include(Catch)

	add_executable(dsd_tests
		tests/TranslationStoreTests.cpp
		tests/Fallout4JsonTests.cpp
		tests/NameRulesTests.cpp
		tests/ChargenLayoutTests.cpp
		tests/DialogueOptionsTests.cpp
		tests/QuestLogNumbersTests.cpp
	)
	target_compile_options(dsd_tests PRIVATE ${DSD_MSVC_OPTIONS})
	target_compile_definitions(dsd_tests PRIVATE
		_CRT_SECURE_NO_WARNINGS
		DSD_DEFAULT_TRANSLATION_ROOT="${CMAKE_CURRENT_SOURCE_DIR}/F4SE/Plugins/DynamicStringDistributor"
	)
	target_link_libraries(dsd_tests PRIVATE dsd_core Catch2::Catch2WithMain)
	catch_discover_tests(dsd_tests)
endif()

# Standalone menu refcount fix (its own DLL; not part of DynamicStringDistributor)
add_subdirectory(plugins/MenuRefCountFix)
