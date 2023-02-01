#include <iostream>

// Include the GTest headers
#include "gtest_headers.h"

// Include the SYSTEM MOCK and PFR headers
#include "ut_nios_wrapper.h"

class PFRCertUtilsTest : public testing::Test
{
public:

    virtual void SetUp()
    {
        SYSTEM_MOCK::get()->reset();
    }

    virtual void TearDown() {}
};

TEST_F(PFRCertUtilsTest, test_decode_der_datetime_decode_utctime_test_case1)
{
    alt_u8 datetime_length = CERT_VALIDITY_UTCTIME_LENGTH;
    alt_u8 utctime_testdata[CERT_VALIDITY_UTCTIME_LENGTH] = {'2','1','1','0','1','7','2','2','2','5','1','0','Z'};

    CERT_DER_DATETIME datetime;
    CERT_DER_DATETIME* datetime_ptr = &datetime;

    if ( CERTIFICATE_VERIFY_FAIL == pfr_decode_der_datetime_value(utctime_testdata, &datetime_length, CERT_DER_DATETIME_UTCTIME, datetime_ptr) ) {
        GTEST_NONFATAL_FAILURE_("Failed: Fail to decode DER UTCTIME");
    }

    EXPECT_EQ(20, datetime_ptr->year_0);
    EXPECT_EQ(21, datetime_ptr->year_1);
    EXPECT_EQ(10, datetime_ptr->month);
    EXPECT_EQ(17, datetime_ptr->day);
    EXPECT_EQ(22, datetime_ptr->hour);
    EXPECT_EQ(25, datetime_ptr->minute);
    EXPECT_EQ(10, datetime_ptr->second);
}

TEST_F(PFRCertUtilsTest, test_decode_der_datetime_decode_utctime_test_case1_neg)
{
    alt_u8 datetime_length = CERT_VALIDITY_UTCTIME_LENGTH;
    alt_u8 utctime_testdata[CERT_VALIDITY_UTCTIME_LENGTH] = {'2','1','1','0','1','7','2','2','2','5','1','0','1'};

    CERT_DER_DATETIME datetime;
    CERT_DER_DATETIME* datetime_ptr = &datetime;

    if ( CERTIFICATE_VERIFY_PASS == pfr_decode_der_datetime_value(utctime_testdata, &datetime_length, CERT_DER_DATETIME_UTCTIME, datetime_ptr) ) {
        GTEST_NONFATAL_FAILURE_("Failed: Fail to detect missing 'Z' char in DER UTCTIME");
    }
}

TEST_F(PFRCertUtilsTest, test_decode_der_datetime_decode_generalizedtime_test_case1)
{
    alt_u8 datetime_length = 15;
    alt_u8 generalizedtime_testdata[15] = {'2','3','7','7','0','8','3','1','1','8','3','0','4','5','Z'};

    CERT_DER_DATETIME datetime;
    CERT_DER_DATETIME* datetime_ptr = &datetime;

    if ( CERTIFICATE_VERIFY_FAIL == pfr_decode_der_datetime_value(generalizedtime_testdata, &datetime_length, CERT_DER_DATETIME_GENERALIZEDTIME, datetime_ptr) ) {
        GTEST_NONFATAL_FAILURE_("Failed: Fail to decode DER GENERALIZEDTIME");
    }

    EXPECT_EQ(23, datetime_ptr->year_0);
    EXPECT_EQ(77, datetime_ptr->year_1);
    EXPECT_EQ(8, datetime_ptr->month);
    EXPECT_EQ(31, datetime_ptr->day);
    EXPECT_EQ(18, datetime_ptr->hour);
    EXPECT_EQ(30, datetime_ptr->minute);
    EXPECT_EQ(45, datetime_ptr->second);
}

TEST_F(PFRCertUtilsTest, test_decode_der_datetime_decode_generalizedtime_test_case2)
{
    // fw can handle fractional seconds in GENERALIZEDTIME values
    alt_u8 datetime_length = 17;
    alt_u8 generalizedtime_testdata[17] = {'2','3','7','7','0','8','3','1','1','8','3','0','4','5','.','3','Z'};

    CERT_DER_DATETIME datetime;
    CERT_DER_DATETIME* datetime_ptr = &datetime;

    if ( CERTIFICATE_VERIFY_FAIL == pfr_decode_der_datetime_value(generalizedtime_testdata, &datetime_length, CERT_DER_DATETIME_GENERALIZEDTIME, datetime_ptr) ) {
        GTEST_NONFATAL_FAILURE_("Failed: Fail to decode DER GENERALIZEDTIME");
    }

    EXPECT_EQ(23, datetime_ptr->year_0);
    EXPECT_EQ(77, datetime_ptr->year_1);
    EXPECT_EQ(8, datetime_ptr->month);
    EXPECT_EQ(31, datetime_ptr->day);
    EXPECT_EQ(18, datetime_ptr->hour);
    EXPECT_EQ(30, datetime_ptr->minute);
    EXPECT_EQ(45, datetime_ptr->second);
}

TEST_F(PFRCertUtilsTest, test_decode_der_datetime_decode_generalizedtime_test_case1_neg)
{
    // GENERALIZEDTIME values must end with 'Z'
    alt_u8 datetime_length = 21;
    alt_u8 generalizedtime_testdata[21] = {'2','3','7','7','0','8','3','1','1','8','3','0','4','5','.','3','-','0','5','0','0'};

    CERT_DER_DATETIME datetime;
    CERT_DER_DATETIME* datetime_ptr = &datetime;

    if ( CERTIFICATE_VERIFY_PASS == pfr_decode_der_datetime_value(generalizedtime_testdata, &datetime_length, CERT_DER_DATETIME_GENERALIZEDTIME, datetime_ptr) ) {
        GTEST_NONFATAL_FAILURE_("Failed: Fail to detect missing 'Z' char in DER GENERALIZEDTIME");
    }
}

TEST_F(PFRCertUtilsTest, test_decode_der_datetime_validity_test_case1_neg)
{
    alt_u8 datetime_length = 15;
    // second > 59
    alt_u8 generalizedtime_testdata[15] = {'2','0','2','0','1','2','3','1','2','3','5','9','6','0','Z'};

    CERT_DER_DATETIME datetime;
    CERT_DER_DATETIME* datetime_ptr = &datetime;

    alt_u32 expect_cert_datetime_decode_pass = CERTIFICATE_VERIFY_PASS;
    ASSERT_EQ( expect_cert_datetime_decode_pass, pfr_decode_der_datetime_value(generalizedtime_testdata, &datetime_length, CERT_DER_DATETIME_GENERALIZEDTIME, datetime_ptr) );
    if ( CERTIFICATE_VERIFY_PASS == pfr_decode_der_datetime_validity_check(datetime_ptr) ) {
        GTEST_NONFATAL_FAILURE_("Failed: Fail to detecct invalid datetime value");
    }
}

TEST_F(PFRCertUtilsTest, test_decode_der_datetime_validity_test_case2_neg)
{
    alt_u8 datetime_length = 15;
    // minute > 59
    alt_u8 generalizedtime_testdata[15] = {'2','0','2','0','1','2','3','1','2','3','6','0','5','9','Z'};

    CERT_DER_DATETIME datetime;
    CERT_DER_DATETIME* datetime_ptr = &datetime;

    alt_u32 expect_cert_datetime_decode_pass = CERTIFICATE_VERIFY_PASS;
    ASSERT_EQ( expect_cert_datetime_decode_pass, pfr_decode_der_datetime_value(generalizedtime_testdata, &datetime_length, CERT_DER_DATETIME_GENERALIZEDTIME, datetime_ptr) );
    if ( CERTIFICATE_VERIFY_PASS == pfr_decode_der_datetime_validity_check(datetime_ptr) ) {
        GTEST_NONFATAL_FAILURE_("Failed: Fail to detecct invalid datetime value");
    }
}

TEST_F(PFRCertUtilsTest, test_decode_der_datetime_validity_test_case3_neg)
{
    alt_u8 datetime_length = 15;
    // hour > 23
    alt_u8 generalizedtime_testdata[15] = {'2','0','2','0','1','2','3','1','2','4','5','9','5','9','Z'};

    CERT_DER_DATETIME datetime;
    CERT_DER_DATETIME* datetime_ptr = &datetime;

    alt_u32 expect_cert_datetime_decode_pass = CERTIFICATE_VERIFY_PASS;
    ASSERT_EQ( expect_cert_datetime_decode_pass, pfr_decode_der_datetime_value(generalizedtime_testdata, &datetime_length, CERT_DER_DATETIME_GENERALIZEDTIME, datetime_ptr) );
    if ( CERTIFICATE_VERIFY_PASS == pfr_decode_der_datetime_validity_check(datetime_ptr) ) {
        GTEST_NONFATAL_FAILURE_("Failed: Fail to detecct invalid datetime value");
    }
}

TEST_F(PFRCertUtilsTest, test_decode_der_datetime_validity_test_case4_neg)
{
    alt_u8 datetime_length = 15;
    // month > 12
    alt_u8 generalizedtime_testdata[15] = {'2','0','2','0','1','3','3','1','2','4','5','9','5','9','Z'};

    CERT_DER_DATETIME datetime;
    CERT_DER_DATETIME* datetime_ptr = &datetime;

    alt_u32 expect_cert_datetime_decode_pass = CERTIFICATE_VERIFY_PASS;
    ASSERT_EQ( expect_cert_datetime_decode_pass, pfr_decode_der_datetime_value(generalizedtime_testdata, &datetime_length, CERT_DER_DATETIME_GENERALIZEDTIME, datetime_ptr) );
    if ( CERTIFICATE_VERIFY_PASS == pfr_decode_der_datetime_validity_check(datetime_ptr) ) {
        GTEST_NONFATAL_FAILURE_("Failed: Fail to detecct invalid datetime value");
    }
}

TEST_F(PFRCertUtilsTest, test_decode_der_datetime_validity_test_case5_neg)
{
    alt_u8 datetime_length = 15;
    // November only has 30 days in the month
    alt_u8 generalizedtime_testdata[15] = {'2','0','2','0','1','1','3','1','2','4','5','9','5','9','Z'};

    CERT_DER_DATETIME datetime;
    CERT_DER_DATETIME* datetime_ptr = &datetime;

    alt_u32 expect_cert_datetime_decode_pass = CERTIFICATE_VERIFY_PASS;
    ASSERT_EQ( expect_cert_datetime_decode_pass, pfr_decode_der_datetime_value(generalizedtime_testdata, &datetime_length, CERT_DER_DATETIME_GENERALIZEDTIME, datetime_ptr) );
    if ( CERTIFICATE_VERIFY_PASS == pfr_decode_der_datetime_validity_check(datetime_ptr) ) {
        GTEST_NONFATAL_FAILURE_("Failed: Fail to detecct invalid datetime value");
    }
}

TEST_F(PFRCertUtilsTest, test_decode_der_datetime_validity_leap_year_test_case1)
{
    alt_u8 datetime_length = 15;
    alt_u8 generalizedtime_testdata[15] = {'2','0','0','0','0','2','2','9','2','3','5','9','5','9','Z'};
    // 2000 is a leap year, Feb. dates should <= 29

    CERT_DER_DATETIME datetime;
    CERT_DER_DATETIME* datetime_ptr = &datetime;

    alt_u32 expect_cert_datetime_decode_pass = CERTIFICATE_VERIFY_PASS;
    ASSERT_EQ( expect_cert_datetime_decode_pass, pfr_decode_der_datetime_value(generalizedtime_testdata, &datetime_length, CERT_DER_DATETIME_GENERALIZEDTIME, datetime_ptr) );
    if ( CERTIFICATE_VERIFY_FAIL == pfr_decode_der_datetime_validity_check(datetime_ptr) ) {
        GTEST_NONFATAL_FAILURE_("Failed: Wrongly determine leap year");
    }
}

TEST_F(PFRCertUtilsTest, test_decode_der_datetime_validity_leap_year_test_case2)
{
    alt_u8 datetime_length = 15;
    alt_u8 generalizedtime_testdata[15] = {'2','0','2','0','0','2','2','9','2','3','5','9','5','9','Z'};
    // 2020 is a leap year, Feb. dates should <= 29

    CERT_DER_DATETIME datetime;
    CERT_DER_DATETIME* datetime_ptr = &datetime;

    alt_u32 expect_cert_datetime_decode_pass = CERTIFICATE_VERIFY_PASS;
    ASSERT_EQ( expect_cert_datetime_decode_pass, pfr_decode_der_datetime_value(generalizedtime_testdata, &datetime_length, CERT_DER_DATETIME_GENERALIZEDTIME, datetime_ptr) );
    if ( CERTIFICATE_VERIFY_FAIL == pfr_decode_der_datetime_validity_check(datetime_ptr) ) {
        GTEST_NONFATAL_FAILURE_("Failed: Wrongly determine leap year");
    }
}

TEST_F(PFRCertUtilsTest, test_decode_der_datetime_validity_leap_year_test_case1_neg)
{
    alt_u8 datetime_length = 15;
    alt_u8 generalizedtime_testdata[15] = {'2','0','2','1','0','2','2','9','2','3','5','9','5','9','Z'};
    // 2021 is not a leap year, Feb. dates should <= 28

    CERT_DER_DATETIME datetime;
    CERT_DER_DATETIME* datetime_ptr = &datetime;

    alt_u32 expect_cert_datetime_decode_pass = CERTIFICATE_VERIFY_PASS;
    ASSERT_EQ( expect_cert_datetime_decode_pass, pfr_decode_der_datetime_value(generalizedtime_testdata, &datetime_length, CERT_DER_DATETIME_GENERALIZEDTIME, datetime_ptr) );
    if ( CERTIFICATE_VERIFY_PASS == pfr_decode_der_datetime_validity_check(datetime_ptr) ) {
        GTEST_NONFATAL_FAILURE_("Failed: Wrongly determine leap year");
    }
}

TEST_F(PFRCertUtilsTest, test_decode_der_datetime_validity_leap_year_test_case2_neg)
{
    alt_u8 datetime_length = 15;
    alt_u8 generalizedtime_testdata[15] = {'1','9','0','0','0','2','2','9','2','3','5','9','5','9','Z'};
    // 1900 is not a leap year, Feb. dates should <= 28

    CERT_DER_DATETIME datetime;
    CERT_DER_DATETIME* datetime_ptr = &datetime;

    alt_u32 expect_cert_datetime_decode_pass = CERTIFICATE_VERIFY_PASS;
    ASSERT_EQ( expect_cert_datetime_decode_pass, pfr_decode_der_datetime_value(generalizedtime_testdata, &datetime_length, CERT_DER_DATETIME_GENERALIZEDTIME, datetime_ptr) );
    if ( CERTIFICATE_VERIFY_PASS == pfr_decode_der_datetime_validity_check(datetime_ptr) ) {
        GTEST_NONFATAL_FAILURE_("Failed: Wrongly determine leap year");
    }
}

TEST_F(PFRCertUtilsTest, test_decode_der_compare_utctime_case1)
{
    // pass when time is equal
    alt_u8 datetime_length = CERT_VALIDITY_UTCTIME_LENGTH;
    alt_u8 test_valid_from[CERT_VALIDITY_UTCTIME_LENGTH] = {'2','0','0','1','0','1','1','2','0','0','0','0','Z'};
    alt_u8 test_valid_till[CERT_VALIDITY_UTCTIME_LENGTH] = {'2','0','0','1','0','1','1','2','0','0','0','0','Z'};

    CERT_DER_DATETIME datetime_from;
    CERT_DER_DATETIME* datetime_from_ptr = &datetime_from;
    CERT_DER_DATETIME datetime_till;
    CERT_DER_DATETIME* datetime_till_ptr = &datetime_till;

    alt_u32 expect_certificate_verify_pass = CERTIFICATE_VERIFY_PASS;
    ASSERT_EQ( expect_certificate_verify_pass, pfr_decode_der_datetime_value(test_valid_from, &datetime_length, CERT_DER_DATETIME_UTCTIME, datetime_from_ptr) );
    ASSERT_EQ( expect_certificate_verify_pass, pfr_decode_der_datetime_validity_check(datetime_from_ptr) );
    ASSERT_EQ( expect_certificate_verify_pass, pfr_decode_der_datetime_value(test_valid_till, &datetime_length, CERT_DER_DATETIME_UTCTIME, datetime_till_ptr) );
    ASSERT_EQ( expect_certificate_verify_pass, pfr_decode_der_datetime_validity_check(datetime_till_ptr) );

    if ( !pfr_decode_der_datetime_compare( datetime_from_ptr, datetime_till_ptr) ) {
        GTEST_NONFATAL_FAILURE_("Failed: unexpected output for pfr_decode_der_datetime_compare()");
    }

}

TEST_F(PFRCertUtilsTest, test_decode_der_compare_utctime_case2)
{
    alt_u8 datetime_length = CERT_VALIDITY_UTCTIME_LENGTH;
    alt_u8 test_valid_from[CERT_VALIDITY_UTCTIME_LENGTH] = {'2','0','0','1','0','1','1','2','0','0','0','0','Z'};
    alt_u8 test_valid_till[CERT_VALIDITY_UTCTIME_LENGTH] = {'2','0','0','1','0','1','1','9','0','0','0','0','Z'};

    CERT_DER_DATETIME datetime_from;
    CERT_DER_DATETIME* datetime_from_ptr = &datetime_from;
    CERT_DER_DATETIME datetime_till;
    CERT_DER_DATETIME* datetime_till_ptr = &datetime_till;

    alt_u32 expect_certificate_verify_pass = CERTIFICATE_VERIFY_PASS;
    ASSERT_EQ( expect_certificate_verify_pass, pfr_decode_der_datetime_value(test_valid_from, &datetime_length, CERT_DER_DATETIME_UTCTIME, datetime_from_ptr) );
    ASSERT_EQ( expect_certificate_verify_pass, pfr_decode_der_datetime_validity_check(datetime_from_ptr) );
    ASSERT_EQ( expect_certificate_verify_pass, pfr_decode_der_datetime_value(test_valid_till, &datetime_length, CERT_DER_DATETIME_UTCTIME, datetime_till_ptr) );
    ASSERT_EQ( expect_certificate_verify_pass, pfr_decode_der_datetime_validity_check(datetime_till_ptr) );

    if ( !pfr_decode_der_datetime_compare( datetime_from_ptr, datetime_till_ptr) ) {
        GTEST_NONFATAL_FAILURE_("Failed: unexpected output for pfr_decode_der_datetime_compare()");
    }
}

TEST_F(PFRCertUtilsTest, test_decode_der_compare_utctime_case3)
{
    alt_u8 datetime_length = CERT_VALIDITY_UTCTIME_LENGTH;
    alt_u8 test_valid_from[CERT_VALIDITY_UTCTIME_LENGTH] = {'2','0','1','2','3','1','2','3','5','9','5','8','Z'};
    alt_u8 test_valid_till[CERT_VALIDITY_UTCTIME_LENGTH] = {'2','0','1','2','3','1','2','3','5','9','5','9','Z'};

    CERT_DER_DATETIME datetime_from;
    CERT_DER_DATETIME* datetime_from_ptr = &datetime_from;
    CERT_DER_DATETIME datetime_till;
    CERT_DER_DATETIME* datetime_till_ptr = &datetime_till;

    alt_u32 expect_certificate_verify_pass = CERTIFICATE_VERIFY_PASS;
    ASSERT_EQ( expect_certificate_verify_pass, pfr_decode_der_datetime_value(test_valid_from, &datetime_length, CERT_DER_DATETIME_UTCTIME, datetime_from_ptr) );
    ASSERT_EQ( expect_certificate_verify_pass, pfr_decode_der_datetime_validity_check(datetime_from_ptr) );
    ASSERT_EQ( expect_certificate_verify_pass, pfr_decode_der_datetime_value(test_valid_till, &datetime_length, CERT_DER_DATETIME_UTCTIME, datetime_till_ptr) );
    ASSERT_EQ( expect_certificate_verify_pass, pfr_decode_der_datetime_validity_check(datetime_till_ptr) );

    if ( !pfr_decode_der_datetime_compare( datetime_from_ptr, datetime_till_ptr) ) {
        GTEST_NONFATAL_FAILURE_("Failed: unexpected output for pfr_decode_der_datetime_compare()");
    }
}

TEST_F(PFRCertUtilsTest, test_decode_der_compare_utctime_case1_neg)
{
    alt_u8 datetime_length = CERT_VALIDITY_UTCTIME_LENGTH;
    alt_u8 test_valid_from[CERT_VALIDITY_UTCTIME_LENGTH] = {'2','0','0','1','3','1','1','0','0','0','0','0','Z'};
    alt_u8 test_valid_till[CERT_VALIDITY_UTCTIME_LENGTH] = {'2','0','0','1','0','1','1','0','0','0','0','0','Z'};

    CERT_DER_DATETIME datetime_from;
    CERT_DER_DATETIME* datetime_from_ptr = &datetime_from;
    CERT_DER_DATETIME datetime_till;
    CERT_DER_DATETIME* datetime_till_ptr = &datetime_till;

    alt_u32 expect_certificate_verify_pass = CERTIFICATE_VERIFY_PASS;
    ASSERT_EQ( expect_certificate_verify_pass, pfr_decode_der_datetime_value(test_valid_from, &datetime_length, CERT_DER_DATETIME_UTCTIME, datetime_from_ptr) );
    ASSERT_EQ( expect_certificate_verify_pass, pfr_decode_der_datetime_validity_check(datetime_from_ptr) );
    ASSERT_EQ( expect_certificate_verify_pass, pfr_decode_der_datetime_value(test_valid_till, &datetime_length, CERT_DER_DATETIME_UTCTIME, datetime_till_ptr) );
    ASSERT_EQ( expect_certificate_verify_pass, pfr_decode_der_datetime_validity_check(datetime_till_ptr) );

    if ( pfr_decode_der_datetime_compare( datetime_from_ptr, datetime_till_ptr) ) {
        GTEST_NONFATAL_FAILURE_("Failed: unexpected output for pfr_decode_der_datetime_compare()");
    }
}

TEST_F(PFRCertUtilsTest, test_decode_der_compare_utctime_case2_neg)
{
    alt_u8 datetime_length = CERT_VALIDITY_UTCTIME_LENGTH;
    alt_u8 test_valid_from[CERT_VALIDITY_UTCTIME_LENGTH] = {'2','0','1','2','3','1','2','3','5','9','5','9','Z'};
    alt_u8 test_valid_till[CERT_VALIDITY_UTCTIME_LENGTH] = {'2','0','1','2','3','1','2','3','5','9','5','8','Z'};

    CERT_DER_DATETIME datetime_from;
    CERT_DER_DATETIME* datetime_from_ptr = &datetime_from;
    CERT_DER_DATETIME datetime_till;
    CERT_DER_DATETIME* datetime_till_ptr = &datetime_till;

    alt_u32 expect_certificate_verify_pass = CERTIFICATE_VERIFY_PASS;
    ASSERT_EQ( expect_certificate_verify_pass, pfr_decode_der_datetime_value(test_valid_from, &datetime_length, CERT_DER_DATETIME_UTCTIME, datetime_from_ptr) );
    ASSERT_EQ( expect_certificate_verify_pass, pfr_decode_der_datetime_validity_check(datetime_from_ptr) );
    ASSERT_EQ( expect_certificate_verify_pass, pfr_decode_der_datetime_value(test_valid_till, &datetime_length, CERT_DER_DATETIME_UTCTIME, datetime_till_ptr) );
    ASSERT_EQ( expect_certificate_verify_pass, pfr_decode_der_datetime_validity_check(datetime_till_ptr) );

    if ( pfr_decode_der_datetime_compare( datetime_from_ptr, datetime_till_ptr) ) {
        GTEST_NONFATAL_FAILURE_("Failed: unexpected output for pfr_decode_der_datetime_compare()");
    }
}
