#copy config file from source tree to binary tree
configure_file(phoenix.config ../phoenix.config)
configure_file(run.sh ../run.sh)
add_executable(BasicTest test.c)
target_link_libraries(BasicTest phoenix ssl)

add_executable(ProdConsumerTest prodconsumer.c)
target_link_libraries(ProdConsumerTest phoenix ssl)

add_executable(stepProdConsumerTest stepprodconsumer.c)
target_link_libraries(stepProdConsumerTest phoenix ssl)
