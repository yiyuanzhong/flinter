#include <gtest/gtest.h>

#include <flinter/object_map.h>

class C {
public:
    C() { _i = ++*_c; }
    ~C() { --*_c; }
    int _i;
    static int *_c;

}; // class C

class O : public flinter::ObjectMap<int, C> {
public:
    virtual ~O() { Clear(); }
protected:
    virtual C *Create(const int &i)
    {
        return new C;
    }

    virtual void Destroy(C *c)
    {
        delete c;
    }

}; // class O

int *C::_c = NULL;

TEST(ObjectMapTest, TestAllocation)
{
    int i = 0;
    C::_c = &i;

    EXPECT_EQ(i, 0);
    O *p = new O;

    EXPECT_EQ(i, 0);

    C *c1, *c2, *c3, *c4;

    c1 = p->Get(3);
    EXPECT_FALSE(c1);
    EXPECT_EQ(i, 0);

    c1 = p->Add(3);
    EXPECT_TRUE(c1);
    EXPECT_EQ(i, 1);

    c2 = p->Get(3);
    EXPECT_EQ(c1, c2);
    EXPECT_EQ(i, 1);

    c3 = p->Get(3);
    EXPECT_EQ(c1, c3);
    EXPECT_EQ(i, 1);

    c4 = p->Add(5);
    EXPECT_TRUE(c4);
    EXPECT_NE(c1, c4);
    EXPECT_EQ(i, 2);

    p->Erase(4);
    EXPECT_EQ(i, 2);
    EXPECT_TRUE(p->Get(3));

    p->Add(7);
    EXPECT_EQ(i, 3);
    p->Release(7);
    EXPECT_EQ(i, 3);
    p->Erase(7);
    EXPECT_EQ(i, 2);

    p->Erase(3);
    EXPECT_EQ(i, 2);
    EXPECT_FALSE(p->Get(3));

    p->Release(3);
    EXPECT_EQ(i, 2);
    p->Release(3);
    EXPECT_EQ(i, 2);
    p->Release(3);
    EXPECT_EQ(i, 2);
    p->Release(3);
    EXPECT_EQ(i, 1);
    p->Release(3);
    EXPECT_EQ(i, 1);

    delete p;
    EXPECT_EQ(i, 0);
}

TEST(ObjectMapTest, TestGetNextMany)
{
    int i = 0;
    C::_c = &i;

    O p;
    p.Add(3); // 3,1
    p.Add(7); // 7,2
    p.Add(4); // 4,3
    p.Add(6); // 6,4

    C *c;
    c = p.GetNext();
    EXPECT_TRUE(c);
    EXPECT_EQ(c->_i, 1); // 3

    c = p.GetNext();
    EXPECT_TRUE(c);
    EXPECT_EQ(c->_i, 3); // 4

    c = p.GetNext();
    EXPECT_TRUE(c);
    EXPECT_EQ(c->_i, 4); // 6

    c = p.GetNext();
    EXPECT_TRUE(c);
    EXPECT_EQ(c->_i, 2); // 7

    c = p.GetNext();
    EXPECT_TRUE(c);
    EXPECT_EQ(c->_i, 1); // 3

    c = p.GetNext();
    EXPECT_TRUE(c);
    EXPECT_EQ(c->_i, 3); // 4

    c = p.GetNext();
    EXPECT_TRUE(c);
    EXPECT_EQ(c->_i, 4); // 6

    c = p.GetNext();
    EXPECT_TRUE(c);
    EXPECT_EQ(c->_i, 2); // 7

    c = p.GetNext();
    EXPECT_TRUE(c);
    EXPECT_EQ(c->_i, 1); // 3
}

TEST(ObjectMapTest, TestSetAll)
{
    int i = 0;
    C::_c = &i;

    O p;
    p.Add(3);
    p.Add(7);
    p.Add(4);
    p.Add(6);
    EXPECT_EQ(i, 4);

    std::set<int> a;
    a.insert(3);
    a.insert(8);
    a.insert(6);
    p.SetAll(a);
    EXPECT_EQ(i, 5);

    EXPECT_TRUE(p.Get(3));
    EXPECT_TRUE(p.Get(6));
    EXPECT_TRUE(p.Get(8));

    EXPECT_FALSE(p.Get(4));
    EXPECT_FALSE(p.Get(7));

    p.Release(7);
    EXPECT_EQ(i, 4);
    p.Release(4);
    EXPECT_EQ(i, 3);
}
