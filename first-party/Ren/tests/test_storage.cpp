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

void test_storage_new() {
    using namespace Ren;

    printf("Test storage_new        | ");

    { // Simple storage
        struct DataMain {
            int light;
        };

        struct DataCold {
            int heavy[8];
        };

        DualStorage<DataMain, DataCold> new_storage;

        const Handle<DataMain> handle0 = new_storage.Emplace();
        const Handle<DataMain> handle1 = new_storage.Emplace();
        const Handle<DataMain> handle2 = new_storage.Emplace();
        const Handle<DataMain> handle3 = new_storage.Emplace();

        require(handle0.index == 0 && handle0.generation == 0);
        require(handle1.index == 1 && handle1.generation == 0);
        require(handle2.index == 2 && handle2.generation == 0);
        require(handle3.index == 3 && handle3.generation == 0);

        const std::pair<DataMain &, DataCold &> data0 = new_storage.Get(handle0);
        const std::pair<DataMain &, DataCold &> data1 = new_storage.Get(handle1);
        const std::pair<DataMain &, DataCold &> data2 = new_storage.Get(handle2);
        const std::pair<DataMain &, DataCold &> data3 = new_storage.Get(handle3);

        data0.first.light = 0;
        data1.first.light = 1;
        data2.first.light = 2;
        data3.first.light = 3;

        new_storage.Free(handle0);
        new_storage.Free(handle1);
        new_storage.Free(handle2);
        new_storage.Free(handle3);

        const Handle<DataMain> handle4 = new_storage.Emplace();
        const Handle<DataMain> handle5 = new_storage.Emplace();
        const Handle<DataMain> handle6 = new_storage.Emplace();
        const Handle<DataMain> handle7 = new_storage.Emplace();

        require(handle4.index == 3 && handle4.generation == 1);
        require(handle5.index == 2 && handle5.generation == 1);
        require(handle6.index == 1 && handle6.generation == 1);
        require(handle7.index == 0 && handle7.generation == 1);

        new_storage.Free(handle4);
        new_storage.Free(handle5);
        new_storage.Free(handle6);
        new_storage.Free(handle7);
    }

    printf("OK\n");
}