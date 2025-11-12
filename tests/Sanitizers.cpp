#include "helpers/test_helpers.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

TEST_CASE ("use after free", "[ASan]")
{
    int* p = new int (1);
    delete p;
    *p = 5; // BOOM: write after free
}

int counter = 0;

void increment()
{
    for (int i = 0; i < 1000; ++i)
        counter++; // unsynchronized write
}

TEST_CASE ("data race", "[TSan]")
{
#include <thread>
    std::thread t1 (increment);
    std::thread t2 (increment);
    t1.join();
    t2.join();
}

TEST_CASE ("out-of-bounds write", "[UBSan]")
{
    int arr[3];
    arr[3] = 42; // BOOM: write past end of array
}

TEST_CASE ("stack buffer overflow", "[UBSan]")
{
    char buf[8];
    buf[8] = 'x'; // BOOM: write past end of stack buffer
}