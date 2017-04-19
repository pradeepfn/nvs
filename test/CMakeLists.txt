#copy config file from source tree to binary tree
configure_file(phoenix.config ../phoenix.config)
configure_file(run.sh ../run.sh)
add_executable(BasicTest test.cc)
target_link_libraries(BasicTest phoenix ssl)
target_link_libraries(BasicTest phoenix rt)

add_executable(ProdConsumerTest prodconsumer.cc)
target_link_libraries(ProdConsumerTest phoenix ssl)
target_link_libraries(ProdConsumerTest phoenix rt)

add_executable(stepProdConsumerTest stepprodconsumer.cc)
target_link_libraries(stepProdConsumerTest phoenix ssl)
target_link_libraries(stepProdConsumerTest phoenix rt)

add_executable(largeWriteTest largewrite.cc)
target_link_libraries(largeWriteTest phoenix ssl)
target_link_libraries(largeWriteTest phoenix rt)
