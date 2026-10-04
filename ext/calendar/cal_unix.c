/*
   +----------------------------------------------------------------------+
   | Copyright © The PHP Group and Contributors.                          |
   +----------------------------------------------------------------------+
   | This source file is subject to the Modified BSD License that is      |
   | bundled with this package in the file LICENSE, and is available      |
   | through the World Wide Web at <https://www.php.net/license/>.        |
   |                                                                      |
   | SPDX-License-Identifier: BSD-3-Clause                                |
   +----------------------------------------------------------------------+
   | Authors: Shane Caraveo             <shane@caraveo.com>               |
   |          Colin Viebrock            <colin@easydns.com>               |
   |          Hartmut Holzgraefe        <hholzgra@php.net>                |
   +----------------------------------------------------------------------+
 */

#include "php.h"
#include "sdncal.h"
#include <time.h>

#define SECS_PER_DAY (24 * 3600)
#define JULIAN_DAY_UNIX_EPOCH 2440588

/* Convert UNIX timestamp to Julian Day */
PHP_FUNCTION(unixtojd)
{
	time_t ts;
	zend_long tl = 0;
	bool tl_is_null = true;
	struct tm tmbuf;

	ZEND_PARSE_PARAMETERS_START(0, 1)
		Z_PARAM_OPTIONAL
		Z_PARAM_LONG_OR_NULL(tl, tl_is_null)
	ZEND_PARSE_PARAMETERS_END();

	if (!tl_is_null && tl < 0) {
		zend_argument_value_error(1, "must be greater than or equal to 0");
		RETURN_THROWS();
	}

	ts = tl_is_null ? time(NULL) : (time_t) tl;
	if (!php_localtime_r(&ts, &tmbuf)) {
		RETURN_FALSE;
	}

	RETURN_LONG(GregorianToSdn(tmbuf.tm_year + 1900, tmbuf.tm_mon + 1, tmbuf.tm_mday));
}

/* Convert Julian Day to UNIX timestamp */
PHP_FUNCTION(jdtounix)
{
	zend_long julian_day;
	const zend_long max_julian_day = ZEND_LONG_MAX / SECS_PER_DAY + JULIAN_DAY_UNIX_EPOCH;

	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_LONG(julian_day)
	ZEND_PARSE_PARAMETERS_END();

	if (julian_day < JULIAN_DAY_UNIX_EPOCH || julian_day > max_julian_day) {
		zend_argument_value_error(1, "jday must be between %d and " ZEND_LONG_FMT, JULIAN_DAY_UNIX_EPOCH, max_julian_day);
		RETURN_THROWS();
	}

	RETURN_LONG((julian_day - JULIAN_DAY_UNIX_EPOCH) * SECS_PER_DAY);
}
