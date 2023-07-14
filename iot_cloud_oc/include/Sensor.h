#ifndef __E53_IA1_H__
#define __E53_IA1_H__
#include "iot_cloud.h"
typedef enum
{
    CLEAN = 1,
    OFF
} Clean;

void Begin_Clean(Clean status);

#endif
