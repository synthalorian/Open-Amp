// test_main.cpp - Catch2 main entry point (using Catch2WithMain so this is minimal)
#include <catch2/catch_session.hpp>

// Optional: Custom main if needed for global setup/teardown
int main(int argc, char* argv[]) {
    return Catch::Session().run(argc, argv);
}
