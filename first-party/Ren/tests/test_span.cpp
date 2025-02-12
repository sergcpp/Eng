#include "test_common.h"

#include "../Span.h"

void test_span() {
    using namespace Ren;

    printf("Test span               | ");

    { // basic usage
        const int data[] = {1, 2, 3, 4, 5};

        Span<const int> test0;
        require(test0.empty());
        require(test0.size() == 0);

        Span<const int> test1 = {data, 3};
        require(!test1.empty());
        require(test1.size() == 3);
        require(test1[0] == data[0]);
        require(test1[1] == data[1]);
        require(test1[2] == data[2]);

        Span<const int> test2 = {data, data + 3};
        require(!test2.empty());
        require(test2.size() == 3);
        require(test2[0] == data[0]);
        require(test2[1] == data[1]);
        require(test2[2] == data[2]);

        Span<const int> test3 = {data};
        require(!test3.empty());
        require(test3.size() == 5);
        require(test3[0] == data[0]);
        require(test3[1] == data[1]);
        require(test3[2] == data[2]);
        require(test3[3] == data[3]);
        require(test3[4] == data[4]);

        Span<const int> test4 = test3;
        require(!test4.empty());
        require(test4.size() == 5);
        require(test4[0] == data[0]);
        require(test4[1] == data[1]);
        require(test4[2] == data[2]);
        require(test4[3] == data[3]);
        require(test4[4] == data[4]);

        Span<const int> test5;
        test5 = test4;
        require(!test5.empty());
        require(test5.size() == 5);
        require(test5[0] == data[0]);
        require(test5[1] == data[1]);
        require(test5[2] == data[2]);
        require(test5[3] == data[3]);
        require(test5[4] == data[4]);

        const int arr_values[] = {1, 2, 3, 4, 5};
        Span<const int> test6(arr_values);
        require(!test6.empty());
        require(test6.size() == 5);
        require(test6[0] == 1);
        require(test6[1] == 2);
        require(test6[2] == 3);
        require(test6[3] == 4);
        require(test6[4] == 5);

        int data2[] = {1, 2, 3};

        Span<int> test7(data2);
        Span<const int> test8 = test7;
        require(test8[0] == 1);
        require(test8[1] == 2);
        require(test8[2] == 3);
    }

    { // loop
        const int arr_values[] = {1, 2, 3, 4, 5};
        Span<const int> test0(arr_values);
        int sum = 0;
        for (const int i : test0) {
            sum += i;
        }
        require(sum == 15);
    }

    { // comparison operators
        const int v1[] = {1, 2, 3, 4, 5};
        const int v2[] = {1, 2, 3, 4, 6};
        const int v3[] = {1, 2, 3, 4, 5};
        const int v4[] = {1, 2, 3, 4, 6};

        Span<const int> s1 = v1;
        Span<const int> s2 = v2;
        Span<const int> s3 = v3;
        Span<const int> s4 = v4;

        require(s1 < s2);
        require(s1 <= s3);
        require(s2 > s1);
        require(s2 >= s4);
        require(s1 == s3);
        require(s2 == s4);
        require(s1 != s2);
        require(s3 != s4);
    }

    printf("OK\n");
}
