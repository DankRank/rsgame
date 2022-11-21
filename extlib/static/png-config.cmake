if(${CMAKE_SIZEOF_VOID_P} MATCHES 8)
	set(_ZLIB_PREFIX "${CMAKE_CURRENT_LIST_DIR}/../zlib/build64")
	set(_PNG_PREFIX "${CMAKE_CURRENT_LIST_DIR}/../libpng/build64")
else()
	set(_ZLIB_PREFIX "${CMAKE_CURRENT_LIST_DIR}/../zlib/build32")
	set(_PNG_PREFIX "${CMAKE_CURRENT_LIST_DIR}/../libpng/build32")
endif()
add_library(PNG::PNG STATIC IMPORTED)
set_target_properties(PNG::PNG PROPERTIES
	INTERFACE_INCLUDE_DIRECTORIES "${_PNG_PREFIX}/include"
	INTERFACE_LINK_LIBRARIES "${_ZLIB_PREFIX}/Release/zlibstatic.lib"
	IMPORTED_LOCATION "${_PNG_PREFIX}/Release/libpng16_static.lib"
)
set(_PNG_PREFIX)
