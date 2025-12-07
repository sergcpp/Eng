#include "test_common.h"

#include "../utils/SparseStorage.h"

void test_sparse_storage() {
    using namespace Ren;

    printf("Test sparse_storage     | ");

    { // Simple storage
        struct DataMain {
            int light;

            DataMain(int _light) : light(_light) {}
        };

        SparseStorage<DataMain> new_storage;
        require(new_storage.empty());

        const Handle<DataMain, RWTag> handle0 = new_storage.Emplace(0);
        const Handle<DataMain, RWTag> handle1 = new_storage.Emplace(1);
        const Handle<DataMain, RWTag> handle2 = new_storage.Emplace(2);
        const Handle<DataMain, RWTag> handle3 = new_storage.Emplace(3);

        require(new_storage.size() == 4);

        require(handle0.index == 0 && handle0.generation == 0);
        require(handle1.index == 1 && handle1.generation == 0);
        require(handle2.index == 2 && handle2.generation == 0);
        require(handle3.index == 3 && handle3.generation == 0);

        const Handle<void> opaque_handle{handle0};

        [[maybe_unused]] const DataMain &data0 = new_storage[handle0];
        [[maybe_unused]] const DataMain &data1 = new_storage[handle1];
        [[maybe_unused]] const DataMain &data2 = new_storage[handle2];
        [[maybe_unused]] const DataMain &data3 = new_storage[handle3];

        new_storage.Erase(handle0);
        new_storage.Erase(handle1);
        new_storage.Erase(handle2);
        new_storage.Erase(handle3);

        require(new_storage.empty());

        const Handle<DataMain, RWTag> handle4 = new_storage.Emplace(4);
        const Handle<DataMain, RWTag> handle5 = new_storage.Emplace(5);
        const Handle<DataMain, RWTag> handle6 = new_storage.Emplace(6);
        const Handle<DataMain, RWTag> handle7 = new_storage.Emplace(7);

        require(new_storage.size() == 4);

        require(handle4.index == 3 && handle4.generation == 1);
        require(handle5.index == 2 && handle5.generation == 1);
        require(handle6.index == 1 && handle6.generation == 1);
        require(handle7.index == 0 && handle7.generation == 1);

        new_storage.Erase(handle4);
        new_storage.Erase(handle5);
        new_storage.Erase(handle6);
        new_storage.Erase(handle7);

        require(new_storage.empty());
    }

    { // Dual storage
        struct DataMain {
            int light;
        };

        struct DataCold {
            int heavy[8];
        };

        SparseDualStorage<DataMain, DataCold> new_storage;
        require(new_storage.empty());

        const Handle<DataMain> handle0 = new_storage.Emplace();
        const Handle<DataMain> handle1 = new_storage.Emplace();
        const Handle<DataMain> handle2 = new_storage.Emplace();
        const Handle<DataMain> handle3 = new_storage.Emplace();

        require(new_storage.size() == 4);

        require(handle0.index == 0 && handle0.generation == 0);
        require(handle1.index == 1 && handle1.generation == 0);
        require(handle2.index == 2 && handle2.generation == 0);
        require(handle3.index == 3 && handle3.generation == 0);

        const std::pair<DataMain &, DataCold &> data0 = new_storage[handle0];
        const std::pair<DataMain &, DataCold &> data1 = new_storage[handle1];
        const std::pair<DataMain &, DataCold &> data2 = new_storage[handle2];
        const std::pair<DataMain &, DataCold &> data3 = new_storage[handle3];

        data0.first.light = 0;
        data1.first.light = 1;
        data2.first.light = 2;
        data3.first.light = 3;

        new_storage.Erase(handle0);
        new_storage.Erase(handle1);
        new_storage.Erase(handle2);
        new_storage.Erase(handle3);

        require(new_storage.empty());

        const Handle<DataMain> handle4 = new_storage.Emplace();
        const Handle<DataMain> handle5 = new_storage.Emplace();
        const Handle<DataMain> handle6 = new_storage.Emplace();
        const Handle<DataMain> handle7 = new_storage.Emplace();

        require(new_storage.size() == 4);

        require(handle4.index == 3 && handle4.generation == 1);
        require(handle5.index == 2 && handle5.generation == 1);
        require(handle6.index == 1 && handle6.generation == 1);
        require(handle7.index == 0 && handle7.generation == 1);

        new_storage.Erase(handle4);
        new_storage.Erase(handle5);
        new_storage.Erase(handle6);
        new_storage.Erase(handle7);

        require(new_storage.empty());
    }

    printf("OK\n");
}