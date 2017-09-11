function (add_yuma_example file_name)
    add_executable(${file_name} ${file_name}.cc)

    target_link_libraries(${file_name} nvmm pthread)
    target_link_libraries(${file_name} ${ARCH_LIBS})
endfunction()

add_yuma_example(reader)
add_yuma_example(writer)
