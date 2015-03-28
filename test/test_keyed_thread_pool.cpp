#include <gtest/gtest.h>

#include <flinter/thread/keyed_thread_pool.h>
#include <flinter/types/atomic.h>
#include <flinter/logger.h>
#include <flinter/msleep.h>
#include <flinter/runnable.h>

typedef flinter::atomic64_t count_t;

class Sleeper : public flinter::Runnable {
public:
    Sleeper(count_t *b, count_t *a) : _b(b), _a(a) {}
    virtual ~Sleeper() {}
    virtual bool Run()
    {
        _b->AddAndFetch(1);
        LOG(INFO) << "BEFORE";
        msleep(500);
        LOG(INFO) << "AFTER";
        _a->AddAndFetch(1);
        return true;
    }

private:
    count_t *_b;
    count_t *_a;

}; // class Sleeper

TEST(FixedThreadPoolTest, TestKeyed)
{
    count_t a, b;
    flinter::Logger::SetFilter(flinter::Logger::kLevelVerbose);
    flinter::KeyedThreadPool pool;
    ASSERT_TRUE(pool.Initialize(10));

    ASSERT_TRUE(pool.AppendJobWithKey(0, new Sleeper(&b, &a), true));
    ASSERT_TRUE(pool.AppendJobWithKey(1, new Sleeper(&b, &a), true));
    ASSERT_TRUE(pool.AppendJobWithKey(1, new Sleeper(&b, &a), true));
    ASSERT_TRUE(pool.AppendJobWithKey(0, new Sleeper(&b, &a), true));
    ASSERT_TRUE(pool.AppendJobWithKey(1, new Sleeper(&b, &a), true));

    LOG(INFO) << "MAIN";
    msleep(2000);
    LOG(INFO) << "DONE";
    ASSERT_TRUE(pool.Shutdown(true));
    LOG(INFO) << "EXIT";

    ASSERT_EQ(a.Get(), 5);
    ASSERT_EQ(b.Get(), 5);
}
