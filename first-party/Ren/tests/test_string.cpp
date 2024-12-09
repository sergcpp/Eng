#include "test_common.h"

#include "../String.h"

void test_string() {
    using namespace Ren;

    printf("Test string             | ");

    { // Basic usage
        String s1, s2;

        s1 = String{"abcd"};
        s2 = String{"12345"};

        require(s1.length() == 4);
        require(s2.length() == 5);

        require(!(s1 == s2));
        require(s1 != s2);

        require(strcmp(s1.c_str(), "abcd") == 0);
        require(strcmp(s2.c_str(), "12345") == 0);

        String s3 = s1, s4 = s2;

        require(s3.c_str() == s1.c_str());
        require(s4.c_str() == s2.c_str());

        s1 = String{"another"};
        s2 = String{"another"};

        require(strcmp(s3.c_str(), "abcd") == 0);
        require(strcmp(s4.c_str(), "12345") == 0);
    }

    { // Move
        String s1 = String{"test_string"}, s2;

        require(s1.length() == 11);
        require(s2.length() == 0);

        const char *p_str = s1.c_str();
        s2 = std::move(s1);

        require(s2.length() == 11);
        require(s1.length() == 0);

        s1 = {};

        require(s2.c_str() == p_str);
        require(strcmp(s2.c_str(), "test_string") == 0);
    }

    { // Extra things
        auto s1 = String{"my_image.png"};

        require(s1.EndsWith(".png"));
        require(!s1.EndsWith(".ang"));
    }

    { // Part of string
        auto s1 = String{"test_string"}, s2 = String{"another_test_string_bla_bla"};

        std::string_view s3 = {&s2[8], 11}, s4 = {&s2[8], 12};

        require(s1 == s3);
        require(s1 != s4);
        require(s3 == s1);
        require(s4 != s1);

        auto s5 = String{s3}, s6 = String{s4};

        require(s1 == s5);
        require(s1 != s6);
        require(s5 == s1);
        require(s6 != s1);
    }

    printf("OK\n");
}