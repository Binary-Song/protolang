#include <gtest/gtest.h>

class Base
{
public:
	virtual ~Base() = default;
};

class Der : public Base
{
public:

};

void test_func()
{
    Base* base = new Der;
    delete base;
}

// Demonstrate some basic assertions.
TEST(HelloTest, BasicAssertions)
{
    test_func();
}