#include <gtest/gtest.h>

#include <flinter/zookeeper/council.h>
#include <flinter/logger.h>
#include <flinter/msleep.h>

class Callback : public flinter::Council::Callback {
public:
    virtual ~Callback() {}
    virtual void OnFired(const std::string &path, bool recoverable)
    {
        LOG(ERROR) << "FIRED [" << path << "] " << recoverable;
    }

    virtual void OnElected(const std::string &path, int32_t id)
    {
        LOG(ERROR) << "ELECTED [" << path << "] " << id;
    }

}; // class Callback

TEST(CouncilTest, TestAttend)
{
    flinter::ZooKeeper zk;
    ASSERT_EQ(ZOK, zk.Initialize("127.0.0.1:2181"));
    ASSERT_TRUE(zk.WaitUntilConnected(2000000000LL));

    Callback cb;
    flinter::Council c;
    c.set_name("a_b");
    ASSERT_TRUE(c.Initialize(&zk, "/council", &cb));
    ASSERT_TRUE(c.WaitUntilAttended());

    for (int i = 0; i < 60; ++i) {
        LOG(INFO) << c.IsElected();
        msleep(1000);
    }
}
