/*
 * alt_types_mock.h
 *
 *  Created on: Mar 3, 2019
 *      Author: kbrunham
 */

#ifndef INC_ALT_TYPES_MOCK_H
#define INC_ALT_TYPES_MOCK_H

#include <iostream>

// Include alt_types so it doesn't get reincluded. Define types for running on X86 below
#ifndef ALT_ASM_SRC
#define ALT_ASM_SRC
#endif
#include "alt_types.h"

// Equivalent of alt_types for use in testing

// Mock types. These match x86 types
typedef unsigned char  alt_u8;
typedef unsigned short alt_u16;
typedef unsigned int alt_u32;
typedef unsigned long long alt_u64;

// Mock the PFR_ASSERT to throw an exception. This can then be caught with EXPECT_ANY_THROW
#ifdef NO_SYSTEM_MOCK
#define PFR_ASSERT(condition) if (!condition) {throw -1;}
#define PFR_INTERNAL_ERROR(msg) std::cerr << "Internal Error: " << msg << " File: " << __FILE__ << "\tLine: " << __LINE__ << std::endl; throw -1
#endif


#endif /* ALT_TYPES_MOCK_H_ */
