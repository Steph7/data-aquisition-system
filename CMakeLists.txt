cmake_minimum_required(VERSION 3.10)

# Nome do projeto
project(data-aquisition-system)

# Define o padrão C++
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Inclui as bibliotecas necessárias
find_package(Threads REQUIRED)
find_package(Boost 1.65.1 REQUIRED system)

# include directories
include_directories(${Boost_INCLUDE_DIRS} src)

# Adiciona o diretório de código fonte
add_executable(das src/main.cpp)

add_custom_command(TARGET das POST_BUILD
   COMMAND $<TARGET_FILE:das> 9000
   COMMENT "Executando server com a porta 9000")

# link Boost libraries to the target executable
target_link_libraries(das ${Boost_LIBRARIES})
target_link_libraries(das  Threads::Threads)   
