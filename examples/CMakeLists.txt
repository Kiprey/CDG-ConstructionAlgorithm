llvm_map_components_to_libnames(llvm_libs bitwriter core ipo irreader instcombine instrumentation target linker analysis scalaropts support )

# 设置 example内所有程序的编译设置
file (GLOB SOURCES
   *.cpp
)
# 对于每个 examples中的cpp文件
foreach(src_file ${SOURCES})
    # 获取其程序名称
        # 先将路径去除，例如将 proj/examples/guessNum.cpp => guessNum.cpp
    string( REGEX REPLACE ".*\\/" "" example_program ${src_file} )
        # 再将后缀去除
    string( REPLACE ".cpp" "" example_program ${example_program} )
    # 设置编译设置
    add_executable(${example_program} ${src_file})
    target_link_libraries(${example_program} ${SVF_LIB} ${LLVMCudd} ${llvm_libs})
    set_target_properties(${example_program} PROPERTIES
                        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endforeach()