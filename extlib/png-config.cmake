if(${CMAKE_SIZEOF_VOID_P} MATCHES 8)
	set(_PNG_PREFIX "${CMAKE_CURRENT_LIST_DIR}/libpng/build64")
else()
	set(_PNG_PREFIX "${CMAKE_CURRENT_LIST_DIR}/libpng/build32")
endif()
add_library(PNG::PNG SHARED IMPORTED)
set_target_properties(PNG::PNG PROPERTIES
	INTERFACE_INCLUDE_DIRECTORIES "${_PNG_PREFIX}/include"
	IMPORTED_LOCATION "${_PNG_PREFIX}/Release/libpng16.dll"
	IMPORTED_IMPLIB "${_PNG_PREFIX}/Release/libpng16.lib"
)
set(_PNG_PREFIX)
