#include <Arduino.h>
#include <unity.h>
#include "config.h"

void test_isValidIP_accept_valid()
{
    const char *valid[] = {
        "0.0.0.0",
        "8.8.8.8",
        "192.168.4.1",
        "255.255.255.255"};

    for (auto ip : valid)
    {
        TEST_ASSERT_TRUE_MESSAGE(isValidIP(String(ip)), ip);
    }
}

void test_isValidIP_reject_invalid()
{
    const char *invalid[] = {
        "",          // pusty
        "1.1.1",     // za mało oktetów
        "1.1.1.1.1", // za dużo oktetów
        "300.1.1.1", // oktet > 255
        "256..1",    // brakujące oktety
        "abc.def"};  // nie-numeryczne

    for (auto ip : invalid)
    {
        TEST_ASSERT_FALSE_MESSAGE(isValidIP(String(ip)), ip);
    }
}

void test_config_default_pins()
{
    TEST_ASSERT_EQUAL(D1, config.pinRelay);
    TEST_ASSERT_EQUAL(D2, config.pinRelayBackup);
    TEST_ASSERT_EQUAL(D5, config.pinButton);
    TEST_ASSERT_FALSE(config.relayActiveHigh);
}

void test_config_default_timing()
{
    TEST_ASSERT_EQUAL_UINT32(60000UL, config.pingInterval);
    TEST_ASSERT_EQUAL_UINT32(60000UL, config.routerOffTime);
    TEST_ASSERT_EQUAL_UINT32(150000UL, config.baseBootTime);
    TEST_ASSERT_EQUAL(3, config.failLimit);
}

void setup()
{
    delay(2000); // Stabilizacja UART
    UNITY_BEGIN();
    RUN_TEST(test_isValidIP_accept_valid);
    RUN_TEST(test_isValidIP_reject_invalid);
    RUN_TEST(test_config_default_pins);
    RUN_TEST(test_config_default_timing);
    UNITY_END();
}

void loop()
{
    // Nie używamy pętli w testach jednostkowych
}
