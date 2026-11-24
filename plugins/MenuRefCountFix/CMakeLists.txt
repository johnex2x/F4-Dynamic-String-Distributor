# Standalone F4SE plugin: patches the non-atomic Scaleform menu AddRef that some other plugins carry
# (see src/MenuRefCountFix.cpp). Built with the same patched CommonLibF4 as DynamicStringDistributor.
project(MenuRefCountFix VERSION 1.0.0 LANGUAGES CXX)

configure_file("${CMAKE_SOURCE_DIR}/cmake/Plugin.h.in" "${CMAKE_CURRENT_BINARY_DIR}/cmake/Plugin.h" @ONLY)

add_library(MenuRefCountFix SHARED
	src/main.cpp
	src/MenuRefCountFix.cpp
)
target_include_directories(MenuRefCountFix PRIVATE src "${CMAKE_CURRENT_BINARY_DIR}/cmake")
target_precompile_headers(MenuRefCountFix PRIVATE src/PCH.h)
target_compile_features(MenuRefCountFix PRIVATE cxx_std_23)
target_compile_options(MenuRefCountFix PRIVATE ${DSD_MSVC_OPTIONS})
target_compile_definitions(MenuRefCountFix PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_options(MenuRefCountFix PRIVATE
	"$<$<CONFIG:Release>:/DEBUG:FULL;/OPT:REF;/OPT:ICF;/INCREMENTAL:NO;/PDBALTPATH:$<TARGET_PDB_FILE_NAME:MenuRefCountFix>>"
)
target_link_libraries(MenuRefCountFix PRIVATE CommonLibF4::CommonLibF4)
