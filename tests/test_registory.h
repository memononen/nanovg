#pragma once

#include <stdbool.h>
typedef void (*draw_test_function_type)(NVGcontext *, int, int, float);

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
    {"demo", test_demo},
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
