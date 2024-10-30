#include "helpers/test_helpers.h"
#include <DSP/ProPhatProcessor.h>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

TEST_CASE ("one is equal to one", "[dummy]")
{
    REQUIRE (1 == 1);
}

TEST_CASE ("Plugin instance", "[instance]")
{
    ProPhatProcessor testPlugin;

    SECTION ("name")
    {
        CHECK_THAT (testPlugin.getName().toStdString(),
            Catch::Matchers::Equals ("ProPhat"));
    }
}


#ifdef PAMPLEJUCE_IPP
    #include <ipp.h>

TEST_CASE ("IPP version", "[ipp]")
{
    CHECK_THAT (ippsGetLibVersion()->Version, Catch::Matchers::Equals ("2021.11.0 (r0xcd107b02)"));
}
#endif
