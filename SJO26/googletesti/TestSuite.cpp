#include <gtest/gtest.h>
#include "TimeParser.h"

// Test suite: TimeParserTest
TEST(TimeParserTest, TestCaseCorrectTime) {

    // Note that this test fails on purpose!!

    // Test with correct time string
    char time_test[] = "141205";
    ASSERT_EQ(time_parse(time_test), 14*3600 + 12*60 + 5);

}

TEST(TimeParserTest, TestCaseSecondsLowerLimit) {
    char time_test[] = "000000";
    ASSERT_EQ(time_parse(time_test), 0);
}

TEST(TimeParserTest, TestCaseSecondsUpperLimit) {
    char time_test[] = "000059";
    ASSERT_EQ(time_parse(time_test), 59);
}

TEST(TimeParserTest, TestCaseSecondsOverLimit) {
    char time_test[] = "000060";
    ASSERT_LT(time_parse(time_test), 0);
}

TEST(TimeParserTest, TestCaseSecondsNegative) {
    char time_test[] = "0000-1";
    ASSERT_LT(time_parse(time_test), 0);
}

TEST(TimeParserTest, TestCaseMinutesLowerLimit) {
    char time_test[] = "000000";
    ASSERT_EQ(time_parse(time_test), 0);
}

TEST(TimeParserTest, TestCaseMinutesUpperLimit) {
    char time_test[] = "005900";
    ASSERT_EQ(time_parse(time_test), 59*60);
}

TEST(TimeParserTest, TestCaseMinutesOverLimit) {
    char time_test[] = "006000";
    ASSERT_LT(time_parse(time_test), 0);
}

TEST(TimeParserTest, TestCaseMinutesNegative) {
    char time_test[] = "00-100";
    ASSERT_LT(time_parse(time_test), 0);
}

TEST(TimeParserTest, TestCaseHoursLowerLimit) {
    char time_test[] = "000000";
    ASSERT_EQ(time_parse(time_test), 0);
}

TEST(TimeParserTest, TestCaseHoursUpperLimit) {
    char time_test[] = "230000";
    ASSERT_EQ(time_parse(time_test), 23*3600);
}

TEST(TimeParserTest, TestCaseHoursOverLimit) {
    char time_test[] = "240000";
    ASSERT_LT(time_parse(time_test), 0);
}

TEST(TimeParserTest, TestCaseHoursNegative) {
    char time_test[] = "-10000";
    ASSERT_LT(time_parse(time_test), 0);
}

TEST(TimeParserTest, TestCaseAllValuesAtUpperLimit) {
    char time_test[] = "235959";
    ASSERT_EQ(time_parse(time_test), 23*3600 + 59*60 + 59);
}

TEST(TimeParserTest, TestCaseExampleOneMinuteTwentySeconds) {
    char time_test[] = "000120";
    ASSERT_EQ(time_parse(time_test), 80);
}

