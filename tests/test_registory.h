#pragma once

#include <stdbool.h>
typedef void (*draw_test_function_type)(NVGcontext *, int, int);

typedef struct
{
    const char *name;
    draw_test_function_type function;
} draw_test_info;

#include "testcase/demo.h"
#include "testcase/fillRect.h"
#include "testcase/compositePaths.h"
#include "testcase/compositeOperation.h"

draw_test_info DRAW_TEST_CASES[] = {
    {"demo01", test_demo01},
    {"demo02", test_demo02},
    {"demo03", test_demo03},
    {"demo04", test_demo04},
    {"demo05", test_demo05},
    {"demo06", test_demo06},
    {"demo07", test_demo07},
    {"demo08", test_demo08},
    {"demo09", test_demo09},
    {"demo10", test_demo10},
    {"demo11", test_demo11},
    {"demo12", test_demo12},
    {"fillRect1", test_fillRect1},
    {"compositePaths1", test_compositePaths1},
    {"compositeOperationSourceOver", test_compositeOperationSourceOver},
    {"compositeOperationSourceIn", test_compositeOperationSourceIn},
    {"compositeOperationSourceOut", test_compositeOperationSourceOut},
    {"compositeOperationSourceAtop", test_compositeOperationSourceAtop},
    {"compositeOperationDestinationOver", test_compositeOperationDestinationOver},
    {"compositeOperationDestinationIn", test_compositeOperationDestinationIn},
    {"compositeOperationDestinationOut", test_compositeOperationDestinationOut},
    {"compositeOperationDestinationAtop", test_compositeOperationDestinationAtop},
    {"compositeOperationLighter", test_compositeOperationLighter},
    {"compositeOperationCopy", test_compositeOperationCopy},
    {"compositeOperationXOR", test_compositeOperationXOR},
};
