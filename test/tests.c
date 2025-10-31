#include "avr/io.h"
#include "unity.h"
#include "filter.h"
#include "tone.h"
#include "util.h"

void setUp(void) {}
void tearDown(void) {}

void test_filter_size_1() {
  set_filter_size(1);
  clear_filter();

  push_filter_value(10.0);
  TEST_ASSERT_EQUAL_FLOAT(10.0, get_filtered_distance());

  push_filter_value(20.0);
  TEST_ASSERT_EQUAL_FLOAT(20.0, get_filtered_distance());
}

void test_filter_size_4() {
  set_filter_size(4);
  clear_filter();

  push_filter_value(10.0);
  TEST_ASSERT_EQUAL_FLOAT(10.0, get_filtered_distance());

  push_filter_value(20.0);
  TEST_ASSERT_EQUAL_FLOAT(20.0, get_filtered_distance());

  push_filter_value(30.0);
  TEST_ASSERT_EQUAL_FLOAT(20.0, get_filtered_distance());

  push_filter_value(40.0);
  TEST_ASSERT_EQUAL_FLOAT(30.0, get_filtered_distance());

  push_filter_value(50.0);
  TEST_ASSERT_EQUAL_FLOAT(40.0, get_filtered_distance());

  push_filter_value(60.0);
  TEST_ASSERT_EQUAL_FLOAT(50.0, get_filtered_distance());

  push_filter_value(70.0);
  TEST_ASSERT_EQUAL_FLOAT(60.0, get_filtered_distance());

  push_filter_value(10.0);
  TEST_ASSERT_EQUAL_FLOAT(60.0, get_filtered_distance());

  push_filter_value(20.0);
  TEST_ASSERT_EQUAL_FLOAT(60.0, get_filtered_distance());

  push_filter_value(30.0);
  TEST_ASSERT_EQUAL_FLOAT(30.0, get_filtered_distance());
}

void test_tone_mod() {
  adjust_tone(440.0, 128);
  TEST_ASSERT_EQUAL_UINT8(71, OCR0A);
  TEST_ASSERT_EQUAL_UINT8(128, OCR2B);
  
  adjust_tone(880.0, 255);
  TEST_ASSERT_EQUAL_UINT8(35, OCR0A);
  TEST_ASSERT_EQUAL_UINT8(255, OCR2B);
}

void test_util() {
  TEST_ASSERT_EQUAL_INT(2000, US_TO_TICKS(1000, 8));
  TEST_ASSERT_EQUAL_INT(250, US_TO_TICKS(1000, 64));
  TEST_ASSERT_EQUAL_INT(10, MAX(5, 10));
  TEST_ASSERT_EQUAL_INT(5, MIN(5, 10));
  TEST_ASSERT_EQUAL_INT(10, LIMIT(15, 0, 10));
  TEST_ASSERT_EQUAL_INT(0, LIMIT(-5, 0, 10));
  TEST_ASSERT_EQUAL_FLOAT(1400.0, MAP_FREQUENCY(0.0));
  TEST_ASSERT_EQUAL_FLOAT(230.0, MAP_FREQUENCY(65.0));
}

int runUnityTests(void) {
    UNITY_BEGIN();
    RUN_TEST(test_filter_size_1);
    RUN_TEST(test_filter_size_4);
    RUN_TEST(test_tone_mod);
    RUN_TEST(test_util);
    return UNITY_END();
}

int main(void) {
    return runUnityTests();
}
