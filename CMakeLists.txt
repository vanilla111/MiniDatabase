cmake_minimum_required(VERSION 3.15)
project(WangSQL)

set(CMAKE_CXX_STANDARD 14)

include_directories(${PROJECT_SOURCE_DIR}/src/)

add_executable(WangSQL src/main.cpp
        src/utils/utils.cpp
        src/file/heapFile.cpp
        src/buffer/bufferManager.cpp
        src/catalog/catalogManager.cpp
        src/record/recordManager.cpp
        src/struct/table.cpp
        src/struct/index.cpp
        src/interpreter/tokenizer.cpp
        src/interpreter/interpreter.cpp
        src/api/api.cpp
        src/index/indexManager.cpp
        src/index/bPlusTreeNode.cpp
        src/index/bPlusTree.cpp
        src/server/server.cpp)
