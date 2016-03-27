#include <gtest/gtest.h>

#include <flinter/zookeeper/zookeeper.h>

TEST(ZooKeeperTest, TestConnect)
{
    flinter::ZooKeeper zk;
    ASSERT_EQ(ZOK, zk.Initialize("127.0.0.1:2181"));
    ASSERT_TRUE(zk.WaitUntilConnected(5000000000LL));
    ASSERT_EQ(ZOK, zk.Shutdown());
}
