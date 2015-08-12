/* Copyright 2014 yiyuanzhong@gmail.com (Yiyuan Zhong)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <gtest/gtest.h>

#include <ClearSilver/ClearSilver.h>
#include <flinter/types/tree.h>
#include <flinter/xml.h>

using flinter::Tree;
using flinter::Xml;

class TreeTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        Xml::Initialize();
    }

    static void TearDownTestCase()
    {
        NEOERR *err = nerr_shutdown();
        if (err != STATUS_OK) {
            nerr_ignore(&err);
        }

        Xml::Shutdown();
    }
};

TEST_F(TreeTest, TestHdf)
{
    Tree t;
    ASSERT_TRUE(t.ParseFromHdfString("a=b\nc{\nd=e\n}"));

    std::string str;
    ASSERT_TRUE(t.SerializeToHdfString(&str, false));
    printf("HDF\n%s\n", str.c_str());
}

TEST_F(TreeTest, TestXML)
{
    Tree t;
    ASSERT_TRUE(t.ParseFromXmlString("<?xml version=\"1.0\" ?><root><a><b><k>z</k><i>sasa</i></b><b>d</b><b><ss /></b><c /></a></root>"));

    std::string str;
    ASSERT_TRUE(t.SerializeToHdfString(&str, false));
    printf("HDF\n%s\n", str.c_str());

    ASSERT_TRUE(t.SerializeToXmlString(&str, false));
    printf("XML\n%s\n", str.c_str());

    ASSERT_TRUE(t.SerializeToHdfString(&str, true));
    printf("eHDF\n%s\n", str.c_str());

    ASSERT_TRUE(t.SerializeToXmlString(&str, true));
    printf("eXML\n%s\n", str.c_str());
}

TEST_F(TreeTest, TestJson)
{
    Tree t;
    ASSERT_TRUE(t.ParseFromJsonString("{\"a\":5,\"b\":{\"c\":\"dd\",\"d\":[1,{\"z\":282},5,\"dd\",7]}}"));

    std::string str;
    ASSERT_TRUE(t.SerializeToHdfString(&str, false));
    printf("HDF\n%s\n", str.c_str());
}

TEST_F(TreeTest, TestBool)
{
    Tree t;
    ASSERT_TRUE(t.ParseFromHdfString("a=1\nb=true\nc=0\nd=FAlse\ne=ab"));

    bool valid;
    EXPECT_TRUE(t["a"].as<bool>(true, &valid));
    EXPECT_TRUE(valid);
    EXPECT_TRUE(t["b"].as<bool>(true, &valid));
    EXPECT_TRUE(valid);
    EXPECT_FALSE(t["c"].as<bool>(true, &valid));
    EXPECT_TRUE(valid);
    EXPECT_FALSE(t["d"].as<bool>(true, &valid));
    EXPECT_TRUE(valid);
    EXPECT_TRUE(t["e"].as<bool>(true, &valid));
    EXPECT_FALSE(valid);
    EXPECT_FALSE(t["e"].as<bool>(false, &valid));
    EXPECT_FALSE(valid);
}

TEST_F(TreeTest, TestString)
{
    Tree t;
    ASSERT_TRUE(t.ParseFromHdfString("a=a\nb="));

    bool valid;
    EXPECT_EQ(t["a"].as<std::string>("z", &valid), "a");
    EXPECT_TRUE(valid);
    EXPECT_EQ(t["b"].as<std::string>("z", &valid), "z");
    EXPECT_FALSE(valid);
    EXPECT_STREQ(t["a"].as<const char *>("z", &valid), "a");
    EXPECT_TRUE(valid);
    EXPECT_STREQ(t["b"].as<const char *>("z", &valid), "z");
    EXPECT_FALSE(valid);
    EXPECT_STREQ(t["a"].as("z", &valid), "a");
    EXPECT_TRUE(valid);
    EXPECT_STREQ(t["b"].as("z", &valid), "z");
    EXPECT_FALSE(valid);
}
