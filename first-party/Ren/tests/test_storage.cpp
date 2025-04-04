#include "test_common.h"

#include "../Storage.h"

void test_storage() {
    using namespace Ren;

    printf("Test storage            | ");

    class MyObj : public RefCounter {
        String name_;

      public:
        int *ref;

        MyObj() : ref(nullptr) {}
        MyObj(std::string_view name, int *r) : name_(name), ref(r) { (*ref)++; }
        MyObj(const MyObj &rhs) = delete;
        MyObj(MyObj &&rhs) noexcept {
            name_ = std::move(rhs.name_);
            ref = rhs.ref;
            rhs.ref = nullptr;
        }
        MyObj &operator=(const MyObj &rhs) = delete;
        MyObj &operator=(MyObj &&rhs) noexcept {
            if (ref) {
                (*ref)--;
            }
            name_ = std::move(rhs.name_);
            ref = rhs.ref;
            rhs.ref = nullptr;
            return (*this);
        }

        const String &name() { return name_; }

        ~MyObj() {
            if (ref) {
                (*ref)--;
            }
        }
    };

    { // test create/delete
        NamedStorage<MyObj> my_obj_storage;
        int counter = 0;

        auto ref1 = my_obj_storage.Insert("obj", &counter);
        require(counter == 1);
        ref1 = {};
        require(counter == 0);
    }

    { // test copy/move reference
        NamedStorage<MyObj> my_obj_storage;
        int counter = 0;

        auto ref1 = my_obj_storage.Insert("obj1", &counter);
        require(counter == 1);
        auto ref2 = ref1;
        require(counter == 1);
        ref1 = {};
        require(counter == 1);
        ref2 = {};
        require(counter == 0);

        ref1 = my_obj_storage.Insert("obj2", &counter);
        require(counter == 1);
        ref2 = std::move(ref1);
        require(counter == 1);
        ref2 = {};
        require(counter == 0);
    }

    class MyObj2 : public RefCounter {
      public:
        int val;

        explicit MyObj2(int _val) : val(_val) {}

        bool operator==(const MyObj2 &rhs) const { return val == rhs.val; }
        bool operator==(const int rhs_val) const { return val == rhs_val; }
        bool operator<(const MyObj2 &rhs) const { return val < rhs.val; }
        bool operator<(const int rhs_val) const { return val < rhs_val; }
    };

    { // sorted storage
        SortedStorage<MyObj2> test_storage;

        auto ref1 = test_storage.Insert(0);
        auto ref2 = test_storage.Insert(42);
        auto ref3 = test_storage.Insert(11);

        auto ref4 = test_storage.LowerBound([](const MyObj2 &item) { return item.val < 11; });
        require(bool(ref4));
        require(*ref4 == 11);

        auto ref5 = test_storage.LowerBound([](const MyObj2 &item) { return item.val < 41; });
        require(bool(ref5));
        require(*ref5 == 42);

        auto ref6 = test_storage.LowerBound([](const MyObj2 &item) { return item.val < 43; });
        require(!bool(ref6));

        require(test_storage.sorted_items()[0] == 0);
        require(test_storage.sorted_items()[1] == 2);
        require(test_storage.sorted_items()[2] == 1);
    }

    printf("OK\n");
}