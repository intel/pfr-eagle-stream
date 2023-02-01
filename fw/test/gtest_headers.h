#ifndef INC_GTEST_HEADERS_H
#define INC_GTEST_HEADERS_H

// Because of the conflict on the io.h name, include it explicitly here before googletest
#ifdef _WIN32
#include "C:\Program Files (x86)\Windows Kits\10\Include\10.0.10240.0\ucrt\io.h"
#endif

#include "gtest/gtest.h"

#include <future>
#include <chrono>
#define ASSERT_DURATION_LE(secs, stmt)                                                       \
    {                                                                                        \
        std::promise<bool> completed;                                                        \
        auto stmt_future = completed.get_future();                                           \
        std::thread(                                                                         \
            [&](std::promise<bool>& completed) {                                             \
                stmt;                                                                        \
                completed.set_value(true);                                                   \
            },                                                                               \
            std::ref(completed))                                                             \
            .detach();                                                                       \
        if (stmt_future.wait_for(std::chrono::seconds(secs)) == std::future_status::timeout) \
		{ \
        	std::cout << "Internal Error: Test exceeded timeout of " << #secs << " seconds. Check code for infinite loops. Unit test exe will now crash, File: " << __FILE__ << ", Line: " << __LINE__ << std::endl; \
			std::abort(); \
		} \
    }


#endif  // INC_GTEST_HEADERS_H
