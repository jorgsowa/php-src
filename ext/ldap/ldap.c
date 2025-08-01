/*
   +----------------------------------------------------------------------+
   | Copyright (c) The PHP Group                                          |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | https://www.php.net/license/3_01.txt                                 |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Amitay Isaacs  <amitay@w-o-i.com>                           |
   |          Eric Warnke    <ericw@albany.edu>                           |
   |          Rasmus Lerdorf <rasmus@php.net>                             |
   |          Gerrit Thomson <334647@swin.edu.au>                         |
   |          Jani Taskinen  <sniper@iki.fi>                              |
   |          Stig Venaas    <venaas@uninett.no>                          |
   |          Doug Goldstein <cardoe@cardoe.com>                          |
   |          Côme Chilliet  <mcmic@php.net>                              |
   | PHP 4.0 updates:  Zeev Suraski <zeev@php.net>                        |
   +----------------------------------------------------------------------+
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "php.h"
#include "php_ini.h"
#include "Zend/zend_attributes.h"

#include <stddef.h>

#include "ext/standard/dl.h"
#include "php_ldap.h"

#ifdef PHP_WIN32
#include <string.h>
#include "config.w32.h"
#undef strcasecmp
#undef strncasecmp
#define WINSOCK 1
#define __STDC__ 1
#endif

#include "ext/standard/info.h"

#ifdef HAVE_LDAP_SASL
#include <sasl/sasl.h>
#endif

#define PHP_LDAP_ESCAPE_FILTER 0x01
#define PHP_LDAP_ESCAPE_DN     0x02

#include "ldap_arginfo.h"

#if defined(LDAP_CONTROL_PAGEDRESULTS) && !defined(HAVE_LDAP_CONTROL_FIND)
LDAPControl *ldap_control_find( const char *oid, LDAPControl **ctrls, LDAPControl ***nextctrlp)
{
	assert(nextctrlp == NULL);
	return ldap_find_control(oid, ctrls);
}
#endif

#if !defined(LDAP_API_FEATURE_X_OPENLDAP)
void ldap_memvfree(void **v)
{
	ldap_value_free((char **)v);
}
#endif

typedef struct {
	LDAP *link;
#if defined(LDAP_API_FEATURE_X_OPENLDAP) && defined(HAVE_3ARG_SETREBINDPROC)
	zend_fcall_info_cache rebind_proc_fcc;
#endif
	zend_object std;
} ldap_linkdata;

typedef struct {
	LDAPMessage *result;
	zend_object std;
} ldap_resultdata;

typedef struct {
	LDAPMessage *data;
	BerElement  *ber;
	zval         res;
	zend_object std;
} ldap_result_entry;

ZEND_DECLARE_MODULE_GLOBALS(ldap)
static PHP_GINIT_FUNCTION(ldap);

static zend_class_entry *ldap_link_ce, *ldap_result_ce, *ldap_result_entry_ce;
static zend_object_handlers ldap_link_object_handlers, ldap_result_object_handlers, ldap_result_entry_object_handlers;

#ifdef COMPILE_DL_LDAP
#ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
#endif
ZEND_GET_MODULE(ldap)
#endif

static inline ldap_linkdata *ldap_link_from_obj(zend_object *obj) {
	return (ldap_linkdata *)((char *)(obj) - XtOffsetOf(ldap_linkdata, std));
}

#define Z_LDAP_LINK_P(zv) ldap_link_from_obj(Z_OBJ_P(zv))

static zend_object *ldap_link_create_object(zend_class_entry *class_type) {
	ldap_linkdata *intern = zend_object_alloc(sizeof(ldap_linkdata), class_type);

	zend_object_std_init(&intern->std, class_type);
	object_properties_init(&intern->std, class_type);

	return &intern->std;
}

static zend_function *ldap_link_get_constructor(zend_object *object) {
	zend_throw_error(NULL, "Cannot directly construct LDAP\\Connection, use ldap_connect() instead");
	return NULL;
}

static void ldap_link_free(ldap_linkdata *ld)
{
	/* We use ldap_destroy rather than ldap_unbind here, because ldap_unbind
	 * will skip the destructor entirely if a critical client control is set. */
	ldap_destroy(ld->link);
	ld->link = NULL;

#if defined(LDAP_API_FEATURE_X_OPENLDAP) && defined(HAVE_3ARG_SETREBINDPROC)
	if (ZEND_FCC_INITIALIZED(ld->rebind_proc_fcc)) {
		zend_fcc_dtor(&ld->rebind_proc_fcc);
	}
#endif

	LDAPG(num_links)--;
}

static void ldap_link_free_obj(zend_object *obj)
{
	ldap_linkdata *ld = ldap_link_from_obj(obj);

	if (ld->link) {
		ldap_link_free(ld);
	}

	zend_object_std_dtor(&ld->std);
}

static inline ldap_resultdata *ldap_result_from_obj(zend_object *obj) {
	return (ldap_resultdata *)((char *)(obj) - XtOffsetOf(ldap_resultdata, std));
}

#define Z_LDAP_RESULT_P(zv) ldap_result_from_obj(Z_OBJ_P(zv))

static zend_object *ldap_result_create_object(zend_class_entry *class_type) {
	ldap_resultdata *intern = zend_object_alloc(sizeof(ldap_resultdata), class_type);

	zend_object_std_init(&intern->std, class_type);
	object_properties_init(&intern->std, class_type);

	return &intern->std;
}

static zend_function *ldap_result_get_constructor(zend_object *object) {
	zend_throw_error(NULL, "Cannot directly construct LDAP\\Result, use the dedicated functions instead");
	return NULL;
}

static void ldap_result_free(ldap_resultdata *result)
{
	ldap_msgfree(result->result);
	result->result = NULL;
}

static void ldap_result_free_obj(zend_object *obj)
{
	ldap_resultdata *result = ldap_result_from_obj(obj);

	if (result->result) {
		ldap_result_free(result);
	}

	zend_object_std_dtor(&result->std);
}

static inline ldap_result_entry *ldap_result_entry_from_obj(zend_object *obj) {
	return (ldap_result_entry *)((char *)(obj) - XtOffsetOf(ldap_result_entry, std));
}

#define Z_LDAP_RESULT_ENTRY_P(zv) ldap_result_entry_from_obj(Z_OBJ_P(zv))

static zend_object *ldap_result_entry_create_object(zend_class_entry *class_type) {
	ldap_result_entry *intern = zend_object_alloc(sizeof(ldap_result_entry), class_type);

	zend_object_std_init(&intern->std, class_type);
	object_properties_init(&intern->std, class_type);

	return &intern->std;
}

static zend_function *ldap_result_entry_get_constructor(zend_object *obj) {
	zend_throw_error(NULL, "Cannot directly construct LDAP\\ResultEntry, use the dedicated functions instead");
	return NULL;
}

static void ldap_result_entry_free_obj(zend_object *obj)
{
	ldap_result_entry *entry = ldap_result_entry_from_obj(obj);

	if (entry->ber != NULL) {
		ber_free(entry->ber, 0);
		entry->ber = NULL;
	}
	zval_ptr_dtor(&entry->res);

	zend_object_std_dtor(&entry->std);
}

#define VERIFY_LDAP_LINK_CONNECTED(ld) \
{ \
	if (!ld->link) { \
		zend_throw_error(NULL, "LDAP connection has already been closed"); \
		RETURN_THROWS(); \
	} \
}

#define VERIFY_LDAP_RESULT_OPEN(lr) \
{ \
	if (!lr->result) { \
		zend_throw_error(NULL, "LDAP result has already been closed"); \
		RETURN_THROWS(); \
	} \
}

static bool php_ldap_is_numerically_indexed_array(const zend_array *arr)
{
	if (zend_hash_num_elements(arr) == 0 || HT_IS_PACKED(arr)) {
		return true;
	}

	zend_string *str_key;
	ZEND_HASH_MAP_FOREACH_STR_KEY(arr, str_key) {
		if (str_key) {
			return false;
		}
	} ZEND_HASH_FOREACH_END();

	return true;
}

/* An LDAP value must be a string, however it defines a format for integer and
 * booleans, thus we parse zvals to the corresponding string if possible
 * See RFC 4517: https://datatracker.ietf.org/doc/html/rfc4517 */
static zend_string* php_ldap_try_get_ldap_value_from_zval(zval *zv) {
	switch (Z_TYPE_P(zv)) {
		case IS_STRING:
		case IS_LONG:
		/* Object might be stringable */
		case IS_OBJECT:
			return zval_try_get_string(zv);
		case IS_TRUE:
			return ZSTR_INIT_LITERAL("TRUE", false);
		case IS_FALSE:
			return ZSTR_INIT_LITERAL("FALSE", false);
		default:
			zend_type_error("LDAP value must be of type string|int|bool, %s given", zend_zval_value_name(zv));
			return NULL;
	}
}

/* The char pointer MUST refer to the char* of a zend_string struct */
static void php_ldap_zend_string_release_from_char_pointer(char *ptr) {
	zend_string_release((zend_string*) (ptr - XtOffsetOf(zend_string, val)));
}

/* {{{ Parse controls from and to arrays */
static void _php_ldap_control_to_array(LDAP *ld, LDAPControl* ctrl, zval* array, int request)
{
	array_init(array);

	add_assoc_string(array, "oid", ctrl->ldctl_oid);
	if (request) {
		/* iscritical field only makes sense in request controls (which may be obtained by ldap_get_option) */
		add_assoc_bool(array, "iscritical", (ctrl->ldctl_iscritical != 0));
	}

	/* If it is a known oid, parse to values */
	if (strcmp(ctrl->ldctl_oid, LDAP_CONTROL_PASSWORDPOLICYRESPONSE) == 0) {
		int expire = 0, grace = 0, rc;
		LDAPPasswordPolicyError pperr;
		zval value;

		rc = ldap_parse_passwordpolicy_control(ld, ctrl, &expire, &grace, &pperr);
		if ( rc == LDAP_SUCCESS ) {
			array_init(&value);
			add_assoc_long(&value, "expire", expire);
			add_assoc_long(&value, "grace", grace);

			if ( pperr != PP_noError ) {
				add_assoc_long(&value, "error", pperr);
			}
			add_assoc_zval(array, "value", &value);
		} else {
			add_assoc_null(array, "value");
		}
	} else if (strcmp(ctrl->ldctl_oid, LDAP_CONTROL_PAGEDRESULTS) == 0) {
		int lestimated, rc;
		struct berval lcookie = { 0L, NULL };
		zval value;

		if (ctrl->ldctl_value.bv_len) {
			/* ldap_parse_pageresponse_control() allocates lcookie.bv_val */
			rc = ldap_parse_pageresponse_control(ld, ctrl, &lestimated, &lcookie);
		} else {
			/* ldap_parse_pageresponse_control will crash if value is empty */
			rc = -1;
		}

		if ( rc == LDAP_SUCCESS ) {
			array_init(&value);
			add_assoc_long(&value, "size", lestimated);
			add_assoc_stringl(&value, "cookie", lcookie.bv_val, lcookie.bv_len);
			add_assoc_zval(array, "value", &value);
		} else {
			add_assoc_null(array, "value");
		}

		if (lcookie.bv_val) {
			ldap_memfree(lcookie.bv_val);
		}
	} else if ((strcmp(ctrl->ldctl_oid, LDAP_CONTROL_PRE_READ) == 0) || (strcmp(ctrl->ldctl_oid, LDAP_CONTROL_POST_READ) == 0)) {
		BerElement *ber;
		struct berval bv;

		ber = ber_init(&ctrl->ldctl_value);
		if (ber == NULL) {
			add_assoc_null(array, "value");
		} else if (ber_scanf(ber, "{m{" /*}}*/, &bv) == LBER_ERROR) {
			add_assoc_null(array, "value");
		} else {
			zval value;

			array_init(&value);
			add_assoc_stringl(&value, "dn", bv.bv_val, bv.bv_len);

			while (ber_scanf(ber, "{m" /*}*/, &bv) != LBER_ERROR) {
				int	 i;
				BerVarray vals = NULL;
				zval tmp;

				if (ber_scanf(ber, "[W]", &vals) == LBER_ERROR || vals == NULL)
				{
					break;
				}

				array_init(&tmp);
				for (i = 0; vals[i].bv_val != NULL; i++) {
					add_next_index_stringl(&tmp, vals[i].bv_val, vals[i].bv_len);
				}
				add_assoc_zval(&value, bv.bv_val, &tmp);

				ber_bvarray_free(vals);
			}
			add_assoc_zval(array, "value", &value);
		}

		if (ber != NULL) {
			ber_free(ber, 1);
		}
	} else if (strcmp(ctrl->ldctl_oid, LDAP_CONTROL_SORTRESPONSE) == 0) {
		zval value;
		int errcode, rc;
		char* attribute;

		if (ctrl->ldctl_value.bv_len) {
			rc = ldap_parse_sortresponse_control(ld, ctrl, &errcode, &attribute);
		} else {
			rc = -1;
		}
		if ( rc == LDAP_SUCCESS ) {
			array_init(&value);
			add_assoc_long(&value, "errcode", errcode);
			if (attribute) {
				add_assoc_string(&value, "attribute", attribute);
				ldap_memfree(attribute);
			}
			add_assoc_zval(array, "value", &value);
		} else {
			add_assoc_null(array, "value");
		}
	} else if (strcmp(ctrl->ldctl_oid, LDAP_CONTROL_VLVRESPONSE) == 0) {
		int target, count, errcode, rc;
		struct berval *context;
		zval value;

		if (ctrl->ldctl_value.bv_len) {
			rc = ldap_parse_vlvresponse_control(ld, ctrl, &target, &count, &context, &errcode);
		} else {
			rc = -1;
		}
		if ( rc == LDAP_SUCCESS ) {
			array_init(&value);
			add_assoc_long(&value, "target", target);
			add_assoc_long(&value, "count", count);
			add_assoc_long(&value, "errcode", errcode);
			if (context) {
				add_assoc_stringl(&value, "context", context->bv_val, context->bv_len);
			}
			add_assoc_zval(array, "value", &value);
			ber_bvfree(context);
		} else {
			add_assoc_null(array, "value");
		}
	} else {
		if (ctrl->ldctl_value.bv_len) {
			add_assoc_stringl(array, "value", ctrl->ldctl_value.bv_val, ctrl->ldctl_value.bv_len);
		} else {
			add_assoc_null(array, "value");
		}
	}
}

static int php_ldap_control_from_array(LDAP *ld, LDAPControl** ctrl, const HashTable *control_ht)
{
	zval* val;
	zend_string *control_oid;
	char** ldap_attrs = NULL;
	LDAPSortKey** sort_keys = NULL;
	zend_string *tmpstring = NULL, **tmpstrings1 = NULL, **tmpstrings2 = NULL;
	size_t num_tmpstrings1 = 0, num_tmpstrings2 = 0;

	if ((val = zend_hash_str_find(control_ht, "oid", sizeof("oid") - 1)) == NULL) {
		zend_value_error("%s(): Control must have an \"oid\" key", get_active_function_name());
		return -1;
	}

	control_oid = zval_get_string(val);
	if (EG(exception)) {
		return -1;
	}

	bool control_iscritical = false;
	if ((val = zend_hash_str_find(control_ht, "iscritical", sizeof("iscritical") - 1)) != NULL) {
		control_iscritical = zend_is_true(val);
	}

	BerElement *ber = NULL;
	struct berval control_value = { 0L, NULL };
	bool control_value_alloc = false;
	int rc = LDAP_SUCCESS;

	if ((val = zend_hash_find(control_ht, ZSTR_KNOWN(ZEND_STR_VALUE))) != NULL) {
		if (Z_TYPE_P(val) != IS_ARRAY) {
			tmpstring = zval_get_string(val);
			if (EG(exception)) {
				rc = -1;
				goto failure;
			}
			control_value.bv_val = ZSTR_VAL(tmpstring);
			control_value.bv_len = ZSTR_LEN(tmpstring);
		} else if (zend_string_equals_literal(control_oid, LDAP_CONTROL_PAGEDRESULTS)) {
			zval* tmp;
			int pagesize = 1;
			struct berval cookie = { 0L, NULL };
			if ((tmp = zend_hash_str_find(Z_ARRVAL_P(val), "size", sizeof("size") - 1)) != NULL) {
				pagesize = zval_get_long(tmp);
			}
			if ((tmp = zend_hash_str_find(Z_ARRVAL_P(val), "cookie", sizeof("cookie") - 1)) != NULL) {
				tmpstring = zval_get_string(tmp);
				if (EG(exception)) {
					rc = -1;
					goto failure;
				}
				cookie.bv_val = ZSTR_VAL(tmpstring);
				cookie.bv_len = ZSTR_LEN(tmpstring);
			}
			/* ldap_create_page_control_value() allocates memory for control_value.bv_val */
			control_value_alloc = 1;
			rc = ldap_create_page_control_value(ld, pagesize, &cookie, &control_value);
			if (rc != LDAP_SUCCESS) {
				php_error_docref(NULL, E_WARNING, "Failed to create paged result control value: %s (%d)", ldap_err2string(rc), rc);
			}
		} else if (zend_string_equals_literal(control_oid, LDAP_CONTROL_ASSERT)) {
			zval* tmp;
			zend_string* assert;
			if ((tmp = zend_hash_str_find(Z_ARRVAL_P(val), "filter", sizeof("filter") - 1)) == NULL) {
				rc = -1;
				zend_value_error("%s(): Control must have a \"filter\" key", get_active_function_name());
			} else {
				assert = zval_get_string(tmp);
				if (EG(exception)) {
					rc = -1;
					goto failure;
				}
				/* ldap_create_assertion_control_value does not reset ld_errno, we need to do it ourselves
					 See http://www.openldap.org/its/index.cgi/Incoming?id=8674 */
				int success = LDAP_SUCCESS;
				ldap_set_option(ld, LDAP_OPT_RESULT_CODE, &success);
				/* ldap_create_assertion_control_value() allocates memory for control_value.bv_val */
				control_value_alloc = 1;
				rc = ldap_create_assertion_control_value(ld, ZSTR_VAL(assert), &control_value);
				if (rc != LDAP_SUCCESS) {
					php_error_docref(NULL, E_WARNING, "Failed to create assert control value: %s (%d)", ldap_err2string(rc), rc);
				}
				zend_string_release(assert);
			}
		} else if (zend_string_equals_literal(control_oid, LDAP_CONTROL_VALUESRETURNFILTER)) {
			zval* tmp;
			if ((tmp = zend_hash_str_find(Z_ARRVAL_P(val), "filter", sizeof("filter") - 1)) == NULL) {
				rc = -1;
				zend_value_error("%s(): Control must have a \"filter\" key", get_active_function_name());
			} else {
				ber = ber_alloc_t(LBER_USE_DER);
				if (ber == NULL) {
					rc = -1;
					php_error_docref(NULL, E_WARNING, "Failed to allocate control value");
				} else {
					tmpstring = zval_get_string(tmp);
					if (EG(exception)) {
						rc = -1;
						goto failure;
					}
					if (ldap_put_vrFilter(ber, ZSTR_VAL(tmpstring)) == -1) {
						rc = -1;
						php_error_docref(NULL, E_WARNING, "Failed to create control value: Bad ValuesReturnFilter: %s", ZSTR_VAL(tmpstring));
					} else if (ber_flatten2(ber, &control_value, control_value_alloc) == -1) {
						rc = -1;
					}
				}
			}
		} else if (zend_string_equals_literal(control_oid, LDAP_CONTROL_PRE_READ) || zend_string_equals_literal(control_oid, LDAP_CONTROL_POST_READ)) {
			zval* tmp;
			if ((tmp = zend_hash_str_find(Z_ARRVAL_P(val), "attrs", sizeof("attrs") - 1)) == NULL) {
				rc = -1;
				zend_value_error("%s(): Control must have an \"attrs\" key", get_active_function_name());
			} else {
				ber = ber_alloc_t(LBER_USE_DER);

				if (ber == NULL) {
					rc = -1;
					php_error_docref(NULL, E_WARNING, "Failed to allocate control value");
				} else {
					int num_attribs, i;
					zval* attr;

					num_attribs = zend_hash_num_elements(Z_ARRVAL_P(tmp));
					ldap_attrs = safe_emalloc((num_attribs+1), sizeof(char *), 0);
					tmpstrings1 = safe_emalloc(num_attribs, sizeof(zend_string*), 0);
					num_tmpstrings1 = 0;

					for (i = 0; i<num_attribs; i++) {
						if ((attr = zend_hash_index_find(Z_ARRVAL_P(tmp), i)) == NULL) {
							rc = -1;
							php_error_docref(NULL, E_WARNING, "Failed to encode attribute list");
							goto failure;
						}

						tmpstrings1[num_tmpstrings1] = zval_get_string(attr);
						if (EG(exception)) {
							rc = -1;
							goto failure;
						}
						ldap_attrs[i] = ZSTR_VAL(tmpstrings1[num_tmpstrings1]);
						++num_tmpstrings1;
					}
					ldap_attrs[num_attribs] = NULL;

					ber_init2( ber, NULL, LBER_USE_DER );

					if (ber_printf(ber, "{v}", ldap_attrs) == -1) {
						rc = -1;
						php_error_docref(NULL, E_WARNING, "Failed to encode attribute list");
					} else {
						int err;
						err = ber_flatten2(ber, &control_value, control_value_alloc);
						if (err < 0) {
							rc = -1;
							php_error_docref(NULL, E_WARNING, "Failed to encode control value (%d)", err);
						}
					}
				}
			}
		} else if (zend_string_equals_literal(control_oid, LDAP_CONTROL_SORTREQUEST)) {
			int num_keys, i;
			zval *sortkey, *tmp;

			num_keys = zend_hash_num_elements(Z_ARRVAL_P(val));
			sort_keys = safe_emalloc((num_keys+1), sizeof(LDAPSortKey*), 0);
			tmpstrings1 = safe_emalloc(num_keys, sizeof(zend_string*), 0);
			tmpstrings2 = safe_emalloc(num_keys, sizeof(zend_string*), 0);
			num_tmpstrings1 = 0;
			num_tmpstrings2 = 0;

			for (i = 0; i<num_keys; i++) {
				if ((sortkey = zend_hash_index_find(Z_ARRVAL_P(val), i)) == NULL) {
					rc = -1;
					php_error_docref(NULL, E_WARNING, "Failed to encode sort keys list");
					goto failure;
				}

				if ((tmp = zend_hash_str_find(Z_ARRVAL_P(sortkey), "attr", sizeof("attr") - 1)) == NULL) {
					rc = -1;
					zend_value_error("%s(): Sort key list must have an \"attr\" key", get_active_function_name());
					goto failure;
				}
				sort_keys[i] = emalloc(sizeof(LDAPSortKey));
				tmpstrings1[num_tmpstrings1] = zval_get_string(tmp);
				if (EG(exception)) {
					rc = -1;
					goto failure;
				}
				sort_keys[i]->attributeType = ZSTR_VAL(tmpstrings1[num_tmpstrings1]);
				++num_tmpstrings1;

				if ((tmp = zend_hash_str_find(Z_ARRVAL_P(sortkey), "oid", sizeof("oid") - 1)) != NULL) {
					tmpstrings2[num_tmpstrings2] = zval_get_string(tmp);
					if (EG(exception)) {
						rc = -1;
						goto failure;
					}
					sort_keys[i]->orderingRule = ZSTR_VAL(tmpstrings2[num_tmpstrings2]);
					++num_tmpstrings2;
				} else {
					sort_keys[i]->orderingRule = NULL;
				}

				if ((tmp = zend_hash_str_find(Z_ARRVAL_P(sortkey), "reverse", sizeof("reverse") - 1)) != NULL) {
					sort_keys[i]->reverseOrder = zend_is_true(tmp);
				} else {
					sort_keys[i]->reverseOrder = 0;
				}
			}
			sort_keys[num_keys] = NULL;
			/* ldap_create_sort_control_value() allocates memory for control_value.bv_val */
			control_value_alloc = 1;
			rc = ldap_create_sort_control_value(ld, sort_keys, &control_value);
			if (rc != LDAP_SUCCESS) {
				php_error_docref(NULL, E_WARNING, "Failed to create sort control value: %s (%d)", ldap_err2string(rc), rc);
			}
		} else if (zend_string_equals_literal(control_oid, LDAP_CONTROL_VLVREQUEST)) {
			zval* tmp;
			LDAPVLVInfo vlvInfo;
			struct berval attrValue;
			struct berval context;

			if ((tmp = zend_hash_str_find(Z_ARRVAL_P(val), "before", sizeof("before") - 1)) != NULL) {
				vlvInfo.ldvlv_before_count = zval_get_long(tmp);
			} else {
				rc = -1;
				zend_value_error("%s(): Array value for VLV control must have a \"before\" key", get_active_function_name());
				goto failure;
			}

			if ((tmp = zend_hash_str_find(Z_ARRVAL_P(val), "after", sizeof("after") - 1)) != NULL) {
				vlvInfo.ldvlv_after_count = zval_get_long(tmp);
			} else {
				rc = -1;
				zend_value_error("%s(): Array value for VLV control must have an \"after\" key", get_active_function_name());
				goto failure;
			}

			if ((tmp = zend_hash_str_find(Z_ARRVAL_P(val), "attrvalue", sizeof("attrvalue") - 1)) != NULL) {
				tmpstring = zval_get_string(tmp);
				if (EG(exception)) {
					rc = -1;
					goto failure;
				}
				attrValue.bv_val = ZSTR_VAL(tmpstring);
				attrValue.bv_len = ZSTR_LEN(tmpstring);
				vlvInfo.ldvlv_attrvalue = &attrValue;
			} else if ((tmp = zend_hash_str_find(Z_ARRVAL_P(val), "offset", sizeof("offset") - 1)) != NULL) {
				vlvInfo.ldvlv_attrvalue = NULL;
				vlvInfo.ldvlv_offset = zval_get_long(tmp);
				/* Find "count" key */
				if ((tmp = zend_hash_find(Z_ARRVAL_P(val), ZSTR_KNOWN(ZEND_STR_COUNT))) != NULL) {
					vlvInfo.ldvlv_count = zval_get_long(tmp);
				} else {
					rc = -1;
					zend_value_error("%s(): Array value for VLV control must have a \"count\" key", get_active_function_name());
					goto failure;
				}
			} else {
				rc = -1;
				zend_value_error("%s(): Array value for VLV control must have either an \"attrvalue\" or an \"offset\" key", get_active_function_name());
				goto failure;
			}

			if ((tmp = zend_hash_str_find(Z_ARRVAL_P(val), "context", sizeof("context") - 1)) != NULL) {
				tmpstring = zval_get_string(tmp);
				if (EG(exception)) {
					rc = -1;
					goto failure;
				}
				context.bv_val = ZSTR_VAL(tmpstring);
				context.bv_len = ZSTR_LEN(tmpstring);
				vlvInfo.ldvlv_context = &context;
			} else {
				vlvInfo.ldvlv_context = NULL;
			}

			/* ldap_create_vlv_control_value() allocates memory for control_value.bv_val */
			control_value_alloc = 1;
			rc = ldap_create_vlv_control_value(ld, &vlvInfo, &control_value);
			if (rc != LDAP_SUCCESS) {
				php_error_docref(NULL, E_WARNING, "Failed to create VLV control value: %s (%d)", ldap_err2string(rc), rc);
			}
		} else {
			zend_type_error("%s(): Control OID %s cannot be of type array", get_active_function_name(), ZSTR_VAL(control_oid));
			rc = -1;
		}
	}

	if (rc == LDAP_SUCCESS) {
		rc = ldap_control_create(ZSTR_VAL(control_oid), control_iscritical, &control_value, 1, ctrl);
	}

failure:
	zend_string_release(control_oid);
	if (tmpstring != NULL) {
		zend_string_release(tmpstring);
	}
	if (tmpstrings1 != NULL) {
		int i;
		for (i = 0; i < num_tmpstrings1; ++i) {
			zend_string_release(tmpstrings1[i]);
		}
		efree(tmpstrings1);
	}
	if (tmpstrings2 != NULL) {
		int i;
		for (i = 0; i < num_tmpstrings2; ++i) {
			zend_string_release(tmpstrings2[i]);
		}
		efree(tmpstrings2);
	}
	if (control_value.bv_val != NULL && control_value_alloc != 0) {
		ber_memfree(control_value.bv_val);
	}
	if (ber != NULL) {
		ber_free(ber, 1);
	}
	if (ldap_attrs != NULL) {
		efree(ldap_attrs);
	}
	if (sort_keys != NULL) {
		LDAPSortKey** sortp = sort_keys;
		while (*sortp) {
			efree(*sortp);
			sortp++;
		}
		efree(sort_keys);
		sort_keys = NULL;
	}

	if (rc == LDAP_SUCCESS) {
		return LDAP_SUCCESS;
	}

	/* Failed */
	*ctrl = NULL;
	return -1;
}

static void _php_ldap_controls_to_array(LDAP *ld, LDAPControl** ctrls, zval* array, int request)
{
	zval tmp1;
	LDAPControl **ctrlp;

	array = zend_try_array_init(array);
	if (!array) {
		return;
	}

	if (ctrls == NULL) {
		return;
	}
	ctrlp = ctrls;
	while (*ctrlp != NULL) {
		_php_ldap_control_to_array(ld, *ctrlp, &tmp1, request);
		add_assoc_zval(array, (*ctrlp)->ldctl_oid, &tmp1);
		ctrlp++;
	}
	ldap_controls_free(ctrls);
}

static LDAPControl** php_ldap_controls_from_array(LDAP *ld, const HashTable *controls, uint32_t arg_num)
{
	LDAPControl** ctrlp, **ctrls = NULL;
	zval* ctrlarray;
	int error = 0;

	uint32_t num_controls = zend_hash_num_elements(controls);
	ctrls = safe_emalloc((1 + num_controls), sizeof(*ctrls), 0);
	*ctrls = NULL;
	ctrlp = ctrls;
	ZEND_HASH_FOREACH_VAL(controls, ctrlarray) {
		if (Z_TYPE_P(ctrlarray) != IS_ARRAY) {
			zend_argument_type_error(arg_num, "must contain only arrays, where each array is a control");
			error = 1;
			break;
		}

		if (php_ldap_control_from_array(ld, ctrlp, Z_ARRVAL_P(ctrlarray)) == LDAP_SUCCESS) {
			++ctrlp;
		} else {
			error = 1;
			break;
		}

		*ctrlp = NULL;
	} ZEND_HASH_FOREACH_END();

	if (error) {
		ctrlp = ctrls;
		while (*ctrlp) {
			ldap_control_free(*ctrlp);
			ctrlp++;
		}
		efree(ctrls);
		ctrls = NULL;
	}

	return ctrls;
}

static void _php_ldap_controls_free (LDAPControl*** ctrls)
{
	LDAPControl **ctrlp;

	if (*ctrls) {
		ctrlp = *ctrls;
		while (*ctrlp) {
			ldap_control_free(*ctrlp);
			ctrlp++;
		}
		efree(*ctrls);
		*ctrls = NULL;
	}
}
/* }}} */

/* {{{ PHP_INI_BEGIN */
PHP_INI_BEGIN()
	STD_PHP_INI_ENTRY_EX("ldap.max_links", "-1", PHP_INI_SYSTEM, OnUpdateLong, max_links, zend_ldap_globals, ldap_globals, display_link_numbers)
PHP_INI_END()
/* }}} */

/* {{{ PHP_GINIT_FUNCTION */
static PHP_GINIT_FUNCTION(ldap)
{
#if defined(COMPILE_DL_LDAP) && defined(ZTS)
	ZEND_TSRMLS_CACHE_UPDATE();
#endif
	ldap_globals->num_links = 0;
}
/* }}} */

/* {{{ PHP_RINIT_FUNCTION */
static PHP_RINIT_FUNCTION(ldap)
{
#if defined(COMPILE_DL_LDAP) && defined(ZTS)
	ZEND_TSRMLS_CACHE_UPDATE();
#endif

	/* needed before first connect and after TLS option changes */
	LDAPG(tls_newctx) = true;

	return SUCCESS;
}
/* }}} */


/* {{{ PHP_MINIT_FUNCTION */
PHP_MINIT_FUNCTION(ldap)
{
	REGISTER_INI_ENTRIES();

	ldap_link_ce = register_class_LDAP_Connection();
	ldap_link_ce->create_object = ldap_link_create_object;
	ldap_link_ce->default_object_handlers = &ldap_link_object_handlers;

	memcpy(&ldap_link_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));
	ldap_link_object_handlers.offset = XtOffsetOf(ldap_linkdata, std);
	ldap_link_object_handlers.free_obj = ldap_link_free_obj;
	ldap_link_object_handlers.get_constructor = ldap_link_get_constructor;
	ldap_link_object_handlers.clone_obj = NULL;
	ldap_link_object_handlers.compare = zend_objects_not_comparable;

	ldap_result_ce = register_class_LDAP_Result();
	ldap_result_ce->create_object = ldap_result_create_object;
	ldap_result_ce->default_object_handlers = &ldap_result_object_handlers;

	memcpy(&ldap_result_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));
	ldap_result_object_handlers.offset = XtOffsetOf(ldap_resultdata, std);
	ldap_result_object_handlers.free_obj = ldap_result_free_obj;
	ldap_result_object_handlers.get_constructor = ldap_result_get_constructor;
	ldap_result_object_handlers.clone_obj = NULL;
	ldap_result_object_handlers.compare = zend_objects_not_comparable;

	ldap_result_entry_ce = register_class_LDAP_ResultEntry();
	ldap_result_entry_ce->create_object = ldap_result_entry_create_object;
	ldap_result_entry_ce->default_object_handlers = &ldap_result_entry_object_handlers;

	memcpy(&ldap_result_entry_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));
	ldap_result_entry_object_handlers.offset = XtOffsetOf(ldap_result_entry, std);
	ldap_result_entry_object_handlers.free_obj = ldap_result_entry_free_obj;
	ldap_result_entry_object_handlers.get_constructor = ldap_result_entry_get_constructor;
	ldap_result_entry_object_handlers.clone_obj = NULL;
	ldap_result_entry_object_handlers.compare = zend_objects_not_comparable;

	register_ldap_symbols(module_number);

	ldap_module_entry.type = type;

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION */
PHP_MSHUTDOWN_FUNCTION(ldap)
{
	UNREGISTER_INI_ENTRIES();
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION */
PHP_MINFO_FUNCTION(ldap)
{
	char tmp[32];

	php_info_print_table_start();
	php_info_print_table_row(2, "LDAP Support", "enabled");

	if (LDAPG(max_links) == -1) {
		snprintf(tmp, 31, ZEND_LONG_FMT "/unlimited", LDAPG(num_links));
	} else {
		snprintf(tmp, 31, ZEND_LONG_FMT "/" ZEND_LONG_FMT, LDAPG(num_links), LDAPG(max_links));
	}
	php_info_print_table_row(2, "Total Links", tmp);

#ifdef LDAP_API_VERSION
	snprintf(tmp, 31, "%d", LDAP_API_VERSION);
	php_info_print_table_row(2, "API Version", tmp);
#endif

#ifdef LDAP_VENDOR_NAME
	php_info_print_table_row(2, "Vendor Name", LDAP_VENDOR_NAME);
#endif

#ifdef LDAP_VENDOR_VERSION
	snprintf(tmp, 31, "%d", LDAP_VENDOR_VERSION);
	php_info_print_table_row(2, "Vendor Version", tmp);
#endif

#ifdef HAVE_LDAP_SASL
	php_info_print_table_row(2, "SASL Support", "Enabled");
#endif

	php_info_print_table_end();
	DISPLAY_INI_ENTRIES();
}
/* }}} */

/* {{{ Connect to an LDAP server */
PHP_FUNCTION(ldap_connect)
{
	char *host = NULL;
	size_t hostlen = 0;
	zend_long port = LDAP_PORT;
#ifdef HAVE_ORALDAP
	char *wallet = NULL, *walletpasswd = NULL;
	size_t walletlen = 0, walletpasswdlen = 0;
	zend_long authmode = GSLC_SSL_NO_AUTH;
	int ssl=0;
#endif
	ldap_linkdata *ld;
	LDAP *ldap = NULL;

	if (ZEND_NUM_ARGS() > 2) {
	    zend_error(E_DEPRECATED, "Calling ldap_connect() with Oracle-specific arguments is deprecated, "
			"use ldap_connect_wallet() instead");
	} else if (ZEND_NUM_ARGS() == 2) {
		zend_error(E_DEPRECATED, "Usage of ldap_connect with two arguments is deprecated");
	}

#ifdef HAVE_ORALDAP
	if (ZEND_NUM_ARGS() == 3 || ZEND_NUM_ARGS() == 4) {
		WRONG_PARAM_COUNT;
	}

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "|s!lssl", &host, &hostlen, &port, &wallet, &walletlen, &walletpasswd, &walletpasswdlen, &authmode) != SUCCESS) {
		RETURN_THROWS();
	}

	if (ZEND_NUM_ARGS() == 5) {
		ssl = 1;
	}
#else
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "|s!l", &host, &hostlen, &port) != SUCCESS) {
		RETURN_THROWS();
	}
#endif

	if (LDAPG(max_links) != -1 && LDAPG(num_links) >= LDAPG(max_links)) {
		php_error_docref(NULL, E_WARNING, "Too many open links (" ZEND_LONG_FMT ")", LDAPG(num_links));
		RETURN_FALSE;
	}


	{
		int rc = LDAP_SUCCESS;
		char	*url = host;
		if (url && !ldap_is_ldap_url(url)) {
			size_t urllen = hostlen + sizeof( "ldap://:65535" );

			if (port <= 0 || port > 65535) {
				zend_argument_value_error(2, "must be between 1 and 65535");
				RETURN_THROWS();
			}

			url = emalloc(urllen);
			snprintf( url, urllen, "ldap://%s:" ZEND_LONG_FMT, host, port );
		}

#ifdef LDAP_OPT_X_TLS_NEWCTX
		if (LDAPG(tls_newctx) && url && !strncmp(url, "ldaps:", 6)) {
			int val = 0;

			/* ensure all pending TLS options are applied in a new context */
			if (ldap_set_option(NULL, LDAP_OPT_X_TLS_NEWCTX, &val) != LDAP_OPT_SUCCESS) {
				if (url != host) {
					efree(url);
				}
				php_error_docref(NULL, E_WARNING, "Could not create new security context");
				RETURN_FALSE;
			}
			LDAPG(tls_newctx) = false;
		}
#endif
		object_init_ex(return_value, ldap_link_ce);
		ld = Z_LDAP_LINK_P(return_value);

#ifdef LDAP_API_FEATURE_X_OPENLDAP
		/* ldap_init() is deprecated, use ldap_initialize() instead.
		 */
		rc = ldap_initialize(&ldap, url);
#else /* ! LDAP_API_FEATURE_X_OPENLDAP */
		/* ldap_init does not support URLs.
		 * We must try the original host and port information.
		 */
		ldap = ldap_init(host, port);
		if (ldap == NULL) {
			zval_ptr_dtor(return_value);
			if (url != host) {
				efree(url);
			}
			php_error_docref(NULL, E_WARNING, "Could not create session handle");
			RETURN_FALSE;
		}
#endif /* ! LDAP_API_FEATURE_X_OPENLDAP */
		if (url != host) {
			efree(url);
		}
		if (rc != LDAP_SUCCESS) {
			zval_ptr_dtor(return_value);
			php_error_docref(NULL, E_WARNING, "Could not create session handle: %s", ldap_err2string(rc));
			RETURN_FALSE;
		}
	}

	if (ldap == NULL) {
		zval_ptr_dtor(return_value);
		RETURN_FALSE;
	} else {
#ifdef HAVE_ORALDAP
		if (ssl) {
			if (ldap_init_SSL(&ldap->ld_sb, wallet, walletpasswd, authmode)) {
				zval_ptr_dtor(return_value);
				php_error_docref(NULL, E_WARNING, "SSL init failed");
				RETURN_FALSE;
			}
		}
#endif
		LDAPG(num_links)++;
		ld->link = ldap;
	}

}
/* }}} */

#if defined(HAVE_ORALDAP) && defined(LDAP_API_FEATURE_X_OPENLDAP)
PHP_FUNCTION(ldap_connect_wallet) {
	char *host = NULL;
	size_t hostlen = 0;
	char *wallet = NULL, *walletpasswd = NULL;
	size_t walletlen = 0, walletpasswdlen = 0;
	zend_long authmode = GSLC_SSL_NO_AUTH;
	bool ssl = false;

	ldap_linkdata *ld;
	LDAP *ldap = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "s!ss|l",
		&host, &hostlen, &wallet, &walletlen, &walletpasswd, &walletpasswdlen, &authmode) != SUCCESS
	) {
		RETURN_THROWS();
	}

	if (authmode != 0) {
		ssl = true;
	}

	if (LDAPG(max_links) != -1 && LDAPG(num_links) >= LDAPG(max_links)) {
		php_error_docref(NULL, E_WARNING, "Too many open links (" ZEND_LONG_FMT ")", LDAPG(num_links));
		RETURN_FALSE;
	}

	object_init_ex(return_value, ldap_link_ce);
	ld = Z_LDAP_LINK_P(return_value);

	{
		int rc = LDAP_SUCCESS;
		char *url = host;
		if (url && !ldap_is_ldap_url(url)) {
			size_t urllen = hostlen + sizeof( "ldap://:65535" );

			url = emalloc(urllen);
			snprintf( url, urllen, "ldap://%s", host );
		}

		/* ldap_init() is deprecated, use ldap_initialize() instead. */
		rc = ldap_initialize(&ldap, url);
		if (url != host) {
			efree(url);
		}
		if (rc != LDAP_SUCCESS) {
			zval_ptr_dtor(return_value);
			php_error_docref(NULL, E_WARNING, "Could not create session handle: %s", ldap_err2string(rc));
			RETURN_FALSE;
		}
	}

	if (ldap == NULL) {
		zval_ptr_dtor(return_value);
		RETURN_FALSE;
	} else {
		if (ssl) {
			if (ldap_init_SSL(&ldap->ld_sb, wallet, walletpasswd, authmode)) {
				zval_ptr_dtor(return_value);
				php_error_docref(NULL, E_WARNING, "SSL init failed");
				RETURN_FALSE;
			}
		}
		LDAPG(num_links)++;
		ld->link = ldap;
	}
}
#endif

/* {{{ _get_lderrno */
static int _get_lderrno(LDAP *ldap)
{
#if LDAP_API_VERSION > 2000 || defined(HAVE_ORALDAP)
	int lderr;

	/* New versions of OpenLDAP do it this way */
	ldap_get_option(ldap, LDAP_OPT_ERROR_NUMBER, &lderr);
	return lderr;
#else
	return ldap->ld_errno;
#endif
}
/* }}} */

/* {{{ Bind to LDAP directory */
PHP_FUNCTION(ldap_bind)
{
	zval *link;
	char *ldap_bind_dn = NULL, *ldap_bind_pw = NULL;
	size_t ldap_bind_dnlen, ldap_bind_pwlen;
	ldap_linkdata *ld;
	int rc;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "O|p!p!", &link, ldap_link_ce, &ldap_bind_dn, &ldap_bind_dnlen, &ldap_bind_pw, &ldap_bind_pwlen) != SUCCESS) {
		RETURN_THROWS();
	}

	ld = Z_LDAP_LINK_P(link);
	VERIFY_LDAP_LINK_CONNECTED(ld);

	{
#ifdef LDAP_API_FEATURE_X_OPENLDAP
		/* ldap_simple_bind_s() is deprecated, use ldap_sasl_bind_s() instead.
		 */
		struct berval   cred;

		cred.bv_val = ldap_bind_pw;
		cred.bv_len = ldap_bind_pw ? ldap_bind_pwlen : 0;
		rc = ldap_sasl_bind_s(ld->link, ldap_bind_dn, LDAP_SASL_SIMPLE, &cred,
				NULL, NULL,     /* no controls right now */
				NULL);	  /* we don't care about the server's credentials */
#else /* ! LDAP_API_FEATURE_X_OPENLDAP */
		rc = ldap_simple_bind_s(ld->link, ldap_bind_dn, ldap_bind_pw);
#endif /* ! LDAP_API_FEATURE_X_OPENLDAP */
	}
	if ( rc != LDAP_SUCCESS) {
		php_error_docref(NULL, E_WARNING, "Unable to bind to server: %s", ldap_err2string(rc));
		RETURN_FALSE;
	} else {
		RETURN_TRUE;
	}
}
/* }}} */

/* {{{ Bind to LDAP directory */
PHP_FUNCTION(ldap_bind_ext)
{
	zval *link;
	char *ldap_bind_dn = NULL, *ldap_bind_pw = NULL;
	size_t ldap_bind_dnlen, ldap_bind_pwlen;
	HashTable *server_controls_ht = NULL;
	ldap_linkdata *ld;
	LDAPControl **lserverctrls = NULL;
	ldap_resultdata *result;
	LDAPMessage *ldap_res;
	int rc;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "O|p!p!h!", &link, ldap_link_ce, &ldap_bind_dn, &ldap_bind_dnlen, &ldap_bind_pw, &ldap_bind_pwlen, &server_controls_ht) != SUCCESS) {
		RETURN_THROWS();
	}

	ld = Z_LDAP_LINK_P(link);
	VERIFY_LDAP_LINK_CONNECTED(ld);

	if (server_controls_ht) {
		lserverctrls = php_ldap_controls_from_array(ld->link, server_controls_ht, 4);
		if (lserverctrls == NULL) {
			RETVAL_FALSE;
			goto cleanup;
		}
	}

	{
		/* ldap_simple_bind() is deprecated, use ldap_sasl_bind() instead */
		struct berval   cred;
		int msgid;

		cred.bv_val = ldap_bind_pw;
		cred.bv_len = ldap_bind_pw ? ldap_bind_pwlen : 0;
		/* asynchronous call */
		rc = ldap_sasl_bind(ld->link, ldap_bind_dn, LDAP_SASL_SIMPLE, &cred,
				lserverctrls, NULL, &msgid);
		if (rc != LDAP_SUCCESS ) {
			php_error_docref(NULL, E_WARNING, "Unable to bind to server: %s (%d)", ldap_err2string(rc), rc);
			RETVAL_FALSE;
			goto cleanup;
		}

		rc = ldap_result(ld->link, msgid, 1 /* LDAP_MSG_ALL */, NULL, &ldap_res);
		if (rc == -1) {
			php_error_docref(NULL, E_WARNING, "Bind operation failed");
			RETVAL_FALSE;
			goto cleanup;
		}

		/* return a PHP control object */
		object_init_ex(return_value, ldap_result_ce);
		result = Z_LDAP_RESULT_P(return_value);
		result->result = ldap_res;
	}

cleanup:
	if (lserverctrls) {
		_php_ldap_controls_free(&lserverctrls);
	}

	return;
}
/* }}} */

#ifdef HAVE_LDAP_SASL
typedef struct {
	char *mech;
	char *realm;
	char *authcid;
	char *passwd;
	char *authzid;
} php_ldap_bictx;

/* {{{ _php_sasl_setdefs */
static php_ldap_bictx *_php_sasl_setdefs(LDAP *ld, char *sasl_mech, char *sasl_realm, char *sasl_authc_id, char *passwd, char *sasl_authz_id)
{
	php_ldap_bictx *ctx;

	ctx = ber_memalloc(sizeof(php_ldap_bictx));
	ctx->mech    = (sasl_mech) ? ber_strdup(sasl_mech) : NULL;
	ctx->realm   = (sasl_realm) ? ber_strdup(sasl_realm) : NULL;
	ctx->authcid = (sasl_authc_id) ? ber_strdup(sasl_authc_id) : NULL;
	ctx->passwd  = (passwd) ? ber_strdup(passwd) : NULL;
	ctx->authzid = (sasl_authz_id) ? ber_strdup(sasl_authz_id) : NULL;

	if (ctx->mech == NULL) {
		ldap_get_option(ld, LDAP_OPT_X_SASL_MECH, &ctx->mech);
	}
	if (ctx->realm == NULL) {
		ldap_get_option(ld, LDAP_OPT_X_SASL_REALM, &ctx->realm);
	}
	if (ctx->authcid == NULL) {
		ldap_get_option(ld, LDAP_OPT_X_SASL_AUTHCID, &ctx->authcid);
	}
	if (ctx->authzid == NULL) {
		ldap_get_option(ld, LDAP_OPT_X_SASL_AUTHZID, &ctx->authzid);
	}

	return ctx;
}
/* }}} */

/* {{{ _php_sasl_freedefs */
static void _php_sasl_freedefs(php_ldap_bictx *ctx)
{
	if (ctx->mech) ber_memfree(ctx->mech);
	if (ctx->realm) ber_memfree(ctx->realm);
	if (ctx->authcid) ber_memfree(ctx->authcid);
	if (ctx->passwd) ber_memfree(ctx->passwd);
	if (ctx->authzid) ber_memfree(ctx->authzid);
	ber_memfree(ctx);
}
/* }}} */

/* {{{ _php_sasl_interact
   Internal interact function for SASL */
static int _php_sasl_interact(LDAP *ld, unsigned flags, void *defaults, void *in)
{
	sasl_interact_t *interact = in;
	const char *p;
	php_ldap_bictx *ctx = defaults;

	for (;interact->id != SASL_CB_LIST_END;interact++) {
		p = NULL;
		switch(interact->id) {
			case SASL_CB_GETREALM:
				p = ctx->realm;
				break;
			case SASL_CB_AUTHNAME:
				p = ctx->authcid;
				break;
			case SASL_CB_USER:
				p = ctx->authzid;
				break;
			case SASL_CB_PASS:
				p = ctx->passwd;
				break;
		}
		if (p) {
			interact->result = p;
			interact->len = strlen(interact->result);
		}
	}
	return LDAP_SUCCESS;
}
/* }}} */

/* {{{ Bind to LDAP directory using SASL */
PHP_FUNCTION(ldap_sasl_bind)
{
	zval *link;
	ldap_linkdata *ld;
	char *binddn = NULL;
	char *passwd = NULL;
	char *sasl_mech = NULL;
	char *sasl_realm = NULL;
	char *sasl_authz_id = NULL;
	char *sasl_authc_id = NULL;
	char *props = NULL;
	size_t rc, dn_len, passwd_len, mech_len, realm_len, authc_id_len, authz_id_len, props_len;
	php_ldap_bictx *ctx;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "O|p!p!p!p!p!p!p!",
		&link, ldap_link_ce,
		&binddn, &dn_len,
		&passwd, &passwd_len,
		&sasl_mech, &mech_len,
		&sasl_realm, &realm_len,
		&sasl_authc_id,
		&authc_id_len,
		&sasl_authz_id,
		&authz_id_len,
		&props, &props_len
	) != SUCCESS) {
		RETURN_THROWS();
	}

	ld = Z_LDAP_LINK_P(link);
	VERIFY_LDAP_LINK_CONNECTED(ld);

	ctx = _php_sasl_setdefs(ld->link, sasl_mech, sasl_realm, sasl_authc_id, passwd, sasl_authz_id);

	if (props) {
		ldap_set_option(ld->link, LDAP_OPT_X_SASL_SECPROPS, props);
	}

	rc = ldap_sasl_interactive_bind_s(ld->link, binddn, ctx->mech, NULL, NULL, LDAP_SASL_QUIET, _php_sasl_interact, ctx);
	if (rc != LDAP_SUCCESS) {
		php_error_docref(NULL, E_WARNING, "Unable to bind to server: %s", ldap_err2string(rc));
		RETVAL_FALSE;
	} else {
		RETVAL_TRUE;
	}
	_php_sasl_freedefs(ctx);
}
/* }}} */
#endif /* HAVE_LDAP_SASL */

/* {{{ Unbind from LDAP directory */
PHP_FUNCTION(ldap_unbind)
{
	zval *link;
	ldap_linkdata *ld;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "O", &link, ldap_link_ce) != SUCCESS) {
		RETURN_THROWS();
	}

	ld = Z_LDAP_LINK_P(link);
	VERIFY_LDAP_LINK_CONNECTED(ld);

	ldap_link_free(ld);

	RETURN_TRUE;
}
/* }}} */

/* {{{ php_set_opts */
static void php_set_opts(LDAP *ldap, int sizelimit, int timelimit, int deref, int *old_sizelimit, int *old_timelimit, int *old_deref)
{
	/* sizelimit */
	if (sizelimit > -1) {
#if (LDAP_API_VERSION >= 2004) || defined(HAVE_ORALDAP)
		ldap_get_option(ldap, LDAP_OPT_SIZELIMIT, old_sizelimit);
		ldap_set_option(ldap, LDAP_OPT_SIZELIMIT, &sizelimit);
#else
		*old_sizelimit = ldap->ld_sizelimit;
		ldap->ld_sizelimit = sizelimit;
#endif
	}

	/* timelimit */
	if (timelimit > -1) {
#if (LDAP_API_VERSION >= 2004) || defined(HAVE_ORALDAP)
		ldap_get_option(ldap, LDAP_OPT_TIMELIMIT, old_timelimit);
		ldap_set_option(ldap, LDAP_OPT_TIMELIMIT, &timelimit);
#else
		*old_timelimit = ldap->ld_timelimit;
		ldap->ld_timelimit = timelimit;
#endif
	}

	/* deref */
	if (deref > -1) {
#if (LDAP_API_VERSION >= 2004) || defined(HAVE_ORALDAP)
		ldap_get_option(ldap, LDAP_OPT_DEREF, old_deref);
		ldap_set_option(ldap, LDAP_OPT_DEREF, &deref);
#else
		*old_deref = ldap->ld_deref;
		ldap->ld_deref = deref;
#endif
	}
}
/* }}} */

/* {{{ php_ldap_do_search */
static void php_ldap_do_search(INTERNAL_FUNCTION_PARAMETERS, int scope)
{
	zval *link, *attrs = NULL;
	HashTable *base_dn_ht = NULL;
	zend_string *base_dn_str = NULL;
	HashTable *filter_ht = NULL;
	zend_string *filter_str = NULL;
	zend_long attrsonly, sizelimit, timelimit, deref;
	HashTable *server_controls_ht = NULL;
	char **ldap_attrs = NULL;
	ldap_linkdata *ld = NULL;
	ldap_resultdata *result;
	LDAPMessage *ldap_res = NULL;
	LDAPControl **lserverctrls = NULL;
	int ldap_attrsonly = 0, ldap_sizelimit = -1, ldap_timelimit = -1, ldap_deref = -1;
	int old_ldap_sizelimit = -1, old_ldap_timelimit = -1, old_ldap_deref = -1;
	int ret = 1, ldap_errno, argcount = ZEND_NUM_ARGS();

	ZEND_PARSE_PARAMETERS_START(3, 9)
		Z_PARAM_ZVAL(link)
		Z_PARAM_ARRAY_HT_OR_STR(base_dn_ht, base_dn_str)
		Z_PARAM_ARRAY_HT_OR_STR(filter_ht, filter_str)
		Z_PARAM_OPTIONAL
		Z_PARAM_ARRAY_EX(attrs, 0, 1)
		Z_PARAM_LONG(attrsonly)
		Z_PARAM_LONG(sizelimit)
		Z_PARAM_LONG(timelimit)
		Z_PARAM_LONG(deref)
		Z_PARAM_ARRAY_HT_EX(server_controls_ht, 1, 1)
	ZEND_PARSE_PARAMETERS_END();

	/* Reverse -> fall through */
	switch (argcount) {
		case 9:
		case 8:
			ldap_deref = deref;
			ZEND_FALLTHROUGH;
		case 7:
			ldap_timelimit = timelimit;
			ZEND_FALLTHROUGH;
		case 6:
			ldap_sizelimit = sizelimit;
			ZEND_FALLTHROUGH;
		case 5:
			ldap_attrsonly = attrsonly;
			ZEND_FALLTHROUGH;
		default:
			break;
	}

	if (attrs) {
		const HashTable *attributes = Z_ARRVAL_P(attrs);
		uint32_t num_attribs = zend_hash_num_elements(attributes);

		if (num_attribs == 0) {
			/* We don't allocate ldap_attrs for an empty array */
			goto process;
		}
		if (!php_ldap_is_numerically_indexed_array(attributes)) {
			zend_argument_value_error(4, "must be an array with numeric keys");
			RETURN_THROWS();
		}
		/* Allocate +1 as we need an extra entry to NULL terminate the list */
		ldap_attrs = safe_emalloc(num_attribs+1, sizeof(char *), 0);

		zend_ulong attribute_index = 0;
		zval *attribute_zv = NULL;
		ZEND_HASH_FOREACH_VAL(attributes, attribute_zv) {
			ZVAL_DEREF(attribute_zv);
			if (Z_TYPE_P(attribute_zv) != IS_STRING) {
				zend_argument_type_error(4, "must be a list of strings, %s given", zend_zval_value_name(attribute_zv));
				ret = 0;
				goto cleanup;
			}
			zend_string *attribute = Z_STR_P(attribute_zv);
			if (zend_str_has_nul_byte(attribute)) {
				zend_argument_value_error(4, "must not contain strings with any null bytes");
				ret = 0;
				goto cleanup;
			}
			ldap_attrs[attribute_index++] = ZSTR_VAL(attribute);
		} ZEND_HASH_FOREACH_END();
		ldap_attrs[num_attribs] = NULL;
	}
process:

	/* parallel search? */
	if (Z_TYPE_P(link) == IS_ARRAY) {
		const zend_string *ldap_base_dn = NULL;
		const zend_string *ldap_filter = NULL;
		uint32_t num_links = zend_hash_num_elements(Z_ARRVAL_P(link));
		if (num_links == 0) {
			zend_argument_must_not_be_empty_error(1);
			ret = 0;
			goto cleanup;
		}
		if (!zend_array_is_list(Z_ARRVAL_P(link))) {
			zend_argument_value_error(1, "must be a list");
			ret = 0;
			goto cleanup;
		}

		uint32_t num_base_dns = 0; /* If 0 this means we are working with a unique base dn */
		if (base_dn_ht) {
			if (!zend_array_is_list(base_dn_ht)) {
				zend_argument_value_error(2, "must be a list");
				ret = 0;
				goto cleanup;
			}
			num_base_dns = zend_hash_num_elements(base_dn_ht);
			if (num_base_dns != num_links) {
				zend_argument_value_error(2, "must be the same size as argument #1");
				ret = 0;
				goto cleanup;
			}
		} else {
			if (zend_str_has_nul_byte(base_dn_str)) {
				zend_argument_value_error(2, "must not contain null bytes");
				ret = 0;
				goto cleanup;
			}
			ldap_base_dn = base_dn_str;
		}

		uint32_t num_filters = 0; /* If 0 this means we are working with a unique base dn */
		if (filter_ht) {
			if (!zend_array_is_list(filter_ht)) {
				zend_argument_value_error(3, "must be a list");
				ret = 0;
				goto cleanup;
			}
			num_filters = zend_hash_num_elements(filter_ht);
			if (num_filters != num_links) {
				zend_argument_value_error(3, "must be the same size as argument #1");
				ret = 0;
				goto cleanup;
			}
		} else {
			if (zend_str_has_nul_byte(filter_str)) {
				zend_argument_value_error(3, "must not contain null bytes");
				ret = 0;
				goto cleanup;
			}
			ldap_filter = filter_str;
		}

		int *rcs;
		ldap_linkdata **lds;
		lds = safe_emalloc(num_links, sizeof(ldap_linkdata), 0);
		rcs = safe_emalloc(num_links, sizeof(*rcs), 0);

		zend_ulong ldap_link_index = 0;
		zval *link_zv = NULL;
		ZEND_HASH_FOREACH_NUM_KEY_VAL(Z_ARRVAL_P(link), ldap_link_index, link_zv) {
			ZVAL_DEREF(link_zv);
			if (Z_TYPE_P(link_zv) != IS_OBJECT || !instanceof_function(Z_OBJCE_P(link_zv), ldap_link_ce)) {
				zend_argument_value_error(1, "must be a list of LDAP\\Connection");
				ret = 0;
				goto cleanup_parallel;
			}

			ldap_linkdata *current_ld = Z_LDAP_LINK_P(link_zv);
			if (!current_ld->link) {
				zend_throw_error(NULL, "LDAP connection has already been closed");
				ret = 0;
				goto cleanup_parallel;
			}

			if (num_base_dns != 0) { /* base_dn an array? */
				zval *base_dn_zv = zend_hash_index_find(base_dn_ht, ldap_link_index);
				ZEND_ASSERT(base_dn_zv);
				ZVAL_DEREF(base_dn_zv);
				if (Z_TYPE_P(base_dn_zv) != IS_STRING) {
					zend_argument_type_error(2, "must be a list of strings, %s given", zend_zval_value_name(base_dn_zv));
					ret = 0;
					goto cleanup_parallel;
				}
				ldap_base_dn = Z_STR_P(base_dn_zv);
				if (zend_str_has_nul_byte(ldap_base_dn)) {
					zend_argument_value_error(2, "must not contain null bytes");
					ret = 0;
					goto cleanup_parallel;
				}
			}
			if (num_filters != 0) { /* filter an array? */
				zval *filter_zv = zend_hash_index_find(filter_ht, ldap_link_index);
				ZEND_ASSERT(filter_zv);
				ZVAL_DEREF(filter_zv);
				if (Z_TYPE_P(filter_zv) != IS_STRING) {
					zend_argument_type_error(3, "must be a list of strings, %s given", zend_zval_value_name(filter_zv));
					ret = 0;
					goto cleanup_parallel;
				}
				ldap_filter = Z_STR_P(filter_zv);
				if (zend_str_has_nul_byte(ldap_filter)) {
					zend_argument_value_error(3, "must not contain null bytes");
					ret = 0;
					goto cleanup_parallel;
				}
			}

			if (server_controls_ht) {
				/* We have to parse controls again for each link as they use it */
				_php_ldap_controls_free(&lserverctrls);
				lserverctrls = php_ldap_controls_from_array(current_ld->link, server_controls_ht, 9);
				if (lserverctrls == NULL) {
					rcs[ldap_link_index] = -1;
					// TODO Throw an exception/cleanup?
					continue;
				}
			}

			php_set_opts(current_ld->link, ldap_sizelimit, ldap_timelimit, ldap_deref, &old_ldap_sizelimit, &old_ldap_timelimit, &old_ldap_deref);

			/* Run the actual search */
			ldap_search_ext(current_ld->link, ZSTR_VAL(ldap_base_dn), scope, ZSTR_VAL(ldap_filter), ldap_attrs, ldap_attrsonly, lserverctrls, NULL, NULL, ldap_sizelimit, &rcs[ldap_link_index]);
			lds[ldap_link_index] = current_ld;

			// TODO Reset the options of the link?
		} ZEND_HASH_FOREACH_END();

		array_init(return_value);

		/* Collect results from the searches */
		for (uint32_t i = 0; i < num_links; i++) {
			if (rcs[i] != -1) {
				rcs[i] = ldap_result(lds[i]->link, LDAP_RES_ANY, 1 /* LDAP_MSG_ALL */, NULL, &ldap_res);
			}
			if (rcs[i] != -1) {
				zval object;
				object_init_ex(&object, ldap_result_ce);
				result = Z_LDAP_RESULT_P(&object);
				result->result = ldap_res;
				add_next_index_zval(return_value, &object);
			} else {
				add_next_index_bool(return_value, 0);
			}
		}

cleanup_parallel:
		efree(lds);
		efree(rcs);
	} else if (Z_TYPE_P(link) == IS_OBJECT && instanceof_function(Z_OBJCE_P(link), ldap_link_ce)) {
		ld = Z_LDAP_LINK_P(link);
		if (!ld->link) {
			zend_throw_error(NULL, "LDAP connection has already been closed");
			ret = 0;
			goto cleanup;
		}

		if (!base_dn_str) {
			zend_argument_type_error(2, "must be of type string when argument #1 ($ldap) is an LDAP\\Connection instance");
			ret = 0;
			goto cleanup;
		}

		if (!filter_str) {
			zend_argument_type_error(3, "must be of type string when argument #1 ($ldap) is an LDAP\\Connection instance");
			ret = 0;
			goto cleanup;
		}

		if (server_controls_ht) {
			lserverctrls = php_ldap_controls_from_array(ld->link, server_controls_ht, 9);
			if (lserverctrls == NULL) {
				ret = 0;
				goto cleanup;
			}
		}

		php_set_opts(ld->link, ldap_sizelimit, ldap_timelimit, ldap_deref, &old_ldap_sizelimit, &old_ldap_timelimit, &old_ldap_deref);

		/* Run the actual search */
		ldap_errno = ldap_search_ext_s(ld->link, ZSTR_VAL(base_dn_str), scope, ZSTR_VAL(filter_str), ldap_attrs, ldap_attrsonly, lserverctrls, NULL, NULL, ldap_sizelimit, &ldap_res);

		if (ldap_errno != LDAP_SUCCESS
			&& ldap_errno != LDAP_SIZELIMIT_EXCEEDED
#ifdef LDAP_ADMINLIMIT_EXCEEDED
			&& ldap_errno != LDAP_ADMINLIMIT_EXCEEDED
#endif
#ifdef LDAP_REFERRAL
			&& ldap_errno != LDAP_REFERRAL
#endif
		) {
			/* ldap_res should be freed regardless of return value of ldap_search_ext_s()
			 * see: https://linux.die.net/man/3/ldap_search_ext_s */
			if (ldap_res != NULL) {
				ldap_msgfree(ldap_res);
			}
			php_error_docref(NULL, E_WARNING, "Search: %s", ldap_err2string(ldap_errno));
			ret = 0;
		} else {
			if (ldap_errno == LDAP_SIZELIMIT_EXCEEDED) {
				php_error_docref(NULL, E_WARNING, "Partial search results returned: Sizelimit exceeded");
			}
#ifdef LDAP_ADMINLIMIT_EXCEEDED
			else if (ldap_errno == LDAP_ADMINLIMIT_EXCEEDED) {
				php_error_docref(NULL, E_WARNING, "Partial search results returned: Adminlimit exceeded");
			}
#endif
			object_init_ex(return_value, ldap_result_ce);
			result = Z_LDAP_RESULT_P(return_value);
			result->result = ldap_res;
		}
	} else {
		zend_argument_type_error(1, "must be of type LDAP\\Connection|array, %s given", zend_zval_value_name(link));
	}

cleanup:
	if (ld) {
		/* Restoring previous options */
		php_set_opts(ld->link, old_ldap_sizelimit, old_ldap_timelimit, old_ldap_deref, &ldap_sizelimit, &ldap_timelimit, &ldap_deref);
	}
	if (ldap_attrs != NULL) {
		efree(ldap_attrs);
	}
	if (!ret) {
		RETVAL_BOOL(ret);
	}
	if (lserverctrls) {
		_php_ldap_controls_free(&lserverctrls);
	}
}
/* }}} */

/* {{{ Read an entry */
PHP_FUNCTION(ldap_read)
{
	php_ldap_do_search(INTERNAL_FUNCTION_PARAM_PASSTHRU, LDAP_SCOPE_BASE);
}
/* }}} */

/* {{{ Single-level search */
PHP_FUNCTION(ldap_list)
{
	php_ldap_do_search(INTERNAL_FUNCTION_PARAM_PASSTHRU, LDAP_SCOPE_ONELEVEL);
}
/* }}} */

/* {{{ Search LDAP tree under base_dn */
PHP_FUNCTION(ldap_search)
{
	php_ldap_do_search(INTERNAL_FUNCTION_PARAM_PASSTHRU, LDAP_SCOPE_SUBTREE);
}
/* }}} */

/* {{{ Free result memory */
PHP_FUNCTION(ldap_free_result)
{
	zval *result;
	ldap_resultdata *ldap_result;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "O", &result, ldap_result_ce) != SUCCESS) {
		RETURN_THROWS();
	}

	ldap_result = Z_LDAP_RESULT_P(result);
	VERIFY_LDAP_RESULT_OPEN(ldap_result);

	ldap_result_free(ldap_result);

	RETVAL_TRUE;
}
/* }}} */

/* {{{ Count the number of entries in a search result */
PHP_FUNCTION(ldap_count_entries)
{
	zval *link, *result;
	ldap_linkdata *ld;
	ldap_resultdata *ldap_result;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "OO", &link, ldap_link_ce, &result, ldap_result_ce) != SUCCESS) {
		RETURN_THROWS();
	}

	ld = Z_LDAP_LINK_P(link);
	VERIFY_LDAP_LINK_CONNECTED(ld);

	ldap_result = Z_LDAP_RESULT_P(result);
	VERIFY_LDAP_RESULT_OPEN(ldap_result);

	RETURN_LONG(ldap_count_entries(ld->link, ldap_result->result));
}
/* }}} */

/* {{{ Return first result id */
PHP_FUNCTION(ldap_first_entry)
{
	zval *link, *result;
	ldap_linkdata *ld;
	ldap_result_entry *resultentry;
	ldap_resultdata *ldap_result;
	LDAPMessage *entry;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "OO", &link, ldap_link_ce, &result, ldap_result_ce) != SUCCESS) {
		RETURN_THROWS();
	}

	ld = Z_LDAP_LINK_P(link);
	VERIFY_LDAP_LINK_CONNECTED(ld);

	ldap_result = Z_LDAP_RESULT_P(result);
	VERIFY_LDAP_RESULT_OPEN(ldap_result);

	if ((entry = ldap_first_entry(ld->link, ldap_result->result)) == NULL) {
		RETVAL_FALSE;
	} else {
		object_init_ex(return_value, ldap_result_entry_ce);
		resultentry = Z_LDAP_RESULT_ENTRY_P(return_value);
		ZVAL_COPY(&resultentry->res, result);
		resultentry->data = entry;
		resultentry->ber = NULL;
	}
}
/* }}} */

/* {{{ Get next result entry */
PHP_FUNCTION(ldap_next_entry)
{
	zval *link, *result_entry;
	ldap_linkdata *ld;
	ldap_result_entry *resultentry, *resultentry_next;
	LDAPMessage *entry_next;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "OO", &link, ldap_link_ce, &result_entry, ldap_result_entry_ce) != SUCCESS) {
		RETURN_THROWS();
	}

	ld = Z_LDAP_LINK_P(link);
	VERIFY_LDAP_LINK_CONNECTED(ld);

	resultentry = Z_LDAP_RESULT_ENTRY_P(result_entry);

	if ((entry_next = ldap_next_entry(ld->link, resultentry->data)) == NULL) {
		RETVAL_FALSE;
	} else {
		object_init_ex(return_value, ldap_result_entry_ce);
		resultentry_next = Z_LDAP_RESULT_ENTRY_P(return_value);
		ZVAL_COPY(&resultentry_next->res, &resultentry->res);
		resultentry_next->data = entry_next;
		resultentry_next->ber = NULL;
	}
}
/* }}} */

/* {{{ Get all result entries */
PHP_FUNCTION(ldap_get_entries)
{
	zval *link, *result;
	ldap_resultdata *ldap_result;
	LDAPMessage *ldap_result_entry;
	zval tmp1, tmp2;
	ldap_linkdata *ld;
	LDAP *ldap;
	int num_entries, num_attrib, num_values, i;
	BerElement *ber;
	char *attribute;
	size_t attr_len;
	struct berval **ldap_value;
	char *dn;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "OO", &link, ldap_link_ce, &result, ldap_result_ce) != SUCCESS) {
		RETURN_THROWS();
	}

	ld = Z_LDAP_LINK_P(link);
	VERIFY_LDAP_LINK_CONNECTED(ld);

	ldap_result = Z_LDAP_RESULT_P(result);
	VERIFY_LDAP_RESULT_OPEN(ldap_result);

	ldap = ld->link;
	num_entries = ldap_count_entries(ldap, ldap_result->result);

	array_init(return_value);
	add_assoc_long(return_value, "count", num_entries);

	if (num_entries == 0) {
		return;
	}

	ldap_result_entry = ldap_first_entry(ldap, ldap_result->result);
	if (ldap_result_entry == NULL) {
		zend_array_destroy(Z_ARR_P(return_value));
		RETURN_FALSE;
	}

	num_entries = 0;
	while (ldap_result_entry != NULL) {
		array_init(&tmp1);

		num_attrib = 0;
		attribute = ldap_first_attribute(ldap, ldap_result_entry, &ber);

		while (attribute != NULL) {
			ldap_value = ldap_get_values_len(ldap, ldap_result_entry, attribute);
			num_values = ldap_count_values_len(ldap_value);

			array_init(&tmp2);
			add_assoc_long(&tmp2, "count", num_values);
			for (i = 0; i < num_values; i++) {
				add_index_stringl(&tmp2, i, ldap_value[i]->bv_val, ldap_value[i]->bv_len);
			}
			ldap_value_free_len(ldap_value);

			attr_len = strlen(attribute);
			zend_str_tolower(attribute, attr_len);
			zend_hash_str_update(Z_ARRVAL(tmp1), attribute, attr_len, &tmp2);
			add_index_string(&tmp1, num_attrib, attribute);

			num_attrib++;
#if (LDAP_API_VERSION > 2000) || defined(HAVE_ORALDAP)
			ldap_memfree(attribute);
#endif
			attribute = ldap_next_attribute(ldap, ldap_result_entry, ber);
		}
#if (LDAP_API_VERSION > 2000) || defined(HAVE_ORALDAP)
		if (ber != NULL) {
			ber_free(ber, 0);
		}
#endif

		add_assoc_long(&tmp1, "count", num_attrib);
		dn = ldap_get_dn(ldap, ldap_result_entry);
		if (dn) {
			add_assoc_string(&tmp1, "dn", dn);
		} else {
			add_assoc_null(&tmp1, "dn");
		}
#if (LDAP_API_VERSION > 2000) || defined(HAVE_ORALDAP)
		ldap_memfree(dn);
#else
		free(dn);
#endif

		zend_hash_index_update(Z_ARRVAL_P(return_value), num_entries, &tmp1);

		num_entries++;
		ldap_result_entry = ldap_next_entry(ldap, ldap_result_entry);
	}

	add_assoc_long(return_value, "count", num_entries);

}
/* }}} */

/* {{{ Return first attribute */
PHP_FUNCTION(ldap_first_attribute)
{
	zval *link, *result_entry;
	ldap_linkdata *ld;
	ldap_result_entry *resultentry;
	char *attribute;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "OO", &link, ldap_link_ce, &result_entry, ldap_result_entry_ce) != SUCCESS) {
		RETURN_THROWS();
	}

	ld = Z_LDAP_LINK_P(link);
	VERIFY_LDAP_LINK_CONNECTED(ld);

	resultentry = Z_LDAP_RESULT_ENTRY_P(result_entry);

	if ((attribute = ldap_first_attribute(ld->link, resultentry->data, &resultentry->ber)) == NULL) {
		RETURN_FALSE;
	} else {
		RETVAL_STRING(attribute);
#if (LDAP_API_VERSION > 2000) || defined(HAVE_ORALDAP)
		ldap_memfree(attribute);
#endif
	}
}
/* }}} */

/* {{{ Get the next attribute in result */
PHP_FUNCTION(ldap_next_attribute)
{
	zval *link, *result_entry;
	ldap_linkdata *ld;
	ldap_result_entry *resultentry;
	char *attribute;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "OO", &link, ldap_link_ce, &result_entry, ldap_result_entry_ce) != SUCCESS) {
		RETURN_THROWS();
	}

	ld = Z_LDAP_LINK_P(link);
	VERIFY_LDAP_LINK_CONNECTED(ld);

	resultentry = Z_LDAP_RESULT_ENTRY_P(result_entry);

	if (resultentry->ber == NULL) {
		php_error_docref(NULL, E_WARNING, "Called before calling ldap_first_attribute() or no attributes found in result entry");
		RETURN_FALSE;
	}

	if ((attribute = ldap_next_attribute(ld->link, resultentry->data, resultentry->ber)) == NULL) {
#if (LDAP_API_VERSION > 2000) || defined(HAVE_ORALDAP)
		if (resultentry->ber != NULL) {
			ber_free(resultentry->ber, 0);
			resultentry->ber = NULL;
		}
#endif
		RETURN_FALSE;
	} else {
		RETVAL_STRING(attribute);
#if (LDAP_API_VERSION > 2000) || defined(HAVE_ORALDAP)
		ldap_memfree(attribute);
#endif
	}
}
/* }}} */

/* {{{ Get attributes from a search result entry */
PHP_FUNCTION(ldap_get_attributes)
{
	zval *link, *result_entry;
	zval tmp;
	ldap_linkdata *ld;
	ldap_result_entry *resultentry;
	char *attribute;
	struct berval **ldap_value;
	int i, num_values, num_attrib;
	BerElement *ber;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "OO", &link, ldap_link_ce, &result_entry, ldap_result_entry_ce) != SUCCESS) {
		RETURN_THROWS();
	}

	ld = Z_LDAP_LINK_P(link);
	VERIFY_LDAP_LINK_CONNECTED(ld);

	resultentry = Z_LDAP_RESULT_ENTRY_P(result_entry);

	array_init(return_value);
	num_attrib = 0;

	attribute = ldap_first_attribute(ld->link, resultentry->data, &ber);
	while (attribute != NULL) {
		ldap_value = ldap_get_values_len(ld->link, resultentry->data, attribute);
		num_values = ldap_count_values_len(ldap_value);

		array_init(&tmp);
		add_assoc_long(&tmp, "count", num_values);
		for (i = 0; i < num_values; i++) {
			add_index_stringl(&tmp, i, ldap_value[i]->bv_val, ldap_value[i]->bv_len);
		}
		ldap_value_free_len(ldap_value);

		zend_hash_str_update(Z_ARRVAL_P(return_value), attribute, strlen(attribute), &tmp);
		add_index_string(return_value, num_attrib, attribute);

		num_attrib++;
#if (LDAP_API_VERSION > 2000) || defined(HAVE_ORALDAP)
		ldap_memfree(attribute);
#endif
		attribute = ldap_next_attribute(ld->link, resultentry->data, ber);
	}
#if (LDAP_API_VERSION > 2000) || defined(HAVE_ORALDAP)
	if (ber != NULL) {
		ber_free(ber, 0);
	}
#endif

	add_assoc_long(return_value, "count", num_attrib);
}
/* }}} */

/* {{{ Get all values with lengths from a result entry */
PHP_FUNCTION(ldap_get_values_len)
{
	zval *link, *result_entry;
	ldap_linkdata *ld;
	ldap_result_entry *resultentry;
	char *attr;
	struct berval **ldap_value_len;
	int i, num_values;
	size_t attr_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "OOp", &link, ldap_link_ce, &result_entry, ldap_result_entry_ce, &attr, &attr_len) != SUCCESS) {
		RETURN_THROWS();
	}

	ld = Z_LDAP_LINK_P(link);
	VERIFY_LDAP_LINK_CONNECTED(ld);

	resultentry = Z_LDAP_RESULT_ENTRY_P(result_entry);

	if ((ldap_value_len = ldap_get_values_len(ld->link, resultentry->data, attr)) == NULL) {
		php_error_docref(NULL, E_WARNING, "Cannot get the value(s) of attribute %s", ldap_err2string(_get_lderrno(ld->link)));
		RETURN_FALSE;
	}

	num_values = ldap_count_values_len(ldap_value_len);
	array_init(return_value);

	for (i=0; i<num_values; i++) {
		add_next_index_stringl(return_value, ldap_value_len[i]->bv_val, ldap_value_len[i]->bv_len);
	}

	add_assoc_long(return_value, "count", num_values);
	ldap_value_free_len(ldap_value_len);

}
/* }}} */

/* {{{ Get the DN of a result entry */
PHP_FUNCTION(ldap_get_dn)
{
	zval *link, *result_entry;
	ldap_linkdata *ld;
	ldap_result_entry *resultentry;
	char *text;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "OO", &link, ldap_link_ce, &result_entry, ldap_result_entry_ce) != SUCCESS) {
		RETURN_THROWS();
	}

	ld = Z_LDAP_LINK_P(link);
	VERIFY_LDAP_LINK_CONNECTED(ld);

	resultentry = Z_LDAP_RESULT_ENTRY_P(result_entry);

	text = ldap_get_dn(ld->link, resultentry->data);
	if (text != NULL) {
		RETVAL_STRING(text);
#if (LDAP_API_VERSION > 2000) || defined(HAVE_ORALDAP)
		ldap_memfree(text);
#else
		free(text);
#endif
	} else {
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ Splits DN into its component parts */
PHP_FUNCTION(ldap_explode_dn)
{
	zend_long with_attrib;
	char *dn, **ldap_value;
	size_t dn_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "pl", &dn, &dn_len, &with_attrib) != SUCCESS) {
		RETURN_THROWS();
	}

	if (!(ldap_value = ldap_explode_dn(dn, with_attrib))) {
		/* Invalid parameters were passed to ldap_explode_dn */
		RETURN_FALSE;
	}

	array_init(return_value);
	int i;
	for (i = 0; ldap_value[i] != NULL; i++) {
		add_index_string(return_value, i, ldap_value[i]);
	}
	add_assoc_long(return_value, "count", i);

	ldap_memvfree((void **)ldap_value);
}
/* }}} */

/* {{{ Convert DN to User Friendly Naming format */
PHP_FUNCTION(ldap_dn2ufn)
{
	char *dn, *ufn;
	size_t dn_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "p", &dn, &dn_len) != SUCCESS) {
		RETURN_THROWS();
	}

	ufn = ldap_dn2ufn(dn);

	if (ufn != NULL) {
		RETVAL_STRING(ufn);
#if (LDAP_API_VERSION > 2000) || defined(HAVE_ORALDAP)
		ldap_memfree(ufn);
#endif
	} else {
		RETURN_FALSE;
	}
}
/* }}} */


/* added to fix use of ldap_modify_add for doing an ldap_add, gerrit thomson. */
#define PHP_LD_FULL_ADD 0xff
/* {{{ php_ldap_do_modify */
static void php_ldap_do_modify(INTERNAL_FUNCTION_PARAMETERS, int oper, int ext)
{
	zval *link;
	ldap_linkdata *ld;
	char *dn;
	HashTable *attributes_ht;
	HashTable *server_controls_ht = NULL;
	LDAPMod **ldap_mods;
	LDAPControl **lserverctrls = NULL;
	ldap_resultdata *result;
	LDAPMessage *ldap_res;
	size_t dn_len;
	int is_full_add=0; /* flag for full add operation so ldap_mod_add can be put back into oper, gerrit THomson */

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "Oph/|h!", &link, ldap_link_ce, &dn, &dn_len, &attributes_ht, &server_controls_ht) != SUCCESS) {
		RETURN_THROWS();
	}

	ld = Z_LDAP_LINK_P(link);
	VERIFY_LDAP_LINK_CONNECTED(ld);

	uint32_t num_attribs = zend_hash_num_elements(attributes_ht);
	if (num_attribs == 0) {
		zend_argument_must_not_be_empty_error(3);
		RETURN_THROWS();
	}

	ldap_mods = safe_emalloc((num_attribs+1), sizeof(LDAPMod *), 0);
	/* Zero out the list */
	memset(ldap_mods, 0, sizeof(LDAPMod *) * (num_attribs+1));

	/* added by gerrit thomson to fix ldap_add using ldap_mod_add */
	if (oper == PHP_LD_FULL_ADD) {
		oper = LDAP_MOD_ADD;
		is_full_add = 1;
	}
	/* end additional , gerrit thomson */

	zend_string *attribute = NULL;
	zval *attribute_values = NULL;
	unsigned int attribute_index = 0;
	ZEND_HASH_FOREACH_STR_KEY_VAL(attributes_ht, attribute, attribute_values) {
		if (attribute == NULL) {
			zend_argument_value_error(3, "must be an associative array of attribute => values");
			RETVAL_FALSE;
			goto cleanup;
		}
		if (ZSTR_LEN(attribute) == 0) {
			zend_argument_value_error(3, "key must not be empty");
			RETVAL_FALSE;
			goto cleanup;
		}
		if (zend_str_has_nul_byte(attribute)) {
			zend_argument_value_error(3, "key must not contain any null bytes");
			RETVAL_FALSE;
			goto cleanup;
		}

		ldap_mods[attribute_index] = emalloc(sizeof(LDAPMod));
		ldap_mods[attribute_index]->mod_op = oper | LDAP_MOD_BVALUES;
		/* No need to duplicate the string as it is not consumed and the zend_string will not be released */
		ldap_mods[attribute_index]->mod_type = ZSTR_VAL(attribute);
		ldap_mods[attribute_index]->mod_bvalues = NULL;

		ZVAL_DEREF(attribute_values);
		/* If the attribute takes a single value it can be passed directly instead of as a list with one element */
		/* allow for arrays with one element, no allowance for arrays with none but probably not required, gerrit thomson. */
		if (Z_TYPE_P(attribute_values) != IS_ARRAY) {
			zend_string *value = php_ldap_try_get_ldap_value_from_zval(attribute_values);
			if (UNEXPECTED(value == NULL)) {
				RETVAL_FALSE;
				goto cleanup;
			}
			ldap_mods[attribute_index]->mod_bvalues = safe_emalloc(2, sizeof(struct berval *), 0);
			ldap_mods[attribute_index]->mod_bvalues[0] = (struct berval *) emalloc (sizeof(struct berval));
			/* The string will be free by php_ldap_zend_string_release_from_char_pointer() during cleanup */
			ldap_mods[attribute_index]->mod_bvalues[0]->bv_val = ZSTR_VAL(value);
			ldap_mods[attribute_index]->mod_bvalues[0]->bv_len = ZSTR_LEN(value);
			ldap_mods[attribute_index]->mod_bvalues[1] = NULL;
		} else {
			SEPARATE_ARRAY(attribute_values);
			uint32_t num_values = zend_hash_num_elements(Z_ARRVAL_P(attribute_values));
			if (num_values == 0) {
				zend_argument_value_error(3, "attribute \"%s\" must be a non-empty list of attribute values", ZSTR_VAL(attribute));
				RETVAL_FALSE;
				goto cleanup;
			}
			if (!php_ldap_is_numerically_indexed_array(Z_ARRVAL_P(attribute_values))) {
				zend_argument_value_error(3, "attribute \"%s\" must be an array of attribute values with numeric keys", ZSTR_VAL(attribute));
				RETVAL_FALSE;
				goto cleanup;
			}

			ldap_mods[attribute_index]->mod_bvalues = safe_emalloc((num_values + 1), sizeof(struct berval *), 0);
			/* Zero out the list */
			memset(ldap_mods[attribute_index]->mod_bvalues, 0, sizeof(struct berval *) * (num_values+1));

			zend_ulong attribute_value_index = 0;
			zval *attribute_value = NULL;
			ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(attribute_values), attribute_value) {
				zend_string *value = php_ldap_try_get_ldap_value_from_zval(attribute_value);
				if (UNEXPECTED(value == NULL)) {
					RETVAL_FALSE;
					goto cleanup;
				}
				ldap_mods[attribute_index]->mod_bvalues[attribute_value_index] = (struct berval *) emalloc (sizeof(struct berval));
				/* The string will be free by php_ldap_zend_string_release_from_char_pointer() during cleanup */
				ldap_mods[attribute_index]->mod_bvalues[attribute_value_index]->bv_val = ZSTR_VAL(value);
				ldap_mods[attribute_index]->mod_bvalues[attribute_value_index]->bv_len = ZSTR_LEN(value);
				attribute_value_index++;
			} ZEND_HASH_FOREACH_END();
			ldap_mods[attribute_index]->mod_bvalues[num_values] = NULL;
		}

		attribute_index++;
	} ZEND_HASH_FOREACH_END();
	ldap_mods[num_attribs] = NULL;

	if (server_controls_ht) {
		lserverctrls = php_ldap_controls_from_array(ld->link, server_controls_ht, 4);
		if (lserverctrls == NULL) {
			RETVAL_FALSE;
			goto cleanup;
		}
	}

	/* check flag to see if do_mod was called to perform full add , gerrit thomson */
	int ldap_status_code = LDAP_SUCCESS;
	int msgid;
	if (is_full_add == 1) {
		if (ext) {
			ldap_status_code = ldap_add_ext(ld->link, dn, ldap_mods, lserverctrls, NULL, &msgid);
		} else {
			ldap_status_code = ldap_add_ext_s(ld->link, dn, ldap_mods, lserverctrls, NULL);
		}
		if (ldap_status_code != LDAP_SUCCESS) {
			php_error_docref(NULL, E_WARNING, "Add: %s", ldap_err2string(ldap_status_code));
			RETVAL_FALSE;
		} else if (ext) {
			ldap_status_code = ldap_result(ld->link, msgid, 1 /* LDAP_MSG_ALL */, NULL, &ldap_res);
			if (ldap_status_code == -1) {
				php_error_docref(NULL, E_WARNING, "Add operation failed");
				RETVAL_FALSE;
				goto cleanup;
			}

			/* return a PHP control object */
			object_init_ex(return_value, ldap_result_ce);
			result = Z_LDAP_RESULT_P(return_value);
			result->result = ldap_res;
		} else RETVAL_TRUE;
	} else {
		if (ext) {
			ldap_status_code = ldap_modify_ext(ld->link, dn, ldap_mods, lserverctrls, NULL, &msgid);
		} else {
			ldap_status_code = ldap_modify_ext_s(ld->link, dn, ldap_mods, lserverctrls, NULL);
		}
		if (ldap_status_code != LDAP_SUCCESS) {
			php_error_docref(NULL, E_WARNING, "Modify: %s", ldap_err2string(ldap_status_code));
			RETVAL_FALSE;
		} else if (ext) {
			ldap_status_code = ldap_result(ld->link, msgid, 1 /* LDAP_MSG_ALL */, NULL, &ldap_res);
			if (ldap_status_code == -1) {
				php_error_docref(NULL, E_WARNING, "Modify operation failed");
				RETVAL_FALSE;
				goto cleanup;
			}

			/* return a PHP control object */
			object_init_ex(return_value, ldap_result_ce);
			result = Z_LDAP_RESULT_P(return_value);
			result->result = ldap_res;
		} else RETVAL_TRUE;
	}

cleanup:
	for (LDAPMod **ptr = ldap_mods; *ptr != NULL; ptr++) {
		LDAPMod *mod = *ptr;
		if (mod->mod_bvalues != NULL) {
			for (struct berval **bval_ptr = mod->mod_bvalues; *bval_ptr != NULL; bval_ptr++) {
				struct berval *bval = *bval_ptr;
				php_ldap_zend_string_release_from_char_pointer(bval->bv_val);
				efree(bval);
			}
			efree(mod->mod_bvalues);
		}
		efree(mod);
	}
	efree(ldap_mods);

	if (lserverctrls) {
		_php_ldap_controls_free(&lserverctrls);
	}

	return;
}
/* }}} */

/* {{{ Add entries to LDAP directory */
PHP_FUNCTION(ldap_add)
{
	/* use a newly define parameter into the do_modify so ldap_mod_add can be used the way it is supposed to be used , Gerrit THomson */
	php_ldap_do_modify(INTERNAL_FUNCTION_PARAM_PASSTHRU, PHP_LD_FULL_ADD, 0);
}
/* }}} */

/* {{{ Add entries to LDAP directory */
PHP_FUNCTION(ldap_add_ext)
{
	php_ldap_do_modify(INTERNAL_FUNCTION_PARAM_PASSTHRU, PHP_LD_FULL_ADD, 1);
}
/* }}} */

/* three functions for attribute base modifications, gerrit Thomson */

/* {{{ Replace attribute values with new ones */
PHP_FUNCTION(ldap_mod_replace)
{
	php_ldap_do_modify(INTERNAL_FUNCTION_PARAM_PASSTHRU, LDAP_MOD_REPLACE, 0);
}
/* }}} */

/* {{{ Replace attribute values with new ones */
PHP_FUNCTION(ldap_mod_replace_ext)
{
	php_ldap_do_modify(INTERNAL_FUNCTION_PARAM_PASSTHRU, LDAP_MOD_REPLACE, 1);
}
/* }}} */

/* {{{ Add attribute values to current */
PHP_FUNCTION(ldap_mod_add)
{
	php_ldap_do_modify(INTERNAL_FUNCTION_PARAM_PASSTHRU, LDAP_MOD_ADD, 0);
}
/* }}} */

/* {{{ Add attribute values to current */
PHP_FUNCTION(ldap_mod_add_ext)
{
	php_ldap_do_modify(INTERNAL_FUNCTION_PARAM_PASSTHRU, LDAP_MOD_ADD, 1);
}
/* }}} */

/* {{{ Delete attribute values */
PHP_FUNCTION(ldap_mod_del)
{
	php_ldap_do_modify(INTERNAL_FUNCTION_PARAM_PASSTHRU, LDAP_MOD_DELETE, 0);
}
/* }}} */

/* {{{ Delete attribute values */
PHP_FUNCTION(ldap_mod_del_ext)
{
	php_ldap_do_modify(INTERNAL_FUNCTION_PARAM_PASSTHRU, LDAP_MOD_DELETE, 1);
}
/* }}} */

/* {{{ php_ldap_do_delete */
static void php_ldap_do_delete(INTERNAL_FUNCTION_PARAMETERS, int ext)
{
	zval *link;
	HashTable *server_controls_ht = NULL;
	ldap_linkdata *ld;
	LDAPControl **lserverctrls = NULL;
	ldap_resultdata *result;
	LDAPMessage *ldap_res;
	char *dn;
	int rc, msgid;
	size_t dn_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "Op|h!", &link, ldap_link_ce, &dn, &dn_len, &server_controls_ht) != SUCCESS) {
		RETURN_THROWS();
	}

	ld = Z_LDAP_LINK_P(link);
	VERIFY_LDAP_LINK_CONNECTED(ld);

	if (server_controls_ht) {
		lserverctrls = php_ldap_controls_from_array(ld->link, server_controls_ht, 3);
		if (lserverctrls == NULL) {
			RETVAL_FALSE;
			goto cleanup;
		}
	}

	if (ext) {
		rc = ldap_delete_ext(ld->link, dn, lserverctrls, NULL, &msgid);
	} else {
		rc = ldap_delete_ext_s(ld->link, dn, lserverctrls, NULL);
	}
	if (rc != LDAP_SUCCESS) {
		php_error_docref(NULL, E_WARNING, "Delete: %s", ldap_err2string(rc));
		RETVAL_FALSE;
		goto cleanup;
	} else if (ext) {
		rc = ldap_result(ld->link, msgid, 1 /* LDAP_MSG_ALL */, NULL, &ldap_res);
		if (rc == -1) {
			php_error_docref(NULL, E_WARNING, "Delete operation failed");
			RETVAL_FALSE;
			goto cleanup;
		}

		/* return a PHP control object */
		object_init_ex(return_value, ldap_result_ce);
		result = Z_LDAP_RESULT_P(return_value);
		result->result = ldap_res;
	} else {
		RETVAL_TRUE;
	}

cleanup:
	if (lserverctrls) {
		_php_ldap_controls_free(&lserverctrls);
	}

	return;
}
/* }}} */

/* {{{ Delete an entry from a directory */
PHP_FUNCTION(ldap_delete)
{
	php_ldap_do_delete(INTERNAL_FUNCTION_PARAM_PASSTHRU, 0);
}
/* }}} */

/* {{{ Delete an entry from a directory */
PHP_FUNCTION(ldap_delete_ext)
{
	php_ldap_do_delete(INTERNAL_FUNCTION_PARAM_PASSTHRU, 1);
}
/* }}} */

/* {{{ Perform multiple modifications as part of one operation */
PHP_FUNCTION(ldap_modify_batch)
{
	zval *link;
	char *dn;
	size_t dn_len;
	HashTable *modifications;
	HashTable *server_controls_ht = NULL;
	LDAPControl **lserverctrls = NULL;

	/*
	$mods = [
		[
			"attrib" => "unicodePwd",
			"modtype" => LDAP_MODIFY_BATCH_REMOVE,
			"values" => [$old_pwd]
		],
		[
			"attrib" => "unicodePwd",
			"modtype" => LDAP_MODIFY_BATCH_ADD,
			"values" => [$new_pwd]
		],
		[
			"attrib" => "userPrincipalName",
			"modtype" => LDAP_MODIFY_BATCH_REPLACE,
			"values" => ["janitor@corp.contoso.com"]
		],
		[
			"attrib" => "userCert",
			"modtype" => LDAP_MODIFY_BATCH_REMOVE_ALL
		],
	];
	*/

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "Oph/|h!", &link, ldap_link_ce, &dn, &dn_len, &modifications, &server_controls_ht) != SUCCESS) {
		RETURN_THROWS();
	}

	ldap_linkdata *ld = Z_LDAP_LINK_P(link);
	VERIFY_LDAP_LINK_CONNECTED(ld);

	/* perform validation */
	/* make sure the top level is a normal array */
	if (zend_hash_num_elements(modifications) == 0) {
		zend_argument_must_not_be_empty_error(3);
		RETURN_THROWS();
	}
	if (!php_ldap_is_numerically_indexed_array(modifications)) {
		zend_argument_value_error(3, "must be an array with numeric keys");
		RETURN_THROWS();
	}

	zval *modification_zv = NULL;
	ZEND_HASH_FOREACH_VAL(modifications, modification_zv) {
		ZVAL_DEREF(modification_zv);
		if (Z_TYPE_P(modification_zv) != IS_ARRAY) {
			zend_argument_type_error(3, "must only contain arrays");
			RETURN_THROWS();
		}

		SEPARATE_ARRAY(modification_zv);
		const HashTable *modification = Z_ARRVAL_P(modification_zv);
		uint32_t modification_size = zend_hash_num_elements(modification);

		if (modification_size != 2 && modification_size != 3) {
			zend_argument_value_error(3, "a modification entry must only contain the keys "
				"\"" LDAP_MODIFY_BATCH_ATTRIB "\", \"" LDAP_MODIFY_BATCH_MODTYPE "\", and \"" LDAP_MODIFY_BATCH_VALUES "\"");
			RETURN_THROWS();
		}

		const zval *attrib = zend_hash_str_find(modification, LDAP_MODIFY_BATCH_ATTRIB, strlen(LDAP_MODIFY_BATCH_ATTRIB));
		if (UNEXPECTED(attrib == NULL)) {
			zend_argument_value_error(3, "a modification entry must contain the \"" LDAP_MODIFY_BATCH_ATTRIB "\" option");
			RETURN_THROWS();
		}

		ZVAL_DEREF(attrib);
		if (UNEXPECTED(Z_TYPE_P(attrib) != IS_STRING)) {
			zend_argument_type_error(3, "the value for option \"" LDAP_MODIFY_BATCH_ATTRIB "\" must be of type string, %s given", zend_zval_value_name(attrib));
			RETURN_THROWS();
		}
		if (zend_str_has_nul_byte(Z_STR_P(attrib))) {
			zend_argument_value_error(3, "the value for option \"" LDAP_MODIFY_BATCH_ATTRIB "\" must not contain null bytes");
			RETURN_THROWS();
		}

		const zval *modtype_zv = zend_hash_str_find(modification, LDAP_MODIFY_BATCH_MODTYPE, strlen(LDAP_MODIFY_BATCH_MODTYPE));
		if (UNEXPECTED(modtype_zv == NULL)) {
			zend_argument_value_error(3, "a modification entry must contain the \"" LDAP_MODIFY_BATCH_MODTYPE "\" option");
			RETURN_THROWS();
		}

		ZVAL_DEREF(modtype_zv);
		if (UNEXPECTED(Z_TYPE_P(modtype_zv) != IS_LONG)) {
			zend_argument_type_error(3, "the value for option \"" LDAP_MODIFY_BATCH_MODTYPE "\" must be of type int, %s given", zend_zval_value_name(attrib));
			RETURN_THROWS();
		}
		zend_long modtype = Z_LVAL_P(modtype_zv);
		if (
			modtype != LDAP_MODIFY_BATCH_ADD &&
			modtype != LDAP_MODIFY_BATCH_REMOVE &&
			modtype != LDAP_MODIFY_BATCH_REPLACE &&
			modtype != LDAP_MODIFY_BATCH_REMOVE_ALL
		) {
			zend_argument_value_error(3, "the value for option \"" LDAP_MODIFY_BATCH_MODTYPE "\" must be"
				" LDAP_MODIFY_BATCH_ADD, LDAP_MODIFY_BATCH_REMOVE, LDAP_MODIFY_BATCH_REPLACE,"
				" or LDAP_MODIFY_BATCH_REMOVE_ALL");
			RETURN_THROWS();
		}
		/* We assume that the modification array is well-formed and only ever contains an extra "values" key */
		if (modtype == LDAP_MODIFY_BATCH_REMOVE_ALL && modification_size == 3) {
			zend_argument_value_error(3, "a modification entry must not contain the "
				"\"" LDAP_MODIFY_BATCH_VALUES "\" option when option \"" LDAP_MODIFY_BATCH_MODTYPE "\" "
				"is LDAP_MODIFY_BATCH_REMOVE_ALL");
			RETURN_THROWS();
		}

		zval *modification_values_zv = zend_hash_str_find(modification, LDAP_MODIFY_BATCH_VALUES, strlen(LDAP_MODIFY_BATCH_VALUES));
		if (modification_values_zv == NULL) {
			if (modtype != LDAP_MODIFY_BATCH_REMOVE_ALL) {
				zend_argument_value_error(3, "a modification entry must contain the "
					"\"" LDAP_MODIFY_BATCH_VALUES "\" option when the \"" LDAP_MODIFY_BATCH_MODTYPE "\" option "
					"is not LDAP_MODIFY_BATCH_REMOVE_ALL");
				RETURN_THROWS();
			}
			continue;
		}

		ZVAL_DEREF(modification_values_zv);
		if (Z_TYPE_P(modification_values_zv) != IS_ARRAY) {
			zend_argument_type_error(3, "the value for option \"" LDAP_MODIFY_BATCH_VALUES "\" must be of type array, %s given", zend_zval_value_name(attrib));
			RETURN_THROWS();
		}

		SEPARATE_ARRAY(modification_values_zv);
		const HashTable *modification_values = Z_ARRVAL_P(modification_values_zv);
		/* is the array not empty? */
		uint32_t num_modvals = zend_hash_num_elements(modification_values);
		if (num_modvals == 0) {
			zend_argument_value_error(3, "the value for option \"" LDAP_MODIFY_BATCH_VALUES "\" must not be empty");
			RETURN_THROWS();
		}
		if (!php_ldap_is_numerically_indexed_array(modification_values)) {
			zend_argument_value_error(3, "the value for option \"" LDAP_MODIFY_BATCH_VALUES "\" must be an array with numeric keys");
			RETURN_THROWS();
		}
	} ZEND_HASH_FOREACH_END();
	/* validation of modifications array was successful */

	/* Check that the LDAP server controls array is valid */
	if (server_controls_ht) {
		lserverctrls = php_ldap_controls_from_array(ld->link, server_controls_ht, 4);
		if (lserverctrls == NULL) {
			_php_ldap_controls_free(&lserverctrls);
			RETURN_FALSE;
		}
	}

	/* allocate array of modifications */
	uint32_t num_mods = zend_hash_num_elements(modifications);
	LDAPMod **ldap_mods = safe_emalloc((num_mods+1), sizeof(LDAPMod *), 0);

	/* for each modification */
	zend_ulong modification_index = 0;
	ZEND_HASH_FOREACH_VAL(modifications, modification_zv) {
		ldap_mods[modification_index] = safe_emalloc(1, sizeof(LDAPMod), 0);

		zval *attrib_zv = zend_hash_str_find_deref(Z_ARRVAL_P(modification_zv), LDAP_MODIFY_BATCH_ATTRIB, strlen(LDAP_MODIFY_BATCH_ATTRIB));
		ZEND_ASSERT(Z_TYPE_P(attrib_zv) == IS_STRING);
		zval *modtype_zv = zend_hash_str_find_deref(Z_ARRVAL_P(modification_zv), LDAP_MODIFY_BATCH_MODTYPE, strlen(LDAP_MODIFY_BATCH_MODTYPE));
		ZEND_ASSERT(Z_TYPE_P(modtype_zv) == IS_LONG);
		zval *modification_values = zend_hash_str_find_deref(Z_ARRVAL_P(modification_zv), LDAP_MODIFY_BATCH_VALUES, strlen(LDAP_MODIFY_BATCH_VALUES));
		ZEND_ASSERT(modification_values == NULL || Z_TYPE_P(modification_values) == IS_ARRAY);

		/* map the modification type */
		int ldap_operation;
		switch (Z_LVAL_P(modtype_zv)) {
			case LDAP_MODIFY_BATCH_ADD:
				ldap_operation = LDAP_MOD_ADD;
				break;
			case LDAP_MODIFY_BATCH_REMOVE:
			case LDAP_MODIFY_BATCH_REMOVE_ALL:
				ldap_operation = LDAP_MOD_DELETE;
				break;
			case LDAP_MODIFY_BATCH_REPLACE:
				ldap_operation = LDAP_MOD_REPLACE;
				break;
			EMPTY_SWITCH_DEFAULT_CASE();
		}

		/* fill in the basic info */
		ldap_mods[modification_index]->mod_op = ldap_operation | LDAP_MOD_BVALUES;
		ldap_mods[modification_index]->mod_type = estrndup(Z_STRVAL_P(attrib_zv), Z_STRLEN_P(attrib_zv));

		if (Z_LVAL_P(modtype_zv) == LDAP_MODIFY_BATCH_REMOVE_ALL) {
			/* no values */
			ldap_mods[modification_index]->mod_bvalues = NULL;
		} else {
			/* allocate space for the values as part of this modification */
			uint32_t num_modification_values = zend_hash_num_elements(Z_ARRVAL_P(modification_values));
			ldap_mods[modification_index]->mod_bvalues = safe_emalloc((num_modification_values+1), sizeof(struct berval *), 0);

			/* for each value */
			zend_ulong value_index = 0;
			zval *modification_value_zv = NULL;
			ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(modification_values), modification_value_zv) {
				zend_string *modval = zval_get_string(modification_value_zv);
				if (EG(exception)) {
					RETVAL_FALSE;
					ldap_mods[modification_index]->mod_bvalues[value_index] = NULL;
					num_mods = modification_index + 1;
					goto cleanup;
				}

				/* allocate the data struct */
				ldap_mods[modification_index]->mod_bvalues[value_index] = safe_emalloc(1, sizeof(struct berval), 0);

				/* fill it */
				ldap_mods[modification_index]->mod_bvalues[value_index]->bv_len = ZSTR_LEN(modval);
				ldap_mods[modification_index]->mod_bvalues[value_index]->bv_val = estrndup(ZSTR_VAL(modval), ZSTR_LEN(modval));
				zend_string_release(modval);
				value_index++;
			} ZEND_HASH_FOREACH_END();

			/* NULL-terminate values */
			ldap_mods[modification_index]->mod_bvalues[num_modification_values] = NULL;
		}

		modification_index++;
	} ZEND_HASH_FOREACH_END();

	/* NULL-terminate modifications */
	ldap_mods[num_mods] = NULL;

	/* perform (finally) */
	int ldap_status = ldap_modify_ext_s(ld->link, dn, ldap_mods, lserverctrls, NULL);
	if (ldap_status != LDAP_SUCCESS) {
		php_error_docref(NULL, E_WARNING, "Batch Modify: %s", ldap_err2string(ldap_status));
		RETVAL_FALSE;
	} else {
		RETVAL_TRUE;
	}

	/* clean up */
	cleanup: {
		for (uint32_t i = 0; i < num_mods; i++) {
			/* attribute */
			efree(ldap_mods[i]->mod_type);

			if (ldap_mods[i]->mod_bvalues != NULL) {
				/* each BER value */
				for (int j = 0; ldap_mods[i]->mod_bvalues[j] != NULL; j++) {
					/* free the data bytes */
					efree(ldap_mods[i]->mod_bvalues[j]->bv_val);

					/* free the bvalue struct */
					efree(ldap_mods[i]->mod_bvalues[j]);
				}

				/* the BER value array */
				efree(ldap_mods[i]->mod_bvalues);
			}

			/* the modification */
			efree(ldap_mods[i]);
		}

		/* the modifications array */
		efree(ldap_mods);

		if (lserverctrls) {
			_php_ldap_controls_free(&lserverctrls);
		}
	}
}
/* }}} */

/* {{{ Get the current ldap error number */
PHP_FUNCTION(ldap_errno)
{
	zval *link;
	ldap_linkdata *ld;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "O", &link, ldap_link_ce) != SUCCESS) {
		RETURN_THROWS();
	}

	ld = Z_LDAP_LINK_P(link);
	VERIFY_LDAP_LINK_CONNECTED(ld);

	RETURN_LONG(_get_lderrno(ld->link));
}
/* }}} */

/* {{{ Convert error number to error string */
PHP_FUNCTION(ldap_err2str)
{
	zend_long perrno;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &perrno) != SUCCESS) {
		RETURN_THROWS();
	}

	RETURN_STRING(ldap_err2string(perrno));
}
/* }}} */

/* {{{ Get the current ldap error string */
PHP_FUNCTION(ldap_error)
{
	zval *link;
	ldap_linkdata *ld;
	int ld_errno;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "O", &link, ldap_link_ce) != SUCCESS) {
		RETURN_THROWS();
	}

	ld = Z_LDAP_LINK_P(link);
	VERIFY_LDAP_LINK_CONNECTED(ld);

	ld_errno = _get_lderrno(ld->link);

	RETURN_STRING(ldap_err2string(ld_errno));
}
/* }}} */

/* {{{ Determine if an entry has a specific value for one of its attributes */
PHP_FUNCTION(ldap_compare)
{
	zval *link;
	char *dn, *attr;
	size_t dn_len, attr_len;
	HashTable *server_controls_ht = NULL;
	ldap_linkdata *ld;
	LDAPControl **lserverctrls = NULL;
	int ldap_errno;
	struct berval lvalue;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "Opps|h!",
		&link, ldap_link_ce,
		&dn, &dn_len,
		&attr, &attr_len,
		&lvalue.bv_val, &lvalue.bv_len,
		&server_controls_ht
	) != SUCCESS) {
		RETURN_THROWS();
	}

	ld = Z_LDAP_LINK_P(link);
	VERIFY_LDAP_LINK_CONNECTED(ld);

	if (server_controls_ht) {
		lserverctrls = php_ldap_controls_from_array(ld->link, server_controls_ht, 5);
		if (lserverctrls == NULL) {
			RETVAL_FALSE;
			goto cleanup;
		}
	}

	ldap_errno = ldap_compare_ext_s(ld->link, dn, attr, &lvalue, lserverctrls, NULL);

	switch (ldap_errno) {
		case LDAP_COMPARE_TRUE:
			RETVAL_TRUE;
			break;

		case LDAP_COMPARE_FALSE:
			RETVAL_FALSE;
			break;

		default:
			php_error_docref(NULL, E_WARNING, "Compare: %s", ldap_err2string(ldap_errno));
			RETVAL_LONG(-1);
	}

cleanup:
	if (lserverctrls) {
		_php_ldap_controls_free(&lserverctrls);
	}

	return;
}
/* }}} */

#if (LDAP_API_VERSION > 2000) || defined(HAVE_ORALDAP)
/* {{{ Get the current value of various session-wide parameters */
PHP_FUNCTION(ldap_get_option)
{
	zval *link = NULL, *retval;
	ldap_linkdata *ld;
	zend_long option;
	LDAP *ldap;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "O!lz", &link, ldap_link_ce, &option, &retval) != SUCCESS) {
		RETURN_THROWS();
	}

	if (!link) {
		ldap = NULL;
	} else {
		ld = Z_LDAP_LINK_P(link);
		VERIFY_LDAP_LINK_CONNECTED(ld);
		ldap = ld->link;
	}

	switch (option) {
	/* options with int value */
	case LDAP_OPT_DEREF:
	case LDAP_OPT_SIZELIMIT:
	case LDAP_OPT_TIMELIMIT:
	case LDAP_OPT_PROTOCOL_VERSION:
	case LDAP_OPT_ERROR_NUMBER:
	case LDAP_OPT_REFERRALS:
#ifdef LDAP_OPT_RESTART
	case LDAP_OPT_RESTART:
#endif
#ifdef LDAP_OPT_X_SASL_NOCANON
	case LDAP_OPT_X_SASL_NOCANON:
#endif
#ifdef LDAP_OPT_X_TLS_REQUIRE_CERT
	case LDAP_OPT_X_TLS_REQUIRE_CERT:
#endif
#ifdef LDAP_OPT_X_TLS_CRLCHECK
	case LDAP_OPT_X_TLS_CRLCHECK:
#endif
#ifdef LDAP_OPT_X_TLS_PROTOCOL_MIN
	case LDAP_OPT_X_TLS_PROTOCOL_MIN:
#endif
#ifdef LDAP_OPT_X_TLS_PROTOCOL_MAX
	case LDAP_OPT_X_TLS_PROTOCOL_MAX:
#endif
#ifdef LDAP_OPT_X_KEEPALIVE_IDLE
	case LDAP_OPT_X_KEEPALIVE_IDLE:
	case LDAP_OPT_X_KEEPALIVE_PROBES:
	case LDAP_OPT_X_KEEPALIVE_INTERVAL:
#endif
		{
			int val;

			if (ldap_get_option(ldap, option, &val)) {
				RETURN_FALSE;
			}
			ZEND_TRY_ASSIGN_REF_LONG(retval, val);
		} break;
#ifdef LDAP_OPT_NETWORK_TIMEOUT
	case LDAP_OPT_NETWORK_TIMEOUT:
		{
			struct timeval *timeout = NULL;

			if (ldap_get_option(ldap, LDAP_OPT_NETWORK_TIMEOUT, (void *) &timeout)) {
				if (timeout) {
					ldap_memfree(timeout);
				}
				RETURN_FALSE;
			}
			if (!timeout) {
				RETURN_FALSE;
			}
			ZEND_TRY_ASSIGN_REF_LONG(retval, timeout->tv_sec);
			ldap_memfree(timeout);
		} break;
#elif defined(LDAP_X_OPT_CONNECT_TIMEOUT)
	case LDAP_X_OPT_CONNECT_TIMEOUT:
		{
			int timeout;

			if (ldap_get_option(ldap, LDAP_X_OPT_CONNECT_TIMEOUT, &timeout)) {
				RETURN_FALSE;
			}
			ZEND_TRY_ASSIGN_REF_LONG(retval, (timeout / 1000));
		} break;
#endif
#ifdef LDAP_OPT_TIMEOUT
	case LDAP_OPT_TIMEOUT:
		{
			struct timeval *timeout = NULL;

			if (ldap_get_option(ldap, LDAP_OPT_TIMEOUT, (void *) &timeout)) {
				if (timeout) {
					ldap_memfree(timeout);
				}
				RETURN_FALSE;
			}
			if (!timeout) {
				RETURN_FALSE;
			}
			ZEND_TRY_ASSIGN_REF_LONG(retval, timeout->tv_sec);
			ldap_memfree(timeout);
		} break;
#endif
	/* options with string value */
	case LDAP_OPT_ERROR_STRING:
#ifdef LDAP_OPT_HOST_NAME
	case LDAP_OPT_HOST_NAME:
#endif
#ifdef HAVE_LDAP_SASL
	case LDAP_OPT_X_SASL_MECH:
	case LDAP_OPT_X_SASL_REALM:
	case LDAP_OPT_X_SASL_AUTHCID:
	case LDAP_OPT_X_SASL_AUTHZID:
#endif
#ifdef LDAP_OPT_X_SASL_USERNAME
	case LDAP_OPT_X_SASL_USERNAME:
#endif
#if (LDAP_API_VERSION > 2000)
	case LDAP_OPT_X_TLS_CACERTDIR:
	case LDAP_OPT_X_TLS_CACERTFILE:
	case LDAP_OPT_X_TLS_CERTFILE:
	case LDAP_OPT_X_TLS_CIPHER_SUITE:
	case LDAP_OPT_X_TLS_KEYFILE:
	case LDAP_OPT_X_TLS_RANDOM_FILE:
#endif
#ifdef LDAP_OPT_X_TLS_PACKAGE
	case LDAP_OPT_X_TLS_PACKAGE:
#endif
#ifdef LDAP_OPT_X_TLS_CRLFILE
	case LDAP_OPT_X_TLS_CRLFILE:
#endif
#ifdef LDAP_OPT_X_TLS_DHFILE
	case LDAP_OPT_X_TLS_DHFILE:
#endif
#ifdef LDAP_OPT_MATCHED_DN
	case LDAP_OPT_MATCHED_DN:
#endif
		{
			char *val = NULL;

			if (ldap_get_option(ldap, option, &val) || val == NULL || *val == '\0') {
				if (val) {
					ldap_memfree(val);
				}
				RETURN_FALSE;
			}
			ZEND_TRY_ASSIGN_REF_STRING(retval, val);
			ldap_memfree(val);
		} break;
	case LDAP_OPT_SERVER_CONTROLS:
	case LDAP_OPT_CLIENT_CONTROLS:
		{
			LDAPControl **ctrls = NULL;

			if (ldap_get_option(ldap, option, &ctrls) || ctrls == NULL) {
				if (ctrls) {
					ldap_memfree(ctrls);
				}
				RETURN_FALSE;
			}
			_php_ldap_controls_to_array(ldap, ctrls, retval, 1);
		} break;
/* options not implemented
	case LDAP_OPT_API_INFO:
	case LDAP_OPT_API_FEATURE_INFO:
*/
	default:
		zend_argument_value_error(2, "must be a valid LDAP option");
		RETURN_THROWS();
	}
	RETURN_TRUE;
}
/* }}} */

/* {{{ Set the value of various session-wide parameters */
PHP_FUNCTION(ldap_set_option)
{
	zval *link = NULL, *newval;
	ldap_linkdata *ld;
	LDAP *ldap;
	zend_long option;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "O!lz", &link, ldap_link_ce, &option, &newval) != SUCCESS) {
		RETURN_THROWS();
	}

	if (!link) {
		ldap = NULL;
	} else {
		ld = Z_LDAP_LINK_P(link);
		VERIFY_LDAP_LINK_CONNECTED(ld);
		ldap = ld->link;
	}

	switch (option) {
	/* TLS options with int value */
#ifdef LDAP_OPT_X_TLS_REQUIRE_CERT
	case LDAP_OPT_X_TLS_REQUIRE_CERT:
#endif
#ifdef LDAP_OPT_X_TLS_CRLCHECK
	case LDAP_OPT_X_TLS_CRLCHECK:
#endif
#ifdef LDAP_OPT_X_TLS_PROTOCOL_MIN
	case LDAP_OPT_X_TLS_PROTOCOL_MIN:
#endif
#ifdef LDAP_OPT_X_TLS_PROTOCOL_MAX
	case LDAP_OPT_X_TLS_PROTOCOL_MAX:
#endif
		/* TLS option change requires resetting TLS context */
		LDAPG(tls_newctx) = true;
		ZEND_FALLTHROUGH;
	/* other options with int value */
	case LDAP_OPT_DEREF:
	case LDAP_OPT_SIZELIMIT:
	case LDAP_OPT_TIMELIMIT:
	case LDAP_OPT_PROTOCOL_VERSION:
	case LDAP_OPT_ERROR_NUMBER:
#ifdef LDAP_OPT_DEBUG_LEVEL
	case LDAP_OPT_DEBUG_LEVEL:
#endif
#ifdef LDAP_OPT_X_KEEPALIVE_IDLE
	case LDAP_OPT_X_KEEPALIVE_IDLE:
	case LDAP_OPT_X_KEEPALIVE_PROBES:
	case LDAP_OPT_X_KEEPALIVE_INTERVAL:
#endif
		{
			bool failed = false;
			zend_long lval = zval_try_get_long(newval, &failed);
			if (failed) {
				zend_argument_type_error(3, "must be of type int for the given option, %s given", zend_zval_value_name(newval));
				RETURN_THROWS();
			}

			if (ZEND_LONG_EXCEEDS_INT(lval)) {
				zend_argument_value_error(3, "is too large");
				RETURN_THROWS();
			}
			int val = (int)lval;
			if (ldap_set_option(ldap, option, &val)) {
				RETURN_FALSE;
			}
		} break;
#ifdef LDAP_OPT_NETWORK_TIMEOUT
	case LDAP_OPT_NETWORK_TIMEOUT:
		{
			struct timeval timeout;
			bool failed = false;
			zend_long lval = zval_try_get_long(newval, &failed);
			if (failed) {
				zend_argument_type_error(3, "must be of type int for the LDAP_OPT_NETWORK_TIMEOUT option, %s given", zend_zval_value_name(newval));
				RETURN_THROWS();
			}
			timeout.tv_sec = lval;
			timeout.tv_usec = 0;
			if (ldap_set_option(ldap, LDAP_OPT_NETWORK_TIMEOUT, (void *) &timeout)) {
				RETURN_FALSE;
			}
		} break;
#elif defined(LDAP_X_OPT_CONNECT_TIMEOUT)
	case LDAP_X_OPT_CONNECT_TIMEOUT:
		{
			int timeout;
			bool failed = false;
			zend_long lval = zval_try_get_long(newval, &failed);
			if (failed) {
				zend_argument_type_error(3, "must be of type int for the LDAP_X_OPT_CONNECT_TIMEOUT option, %s given", zend_zval_value_name(newval));
				RETURN_THROWS();
			}
			timeout = 1000 * lval; /* Convert to milliseconds */
			if (ldap_set_option(ldap, LDAP_X_OPT_CONNECT_TIMEOUT, &timeout)) {
				RETURN_FALSE;
			}
		} break;
#endif
#ifdef LDAP_OPT_TIMEOUT
	case LDAP_OPT_TIMEOUT:
		{
			struct timeval timeout;

			bool failed = false;
			zend_long lval = zval_try_get_long(newval, &failed);
			if (failed) {
				zend_argument_type_error(3, "must be of type int for the LDAP_OPT_TIMEOUT option, %s given", zend_zval_value_name(newval));
				RETURN_THROWS();
			}
			timeout.tv_sec = lval;
			timeout.tv_usec = 0;
			if (ldap_set_option(ldap, LDAP_OPT_TIMEOUT, (void *) &timeout)) {
				RETURN_FALSE;
			}
		} break;
#endif
	/* TLS options with string value */
#if (LDAP_API_VERSION > 2000)
	case LDAP_OPT_X_TLS_CACERTDIR:
	case LDAP_OPT_X_TLS_CACERTFILE:
	case LDAP_OPT_X_TLS_CERTFILE:
	case LDAP_OPT_X_TLS_CIPHER_SUITE:
	case LDAP_OPT_X_TLS_KEYFILE:
	case LDAP_OPT_X_TLS_RANDOM_FILE:
#endif
#ifdef LDAP_OPT_X_TLS_CRLFILE
	case LDAP_OPT_X_TLS_CRLFILE:
#endif
#ifdef LDAP_OPT_X_TLS_DHFILE
	case LDAP_OPT_X_TLS_DHFILE:
#endif
		/* TLS option change requires resetting TLS context */
		LDAPG(tls_newctx) = true;
		ZEND_FALLTHROUGH;
	/* other options with string value */
	case LDAP_OPT_ERROR_STRING:
#ifdef LDAP_OPT_HOST_NAME
	case LDAP_OPT_HOST_NAME:
#endif
#ifdef HAVE_LDAP_SASL
	case LDAP_OPT_X_SASL_MECH:
	case LDAP_OPT_X_SASL_REALM:
	case LDAP_OPT_X_SASL_AUTHCID:
	case LDAP_OPT_X_SASL_AUTHZID:
#endif
#ifdef LDAP_OPT_MATCHED_DN
	case LDAP_OPT_MATCHED_DN:
#endif
		{
			zend_string *val = zval_try_get_string(newval);
			if (val == NULL) {
				RETURN_THROWS();
			}
			if (ldap_set_option(ldap, option, ZSTR_VAL(val))) {
				zend_string_release(val);
				RETURN_FALSE;
			}
			zend_string_release(val);
		} break;
		/* options with boolean value */
	case LDAP_OPT_REFERRALS:
#ifdef LDAP_OPT_RESTART
	case LDAP_OPT_RESTART:
#endif
#ifdef LDAP_OPT_X_SASL_NOCANON
	case LDAP_OPT_X_SASL_NOCANON:
#endif
		{
			void *val = zend_is_true(newval) ? LDAP_OPT_ON : LDAP_OPT_OFF;
			if (ldap_set_option(ldap, option, val)) {
				RETURN_FALSE;
			}
		} break;
		/* options with control list value */
	case LDAP_OPT_SERVER_CONTROLS:
	case LDAP_OPT_CLIENT_CONTROLS:
		{
			LDAPControl **ctrls;
			int rc;

			if (Z_TYPE_P(newval) != IS_ARRAY) {
				zend_argument_type_error(3, "must be of type array for the LDAP_OPT_CLIENT_CONTROLS option, %s given", zend_zval_value_name(newval));
				RETURN_THROWS();
			}

			ctrls = php_ldap_controls_from_array(ldap, Z_ARRVAL_P(newval), 3);

			if (ctrls == NULL) {
				RETURN_FALSE;
			} else {
				rc = ldap_set_option(ldap, option, ctrls);
				_php_ldap_controls_free(&ctrls);
				if (rc != LDAP_SUCCESS) {
					RETURN_FALSE;
				}
			}
		} break;
	default:
		zend_argument_value_error(2, "must be a valid LDAP option");
		RETURN_THROWS();
	}
	RETURN_TRUE;
}
/* }}} */

#ifdef HAVE_LDAP_PARSE_RESULT
/* {{{ Extract information from result */
PHP_FUNCTION(ldap_parse_result)
{
	zval *link, *result, *errcode, *matcheddn = NULL, *errmsg = NULL, *referrals = NULL, *serverctrls = NULL;
	ldap_linkdata *ld;
	ldap_resultdata *ldap_result;
	LDAPControl **lserverctrls = NULL;
	char **lreferrals, **refp;
	char *lmatcheddn, *lerrmsg;
	int rc, lerrcode;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "OOz|zzzz", &link, ldap_link_ce, &result, ldap_result_ce, &errcode, &matcheddn, &errmsg, &referrals, &serverctrls) != SUCCESS) {
		RETURN_THROWS();
	}

	ld = Z_LDAP_LINK_P(link);
	VERIFY_LDAP_LINK_CONNECTED(ld);

	ldap_result = Z_LDAP_RESULT_P(result);
	VERIFY_LDAP_RESULT_OPEN(ldap_result);

	rc = ldap_parse_result(ld->link, ldap_result->result, &lerrcode,
				matcheddn ? &lmatcheddn : NULL,
				errmsg ? &lerrmsg : NULL,
				referrals ? &lreferrals : NULL,
				serverctrls ? &lserverctrls : NULL,
				0);
	if (rc != LDAP_SUCCESS) {
		php_error_docref(NULL, E_WARNING, "Unable to parse result: %s", ldap_err2string(rc));
		RETURN_FALSE;
	}

	ZEND_TRY_ASSIGN_REF_LONG(errcode, lerrcode);

	if (serverctrls) {
		_php_ldap_controls_to_array(ld->link, lserverctrls, serverctrls, 0);
	}
	if (referrals) {
		referrals = zend_try_array_init(referrals);
		if (!referrals) {
			RETURN_THROWS();
		}
		if (lreferrals != NULL) {
			refp = lreferrals;
			while (*refp) {
				add_next_index_string(referrals, *refp);
				refp++;
			}
			ldap_memvfree((void**)lreferrals);
		}
	}
	if (errmsg) {
		if (lerrmsg == NULL) {
			ZEND_TRY_ASSIGN_REF_EMPTY_STRING(errmsg);
		} else {
			ZEND_TRY_ASSIGN_REF_STRING(errmsg, lerrmsg);
			ldap_memfree(lerrmsg);
		}
	}
	if (matcheddn) {
		if (lmatcheddn == NULL) {
			ZEND_TRY_ASSIGN_REF_EMPTY_STRING(matcheddn);
		} else {
			ZEND_TRY_ASSIGN_REF_STRING(matcheddn, lmatcheddn);
			ldap_memfree(lmatcheddn);
		}
	}

	RETURN_TRUE;
}
/* }}} */
#endif

/* {{{ Extended operation response parsing, Pierangelo Masarati */
#ifdef HAVE_LDAP_PARSE_EXTENDED_RESULT
/* {{{ Extract information from extended operation result */
PHP_FUNCTION(ldap_parse_exop)
{
	zval *link, *result, *retdata = NULL, *retoid = NULL;
	ldap_linkdata *ld;
	ldap_resultdata *ldap_result;
	char *lretoid;
	struct berval *lretdata;
	int rc;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "OO|zz", &link, ldap_link_ce, &result, ldap_result_ce, &retdata, &retoid) != SUCCESS) {
		RETURN_THROWS();
	}

	ld = Z_LDAP_LINK_P(link);
	VERIFY_LDAP_LINK_CONNECTED(ld);

	ldap_result = Z_LDAP_RESULT_P(result);
	VERIFY_LDAP_RESULT_OPEN(ldap_result);

	rc = ldap_parse_extended_result(ld->link, ldap_result->result,
				retoid ? &lretoid: NULL,
				retdata ? &lretdata: NULL,
				0);
	if (rc != LDAP_SUCCESS) {
		php_error_docref(NULL, E_WARNING, "Unable to parse extended operation result: %s", ldap_err2string(rc));
		RETURN_FALSE;
	}

	if (retoid) {
		if (lretoid == NULL) {
			ZEND_TRY_ASSIGN_REF_EMPTY_STRING(retoid);
		} else {
			ZEND_TRY_ASSIGN_REF_STRING(retoid, lretoid);
			ldap_memfree(lretoid);
		}
	}

	if (retdata) {
		/* use arg #3 as the data returned by the server */
		if (lretdata == NULL) {
			ZEND_TRY_ASSIGN_REF_EMPTY_STRING(retdata);
		} else {
			ZEND_TRY_ASSIGN_REF_STRINGL(retdata, lretdata->bv_val, lretdata->bv_len);
			ldap_memfree(lretdata->bv_val);
			ldap_memfree(lretdata);
		}
	}

	RETURN_TRUE;
}
/* }}} */
#endif
/* }}} */

/* {{{ Count the number of references in a search result */
PHP_FUNCTION(ldap_count_references)
{
	zval *link, *result;
	ldap_linkdata *ld;
	ldap_resultdata *ldap_result;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "OO", &link, ldap_link_ce, &result, ldap_result_ce) != SUCCESS) {
		RETURN_THROWS();
	}

	ld = Z_LDAP_LINK_P(link);
	VERIFY_LDAP_LINK_CONNECTED(ld);

	ldap_result = Z_LDAP_RESULT_P(result);
	VERIFY_LDAP_RESULT_OPEN(ldap_result);

	RETURN_LONG(ldap_count_references(ld->link, ldap_result->result));
}
/* }}} */

/* {{{ Return first reference */
PHP_FUNCTION(ldap_first_reference)
{
	zval *link, *result;
	ldap_linkdata *ld;
	ldap_result_entry *resultentry;
	ldap_resultdata *ldap_result;
	LDAPMessage *entry;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "OO", &link, ldap_link_ce, &result, ldap_result_ce) != SUCCESS) {
		RETURN_THROWS();
	}

	ld = Z_LDAP_LINK_P(link);
	VERIFY_LDAP_LINK_CONNECTED(ld);

	ldap_result = Z_LDAP_RESULT_P(result);
	VERIFY_LDAP_RESULT_OPEN(ldap_result);

	if ((entry = ldap_first_reference(ld->link, ldap_result->result)) == NULL) {
		RETVAL_FALSE;
	} else {
		object_init_ex(return_value, ldap_result_entry_ce);
		resultentry = Z_LDAP_RESULT_ENTRY_P(return_value);
		ZVAL_COPY(&resultentry->res, result);
		resultentry->data = entry;
		resultentry->ber = NULL;
	}
}
/* }}} */

/* {{{ Get next reference */
PHP_FUNCTION(ldap_next_reference)
{
	zval *link, *result_entry;
	ldap_linkdata *ld;
	ldap_result_entry *resultentry, *resultentry_next;
	LDAPMessage *entry_next;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "OO", &link, ldap_link_ce, &result_entry, ldap_result_entry_ce) != SUCCESS) {
		RETURN_THROWS();
	}

	ld = Z_LDAP_LINK_P(link);
	VERIFY_LDAP_LINK_CONNECTED(ld);

	resultentry = Z_LDAP_RESULT_ENTRY_P(result_entry);

	if ((entry_next = ldap_next_reference(ld->link, resultentry->data)) == NULL) {
		RETVAL_FALSE;
	} else {
		object_init_ex(return_value, ldap_result_entry_ce);
		resultentry_next = Z_LDAP_RESULT_ENTRY_P(return_value);
		ZVAL_COPY(&resultentry_next->res, &resultentry->res);
		resultentry_next->data = entry_next;
		resultentry_next->ber = NULL;
	}
}
/* }}} */

#ifdef HAVE_LDAP_PARSE_REFERENCE
/* {{{ Extract information from reference entry */
PHP_FUNCTION(ldap_parse_reference)
{
	zval *link, *result_entry, *referrals;
	ldap_linkdata *ld;
	ldap_result_entry *resultentry;
	char **lreferrals, **refp;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "OOz", &link, ldap_link_ce, &result_entry, ldap_result_entry_ce, &referrals) != SUCCESS) {
		RETURN_THROWS();
	}

	ld = Z_LDAP_LINK_P(link);
	VERIFY_LDAP_LINK_CONNECTED(ld);

	resultentry = Z_LDAP_RESULT_ENTRY_P(result_entry);

	if (ldap_parse_reference(ld->link, resultentry->data, &lreferrals, NULL /* &serverctrls */, 0) != LDAP_SUCCESS) {
		RETURN_FALSE;
	}

	referrals = zend_try_array_init(referrals);
	if (!referrals) {
		RETURN_THROWS();
	}

	if (lreferrals != NULL) {
		refp = lreferrals;
		while (*refp) {
			add_next_index_string(referrals, *refp);
			refp++;
		}
		ldap_memvfree((void**)lreferrals);
	}
	RETURN_TRUE;
}
/* }}} */
#endif

/* {{{ php_ldap_do_rename */
static void php_ldap_do_rename(INTERNAL_FUNCTION_PARAMETERS, int ext)
{
	zval *link;
	ldap_linkdata *ld;
	LDAPControl **lserverctrls = NULL;
	ldap_resultdata *result;
	LDAPMessage *ldap_res;
	int rc, msgid;
	char *dn, *newrdn, *newparent;
	size_t dn_len, newrdn_len, newparent_len;
	bool deleteoldrdn;
	HashTable *server_controls_ht = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "Opppb|h!", &link, ldap_link_ce, &dn, &dn_len, &newrdn, &newrdn_len, &newparent, &newparent_len, &deleteoldrdn, &server_controls_ht) != SUCCESS) {
		RETURN_THROWS();
	}

	ld = Z_LDAP_LINK_P(link);
	VERIFY_LDAP_LINK_CONNECTED(ld);

	if (newparent_len == 0) {
		newparent = NULL;
	}

#if (LDAP_API_VERSION > 2000) || defined(HAVE_ORALDAP)
	if (server_controls_ht) {
		lserverctrls = php_ldap_controls_from_array(ld->link, server_controls_ht, 6);
		if (lserverctrls == NULL) {
			RETVAL_FALSE;
			goto cleanup;
		}
	}

	if (ext) {
		rc = ldap_rename(ld->link, dn, newrdn, newparent, deleteoldrdn, lserverctrls, NULL, &msgid);
	} else {
		rc = ldap_rename_s(ld->link, dn, newrdn, newparent, deleteoldrdn, lserverctrls, NULL);
	}
#else
	if (newparent_len != 0) {
		php_error_docref(NULL, E_WARNING, "You are using old LDAP API, newparent must be the empty string, can only modify RDN");
		RETURN_FALSE;
	}
	if (serverctrls) {
		php_error_docref(NULL, E_WARNING, "You are using old LDAP API, controls are not supported");
		RETURN_FALSE;
	}
	if (ext) {
		php_error_docref(NULL, E_WARNING, "You are using old LDAP API, ldap_rename_ext is not supported");
		RETURN_FALSE;
	}
/* could support old APIs but need check for ldap_modrdn2()/ldap_modrdn() */
	rc = ldap_modrdn2_s(ld->link, dn, newrdn, deleteoldrdn);
#endif

	if (rc != LDAP_SUCCESS) {
		RETVAL_FALSE;
	} else if (ext) {
		rc = ldap_result(ld->link, msgid, 1 /* LDAP_MSG_ALL */, NULL, &ldap_res);
		if (rc == -1) {
			php_error_docref(NULL, E_WARNING, "Rename operation failed");
			RETVAL_FALSE;
			goto cleanup;
		}

		/* return a PHP control object */
		object_init_ex(return_value, ldap_result_ce);
		result = Z_LDAP_RESULT_P(return_value);
		result->result = ldap_res;
	} else {
		RETVAL_TRUE;
	}

cleanup:
	if (lserverctrls) {
		_php_ldap_controls_free(&lserverctrls);
	}

	return;
}
/* }}} */

/* {{{ Modify the name of an entry */
PHP_FUNCTION(ldap_rename)
{
	php_ldap_do_rename(INTERNAL_FUNCTION_PARAM_PASSTHRU, 0);
}
/* }}} */

/* {{{ Modify the name of an entry */
PHP_FUNCTION(ldap_rename_ext)
{
	php_ldap_do_rename(INTERNAL_FUNCTION_PARAM_PASSTHRU, 1);
}
/* }}} */

#ifdef HAVE_LDAP_START_TLS_S
/*
  Force new tls context creation with string options inherited from global
  Workaround to https://bugs.openldap.org/show_bug.cgi?id=10337
 */
static int _php_ldap_tls_newctx(LDAP *ld)
{
	int val = 0, i;
	int str_opts[] = {
#if (LDAP_API_VERSION > 2000)
		LDAP_OPT_X_TLS_CACERTDIR,
		LDAP_OPT_X_TLS_CACERTFILE,
		LDAP_OPT_X_TLS_CERTFILE,
		LDAP_OPT_X_TLS_CIPHER_SUITE,
		LDAP_OPT_X_TLS_KEYFILE,
		LDAP_OPT_X_TLS_RANDOM_FILE,
#endif
#ifdef LDAP_OPT_X_TLS_CRLFILE
		LDAP_OPT_X_TLS_CRLFILE,
#endif
#ifdef LDAP_OPT_X_TLS_DHFILE
		LDAP_OPT_X_TLS_DHFILE,
#endif
#ifdef LDAP_OPT_X_TLS_ECNAME
		LDAP_OPT_X_TLS_ECNAME,
#endif
	0};

	for (i=0 ; str_opts[i] ; i++) {
		char *path = NULL;

		ldap_get_option(ld, str_opts[i], &path);
		if (path) {			/* already set locally */
			ldap_memfree(path);
		} else {
			ldap_get_option(NULL, str_opts[i], &path);
			if (path) { 	/* set globally, inherit */
				ldap_set_option(ld, str_opts[i], path);
				ldap_memfree(path);
			}
		}
	}

#ifdef LDAP_OPT_X_TLS_PROTOCOL_MIN
	int int_opts[] = {
		LDAP_OPT_X_TLS_PROTOCOL_MIN,
#ifdef LDAP_OPT_X_TLS_PROTOCOL_MAX
		LDAP_OPT_X_TLS_PROTOCOL_MAX,
#endif
		0
	};
	for (i=0 ; int_opts[i] ; i++) {
		int value = 0;

		ldap_get_option(ld, int_opts[i], &value);
		if (value <= 0) { 	/* if value is not set already */
			ldap_get_option(NULL, int_opts[i], &value);
			if (value > 0) { 	/* set globally, inherit */
				ldap_set_option(ld, int_opts[i], &value);
			}
		}
	}
#endif

	return ldap_set_option(ld, LDAP_OPT_X_TLS_NEWCTX, &val);
}

/* {{{ Start TLS */
PHP_FUNCTION(ldap_start_tls)
{
	zval *link;
	ldap_linkdata *ld;
	int rc, protocol = LDAP_VERSION3;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "O", &link, ldap_link_ce) != SUCCESS) {
		RETURN_THROWS();
	}

	ld = Z_LDAP_LINK_P(link);
	VERIFY_LDAP_LINK_CONNECTED(ld);

	if (((rc = ldap_set_option(ld->link, LDAP_OPT_PROTOCOL_VERSION, &protocol)) != LDAP_SUCCESS) ||
#ifdef LDAP_OPT_X_TLS_NEWCTX
		(LDAPG(tls_newctx) && (rc = _php_ldap_tls_newctx(ld->link)) != LDAP_OPT_SUCCESS) ||
#endif
		((rc = ldap_start_tls_s(ld->link, NULL, NULL)) != LDAP_SUCCESS)
	) {
		php_error_docref(NULL, E_WARNING,"Unable to start TLS: %s", ldap_err2string(rc));
		RETURN_FALSE;
	}
	LDAPG(tls_newctx) = false;
	RETURN_TRUE;
}
/* }}} */
#endif
#endif /* (LDAP_API_VERSION > 2000) || defined(HAVE_ORALDAP) */

#if defined(LDAP_API_FEATURE_X_OPENLDAP) && defined(HAVE_3ARG_SETREBINDPROC)
/* {{{ _ldap_rebind_proc() */
int _ldap_rebind_proc(LDAP *ldap, const char *url, ber_tag_t req, ber_int_t msgid, void *params)
{
	ldap_linkdata *ld = NULL;
	int retval;
	zval cb_args[2];
	zval cb_retval;
	zval *cb_link = (zval *) params;

	ld = Z_LDAP_LINK_P(cb_link);
	if (!ld->link) {
		zend_throw_error(NULL, "LDAP connection has already been closed");
		return LDAP_OTHER;
	}

	/* link exists and callback set? */
	if (!ZEND_FCC_INITIALIZED(ld->rebind_proc_fcc)) {
		php_error_docref(NULL, E_WARNING, "No callback set");
		return LDAP_OTHER;
	}

	/* callback */
	ZVAL_COPY_VALUE(&cb_args[0], cb_link);
	ZVAL_STRING(&cb_args[1], url);
	zend_call_known_fcc(&ld->rebind_proc_fcc, &cb_retval, 2, cb_args, NULL);
	if (EXPECTED(!Z_ISUNDEF(cb_retval))) {
		// TODO Use zval_try_get_long()
		retval = zval_get_long(&cb_retval);
		zval_ptr_dtor(&cb_retval);
	} else {
		retval = LDAP_OTHER;
	}
	zval_ptr_dtor(&cb_args[1]);
	return retval;
}
/* }}} */

/* {{{ Set a callback function to do re-binds on referral chasing. */
PHP_FUNCTION(ldap_set_rebind_proc)
{
	zval *link;
	zend_fcall_info dummy_fci;
	zend_fcall_info_cache fcc = empty_fcall_info_cache;
	ldap_linkdata *ld;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "OF!", &link, ldap_link_ce, &dummy_fci, &fcc) == FAILURE) {
		RETURN_THROWS();
	}

	ld = Z_LDAP_LINK_P(link);
	/* Inline VERIFY_LDAP_LINK_CONNECTED(ld); as we need to free trampoline */
	if (!ld->link) {
		zend_release_fcall_info_cache(&fcc);
		zend_throw_error(NULL, "LDAP connection has already been closed");
		RETURN_THROWS();
	}

	/* Free old FCC */
	if (ZEND_FCC_INITIALIZED(ld->rebind_proc_fcc)) {
		zend_fcc_dtor(&ld->rebind_proc_fcc);
	}
	if (ZEND_FCC_INITIALIZED(fcc)) {
		/* register rebind procedure */
		ldap_set_rebind_proc(ld->link, _ldap_rebind_proc, (void *) link);
		zend_fcc_dup(&ld->rebind_proc_fcc, &fcc);
	} else {
		/* unregister rebind procedure */
		ldap_set_rebind_proc(ld->link, NULL, NULL);
    }

	RETURN_TRUE;
}
/* }}} */
#endif

static zend_string* php_ldap_do_escape(const bool *map, const char *value, size_t valuelen, zend_long flags)
{
	char hex[] = "0123456789abcdef";
	size_t i, p = 0;
	size_t len = 0;
	zend_string *ret;

	for (i = 0; i < valuelen; i++) {
		size_t addend = (map[(unsigned char) value[i]]) ? 3 : 1;
		if (len > ZSTR_MAX_LEN - addend) {
			return NULL;
		}
		len += addend;
	}
	/* Per RFC 4514, a leading and trailing space must be escaped */
	if ((flags & PHP_LDAP_ESCAPE_DN) && (value[0] == ' ')) {
		if (len > ZSTR_MAX_LEN - 2) {
			return NULL;
		}
		len += 2;
	}
	if ((flags & PHP_LDAP_ESCAPE_DN) && ((valuelen > 1) && (value[valuelen - 1] == ' '))) {
		if (len > ZSTR_MAX_LEN - 2) {
			return NULL;
		}
		len += 2;
	}

	ret =  zend_string_alloc(len, 0);

	for (i = 0; i < valuelen; i++) {
		unsigned char v = (unsigned char) value[i];

		if (map[v] || ((flags & PHP_LDAP_ESCAPE_DN) && ((i == 0) || (i + 1 == valuelen)) && (v == ' '))) {
			ZSTR_VAL(ret)[p++] = '\\';
			ZSTR_VAL(ret)[p++] = hex[v >> 4];
			ZSTR_VAL(ret)[p++] = hex[v & 0x0f];
		} else {
			ZSTR_VAL(ret)[p++] = v;
		}
	}

	ZSTR_VAL(ret)[p] = '\0';
	ZSTR_LEN(ret) = p;
	return ret;
}

static void php_ldap_escape_map_set_chars(bool *map, const char *chars, const size_t charslen, char escape)
{
	size_t i = 0;
	while (i < charslen) {
		map[(unsigned char) chars[i++]] = escape;
	}
}

PHP_FUNCTION(ldap_escape)
{
	char *value, *ignores;
	size_t valuelen = 0, ignoreslen = 0;
	int i;
	zend_long flags = 0;
	bool map[256] = {0}, havecharlist = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "s|sl", &value, &valuelen, &ignores, &ignoreslen, &flags) != SUCCESS) {
		RETURN_THROWS();
	}

	if (!valuelen) {
		RETURN_EMPTY_STRING();
	}

	if (flags & PHP_LDAP_ESCAPE_FILTER) {
		havecharlist = 1;
		php_ldap_escape_map_set_chars(map, "\\*()\0", sizeof("\\*()\0") - 1, 1);
	}

	if (flags & PHP_LDAP_ESCAPE_DN) {
		havecharlist = 1;
		php_ldap_escape_map_set_chars(map, "\\,=+<>;\"#\r", sizeof("\\,=+<>;\"#\r") - 1, 1);
	}

	if (!havecharlist) {
		for (i = 0; i < 256; i++) {
			map[i] = 1;
		}
	}

	if (ignoreslen) {
		php_ldap_escape_map_set_chars(map, ignores, ignoreslen, 0);
	}

	zend_string *result = php_ldap_do_escape(map, value, valuelen, flags);
	if (UNEXPECTED(!result)) {
		zend_argument_value_error(1, "is too long");
		RETURN_THROWS();
	}

	RETURN_NEW_STR(result);
}

#ifdef STR_TRANSLATION
/* {{{ php_ldap_do_translate */
static void php_ldap_do_translate(INTERNAL_FUNCTION_PARAMETERS, int way)
{
	char *value;
	size_t value_len;
	int result;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", &value, &value_len) != SUCCESS) {
		RETURN_THROWS();
	}

	if (value_len == 0) {
		RETURN_FALSE;
	}

	if (way == 1) {
		result = ldap_8859_to_t61(&value, &value_len, 0);
	} else {
		result = ldap_t61_to_8859(&value, &value_len, 0);
	}

	if (result == LDAP_SUCCESS) {
		RETVAL_STRINGL(value, value_len);
		free(value);
	} else {
		php_error_docref(NULL, E_WARNING, "Conversion from ISO-8859-1 to t61 failed: %s", ldap_err2string(result));
		RETVAL_FALSE;
	}
}
/* }}} */

/* {{{ Translate t61 characters to 8859 characters */
PHP_FUNCTION(ldap_t61_to_8859)
{
	php_ldap_do_translate(INTERNAL_FUNCTION_PARAM_PASSTHRU, 0);
}
/* }}} */

/* {{{ Translate 8859 characters to t61 characters */
PHP_FUNCTION(ldap_8859_to_t61)
{
	php_ldap_do_translate(INTERNAL_FUNCTION_PARAM_PASSTHRU, 1);
}
/* }}} */
#endif

/* {{{ Extended operations, Pierangelo Masarati */
#ifdef HAVE_LDAP_EXTENDED_OPERATION_S
static void php_ldap_exop(INTERNAL_FUNCTION_PARAMETERS, bool force_sync) {
	zval *link, *retdata = NULL, *retoid = NULL;
	char *lretoid = NULL;
	zend_string *reqoid, *reqdata = NULL;
	HashTable *server_controls_ht = NULL;
	struct berval lreqdata, *lretdata = NULL;
	ldap_linkdata *ld;
	ldap_resultdata *result;
	LDAPMessage *ldap_res;
	LDAPControl **lserverctrls = NULL;
	int rc, msgid;

	if (force_sync == false && ZEND_NUM_ARGS() > 4) {
		zend_error(E_DEPRECATED, "Calling ldap_exop() with more than 4 arguments is deprecated, use ldap_exop_sync() instead");
		if (UNEXPECTED(EG(exception))) {
			RETURN_THROWS();
		}
	}

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "OP|S!h!zz", &link, ldap_link_ce, &reqoid, &reqdata, &server_controls_ht, &retdata, &retoid) != SUCCESS) {
		RETURN_THROWS();
	}

	if (ZSTR_LEN(reqoid) == 0) {
		zend_argument_must_not_be_empty_error(2);
		RETURN_THROWS();
	}

	ld = Z_LDAP_LINK_P(link);
	VERIFY_LDAP_LINK_CONNECTED(ld);

	if (reqdata) {
		lreqdata.bv_val = ZSTR_VAL(reqdata);
		lreqdata.bv_len = ZSTR_LEN(reqdata);
	} else {
		lreqdata.bv_len = 0;
	}

	if (server_controls_ht) {
		lserverctrls = php_ldap_controls_from_array(ld->link, server_controls_ht, 4);
		if (lserverctrls == NULL) {
			RETVAL_FALSE;
			goto cleanup;
		}
	}

	if (force_sync || retdata) {
		/* synchronous call */
		rc = ldap_extended_operation_s(ld->link, ZSTR_VAL(reqoid),
			lreqdata.bv_len > 0 ? &lreqdata: NULL,
			lserverctrls,
			NULL,
			retoid ? &lretoid : NULL,
			&lretdata );
		if (rc != LDAP_SUCCESS ) {
			php_error_docref(NULL, E_WARNING, "Extended operation %s failed: %s (%d)", ZSTR_VAL(reqoid), ldap_err2string(rc), rc);
			RETVAL_FALSE;
			goto cleanup;
		}

		if (retoid) {
			if (lretoid) {
				ZEND_TRY_ASSIGN_REF_STRING(retoid, lretoid);
				ldap_memfree(lretoid);
			} else {
				ZEND_TRY_ASSIGN_REF_EMPTY_STRING(retoid);
			}
		}

		if (lretdata) {
			ZEND_TRY_ASSIGN_REF_STRINGL(retdata, lretdata->bv_val, lretdata->bv_len);
			ldap_memfree(lretdata->bv_val);
			ldap_memfree(lretdata);
		} else {
			ZEND_TRY_ASSIGN_REF_EMPTY_STRING(retdata);
		}

		RETVAL_TRUE;
		goto cleanup;
	}

	/* asynchronous call */
	rc = ldap_extended_operation(ld->link, ZSTR_VAL(reqoid),
		lreqdata.bv_len > 0 ? &lreqdata: NULL,
		lserverctrls,
		NULL,
		&msgid);
	if (rc != LDAP_SUCCESS ) {
		php_error_docref(NULL, E_WARNING, "Extended operation %s failed: %s (%d)", ZSTR_VAL(reqoid), ldap_err2string(rc), rc);
		RETVAL_FALSE;
		goto cleanup;
	}

	rc = ldap_result(ld->link, msgid, 1 /* LDAP_MSG_ALL */, NULL, &ldap_res);
	if (rc == -1) {
		php_error_docref(NULL, E_WARNING, "Extended operation %s failed", ZSTR_VAL(reqoid));
		RETVAL_FALSE;
		goto cleanup;
	}

	/* return a PHP control object */
	object_init_ex(return_value, ldap_result_ce);
	result = Z_LDAP_RESULT_P(return_value);
	result->result = ldap_res;

cleanup:
	if (lserverctrls) {
		_php_ldap_controls_free(&lserverctrls);
	}
}

/* {{{ Extended operation */
PHP_FUNCTION(ldap_exop)
{
	php_ldap_exop(INTERNAL_FUNCTION_PARAM_PASSTHRU, false);
}
/* }}} */

PHP_FUNCTION(ldap_exop_sync)
{
	php_ldap_exop(INTERNAL_FUNCTION_PARAM_PASSTHRU, true);
}
#endif

#ifdef HAVE_LDAP_PASSWD
/* {{{ Passwd modify extended operation */
PHP_FUNCTION(ldap_exop_passwd)
{
	zval *link, *serverctrls = NULL;
	struct berval luser = { 0L, NULL };
	struct berval loldpw = { 0L, NULL };
	struct berval lnewpw = { 0L, NULL };
	struct berval lgenpasswd = { 0L, NULL };
	LDAPControl *ctrl, **lserverctrls = NULL, *requestctrls[2] = { NULL, NULL };
	LDAPMessage* ldap_res = NULL;
	ldap_linkdata *ld;
	int rc, msgid, err;
	char* errmsg = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "O|sssz/", &link, ldap_link_ce, &luser.bv_val, &luser.bv_len, &loldpw.bv_val, &loldpw.bv_len, &lnewpw.bv_val, &lnewpw.bv_len, &serverctrls) == FAILURE) {
		RETURN_THROWS();
	}

	ld = Z_LDAP_LINK_P(link);
	VERIFY_LDAP_LINK_CONNECTED(ld);

	if (serverctrls) {
		/* ldap_create_passwordpolicy_control() allocates ctrl */
		if (ldap_create_passwordpolicy_control(ld->link, &ctrl) == LDAP_SUCCESS) {
			requestctrls[0] = ctrl;
		}
		// TODO Should this warn?
	}

	/* asynchronous call to get result and controls */
	rc = ldap_passwd(ld->link, &luser,
		loldpw.bv_len > 0 ? &loldpw : NULL,
		lnewpw.bv_len > 0 ? &lnewpw : NULL,
		requestctrls,
		NULL, &msgid);

	if (requestctrls[0] != NULL) {
		ldap_control_free(requestctrls[0]);
	}

	if (rc != LDAP_SUCCESS ) {
		php_error_docref(NULL, E_WARNING, "Passwd modify extended operation failed: %s (%d)", ldap_err2string(rc), rc);
		RETVAL_FALSE;
		goto cleanup;
	}

	rc = ldap_result(ld->link, msgid, 1 /* LDAP_MSG_ALL */, NULL, &ldap_res);
	if ((rc < 0) || !ldap_res) {
		rc = _get_lderrno(ld->link);
		php_error_docref(NULL, E_WARNING, "Passwd modify extended operation failed: %s (%d)", ldap_err2string(rc), rc);
		RETVAL_FALSE;
		goto cleanup;
	}

	rc = ldap_parse_passwd(ld->link, ldap_res, &lgenpasswd);
	if( rc != LDAP_SUCCESS ) {
		php_error_docref(NULL, E_WARNING, "Passwd modify extended operation failed: %s (%d)", ldap_err2string(rc), rc);
		RETVAL_FALSE;
		goto cleanup;
	}

	rc = ldap_parse_result(ld->link, ldap_res, &err, NULL, &errmsg, NULL, (serverctrls ? &lserverctrls : NULL), 0);
	if( rc != LDAP_SUCCESS ) {
		php_error_docref(NULL, E_WARNING, "Passwd modify extended operation failed: %s (%d)", ldap_err2string(rc), rc);
		RETVAL_FALSE;
		goto cleanup;
	}

	if (serverctrls) {
		_php_ldap_controls_to_array(ld->link, lserverctrls, serverctrls, 0);
	}

	/* return */
	if (lnewpw.bv_len == 0) {
		if (lgenpasswd.bv_len == 0) {
			RETVAL_EMPTY_STRING();
		} else {
			RETVAL_STRINGL(lgenpasswd.bv_val, lgenpasswd.bv_len);
		}
	} else if (err == LDAP_SUCCESS) {
		RETVAL_TRUE;
	} else {
		php_error_docref(NULL, E_WARNING, "Passwd modify extended operation failed: %s (%d)", (errmsg ? errmsg : ldap_err2string(err)), err);
		RETVAL_FALSE;
	}

cleanup:
	if (lgenpasswd.bv_val != NULL) {
		ldap_memfree(lgenpasswd.bv_val);
	}
	if (ldap_res != NULL) {
		ldap_msgfree(ldap_res);
	}
	if (errmsg != NULL) {
		ldap_memfree(errmsg);
	}
}
/* }}} */
#endif

#ifdef HAVE_LDAP_WHOAMI_S
/* {{{ Whoami extended operation */
PHP_FUNCTION(ldap_exop_whoami)
{
	zval *link;
	struct berval *lauthzid;
	ldap_linkdata *ld;
	int rc;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "O", &link, ldap_link_ce) == FAILURE) {
		RETURN_THROWS();
	}

	ld = Z_LDAP_LINK_P(link);
	VERIFY_LDAP_LINK_CONNECTED(ld);

	/* synchronous call */
	rc = ldap_whoami_s(ld->link, &lauthzid, NULL, NULL);
	if (rc != LDAP_SUCCESS ) {
		php_error_docref(NULL, E_WARNING, "Whoami extended operation failed: %s (%d)", ldap_err2string(rc), rc);
		RETURN_FALSE;
	}

	if (lauthzid == NULL) {
		RETVAL_EMPTY_STRING();
	} else {
		RETVAL_STRINGL(lauthzid->bv_val, lauthzid->bv_len);
		ldap_memfree(lauthzid->bv_val);
		ldap_memfree(lauthzid);
	}
}
/* }}} */
#endif

#ifdef HAVE_LDAP_REFRESH_S
/* {{{ DDS refresh extended operation */
PHP_FUNCTION(ldap_exop_refresh)
{
	zval *link;
	zend_long ttl;
	struct berval ldn;
	ber_int_t lttl;
	ber_int_t newttl;
	ldap_linkdata *ld;
	int rc;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "Osl", &link, ldap_link_ce, &ldn.bv_val, &ldn.bv_len, &ttl) != SUCCESS) {
		RETURN_THROWS();
	}

	ld = Z_LDAP_LINK_P(link);
	VERIFY_LDAP_LINK_CONNECTED(ld);

	lttl = (ber_int_t) ttl;

	rc = ldap_refresh_s(ld->link, &ldn, lttl, &newttl, NULL, NULL);
	if (rc != LDAP_SUCCESS ) {
		php_error_docref(NULL, E_WARNING, "Refresh extended operation failed: %s (%d)", ldap_err2string(rc), rc);
		RETURN_FALSE;
	}

	RETURN_LONG(newttl);
}
/* }}} */
#endif

zend_module_entry ldap_module_entry = { /* {{{ */
	STANDARD_MODULE_HEADER,
	"ldap",
	ext_functions,
	PHP_MINIT(ldap),
	PHP_MSHUTDOWN(ldap),
	PHP_RINIT(ldap),
	NULL,
	PHP_MINFO(ldap),
	PHP_LDAP_VERSION,
	PHP_MODULE_GLOBALS(ldap),
	PHP_GINIT(ldap),
	NULL,
	NULL,
	STANDARD_MODULE_PROPERTIES_EX
};
/* }}} */
