/* qv : Handle Operations
 * Copyright(c) 2016 y2c2 */

#include "qv_handleop.h"

/* Attach data on handle */
void qv_handle_set_data(qv_handle_t *handle, void *data)
{
    handle->data = data;
}

/* Set destructor of the data */
void qv_handle_set_data_dtor(struct qv_handle *handle, qv_handle_data_dtor_t data_dtor)
{
    handle->data_dtor = data_dtor;
}

/* Get the attached data from handle */
void *qv_handle_get_data(qv_handle_t *handle)
{
    return handle->data;
}

