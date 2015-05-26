#include <gtest/gtest.h>

#include <flinter/zookeeper/naming.h>
#include <flinter/logger.h>
#include <flinter/msleep.h>

class Callback : public flinter::Naming::Callback {
public:
    virtual ~Callback() {}
    virtual void OnChanged(const std::map<int32_t, std::string> &n)
    {
        LOG(WARN) << "===";
        for (std::map<int32_t, std::string>::const_iterator
             p = n.begin(); p != n.end(); ++p) {

            LOG(WARN) << p->first << " : " << p->second;
        }
        LOG(WARN) << "===";
    }

}; // class Callback

TEST(NamingTest, TestClient)
{
    flinter::ZooKeeper zk;
    ASSERT_EQ(ZOK, zk.Initialize("127.0.0.1:2181"));
    ASSERT_TRUE(zk.WaitUntilConnected(2000));

    Callback cb;
    flinter::Naming n;
    ASSERT_TRUE(n.Attend(&zk, "/naming", "my_name", &cb));

    msleep(20000);
}
