add_library(xpti_syclur_collector SHARED
  syclur_collector.cpp
)
target_compile_definitions(xpti_syclur_collector PUBLIC XPTI_CALLBACK_API_EXPORTS=1)
target_link_libraries(xpti_syclur_collector PRIVATE
  ${xptifw_lib}
  ${CMAKE_DL_LIBS}
)
target_include_directories(xpti_syclur_collector PRIVATE
  ${XPTIFW_DIR}/include
  ${XPTI_DIR}/include
  ${XPTIFW_DIR}/samples/include
)

if (XPTI_ENABLE_TBB)
  target_link_libraries(xpti_syclur_collector PRIVATE tbb)
endif()

