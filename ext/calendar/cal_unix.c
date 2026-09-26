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
#include "php_calendar.h"
#include "sdncal.h"
#include <time.h>

#define SECS_PER_DAY (24 * 3600)

/* {{{ Convert UNIX timestamp to Julian Day */
PHP_FUNCTION(unixtojd)
{
	time_t ts;
	zend_long tl = 0;
	bool tl_is_null = true;
	struct tm *ta, tmbuf;

	ZEND_PARSE_PARAMETERS_START(0, 1)
		Z_PARAM_OPTIONAL
		Z_PARAM_LONG_OR_NULL(tl, tl_is_null)
	ZEND_PARSE_PARAMETERS_END();

	if (tl_is_null) {
		ts = time(NULL);
	} else if (tl >= 0) {
		ts = (time_t) tl;
	} else {
		zend_argument_value_error(1, "must be greater than or equal to 0");
		RETURN_THROWS();
	}

	if (!(ta = php_localtime_r(&ts, &tmbuf))) {
		RETURN_FALSE;
	}

	RETURN_LONG(GregorianToSdn(ta->tm_year+1900, ta->tm_mon+1, ta->tm_mday));
}
/* }}} */

/* {{{ Convert Julian Day to UNIX timestamp */
PHP_FUNCTION(jdtounix)
{
	zend_long uday;

	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_LONG(uday)
	ZEND_PARSE_PARAMETERS_END();

	if (uday < 2440588 || uday > ZEND_LONG_MAX / SECS_PER_DAY + 2440588) {
		zend_argument_value_error(1, "jday must be between 2440588 and " ZEND_LONG_FMT, ZEND_LONG_MAX / SECS_PER_DAY + 2440588);
		RETURN_THROWS();
	}

	uday -= 2440588 /* J.D. of 1.1.1970 */;

	RETURN_LONG(uday * SECS_PER_DAY);
}
/* }}} */
