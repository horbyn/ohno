if(OHNO_ENABLE_STATIC_LIB)
  set(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
  find_library(SSL_STATIC_LIB libssl.a PATHS /usr/lib/x86_64-linux-gnu)
  find_library(CRYPTO_STATIC_LIB libcrypto.a PATHS /usr/lib/x86_64-linux-gnu)
  find_library(Z_STATIC_LIB libz.a PATHS /usr/lib/x86_64-linux-gnu)
  find_library(PSL_STATIC_LIB libpsl.a PATHS /usr/lib/x86_64-linux-gnu)
  find_library(UNISTRING_STATIC_LIB libunistring.a PATHS /usr/lib/x86_64-linux-gnu)
  find_library(IDM2_STATIC_LIB libidn2.a PATHS /usr/lib/x86_64-linux-gnu)
else()
  message(FATAL_ERROR "暂不支持动态链接")
endif()

file(GLOB sources "*.cc")
add_library(ohno_http_client
  STATIC
  ${sources}
)
if(OHNO_ENABLE_STATIC_LIB)
  target_link_libraries(ohno_http_client
    PRIVATE
    curlpp::curlpp_static
    ${SSL_STATIC_LIB}
    ${CRYPTO_STATIC_LIB}
    ${Z_STATIC_LIB}
    ${PSL_STATIC_LIB}
    ${IDM2_STATIC_LIB}
    ${UNISTRING_STATIC_LIB}
  )
else()
  message(FATAL_ERROR "暂不支持动态链接")
endif()
