if(${CMAKE_SIZEOF_VOID_P} MATCHES 8)
	set(_ZLIB_PREFIX "${CMAKE_CURRENT_LIST_DIR}/../zlib/build64")
else()
	set(_ZLIB_PREFIX "${CMAKE_CURRENT_LIST_DIR}/../zlib/build32")
endif()
add_library(ZLIB::ZLIB STATIC IMPORTED)
set_target_properties(ZLIB::ZLIB PROPERTIES
	INTERFACE_INCLUDE_DIRECTORIES "${_ZLIB_PREFIX}/include"
	IMPORTED_LOCATION "${_ZLIB_PREFIX}/Release/zlibstatic.lib"
)
set(_ZLIB_PREFIX)
