#include <gtest/gtest.h>
#include <sys.h>

TEST(StringTest, isEmptyWorks) {
  String str;
  ASSERT_TRUE( str.isEmpty() );
}

TEST(StringTest, setFromStopwatchWorks) {
    String str;
    str.setFromStopwatch(60 * 60 * 24);
    ASSERT_STREQ("1 day, 0 hour", str);

    str.setFromStopwatch(0);
    ASSERT_STREQ("-", str);
}

TEST(StringTest, eqWorks) {
    String s1("abc");
    String s2("abc");
    String s3("ABC");
    String s4("xyz");

    ASSERT_EQ(s1, s2);
    ASSERT_NE(s1, s3);
    ASSERT_NE(s1, s4);
}

TEST(StringTest, containsWorks) {
    String abc("abc");
    String ABC("ABC");
    String xyz("xyz");
    String a("a");

    ASSERT_TRUE(abc.contains(ABC));
    ASSERT_FALSE(abc.contains(xyz));
    ASSERT_FALSE(ABC.contains(xyz));

    ASSERT_TRUE(abc.contains(a));
    ASSERT_FALSE(a.contains(abc));
}

TEST(StringTest, appendWorks) {
    String buf;
    for (int i = 0; i < 500; i++) {
        buf.append('A');
    }

    ASSERT_EQ(255, strlen(buf.data));
}

TEST(StringTest, prependWorks) {
    String buf = " world!";
    buf.prepend("Hello");
    ASSERT_STREQ("Hello world!", buf);

    String buf2 = " world!AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";
    buf2.prepend("Hello");
    ASSERT_STREQ("Hello", buf2);
}

TEST(StringTest, isWhitespaceWorks) {
    ASSERT_TRUE(String::isWhitespace(' '));
    ASSERT_TRUE(String::isWhitespace('\t'));
    ASSERT_FALSE(String::isWhitespace('A'));
    ASSERT_FALSE(String::isWhitespace('-'));
    ASSERT_FALSE(String::isWhitespace('\r'));
    ASSERT_FALSE(String::isWhitespace('\n'));
}

TEST(StringTest, ASCII2HTMLWorks) {
    String str;
    str.ASCII2HTML("AAA");
    ASSERT_STREQ("AAA", str);

    str.ASCII2HTML("   ");
    ASSERT_STREQ("&#x20;&#x20;&#x20;", str);

    String str2("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"); // 255
    str.ASCII2HTML(str2);
    ASSERT_EQ(246, strlen(str.data));

    str.ASCII2HTML("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA ");
    ASSERT_EQ(251, strlen(str.data));
}

TEST(StringTest, HTML2ASCIIWorks) {
    String str;

    str.HTML2ASCII("&#x21;");
    ASSERT_STREQ("!", str);

    str.HTML2ASCII("A");
    ASSERT_STREQ("A", str);

    str.HTML2ASCII("&amp;");
    ASSERT_STREQ("&amp;", str);
}
