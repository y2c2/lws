/* Chrono */

#ifndef CHRONO_H
#define CHRONO_H

#include "qv_types.h"

/* Millisecond */
typedef qv_u64 chrono_time_point_t;

int chrono_now(chrono_time_point_t *time_point);

#endif

