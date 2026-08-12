cmake_minimum_required(VERSION 3.20)
cmake_policy(SET CMP0135 NEW)

project(OrderBookSimulator)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
find_package(Threads REQUIRED)

FetchContent_Declare(
    asio
    URL https://github.com/chriskohlhoff/asio/archive/refs/tags/asio-1-38-0.tar.gz
)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest
    GIT_TAG v1.17.0
)

FetchContent_MakeAvailable(
    asio
    googletest
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

add_library(asio_target INTERFACE)
target_include_directories(asio_target INTERFACE 
    ${asio_SOURCE_DIR}/asio/include
    ${asio_SOURCE_DIR}/include
)
target_compile_definitions(asio_target INTERFACE ASIO_STANDALONE)

add_library(EngineCore STATIC
    src/orderbook/OrderBook.cpp
    src/orderbook/OrderBookManager.cpp
    src/parsers/FIXParser.cpp
    src/parsers/JSONParser.cpp
    src/writers/FIXWriter.cpp
    src/risk/RiskManager.cpp
)
target_include_directories(EngineCore PUBLIC include)

# Console
add_executable(OrderBookConsole 
    src/main_console.cpp
    src/network/TCPServer.cpp
    src/network/TCPSession.cpp
    src/gateways/FIXGateway.cpp
    src/orchestrator/ExchangeOrchestrator.cpp
)
target_include_directories(OrderBookConsole PRIVATE include)

target_link_libraries(OrderBookConsole PRIVATE
    EngineCore
    asio_target
    Threads::Threads
)

# Server
add_executable(OrderBookServer 
    src/main_server.cpp
    src/network/TCPServer.cpp
    src/network/TCPSession.cpp
    src/gateways/FIXGateway.cpp
    src/orchestrator/ExchangeOrchestrator.cpp
)

target_include_directories(OrderBookServer PRIVATE include)

target_link_libraries(OrderBookServer PRIVATE 
    EngineCore
    asio_target 
    Threads::Threads
)

# Tests (add .cpp files when tests are built)
enable_testing()

add_executable(OrderBookTests
    tests/orderbook_tests.cpp
    tests/fix_protocol_tests.cpp
    tests/risk_check_tests.cpp
)

target_link_libraries(OrderBookTests PRIVATE
    EngineCore 
    asio_target 
    GTest::gtest_main  
    Threads::Threads   
)

include(GoogleTest)
gtest_discover_tests(OrderBookTests)