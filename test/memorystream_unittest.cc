#include <gtest/gtest.h>
#include "common/stream.h"

TEST(MemoryStreamTest, justWorks)
{
    MemoryStream mm(1);
    char buf[1024];

    ASSERT_EQ(1, mm.len);
    ASSERT_EQ(0, mm.getPosition());

    ASSERT_NO_THROW(mm.write("abc", 3));
    mm.rewind();
    ASSERT_EQ(3, mm.readUpto(buf, 1024));
    buf[3] = '\0';
    ASSERT_STREQ(buf, "abc");

    char data[] = "hoge";
    MemoryStream mm2(data, 4);
    ASSERT_FALSE(mm2.own);
    ASSERT_EQ(5, sizeof("hoge"));
    ASSERT_THROW(mm2.write("hogehoge", 8), StreamException);
    ASSERT_EQ(4, mm2.readUpto(buf, 1024));
    buf[4] = '\0';
    ASSERT_STREQ("hoge", buf);
}
