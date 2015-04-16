#include <gtest/gtest.h>

#include <flinter/types/shared_ptr.h>
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

TEST(SharedPtrTest, TestAll)
{
    EXPECT_EQ(0, Tester::_count);
    {
        flinter::shared_ptr<Tester> p(new Tester);
        EXPECT_EQ(1, Tester::_count);
        {
            flinter::shared_ptr<Tester> q(p);
            EXPECT_EQ(1, Tester::_count);
        }
        EXPECT_EQ(1, Tester::_count);
        {
            flinter::shared_ptr<Tester> q(p);
            EXPECT_EQ(1, Tester::_count);
            flinter::shared_ptr<Tester> r(new Tester);
            EXPECT_EQ(2, Tester::_count);
        }
        EXPECT_EQ(1, Tester::_count);
    }
    EXPECT_EQ(0, Tester::_count);
}
