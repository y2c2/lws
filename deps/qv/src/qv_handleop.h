/* qv : Handle Operations
 * Copyright(c) 2016 y2c2 */

#ifndef QV_HANDLEOP_H
#define QV_HANDLEOP_H

#include "qv_handle.h"

/* Attach data on handle */
void qv_handle_set_data(struct qv_handle *handle, void *data);

/* Set destructor of the data */
void qv_handle_set_data_dtor(struct qv_handle *handle, qv_handle_data_dtor_t data_dtor);

/* Get the attached data from handle */
void *qv_handle_get_data(struct qv_handle *handle);


#endif

