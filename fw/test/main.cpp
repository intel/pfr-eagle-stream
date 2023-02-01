#include <iostream>
using namespace std;

#include "gtest_headers.h"

GTEST_API_ int main(int argc, char** argv)
{
    cout << "Starting Unit test EXE " << argv[0] << endl;
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
