#pragma once
#ifndef COMPAT_ERROR_H
#define COMPAT_ERROR_H

#include "compat/compat.h"
#include "clarinet/clarinet.h"

/**
 * Get the system's socket API error.
 * This function can be called multiple times without affecting the returned error code.
 * */
int
clarinet_get_sockapi_error(void);

/** Set the system's socket API error */
void
clarinet_set_sockapi_error(int err);

/** Converts a system's socket API error to a clarinet error */
int
clarinet_error_from_sockapi_error(int err);

#endif /* COMPAT_ERROR_H */
