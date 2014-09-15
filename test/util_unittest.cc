#include <gtest/gtest.h>
#include "common/util.h"
using namespace std;
using namespace util;

TEST(UtilTest, splitWorks)
{
    ASSERT_EQ(vector<string>({"a","b"}), split("a&b", "&"));
    ASSERT_EQ(vector<string>(), split("", "&"));
}
