#include <gtest/gtest.h>

#include <flinter/types/decimal.h>

TEST(DecimalTest, TestParseBad)
{
    flinter::Decimal d;
    EXPECT_FALSE(d.Parse(""));
    EXPECT_FALSE(d.Parse("-"));
    EXPECT_FALSE(d.Parse("x"));
    EXPECT_FALSE(d.Parse("5x"));
    EXPECT_FALSE(d.Parse("x5"));
    EXPECT_FALSE(d.Parse("5..3"));
    EXPECT_FALSE(d.Parse("5.."));
    EXPECT_FALSE(d.Parse("5 3"));
    EXPECT_FALSE(d.Parse(" 0"));
    EXPECT_FALSE(d.Parse("0 "));
    EXPECT_FALSE(d.Parse(" 0 "));
    EXPECT_FALSE(d.Parse("   0.0000     "));
    EXPECT_FALSE(d.Parse("-00-0"));
}

TEST(DecimalTest, TestParseGood)
{
    flinter::Decimal d;
    EXPECT_TRUE(d.Parse("0"));
    EXPECT_EQ(d, 0);
    EXPECT_EQ(d.scale(), 0);
    EXPECT_TRUE(d.Parse("00"));
    EXPECT_EQ(d, 0);
    EXPECT_EQ(d.scale(), 0);
    EXPECT_TRUE(d.Parse("0.0"));
    EXPECT_EQ(d, 0);
    EXPECT_EQ(d.scale(), 0);
    EXPECT_TRUE(d.Parse("-0.00000"));
    EXPECT_EQ(d, 0);
    EXPECT_EQ(d.scale(), 0);
    EXPECT_TRUE(d.Parse("-0"));
    EXPECT_EQ(d, 0);
    EXPECT_EQ(d.scale(), 0);
    EXPECT_TRUE(d.Parse("-000500"));
    EXPECT_EQ(d, -500);
    EXPECT_EQ(d.scale(), 0);
    EXPECT_TRUE(d.Parse("-005.0"));
    EXPECT_EQ(d, -5);
    EXPECT_EQ(d.scale(), 0);
    EXPECT_TRUE(d.Parse("-50.0"));
    EXPECT_EQ(d, -50);
    EXPECT_EQ(d.scale(), 0);
    EXPECT_TRUE(d.Parse("-50.50"));
    EXPECT_EQ(d, "-50.5");
    EXPECT_EQ(d.scale(), 1);
}

TEST(DecimalTest, TestSerialize0)
{
    std::string s;
    flinter::Decimal d(0);
    EXPECT_EQ(d.scale(), 0);
    EXPECT_EQ(d.Serialize(&s, -1), 0);
    EXPECT_EQ(s, "0");
    EXPECT_EQ(d.Serialize(&s, 0), 0);
    EXPECT_EQ(s, "0");
    EXPECT_EQ(d.Serialize(&s, 1), 0);
    EXPECT_EQ(s, "0.0");
    EXPECT_EQ(d.Serialize(&s, 2), 0);
    EXPECT_EQ(s, "0.00");
}

TEST(DecimalTest, TestSerializeN0)
{
    std::string s;
    flinter::Decimal d("-0");
    EXPECT_EQ(d.scale(), 0);
    EXPECT_EQ(d.Serialize(&s, -1), 0);
    EXPECT_EQ(s, "0");
    EXPECT_EQ(d.Serialize(&s, 0), 0);
    EXPECT_EQ(s, "0");
    EXPECT_EQ(d.Serialize(&s, 1), 0);
    EXPECT_EQ(s, "0.0");
    EXPECT_EQ(d.Serialize(&s, 2), 0);
    EXPECT_EQ(s, "0.00");
}

TEST(DecimalTest, TestSerialize1)
{
    std::string s;
    flinter::Decimal d(5);
    EXPECT_EQ(d.scale(), 0);
    EXPECT_EQ(d.Serialize(&s, -1), 0);
    EXPECT_EQ(s, "5");
    EXPECT_EQ(d.Serialize(&s, 0), 0);
    EXPECT_EQ(s, "5");
    EXPECT_EQ(d.Serialize(&s, 1), 0);
    EXPECT_EQ(s, "5.0");
    EXPECT_EQ(d.Serialize(&s, 2), 0);
    EXPECT_EQ(s, "5.00");
}

TEST(DecimalTest, TestSerializeN1)
{
    std::string s;
    flinter::Decimal d(-5);
    EXPECT_EQ(d.scale(), 0);
    EXPECT_EQ(d.Serialize(&s, -1), 0);
    EXPECT_EQ(s, "-5");
    EXPECT_EQ(d.Serialize(&s, 0), 0);
    EXPECT_EQ(s, "-5");
    EXPECT_EQ(d.Serialize(&s, 1), 0);
    EXPECT_EQ(s, "-5.0");
    EXPECT_EQ(d.Serialize(&s, 2), 0);
    EXPECT_EQ(s, "-5.00");
}

TEST(DecimalTest, TestSerialize2)
{
    std::string s;
    flinter::Decimal d(".5");
    EXPECT_EQ(d.scale(), 1);
    EXPECT_EQ(d.Serialize(&s, -1), 0);
    EXPECT_EQ(s, "0.5");
    EXPECT_LT(d.Serialize(&s, 0), 0);
    EXPECT_EQ(s, "0");
    EXPECT_EQ(d.Serialize(&s, 1), 0);
    EXPECT_EQ(s, "0.5");
    EXPECT_EQ(d.Serialize(&s, 2), 0);
    EXPECT_EQ(s, "0.50");
}

TEST(DecimalTest, TestSerializeN2)
{
    std::string s;
    flinter::Decimal d("-.5");
    EXPECT_EQ(d.scale(), 1);
    EXPECT_EQ(d.Serialize(&s, -1), 0);
    EXPECT_EQ(s, "-0.5");
    EXPECT_GT(d.Serialize(&s, 0), 0);
    EXPECT_EQ(s, "0");
    EXPECT_EQ(d.Serialize(&s, 1), 0);
    EXPECT_EQ(s, "-0.5");
    EXPECT_EQ(d.Serialize(&s, 2), 0);
    EXPECT_EQ(s, "-0.50");
}

TEST(DecimalTest, TestSerialize3)
{
    std::string s;
    flinter::Decimal d("1.5");
    EXPECT_EQ(d.scale(), 1);
    EXPECT_EQ(d.Serialize(&s, -1), 0);
    EXPECT_EQ(s, "1.5");
    EXPECT_GT(d.Serialize(&s, 0), 0);
    EXPECT_EQ(s, "2");
    EXPECT_EQ(d.Serialize(&s, 1), 0);
    EXPECT_EQ(s, "1.5");
    EXPECT_EQ(d.Serialize(&s, 2), 0);
    EXPECT_EQ(s, "1.50");
}

TEST(DecimalTest, TestSerializeN3)
{
    std::string s;
    flinter::Decimal d("-1.5");
    EXPECT_EQ(d.scale(), 1);
    EXPECT_EQ(d.Serialize(&s, -1), 0);
    EXPECT_EQ(s, "-1.5");
    EXPECT_LT(d.Serialize(&s, 0), 0);
    EXPECT_EQ(s, "-2");
    EXPECT_EQ(d.Serialize(&s, 1), 0);
    EXPECT_EQ(s, "-1.5");
    EXPECT_EQ(d.Serialize(&s, 2), 0);
    EXPECT_EQ(s, "-1.50");
}

TEST(DecimalTest, TestSerialize4)
{
    std::string s;
    flinter::Decimal d("1.49999");
    EXPECT_EQ(d.scale(), 5);
    EXPECT_EQ(d.Serialize(&s, -1), 0);
    EXPECT_EQ(s, "1.49999");
    EXPECT_LT(d.Serialize(&s, 0), 0);
    EXPECT_EQ(s, "1");
    EXPECT_GT(d.Serialize(&s, 1), 0);
    EXPECT_EQ(s, "1.5");
    EXPECT_GT(d.Serialize(&s, 2), 0);
    EXPECT_EQ(s, "1.50");
    EXPECT_GT(d.Serialize(&s, 3), 0);
    EXPECT_EQ(s, "1.500");
    EXPECT_GT(d.Serialize(&s, 4), 0);
    EXPECT_EQ(s, "1.5000");
    EXPECT_EQ(d.Serialize(&s, 5), 0);
    EXPECT_EQ(s, "1.49999");
    EXPECT_EQ(d.Serialize(&s, 6), 0);
    EXPECT_EQ(s, "1.499990");
}

TEST(DecimalTest, TestSerializeN4)
{
    std::string s;
    flinter::Decimal d("-1.49999");
    EXPECT_EQ(d.scale(), 5);
    EXPECT_EQ(d.Serialize(&s, -1), 0);
    EXPECT_EQ(s, "-1.49999");
    EXPECT_GT(d.Serialize(&s, 0), 0);
    EXPECT_EQ(s, "-1");
    EXPECT_LT(d.Serialize(&s, 1), 0);
    EXPECT_EQ(s, "-1.5");
    EXPECT_LT(d.Serialize(&s, 2), 0);
    EXPECT_EQ(s, "-1.50");
    EXPECT_LT(d.Serialize(&s, 3), 0);
    EXPECT_EQ(s, "-1.500");
    EXPECT_LT(d.Serialize(&s, 4), 0);
    EXPECT_EQ(s, "-1.5000");
    EXPECT_EQ(d.Serialize(&s, 5), 0);
    EXPECT_EQ(s, "-1.49999");
    EXPECT_EQ(d.Serialize(&s, 6), 0);
    EXPECT_EQ(s, "-1.499990");
}

TEST(DecimalTest, TestSerialize5)
{
    std::string s;
    flinter::Decimal d("1.50001");
    EXPECT_EQ(d.scale(), 5);
    EXPECT_EQ(d.Serialize(&s, -1), 0);
    EXPECT_EQ(s, "1.50001");
    EXPECT_GT(d.Serialize(&s, 0), 0);
    EXPECT_EQ(s, "2");
    EXPECT_LT(d.Serialize(&s, 1), 0);
    EXPECT_EQ(s, "1.5");
    EXPECT_LT(d.Serialize(&s, 2), 0);
    EXPECT_EQ(s, "1.50");
    EXPECT_LT(d.Serialize(&s, 3), 0);
    EXPECT_EQ(s, "1.500");
    EXPECT_LT(d.Serialize(&s, 4), 0);
    EXPECT_EQ(s, "1.5000");
    EXPECT_EQ(d.Serialize(&s, 5), 0);
    EXPECT_EQ(s, "1.50001");
    EXPECT_EQ(d.Serialize(&s, 6), 0);
    EXPECT_EQ(s, "1.500010");
}

TEST(DecimalTest, TestSerializeN5)
{
    std::string s;
    flinter::Decimal d("-1.50001");
    EXPECT_EQ(d.scale(), 5);
    EXPECT_EQ(d.Serialize(&s, -1), 0);
    EXPECT_EQ(s, "-1.50001");
    EXPECT_LT(d.Serialize(&s, 0), 0);
    EXPECT_EQ(s, "-2");
    EXPECT_GT(d.Serialize(&s, 1), 0);
    EXPECT_EQ(s, "-1.5");
    EXPECT_GT(d.Serialize(&s, 2), 0);
    EXPECT_EQ(s, "-1.50");
    EXPECT_GT(d.Serialize(&s, 3), 0);
    EXPECT_EQ(s, "-1.500");
    EXPECT_GT(d.Serialize(&s, 4), 0);
    EXPECT_EQ(s, "-1.5000");
    EXPECT_EQ(d.Serialize(&s, 5), 0);
    EXPECT_EQ(s, "-1.50001");
    EXPECT_EQ(d.Serialize(&s, 6), 0);
    EXPECT_EQ(s, "-1.500010");
}

TEST(DecimalTest, TestSerializeT1)
{
    std::string s;
    flinter::Decimal d("1.50000");
    EXPECT_EQ(d.scale(), 1);
    EXPECT_EQ(d.Serialize(&s, -1), 0);
    EXPECT_EQ(s, "1.5");
    EXPECT_GT(d.Serialize(&s, 0), 0);
    EXPECT_EQ(s, "2");
    EXPECT_EQ(d.Serialize(&s, 1), 0);
    EXPECT_EQ(s, "1.5");
    EXPECT_EQ(d.Serialize(&s, 2), 0);
    EXPECT_EQ(s, "1.50");
    EXPECT_EQ(d.Serialize(&s, 3), 0);
    EXPECT_EQ(s, "1.500");
    EXPECT_EQ(d.Serialize(&s, 4), 0);
    EXPECT_EQ(s, "1.5000");
    EXPECT_EQ(d.Serialize(&s, 5), 0);
    EXPECT_EQ(s, "1.50000");
    EXPECT_EQ(d.Serialize(&s, 6), 0);
    EXPECT_EQ(s, "1.500000");
}

TEST(DecimalTest, TestSerializeL0)
{
    std::string s;
    flinter::Decimal d("0.0");
    EXPECT_EQ(d.scale(), 0);
    EXPECT_EQ(d.Serialize(&s, -1), 0);
    EXPECT_EQ(s, "0");
    EXPECT_EQ(d.Serialize(&s, 0), 0);
    EXPECT_EQ(s, "0");
    EXPECT_EQ(d.Serialize(&s, 1), 0);
    EXPECT_EQ(s, "0.0");
    EXPECT_EQ(d.Serialize(&s, 2), 0);
    EXPECT_EQ(s, "0.00");
}

TEST(DecimalTest, TestNoUpscaleAdd)
{
    std::string s;
    flinter::Decimal d;
    EXPECT_TRUE(d.Parse("3.57"));
    EXPECT_EQ(d.scale(), 2);

    flinter::Decimal e;
    EXPECT_TRUE(e.Parse("2.13"));
    EXPECT_EQ(e.scale(), 2);

    d.Add(e);
    EXPECT_EQ(d.scale(), 1);
    EXPECT_EQ(d.Serialize(&s, -1), 0);
    EXPECT_EQ(s, "5.7");

    EXPECT_EQ(e.scale(), 2);
    EXPECT_EQ(e.Serialize(&s, -1), 0);
    EXPECT_EQ(s, "2.13");
}

TEST(DecimalTest, TestLeftUpscale)
{
    std::string s;
    flinter::Decimal d;
    EXPECT_TRUE(d.Parse("3.57"));
    EXPECT_EQ(d.scale(), 2);

    flinter::Decimal e;
    EXPECT_TRUE(e.Parse("2.132"));
    EXPECT_EQ(e.scale(), 3);

    d.Add(e);
    EXPECT_EQ(d.scale(), 3);
    EXPECT_EQ(d.Serialize(&s, -1), 0);
    EXPECT_EQ(s, "5.702");

    EXPECT_EQ(e.scale(), 3);
    EXPECT_EQ(e.Serialize(&s, -1), 0);
    EXPECT_EQ(s, "2.132");
}

TEST(DecimalTest, TestRightUpscale)
{
    std::string s;
    flinter::Decimal d;
    EXPECT_TRUE(d.Parse("2.132"));
    EXPECT_EQ(d.scale(), 3);

    flinter::Decimal e;
    EXPECT_TRUE(e.Parse("3.57"));
    EXPECT_EQ(e.scale(), 2);

    d.Add(e);
    EXPECT_EQ(d.scale(), 3);
    EXPECT_EQ(d.Serialize(&s, -1), 0);
    EXPECT_EQ(s, "5.702");

    EXPECT_EQ(e.scale(), 2);
    EXPECT_EQ(e.Serialize(&s, -1), 0);
    EXPECT_EQ(s, "3.57");
}

TEST(DecimalTest, TestDiv1)
{
    flinter::Decimal n;

    EXPECT_TRUE(n.Parse("3"));
    EXPECT_EQ(n.Div(8, 5), 0);
    EXPECT_EQ(n, "0.375");
    EXPECT_EQ(n.scale(), 3);
    EXPECT_EQ(n.str(), "0.375");

    EXPECT_TRUE(n.Parse("3"));
    EXPECT_GT(n.Div(8, 2), 0);
    EXPECT_EQ(n, "0.38");
    EXPECT_EQ(n.scale(), 2);
    EXPECT_EQ(n.str(), "0.38");
}

TEST(DecimalTest, TestDivN1)
{
    flinter::Decimal n;

    EXPECT_TRUE(n.Parse("-3"));
    EXPECT_EQ(n.Div(8, 5), 0);
    EXPECT_EQ(n, "-0.375");
    EXPECT_EQ(n.scale(), 3);
    EXPECT_EQ(n.str(), "-0.375");

    EXPECT_TRUE(n.Parse("3"));
    EXPECT_LT(n.Div(-8, 2), 0);
    EXPECT_EQ(n, "-0.38");
    EXPECT_EQ(n.scale(), 2);
    EXPECT_EQ(n.str(), "-0.38");
}

TEST(DecimalTest, TestDiv2)
{
    flinter::Decimal n;

    n.Parse("3");
    EXPECT_GT(n.Div(7, 8), 0);
    EXPECT_EQ(n.str(), "0.42857143");

    n.Parse("3");
    EXPECT_LT(n.Div(7, 5), 0);
    EXPECT_EQ(n.str(), "0.42857");

    n.Parse("3");
    EXPECT_GT(n.Div(7, 2), 0);
    EXPECT_EQ(n.str(), "0.43");
}

TEST(DecimalTest, TestDivN2)
{
    flinter::Decimal n;

    n.Parse("-3");
    EXPECT_LT(n.Div(7, 8), 0);
    EXPECT_EQ(n.str(), "-0.42857143");

    n.Parse("3");
    EXPECT_GT(n.Div(-7, 5), 0);
    EXPECT_EQ(n.str(), "-0.42857");

    n.Parse("-3");
    EXPECT_LT(n.Div(7, 2), 0);
    EXPECT_EQ(n.str(), "-0.43");
}

TEST(DecimalTest, TestDiv3)
{
    flinter::Decimal n;

    n.Parse("5");
    EXPECT_EQ(n.Div(8, 5), 0);
    EXPECT_EQ(n.str(), "0.625");

    n.Parse("5");
    EXPECT_LT(n.Div(8, 2), 0);
    EXPECT_EQ(n.str(), "0.62");
}

TEST(DecimalTest, TestDivN3)
{
    flinter::Decimal n;

    n.Parse("-5");
    EXPECT_EQ(n.Div(8, 5), 0);
    EXPECT_EQ(n.str(), "-0.625");

    n.Parse("5");
    EXPECT_GT(n.Div(-8, 2), 0);
    EXPECT_EQ(n.str(), "-0.62");
}

TEST(DecimalTest, TestArithmeticOperators)
{
    flinter::Decimal n(3);
    EXPECT_EQ(n, 3);
    EXPECT_EQ(n.scale(), 0);
    n += 2;
    EXPECT_EQ(n, 5);
    EXPECT_EQ(n.scale(), 0);
    n -= "1.7";
    EXPECT_EQ(n.scale(), 1);
    EXPECT_TRUE(n == "3.3");
    EXPECT_NE(n, "3.4");
    EXPECT_TRUE(n != "-3.3");
    EXPECT_FALSE(n == "-3.3");
    n *= "-2.7";
    EXPECT_EQ(n.scale(), 2);
    EXPECT_TRUE(n == "-8.91");
    n /= 4;
    EXPECT_EQ(n, "-2.2275");
    EXPECT_EQ(n.scale(), 4);
    n /= 29;

    // -0.07681034482... -> -0.076810 -> -0.76810
    EXPECT_EQ(n, "-0.07681");
    EXPECT_EQ(n.scale(), 5);
    n /= 3;
    EXPECT_EQ(n, "-0.025603");
    EXPECT_EQ(n.scale(), 6);
}

TEST(DecimalTest, TestArithmeticOperators2)
{
    flinter::Decimal n(3);
    flinter::Decimal q = n + 2;
    EXPECT_EQ(n, 3);
    EXPECT_EQ(n.scale(), 0);
    EXPECT_EQ(q, 5);
    EXPECT_EQ(q.scale(), 0);
}

TEST(DecimalTest, TestArithmeticOperatorsNeg)
{
    flinter::Decimal n(3);
    flinter::Decimal q = -n;
    EXPECT_EQ(n, 3);
    EXPECT_EQ(n.scale(), 0);
    EXPECT_EQ(q, -3);
    EXPECT_EQ(q.scale(), 0);
}

TEST(DecimalTest, TestCleanup)
{
    flinter::Decimal n("3.71500200");
    EXPECT_EQ(n.scale(), 6);
    EXPECT_EQ(n, "3.715002000");
    EXPECT_EQ(n, "3.71500200");
    EXPECT_EQ(n, "3.7150020");
    EXPECT_EQ(n, "3.715002");
    n.Div(1, 5);
    EXPECT_EQ(n, "3.715");
    EXPECT_EQ(n.scale(), 3);
}

TEST(DecimalTest, TestSelf)
{
    flinter::Decimal a;
    flinter::Decimal b;
    a = 3;
    b = a - a;
    EXPECT_EQ(b, "0");
    b = a / a;
    EXPECT_EQ(b, "1");
}

TEST(DecimalTest, TestCompare)
{
    flinter::Decimal a("130.5");
    flinter::Decimal b("134.0");
    flinter::Decimal c("134");
    EXPECT_LT(a, b);
    EXPECT_GT(b, a);
    EXPECT_NE(a, b);
    EXPECT_EQ(b, c);
    EXPECT_LE(a, c);
    EXPECT_GE(c, a);
    EXPECT_EQ(a, a);
    EXPECT_LE(a, a);
    EXPECT_GE(a, a);
}

TEST(DecimalTest, TestDefaultScale)
{
    flinter::Decimal a("3.5");
    flinter::Decimal b;

    EXPECT_EQ(a, "3.5");
    b = a / 3;
    EXPECT_EQ(b, "1.166667");
    EXPECT_EQ(a.str(), "3.5");

    a.set_default_scale(3);
    EXPECT_EQ(a, "3.5");
    b = a / 3;
    EXPECT_EQ(b, "1.167");
    EXPECT_EQ(a.str(), "3.500");
}

TEST(DecimalTest, TestPriority)
{
    flinter::Decimal a(3);
    flinter::Decimal b(2);
    flinter::Decimal c(5);
    flinter::Decimal d(7);
    flinter::Decimal e = a + b * (c - d);
    EXPECT_EQ(e, "-1");
}

#define R(r,p,i,o) \
    a.Parse((i)); \
    EXPECT_##p(a.Serialize(&s, 0, (r)), 0); \
    EXPECT_EQ(s, (o));

TEST(DecimalTest, TestRounding1)
{
    std::string s;
    flinter::Decimal a;
    const flinter::Decimal::Rounding r =
            flinter::Decimal::kRoundingTiesToEven;

    R(r, GT, "11.50",   "12");
    R(r, LT, "12.50",   "12");
    R(r, LT, "-11.50",  "-12");
    R(r, GT, "-12.50",  "-12");

    R(r, EQ, "11.00",   "11");
    R(r, EQ, "-11.00",  "-11");

    R(r, LT, "11.01",   "11");
    R(r, GT, "-11.01",  "-11");
    R(r, LT, "11.49",   "11");
    R(r, GT, "-11.49",  "-11");

    R(r, GT, "11.51",   "12");
    R(r, LT, "-11.51",  "-12");
    R(r, GT, "11.99",   "12");
    R(r, LT, "-11.99",  "-12");
}

TEST(DecimalTest, TestRounding2)
{
    std::string s;
    flinter::Decimal a;
    const flinter::Decimal::Rounding r =
            flinter::Decimal::kRoundingTiesAwayFromZero;

    R(r, GT, "11.50",   "12");
    R(r, GT, "12.50",   "13");
    R(r, LT, "-11.50",  "-12");
    R(r, LT, "-12.50",  "-13");

    R(r, EQ, "11.00",   "11");
    R(r, EQ, "-11.00",  "-11");

    R(r, LT, "11.01",   "11");
    R(r, GT, "-11.01",  "-11");
    R(r, LT, "11.49",   "11");
    R(r, GT, "-11.49",  "-11");

    R(r, GT, "11.51",   "12");
    R(r, LT, "-11.51",  "-12");
    R(r, GT, "11.99",   "12");
    R(r, LT, "-11.99",  "-12");
}

TEST(DecimalTest, TestRounding3)
{
    std::string s;
    flinter::Decimal a;
    const flinter::Decimal::Rounding r =
            flinter::Decimal::kRoundingTowardsZero;

    R(r, LT, "11.50",   "11");
    R(r, LT, "12.50",   "12");
    R(r, GT, "-11.50",  "-11");
    R(r, GT, "-12.50",  "-12");

    R(r, EQ, "11.00",   "11");
    R(r, EQ, "-11.00",  "-11");

    R(r, LT, "11.01",   "11");
    R(r, GT, "-11.01",  "-11");
    R(r, LT, "11.49",   "11");
    R(r, GT, "-11.49",  "-11");

    R(r, LT, "11.51",   "11");
    R(r, GT, "-11.51",  "-11");
    R(r, LT, "11.99",   "11");
    R(r, GT, "-11.99",  "-11");
}

TEST(DecimalTest, TestRounding4)
{
    std::string s;
    flinter::Decimal a;
    const flinter::Decimal::Rounding r =
            flinter::Decimal::kRoundingTowardsPositiveInfinity;

    R(r, GT, "11.50",   "12");
    R(r, GT, "12.50",   "13");
    R(r, GT, "-11.50",  "-11");
    R(r, GT, "-12.50",  "-12");

    R(r, EQ, "11.00",   "11");
    R(r, EQ, "-11.00",  "-11");

    R(r, GT, "11.01",   "12");
    R(r, GT, "-11.01",  "-11");
    R(r, GT, "11.49",   "12");
    R(r, GT, "-11.49",  "-11");

    R(r, GT, "11.51",   "12");
    R(r, GT, "-11.51",  "-11");
    R(r, GT, "11.99",   "12");
    R(r, GT, "-11.99",  "-11");
}

TEST(DecimalTest, TestRounding5)
{
    std::string s;
    flinter::Decimal a;
    const flinter::Decimal::Rounding r =
            flinter::Decimal::kRoundingTowardsNegativeInfinity;

    R(r, LT, "11.50",   "11");
    R(r, LT, "12.50",   "12");
    R(r, LT, "-11.50",  "-12");
    R(r, LT, "-12.50",  "-13");

    R(r, EQ, "11.00",   "11");
    R(r, EQ, "-11.00",  "-11");

    R(r, LT, "11.01",   "11");
    R(r, LT, "-11.01",  "-12");
    R(r, LT, "11.49",   "11");
    R(r, LT, "-11.49",  "-12");

    R(r, LT, "11.51",   "11");
    R(r, LT, "-11.51",  "-12");
    R(r, LT, "11.99",   "11");
    R(r, LT, "-11.99",  "-12");
}
