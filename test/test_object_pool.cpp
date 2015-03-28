#include <gtest/gtest.h>

#include <flinter/object_pool.h>

class Tester {
public:
    Tester()
    {
        printf("C %p\n", this);
    }

    ~Tester()
    {
        printf("D %p\n", this);
    }

}; // class Tester

class Pool : public flinter::ObjectPool<Tester> {
public:
    virtual ~Pool()
    {
        Clear();
    }

protected:
    virtual int64_t max_idle_time()
    {
        return 5000000000LL;
    }

    virtual Tester *Create()
    {
        return new Tester;
    }

    virtual void Destroy(Tester *tester)
    {
        delete tester;
    }

}; // class Pool

TEST(ObjectPoolTest, TestAll)
{
    Pool p;
    Tester *t1 = p.Grab();
    Tester *t2 = p.Grab();
    Tester *t3 = p.Grab();
    printf("%d\n", __LINE__);
    p.Release(t3);
    printf("%d\n", __LINE__);
    p.Remove(t1);
    printf("%d\n", __LINE__);
    Tester *t4 = p.Exchange(t2);
    printf("%d\n", __LINE__);
    (void)t4;
}
