#include <gtest/gtest.h>

#include <flinter/types/scoped_ptr.h>
#include <flinter/logger.h>

class Tester {
public:
    Tester()
    {
        LOG(INFO) << "Construct: " << ++_count;
    }

    ~Tester()
    {
        LOG(INFO) << "Deconstruct: " << --_count;
    }

    static int _count;

}; // class Tester

int Tester::_count = 0;

TEST(ScopedPtrTest, TestAll)
{
    EXPECT_EQ(0, Tester::_count);
    {
        flinter::scoped_ptr<Tester> p(new Tester);
        EXPECT_EQ(1, Tester::_count);
    }
    EXPECT_EQ(0, Tester::_count);
}
