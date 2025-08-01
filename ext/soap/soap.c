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
  | Authors: Brad Lafountain <rodif_bl@yahoo.com>                        |
  |          Shane Caraveo <shane@caraveo.com>                           |
  |          Dmitry Stogov <dmitry@php.net>                              |
  +----------------------------------------------------------------------+
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "php_soap.h"
#include "ext/session/php_session.h"
#include "zend_attributes.h"
#include "soap_arginfo.h"
#include "zend_exceptions.h"
#include "zend_interfaces.h"
#include "ext/standard/php_incomplete_class.h"

/* We only have session support if PHP was configured with session support
 * or if the session module could be loaded dynamically, which will only
 * work if soap is loaded dynamically as well. */
#if defined(HAVE_PHP_SESSION) || defined(COMPILE_DL_SOAP)
# define SOAP_HAS_SESSION_SUPPORT
#endif

typedef struct _soapHeader {
	sdlFunctionPtr                    function;
	zval                              function_name;
	int                               mustUnderstand;
	uint32_t                          num_params;
	zval                             *parameters;
	zval                              retval;
	sdlSoapBindingFunctionHeaderPtr   hdr;
	struct _soapHeader               *next;
} soapHeader;

/* Local functions */
static void function_to_string(sdlFunctionPtr function, smart_str *buf);
static void type_to_string(sdlTypePtr type, smart_str *buf, int level);

static void clear_soap_fault(zval *obj);
static void set_soap_fault(zval *obj, const char *fault_code_ns, const char *fault_code, const char *fault_string, const char *fault_actor, zval *fault_detail, zend_string *name, zend_string *lang);
static void add_soap_fault_en(zval *obj, char *fault_code, char *fault_string, char *fault_actor, zval *fault_detail);
static void add_soap_fault_ex(zval *fault, zval *obj, char *fault_code, char *fault_string, char *fault_actor, zval *fault_detail, zend_string *lang);
static void add_soap_fault_ex_en(zval *fault, zval *obj, char *fault_code, char *fault_string, char *fault_actor, zval *fault_detail);
static ZEND_NORETURN void soap_server_fault(char* code, char* string, char *actor, zval* details, zend_string *name, zend_string *lang);
static void soap_server_fault_ex(sdlFunctionPtr function, zval* fault, soapHeader* hdr);
static ZEND_NORETURN void soap_server_fault_en(char* code, char* string, char *actor, zval* details, zend_string *name);

static sdlParamPtr get_param(sdlFunctionPtr function, const char *param_name, zend_ulong index, int);
static sdlFunctionPtr get_function(sdlPtr sdl, const char *function_name, size_t function_name_length);
static sdlFunctionPtr get_doc_function(sdlPtr sdl, xmlNodePtr params);

static sdlFunctionPtr deserialize_function_call(sdlPtr sdl, xmlDocPtr request, const char* actor, const char *soap_action, zval *function_name, uint32_t *num_params, zval **parameters, int *version, soapHeader **headers);
static xmlDocPtr serialize_response_call(sdlFunctionPtr function, const char *function_name, const char *uri,zval *ret, soapHeader *headers, int version);
static xmlDocPtr serialize_function_call(zval *this_ptr, sdlFunctionPtr function, const char *function_name, const char *uri, zval *arguments, uint32_t arg_count, int version, HashTable *soap_headers);
static xmlNodePtr serialize_parameter(sdlParamPtr param,zval *param_val, uint32_t index,const char *name, int style, xmlNodePtr parent);
static xmlNodePtr serialize_zval(zval *val, sdlParamPtr param, const char *paramName, int style, xmlNodePtr parent);

static void delete_service(soapServicePtr service);
static void delete_argv(struct _soap_class *class);

static void soap_error_handler(int error_num, zend_string *error_filename, uint32_t error_lineno, zend_string *message);

#define SOAP_SERVER_BEGIN_CODE() \
	bool _old_handler = SOAP_GLOBAL(use_soap_error_handler);\
	char* _old_error_code = SOAP_GLOBAL(error_code);\
	zend_object* _old_error_object = Z_OBJ(SOAP_GLOBAL(error_object));\
	int _old_soap_version = SOAP_GLOBAL(soap_version);\
	SOAP_GLOBAL(use_soap_error_handler) = 1;\
	SOAP_GLOBAL(error_code) = "Server";\
	Z_OBJ(SOAP_GLOBAL(error_object)) = Z_OBJ_P(ZEND_THIS);

#define SOAP_SERVER_END_CODE() \
	SOAP_GLOBAL(use_soap_error_handler) = _old_handler;\
	SOAP_GLOBAL(error_code) = _old_error_code;\
	Z_OBJ(SOAP_GLOBAL(error_object)) = _old_error_object;\
	SOAP_GLOBAL(soap_version) = _old_soap_version;

#define SOAP_CLIENT_BEGIN_CODE() \
	bool _old_handler = SOAP_GLOBAL(use_soap_error_handler);\
	char* _old_error_code = SOAP_GLOBAL(error_code);\
	zend_object* _old_error_object = Z_OBJ(SOAP_GLOBAL(error_object));\
	int _old_soap_version = SOAP_GLOBAL(soap_version);\
	bool _old_in_compilation = CG(in_compilation); \
	zend_execute_data *_old_current_execute_data = EG(current_execute_data); \
	zval *_old_stack_top = EG(vm_stack_top); \
	bool _bailout = false;\
	SOAP_GLOBAL(use_soap_error_handler) = 1;\
	SOAP_GLOBAL(error_code) = "Client";\
	Z_OBJ(SOAP_GLOBAL(error_object)) = Z_OBJ_P(ZEND_THIS);\
	zend_try {

#define SOAP_CLIENT_END_CODE() \
	} zend_catch {\
		CG(in_compilation) = _old_in_compilation; \
		EG(current_execute_data) = _old_current_execute_data; \
		if (EG(exception) == NULL || \
		    !instanceof_function(EG(exception)->ce, soap_fault_class_entry)) {\
			_bailout = true;\
		}\
		if (_old_stack_top != EG(vm_stack_top)) { \
			while (EG(vm_stack)->prev != NULL && \
			       ((char*)_old_stack_top < (char*)EG(vm_stack) || \
			        (char*) _old_stack_top > (char*)EG(vm_stack)->end)) { \
				zend_vm_stack tmp = EG(vm_stack)->prev; \
				efree(EG(vm_stack)); \
				EG(vm_stack) = tmp; \
				EG(vm_stack_end) = tmp->end; \
			} \
			EG(vm_stack)->top = _old_stack_top; \
		} \
	} zend_end_try();\
	SOAP_GLOBAL(use_soap_error_handler) = _old_handler;\
	SOAP_GLOBAL(error_code) = _old_error_code;\
	Z_OBJ(SOAP_GLOBAL(error_object)) = _old_error_object;\
	SOAP_GLOBAL(soap_version) = _old_soap_version;\
	if (_bailout) {\
		zend_bailout();\
	}

#define FETCH_THIS_SDL(ss) \
	{ \
		zval *__tmp = Z_CLIENT_SDL_P(ZEND_THIS); \
		if (Z_TYPE_P(__tmp) == IS_OBJECT && instanceof_function(Z_OBJCE_P(__tmp), soap_sdl_class_entry)) { \
			ss = Z_SOAP_SDL_P(__tmp)->sdl; \
		} else { \
			ss = NULL; \
		} \
	}

#define Z_PARAM_NAME_P(zv) php_soap_deref(OBJ_PROP_NUM(Z_OBJ_P(zv), 0))
#define Z_PARAM_DATA_P(zv) php_soap_deref(OBJ_PROP_NUM(Z_OBJ_P(zv), 1))

#define Z_HEADER_NAMESPACE_P(zv) php_soap_deref(OBJ_PROP_NUM(Z_OBJ_P(zv), 0))
#define Z_HEADER_NAME_P(zv) php_soap_deref(OBJ_PROP_NUM(Z_OBJ_P(zv), 1))
#define Z_HEADER_DATA_P(zv) php_soap_deref(OBJ_PROP_NUM(Z_OBJ_P(zv), 2))
#define Z_HEADER_MUST_UNDERSTAND_P(zv) php_soap_deref(OBJ_PROP_NUM(Z_OBJ_P(zv), 3))
#define Z_HEADER_ACTOR_P(zv) php_soap_deref(OBJ_PROP_NUM(Z_OBJ_P(zv), 4))

#define Z_SERVER_SOAP_FAULT_P(zv) php_soap_deref(OBJ_PROP_NUM(Z_OBJ_P(zv), 0))

/* SoapFault extends Exception, so take those properties into account. */
#define FAULT_PROP_START_OFFSET zend_ce_exception->default_properties_count
#define Z_FAULT_STRING_P(zv) php_soap_deref(OBJ_PROP_NUM(Z_OBJ_P(zv), FAULT_PROP_START_OFFSET + 0))
#define Z_FAULT_CODE_P(zv) php_soap_deref(OBJ_PROP_NUM(Z_OBJ_P(zv), FAULT_PROP_START_OFFSET + 1))
#define Z_FAULT_CODENS_P(zv) php_soap_deref(OBJ_PROP_NUM(Z_OBJ_P(zv), FAULT_PROP_START_OFFSET + 2))
#define Z_FAULT_ACTOR_P(zv) php_soap_deref(OBJ_PROP_NUM(Z_OBJ_P(zv), FAULT_PROP_START_OFFSET + 3))
#define Z_FAULT_DETAIL_P(zv) php_soap_deref(OBJ_PROP_NUM(Z_OBJ_P(zv), FAULT_PROP_START_OFFSET + 4))
#define Z_FAULT_NAME_P(zv) php_soap_deref(OBJ_PROP_NUM(Z_OBJ_P(zv), FAULT_PROP_START_OFFSET + 5))
#define Z_FAULT_HEADERFAULT_P(zv) php_soap_deref(OBJ_PROP_NUM(Z_OBJ_P(zv), FAULT_PROP_START_OFFSET + 6))
#define Z_FAULT_LANG_P(zv) php_soap_deref(OBJ_PROP_NUM(Z_OBJ_P(zv), FAULT_PROP_START_OFFSET + 7))

#define FETCH_THIS_SERVICE_NO_BAILOUT(ss) \
	{ \
		ss = soap_server_object_fetch(Z_OBJ_P(ZEND_THIS))->service; \
		if (!ss) { \
			zend_throw_error(NULL, "Cannot fetch SoapServer object"); \
			RETURN_THROWS(); \
		} \
	}

#define FETCH_THIS_SERVICE(ss) \
	{ \
		ss = soap_server_object_fetch(Z_OBJ_P(ZEND_THIS))->service; \
		if (!ss) { \
			zend_throw_error(NULL, "Cannot fetch SoapServer object"); \
			SOAP_SERVER_END_CODE(); \
			RETURN_THROWS(); \
		} \
	}

zend_class_entry* soap_class_entry;
static zend_class_entry* soap_server_class_entry;
static zend_class_entry* soap_fault_class_entry;
static zend_class_entry* soap_header_class_entry;
static zend_class_entry* soap_param_class_entry;
zend_class_entry* soap_var_class_entry;
zend_class_entry *soap_url_class_entry;
zend_class_entry *soap_sdl_class_entry;

static zend_object_handlers soap_server_object_handlers;
static zend_object_handlers soap_url_object_handlers;
static zend_object_handlers soap_sdl_object_handlers;

typedef struct {
	soapServicePtr service;
	zend_object std;
} soap_server_object;

static inline soap_server_object *soap_server_object_fetch(zend_object *obj) {
	return (soap_server_object *) ((char *) obj - XtOffsetOf(soap_server_object, std));
}

static zend_object *soap_server_object_create(zend_class_entry *ce)
{
	soap_server_object *obj = zend_object_alloc(sizeof(soap_server_object), ce);
	zend_object_std_init(&obj->std, ce);
	object_properties_init(&obj->std, ce);
	return &obj->std;
}

static void soap_server_object_free(zend_object *obj) {
	soap_server_object *server_obj = soap_server_object_fetch(obj);
	if (server_obj->service) {
		delete_service(server_obj->service);
	}
	zend_object_std_dtor(obj);
}

static zend_object *soap_url_object_create(zend_class_entry *ce)
{
	soap_url_object *url_obj = zend_object_alloc(sizeof(soap_url_object), ce);

	zend_object_std_init(&url_obj->std, ce);
	object_properties_init(&url_obj->std, ce);

	return &url_obj->std;
}

static void soap_url_object_free(zend_object *obj)
{
	soap_url_object *url_obj = soap_url_object_fetch(obj);

	if (url_obj->url) {
		php_url_free(url_obj->url);
		url_obj->url = NULL;
	}

	zend_object_std_dtor(&url_obj->std);
}

static zend_function *soap_url_object_get_constructor(zend_object *object)
{
	zend_throw_error(NULL, "Cannot directly construct Soap\\Url");

	return NULL;
}

static zend_result soap_url_cast_object(zend_object *obj, zval *result, int type)
{
	if (type == IS_LONG) {
		ZVAL_LONG(result, obj->handle);

		return SUCCESS;
	}

	return zend_std_cast_object_tostring(obj, result, type);
}

static inline soap_sdl_object *soap_sdl_object_fetch(zend_object *obj)
{
	return (soap_sdl_object *) ((char *) obj - XtOffsetOf(soap_sdl_object, std));
}

#define Z_SOAP_SDL_P(zv) soap_sdl_object_fetch(Z_OBJ_P(zv))

static zend_object *soap_sdl_object_create(zend_class_entry *ce)
{
	soap_sdl_object *sdl_obj = zend_object_alloc(sizeof(soap_sdl_object), ce);

	zend_object_std_init(&sdl_obj->std, ce);
	object_properties_init(&sdl_obj->std, ce);

	return &sdl_obj->std;
}

static void soap_sdl_object_free(zend_object *obj)
{
	soap_sdl_object *sdl_obj = soap_sdl_object_fetch(obj);

	if (sdl_obj->sdl) {
		delete_sdl(sdl_obj->sdl);
		sdl_obj->sdl = NULL;
	}

	zend_object_std_dtor(&sdl_obj->std);
}

static zend_function *soap_sdl_object_get_constructor(zend_object *object)
{
	zend_throw_error(NULL, "Cannot directly construct Soap\\Sdl");

	return NULL;
}

static zend_result soap_sdl_cast_object(zend_object *obj, zval *result, int type)
{
	if (type == IS_LONG) {
		ZVAL_LONG(result, obj->handle);

		return SUCCESS;
	}

	return zend_std_cast_object_tostring(obj, result, type);
}

ZEND_DECLARE_MODULE_GLOBALS(soap)

static void (*old_error_handler)(int, zend_string *, const uint32_t, zend_string *);

PHP_RINIT_FUNCTION(soap);
PHP_MINIT_FUNCTION(soap);
PHP_MSHUTDOWN_FUNCTION(soap);
PHP_MINFO_FUNCTION(soap);

static const zend_module_dep soap_deps[] = {
	ZEND_MOD_REQUIRED("date")
	ZEND_MOD_REQUIRED("libxml")
	ZEND_MOD_OPTIONAL("session")
	ZEND_MOD_END
};

zend_module_entry soap_module_entry = {
  STANDARD_MODULE_HEADER_EX, NULL,
  soap_deps,
  "soap",
  ext_functions,
  PHP_MINIT(soap),
  PHP_MSHUTDOWN(soap),
  PHP_RINIT(soap),
  NULL,
  PHP_MINFO(soap),
#ifdef STANDARD_MODULE_HEADER
  PHP_SOAP_VERSION,
#endif
  STANDARD_MODULE_PROPERTIES,
};

#ifdef COMPILE_DL_SOAP
#ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
#endif
ZEND_GET_MODULE(soap)
#endif

ZEND_INI_MH(OnUpdateCacheMode)
{
	char *p = (char *) ZEND_INI_GET_ADDR();
	*p = (char)atoi(ZSTR_VAL(new_value));
	return SUCCESS;
}

static PHP_INI_MH(OnUpdateCacheDir)
{
	/* Only do the open_basedir check at runtime */
	if (stage == PHP_INI_STAGE_RUNTIME || stage == PHP_INI_STAGE_HTACCESS) {
		char *p;

		if (memchr(ZSTR_VAL(new_value), '\0', ZSTR_LEN(new_value)) != NULL) {
			return FAILURE;
		}

		/* we do not use zend_memrchr() since path can contain ; itself */
		if ((p = strchr(ZSTR_VAL(new_value), ';'))) {
			char *p2;
			p++;
			if ((p2 = strchr(p, ';'))) {
				p = p2 + 1;
			}
		} else {
			p = ZSTR_VAL(new_value);
		}

		if (PG(open_basedir) && *p && php_check_open_basedir(p)) {
			return FAILURE;
		}
	}

	OnUpdateString(entry, new_value, mh_arg1, mh_arg2, mh_arg3, stage);
	return SUCCESS;
}

PHP_INI_BEGIN()
STD_PHP_INI_BOOLEAN("soap.wsdl_cache_enabled",     "1", PHP_INI_ALL, OnUpdateBool,
                  cache_enabled, zend_soap_globals, soap_globals)
STD_PHP_INI_ENTRY("soap.wsdl_cache_dir",         "/tmp", PHP_INI_ALL, OnUpdateCacheDir,
                  cache_dir, zend_soap_globals, soap_globals)
STD_PHP_INI_ENTRY("soap.wsdl_cache_ttl",         "86400", PHP_INI_ALL, OnUpdateLong,
                  cache_ttl, zend_soap_globals, soap_globals)
STD_PHP_INI_ENTRY("soap.wsdl_cache",             "1", PHP_INI_ALL, OnUpdateCacheMode,
                  cache_mode, zend_soap_globals, soap_globals)
STD_PHP_INI_ENTRY("soap.wsdl_cache_limit",       "5", PHP_INI_ALL, OnUpdateLong,
                  cache_limit, zend_soap_globals, soap_globals)
PHP_INI_END()

/* Real globals shared for the entire processes across threads, only written during init. */
HashTable php_soap_defEncNs; /* mapping of default namespaces to prefixes */
HashTable php_soap_defEnc, php_soap_defEncIndex;

static void php_soap_prepare_globals(void)
{
	int i;
	encode* enc;

	zend_hash_init(&php_soap_defEnc, 0, NULL, NULL, 1);
	zend_hash_init(&php_soap_defEncIndex, 0, NULL, NULL, 1);
	zend_hash_init(&php_soap_defEncNs, 0, NULL, NULL, 1);

	i = 0;
	do {
		enc = &defaultEncoding[i];

		/* If has a ns and a str_type then index it */
		if (defaultEncoding[i].details.type_str) {
			if (defaultEncoding[i].details.ns != NULL) {
				char *ns_type, *clark_notation;
				size_t clark_notation_len = spprintf(&clark_notation, 0, "{%s}%s", enc->details.ns, enc->details.type_str);
				enc->details.clark_notation = zend_string_init(clark_notation, clark_notation_len, true);
				size_t ns_type_len = spprintf(&ns_type, 0, "%s:%s", enc->details.ns, enc->details.type_str);
				zend_hash_str_add_ptr(&php_soap_defEnc, ns_type, ns_type_len, (void*)enc);
				efree(clark_notation);
				efree(ns_type);
			} else {
				zend_hash_str_add_ptr(&php_soap_defEnc, defaultEncoding[i].details.type_str, strlen(defaultEncoding[i].details.type_str), (void*)enc);
			}
		}
		/* Index everything by number */
		zend_hash_index_add_ptr(&php_soap_defEncIndex, defaultEncoding[i].details.type, (void*)enc);
		i++;
	} while (defaultEncoding[i].details.type != END_KNOWN_TYPES);

	/* hash by namespace */
	zend_hash_str_add_ptr(&php_soap_defEncNs, XSD_1999_NAMESPACE, sizeof(XSD_1999_NAMESPACE)-1, XSD_NS_PREFIX);
	zend_hash_str_add_ptr(&php_soap_defEncNs, XSD_NAMESPACE, sizeof(XSD_NAMESPACE)-1, XSD_NS_PREFIX);
	zend_hash_str_add_ptr(&php_soap_defEncNs, XSI_NAMESPACE, sizeof(XSI_NAMESPACE)-1, XSI_NS_PREFIX);
	zend_hash_str_add_ptr(&php_soap_defEncNs, XML_NAMESPACE, sizeof(XML_NAMESPACE)-1, XML_NS_PREFIX);
	zend_hash_str_add_ptr(&php_soap_defEncNs, SOAP_1_1_ENC_NAMESPACE, sizeof(SOAP_1_1_ENC_NAMESPACE)-1, SOAP_1_1_ENC_NS_PREFIX);
	zend_hash_str_add_ptr(&php_soap_defEncNs, SOAP_1_2_ENC_NAMESPACE, sizeof(SOAP_1_2_ENC_NAMESPACE)-1, SOAP_1_2_ENC_NS_PREFIX);
}

static void php_soap_init_globals(zend_soap_globals *soap_globals)
{
#if defined(COMPILE_DL_SOAP) && defined(ZTS)
	ZEND_TSRMLS_CACHE_UPDATE();
#endif
	soap_globals->typemap = NULL;
	soap_globals->use_soap_error_handler = 0;
	soap_globals->error_code = NULL;
	ZVAL_OBJ(&soap_globals->error_object, NULL);
	soap_globals->sdl = NULL;
	soap_globals->soap_version = SOAP_1_1;
	soap_globals->mem_cache = NULL;
	soap_globals->ref_map = NULL;
}

PHP_MSHUTDOWN_FUNCTION(soap)
{
	int i = 0;
	do {
		if (defaultEncoding[i].details.clark_notation) {
			zend_string_release_ex(defaultEncoding[i].details.clark_notation, 1);
		}
		i++;
	} while (defaultEncoding[i].details.type != END_KNOWN_TYPES);
	zend_error_cb = old_error_handler;
	zend_hash_destroy(&php_soap_defEnc);
	zend_hash_destroy(&php_soap_defEncIndex);
	zend_hash_destroy(&php_soap_defEncNs);
	if (SOAP_GLOBAL(mem_cache)) {
		zend_hash_destroy(SOAP_GLOBAL(mem_cache));
		free(SOAP_GLOBAL(mem_cache));
	}
	zend_string_release_ex(SOAP_GLOBAL(lang_en), true);
	UNREGISTER_INI_ENTRIES();
	return SUCCESS;
}

PHP_RINIT_FUNCTION(soap)
{
	SOAP_GLOBAL(typemap) = NULL;
	SOAP_GLOBAL(use_soap_error_handler) = 0;
	SOAP_GLOBAL(error_code) = NULL;
	ZVAL_OBJ(&SOAP_GLOBAL(error_object), NULL);
	SOAP_GLOBAL(sdl) = NULL;
	SOAP_GLOBAL(soap_version) = SOAP_1_1;
	SOAP_GLOBAL(encoding) = NULL;
	SOAP_GLOBAL(class_map) = NULL;
	SOAP_GLOBAL(features) = 0;
	SOAP_GLOBAL(ref_map) = NULL;
	return SUCCESS;
}

PHP_MINIT_FUNCTION(soap)
{
	/* TODO: add ini entry for always use soap errors */
	php_soap_prepare_globals();
	ZEND_INIT_MODULE_GLOBALS(soap, php_soap_init_globals, NULL);
	REGISTER_INI_ENTRIES();

	/* Register SoapClient class */
	soap_class_entry = register_class_SoapClient();

	/* Register SoapVar class */
	soap_var_class_entry = register_class_SoapVar();

	/* Register SoapServer class */
	soap_server_class_entry = register_class_SoapServer();
	soap_server_class_entry->create_object = soap_server_object_create;
	soap_server_class_entry->default_object_handlers = &soap_server_object_handlers;

	memcpy(&soap_server_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));
	soap_server_object_handlers.offset = XtOffsetOf(soap_server_object, std);
	soap_server_object_handlers.free_obj = soap_server_object_free;
	soap_server_object_handlers.clone_obj = NULL;

	/* Register SoapFault class */
	soap_fault_class_entry = register_class_SoapFault(zend_ce_exception);

	/* Register SoapParam class */
	soap_param_class_entry = register_class_SoapParam();

	soap_header_class_entry = register_class_SoapHeader();

	soap_url_class_entry = register_class_Soap_Url();
	soap_url_class_entry->create_object = soap_url_object_create;
	soap_url_class_entry->default_object_handlers = &soap_url_object_handlers;

	memcpy(&soap_url_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));
	soap_url_object_handlers.offset = XtOffsetOf(soap_url_object, std);
	soap_url_object_handlers.free_obj = soap_url_object_free;
	soap_url_object_handlers.get_constructor = soap_url_object_get_constructor;
	soap_url_object_handlers.clone_obj = NULL;
	soap_url_object_handlers.cast_object = soap_url_cast_object;
	soap_url_object_handlers.compare = zend_objects_not_comparable;

	soap_sdl_class_entry = register_class_Soap_Sdl();
	soap_sdl_class_entry->create_object = soap_sdl_object_create;
	soap_sdl_class_entry->default_object_handlers = &soap_sdl_object_handlers;

	memcpy(&soap_sdl_object_handlers, &std_object_handlers, sizeof(zend_object_handlers));
	soap_sdl_object_handlers.offset = XtOffsetOf(soap_sdl_object, std);
	soap_sdl_object_handlers.free_obj = soap_sdl_object_free;
	soap_sdl_object_handlers.get_constructor = soap_sdl_object_get_constructor;
	soap_sdl_object_handlers.clone_obj = NULL;
	soap_url_object_handlers.cast_object = soap_sdl_cast_object;
	soap_sdl_object_handlers.compare = zend_objects_not_comparable;

	register_soap_symbols(module_number);

	old_error_handler = zend_error_cb;
	zend_error_cb = soap_error_handler;

	SOAP_GLOBAL(lang_en) = zend_string_init_interned(ZEND_STRL("en"), true);

	return SUCCESS;
}

PHP_MINFO_FUNCTION(soap)
{
	php_info_print_table_start();
	php_info_print_table_row(2, "Soap Client", "enabled");
	php_info_print_table_row(2, "Soap Server", "enabled");
	php_info_print_table_end();
	DISPLAY_INI_ENTRIES();
}


/* {{{ SoapParam constructor */
PHP_METHOD(SoapParam, __construct)
{
	zval *data;
	zend_string *name;
	zval *this_ptr;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "zS", &data, &name) == FAILURE) {
		RETURN_THROWS();
	}

	if (ZSTR_LEN(name) == 0) {
		zend_argument_must_not_be_empty_error(2);
		RETURN_THROWS();
	}

	this_ptr = ZEND_THIS;
	ZVAL_STR_COPY(Z_PARAM_NAME_P(this_ptr), name);
	ZVAL_COPY(Z_PARAM_DATA_P(this_ptr), data);
}
/* }}} */


/* {{{ SoapHeader constructor */
PHP_METHOD(SoapHeader, __construct)
{
	zval *data = NULL;
	zend_string *ns, *name, *actor_str = NULL;
	zend_long actor_long;
	bool actor_is_null = 1;
	bool must_understand = 0;
	zval *this_ptr;

	ZEND_PARSE_PARAMETERS_START(2, 5)
		Z_PARAM_STR(ns)
		Z_PARAM_STR(name)
		Z_PARAM_OPTIONAL
		Z_PARAM_ZVAL(data)
		Z_PARAM_BOOL(must_understand)
		Z_PARAM_STR_OR_LONG_OR_NULL(actor_str, actor_long, actor_is_null)
	ZEND_PARSE_PARAMETERS_END();

	if (ZSTR_LEN(ns) == 0) {
		zend_argument_must_not_be_empty_error(1);
		RETURN_THROWS();
	}
	if (ZSTR_LEN(name) == 0) {
		zend_argument_must_not_be_empty_error(2);
		RETURN_THROWS();
	}

	this_ptr = ZEND_THIS;
	ZVAL_STR_COPY(Z_HEADER_NAMESPACE_P(this_ptr), ns);
	ZVAL_STR_COPY(Z_HEADER_NAME_P(this_ptr), name);
	if (data) {
		ZVAL_COPY(Z_HEADER_DATA_P(this_ptr), data);
	}
	ZVAL_BOOL(Z_HEADER_MUST_UNDERSTAND_P(this_ptr), must_understand);

	if (actor_str) {
		if (ZSTR_LEN(actor_str) > 2) {
			ZVAL_STR_COPY(Z_HEADER_ACTOR_P(this_ptr), actor_str);
		} else {
			zend_argument_value_error(5, "must be longer than 2 characters");
			RETURN_THROWS();
		}
	} else if (!actor_is_null) {
		if ((actor_long == SOAP_ACTOR_NEXT || actor_long == SOAP_ACTOR_NONE || actor_long == SOAP_ACTOR_UNLIMATERECEIVER)) {
			ZVAL_LONG(Z_HEADER_ACTOR_P(this_ptr), actor_long);
		} else {
			zend_argument_value_error(5, "must be one of SOAP_ACTOR_NEXT, SOAP_ACTOR_NONE, or SOAP_ACTOR_UNLIMATERECEIVER");
			RETURN_THROWS();
		}
	}
}
/* }}} */

static void soap_fault_dtor_properties(zval *obj)
{
	zval_ptr_dtor(Z_FAULT_STRING_P(obj));
	zval_ptr_dtor(Z_FAULT_CODE_P(obj));
	zval_ptr_dtor(Z_FAULT_CODENS_P(obj));
	zval_ptr_dtor(Z_FAULT_ACTOR_P(obj));
	zval_ptr_dtor(Z_FAULT_DETAIL_P(obj));
	zval_ptr_dtor(Z_FAULT_NAME_P(obj));
	zval_ptr_dtor(Z_FAULT_HEADERFAULT_P(obj));
	zval_ptr_dtor(Z_FAULT_LANG_P(obj));
	ZVAL_EMPTY_STRING(Z_FAULT_STRING_P(obj));
	ZVAL_NULL(Z_FAULT_CODE_P(obj));
	ZVAL_NULL(Z_FAULT_CODENS_P(obj));
	ZVAL_NULL(Z_FAULT_ACTOR_P(obj));
	ZVAL_NULL(Z_FAULT_DETAIL_P(obj));
	ZVAL_NULL(Z_FAULT_NAME_P(obj));
	ZVAL_NULL(Z_FAULT_HEADERFAULT_P(obj));
	ZVAL_EMPTY_STRING(Z_FAULT_LANG_P(obj));
}

/* {{{ SoapFault constructor */
PHP_METHOD(SoapFault, __construct)
{
	char *fault_string = NULL, *fault_code = NULL, *fault_actor = NULL, *fault_code_ns = NULL;
	size_t fault_string_len, fault_actor_len = 0, fault_code_len = 0;
	zend_string *name = NULL;
	zend_string *lang = ZSTR_EMPTY_ALLOC();
	zval *details = NULL, *headerfault = NULL, *this_ptr;
	zend_string *code_str;
	HashTable *code_ht;

	ZEND_PARSE_PARAMETERS_START(2, 7)
		Z_PARAM_ARRAY_HT_OR_STR_OR_NULL(code_ht, code_str)
		Z_PARAM_STRING(fault_string, fault_string_len)
		Z_PARAM_OPTIONAL
		Z_PARAM_STRING_OR_NULL(fault_actor, fault_actor_len)
		Z_PARAM_ZVAL_OR_NULL(details)
		Z_PARAM_STR_OR_NULL(name)
		Z_PARAM_ZVAL_OR_NULL(headerfault)
		Z_PARAM_PATH_STR(lang)
	ZEND_PARSE_PARAMETERS_END();

	if (code_str) {
		fault_code = ZSTR_VAL(code_str);
		fault_code_len = ZSTR_LEN(code_str);
	} else if (code_ht && zend_hash_num_elements(code_ht) == 2) {
		zval *t_ns = zend_hash_index_find(code_ht, 0);
		zval *t_code = zend_hash_index_find(code_ht, 1);
		if (t_ns && t_code && Z_TYPE_P(t_ns) == IS_STRING && Z_TYPE_P(t_code) == IS_STRING) {
			fault_code_ns = Z_STRVAL_P(t_ns);
			fault_code = Z_STRVAL_P(t_code);
			fault_code_len = Z_STRLEN_P(t_code);
		}
	}

	if ((code_str || code_ht) && (fault_code == NULL || fault_code_len == 0)) {
		zend_argument_value_error(1, "is not a valid fault code");
		RETURN_THROWS();
	}

	/* Delete previously set properties */
	soap_fault_dtor_properties(ZEND_THIS);

	if (name != NULL && ZSTR_LEN(name) == 0) {
		name = NULL;
	}

	this_ptr = ZEND_THIS;
	set_soap_fault(this_ptr, fault_code_ns, fault_code, fault_string, fault_actor, details, name, lang);
	if (headerfault != NULL) {
		ZVAL_COPY(Z_FAULT_HEADERFAULT_P(this_ptr), headerfault);
	}
}
/* }}} */

/* {{{ SoapFault constructor */
PHP_METHOD(SoapFault, __toString)
{
	zval *faultcode, *faultstring, *file, *line, trace, rv1, rv2, rv3, rv4;
	zend_string *str;
	zval *this_ptr;
	zend_string *faultcode_val, *faultstring_val, *file_val;
	zend_long line_val;

	if (zend_parse_parameters_none() == FAILURE) {
		RETURN_THROWS();
	}

	this_ptr = ZEND_THIS;
	faultcode = zend_read_property(soap_fault_class_entry, Z_OBJ_P(this_ptr), "faultcode", sizeof("faultcode")-1, 1, &rv1);
	faultstring = zend_read_property(soap_fault_class_entry, Z_OBJ_P(this_ptr), "faultstring", sizeof("faultstring")-1, 1, &rv2);
	file = zend_read_property_ex(soap_fault_class_entry, Z_OBJ_P(this_ptr), ZSTR_KNOWN(ZEND_STR_FILE), /* silent */ true, &rv3);
	line = zend_read_property_ex(soap_fault_class_entry, Z_OBJ_P(this_ptr), ZSTR_KNOWN(ZEND_STR_LINE), /* silent */ true, &rv4);

	zend_call_method_with_0_params(
		Z_OBJ_P(ZEND_THIS), Z_OBJCE_P(ZEND_THIS), NULL, "gettraceasstring", &trace);

	faultcode_val = zval_get_string(faultcode);
	faultstring_val = zval_get_string(faultstring);
	file_val = zval_get_string(file);
	line_val = zval_get_long(line);
	convert_to_string(&trace);

	str = strpprintf(0, "SoapFault exception: [%s] %s in %s:" ZEND_LONG_FMT "\nStack trace:\n%s",
	               ZSTR_VAL(faultcode_val), ZSTR_VAL(faultstring_val), ZSTR_VAL(file_val), line_val,
	               Z_STRLEN(trace) ? Z_STRVAL(trace) : "#0 {main}\n");

	zend_string_release_ex(file_val, 0);
	zend_string_release_ex(faultstring_val, 0);
	zend_string_release_ex(faultcode_val, 0);
	zval_ptr_dtor(&trace);

	RETVAL_STR(str);
}
/* }}} */

/* {{{ SoapVar constructor */
PHP_METHOD(SoapVar, __construct)
{
	zval *data, *this_ptr;
	zend_long type;
	bool type_is_null = 1;
	zend_string *stype = NULL, *ns = NULL, *name = NULL, *namens = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "z!l!|S!S!S!S!", &data, &type, &type_is_null, &stype, &ns, &name, &namens) == FAILURE) {
		RETURN_THROWS();
	}

	this_ptr = ZEND_THIS;
	if (type_is_null) {
		ZVAL_LONG(Z_VAR_ENC_TYPE_P(this_ptr), UNKNOWN_TYPE);
	} else {
		if (zend_hash_index_exists(&php_soap_defEncIndex, type)) {
			ZVAL_LONG(Z_VAR_ENC_TYPE_P(this_ptr), type);
		} else {
			zend_argument_value_error(2, "is not a valid encoding");
			RETURN_THROWS();
		}
	}

	if (data) {
		ZVAL_COPY(Z_VAR_ENC_VALUE_P(this_ptr), data);
	}

	if (stype && ZSTR_LEN(stype) != 0) {
		ZVAL_STR_COPY(Z_VAR_ENC_STYPE_P(this_ptr), stype);
	}
	if (ns && ZSTR_LEN(ns) != 0) {
		ZVAL_STR_COPY(Z_VAR_ENC_NS_P(this_ptr), ns);
	}
	if (name && ZSTR_LEN(name) != 0) {
		ZVAL_STR_COPY(Z_VAR_ENC_NAME_P(this_ptr), name);
	}
	if (namens && ZSTR_LEN(namens) != 0) {
		ZVAL_STR_COPY(Z_VAR_ENC_NAMENS_P(this_ptr), namens);
	}
}
/* }}} */

static HashTable* soap_create_typemap(sdlPtr sdl, HashTable *ht) /* {{{ */
{
	zval *tmp;
	HashTable *ht2;
	HashTable *typemap = NULL;

	ZEND_HASH_FOREACH_VAL(ht, tmp) {
		char *type_name = NULL;
		char *type_ns = NULL;
		zval *to_xml = NULL;
		zval *to_zval = NULL;
		encodePtr enc, new_enc;
		zend_string *name;

		if (Z_TYPE_P(tmp) != IS_ARRAY) {
			zend_type_error("SoapHeader::__construct(): \"typemap\" option must be of type array, %s given", zend_zval_value_name(tmp));
			return NULL;
		}
		ht2 = Z_ARRVAL_P(tmp);

		if (!HT_IS_PACKED(ht2)) {
			ZEND_HASH_MAP_FOREACH_STR_KEY_VAL(ht2, name, tmp) {
				if (name) {
					if (zend_string_equals_literal(name, "type_name")) {
						if (Z_TYPE_P(tmp) == IS_STRING) {
							type_name = Z_STRVAL_P(tmp);
						} else if (Z_TYPE_P(tmp) != IS_NULL) {
						}
					} else if (zend_string_equals_literal(name, "type_ns")) {
						if (Z_TYPE_P(tmp) == IS_STRING) {
							type_ns = Z_STRVAL_P(tmp);
						} else if (Z_TYPE_P(tmp) != IS_NULL) {
						}
					} else if (zend_string_equals_literal(name, "to_xml")) {
						to_xml = tmp;
					} else if (zend_string_equals_literal(name, "from_xml")) {
						to_zval = tmp;
					}
				}
			} ZEND_HASH_FOREACH_END();
		}

		if (type_name) {
			smart_str nscat = {0};

			if (type_ns) {
				enc = get_encoder(sdl, type_ns, type_name);
			} else {
				enc = get_encoder_ex(sdl, type_name, strlen(type_name));
			}

			new_enc = emalloc(sizeof(encode));
			memset(new_enc, 0, sizeof(encode));

			if (enc) {
				new_enc->details.type = enc->details.type;
				new_enc->details.ns = estrdup(enc->details.ns);
				new_enc->details.type_str = estrdup(enc->details.type_str);
				new_enc->details.sdl_type = enc->details.sdl_type;
			} else {
				enc = get_conversion(UNKNOWN_TYPE);
				new_enc->details.type = enc->details.type;
				if (type_ns) {
					new_enc->details.ns = estrdup(type_ns);
				}
				new_enc->details.type_str = estrdup(type_name);
			}
			new_enc->to_xml = enc->to_xml;
			new_enc->to_zval = enc->to_zval;
			new_enc->details.map = emalloc(sizeof(soapMapping));
			memset(new_enc->details.map, 0, sizeof(soapMapping));
			if (to_xml) {
				ZVAL_COPY(&new_enc->details.map->to_xml, to_xml);
				new_enc->to_xml = to_xml_user;
			} else if (enc->details.map && Z_TYPE(enc->details.map->to_xml) != IS_UNDEF) {
				ZVAL_COPY(&new_enc->details.map->to_xml, &enc->details.map->to_xml);
			}
			if (to_zval) {
				ZVAL_COPY(&new_enc->details.map->to_zval, to_zval);
				new_enc->to_zval = to_zval_user;
			} else if (enc->details.map && Z_TYPE(enc->details.map->to_zval) != IS_UNDEF) {
				ZVAL_COPY(&new_enc->details.map->to_zval, &enc->details.map->to_zval);
			}
			if (!typemap) {
				typemap = emalloc(sizeof(HashTable));
				zend_hash_init(typemap, 0, NULL, delete_encoder, 0);
			}

			if (type_ns) {
				smart_str_appends(&nscat, type_ns);
				smart_str_appendc(&nscat, ':');
			}
			smart_str_appends(&nscat, type_name);
			smart_str_0(&nscat);
			zend_hash_update_ptr(typemap, nscat.s, new_enc);
			smart_str_free(&nscat);
		}
	} ZEND_HASH_FOREACH_END();
	return typemap;
}
/* }}} */

/* {{{ SoapServer constructor */
PHP_METHOD(SoapServer, __construct)
{
	soapServicePtr service;
	zval *options = NULL;
	zend_string *wsdl;
	int version = SOAP_1_1;
	zend_long cache_wsdl;
	HashTable *typemap_ht = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "S!|a", &wsdl, &options) == FAILURE) {
		RETURN_THROWS();
	}

	SOAP_SERVER_BEGIN_CODE();

	service = emalloc(sizeof(soapService));
	memset(service, 0, sizeof(soapService));
	service->send_errors = 1;

	cache_wsdl = SOAP_GLOBAL(cache_enabled) ? SOAP_GLOBAL(cache_mode) : 0;

	if (options != NULL) {
		HashTable *ht = Z_ARRVAL_P(options);
		zval *tmp;

		if ((tmp = zend_hash_str_find(ht, "soap_version", sizeof("soap_version")-1)) != NULL) {
			if (Z_TYPE_P(tmp) == IS_LONG &&
			    (Z_LVAL_P(tmp) == SOAP_1_1 || Z_LVAL_P(tmp) == SOAP_1_2)) {
				version = Z_LVAL_P(tmp);
			} else {
				php_error_docref(NULL, E_ERROR, "'soap_version' option must be SOAP_1_1 or SOAP_1_2");
			}
		}

		if ((tmp = zend_hash_str_find(ht, "uri", sizeof("uri")-1)) != NULL &&
		    Z_TYPE_P(tmp) == IS_STRING) {
			service->uri = estrndup(Z_STRVAL_P(tmp), Z_STRLEN_P(tmp));
		} else if (!wsdl) {
			php_error_docref(NULL, E_ERROR, "'uri' option is required in nonWSDL mode");
		}

		if ((tmp = zend_hash_str_find(ht, "actor", sizeof("actor")-1)) != NULL &&
		    Z_TYPE_P(tmp) == IS_STRING) {
			service->actor = estrndup(Z_STRVAL_P(tmp), Z_STRLEN_P(tmp));
		}

		if ((tmp = zend_hash_str_find(ht, "encoding", sizeof("encoding")-1)) != NULL &&
		    Z_TYPE_P(tmp) == IS_STRING) {
			xmlCharEncodingHandlerPtr encoding;

			encoding = xmlFindCharEncodingHandler(Z_STRVAL_P(tmp));
			if (encoding == NULL) {
				php_error_docref(NULL, E_ERROR, "Invalid 'encoding' option - '%s'", Z_STRVAL_P(tmp));
			} else {
			  service->encoding = encoding;
			}
		}

		if ((tmp = zend_hash_str_find(ht, "classmap", sizeof("classmap")-1)) != NULL &&
			Z_TYPE_P(tmp) == IS_ARRAY) {
			if (HT_IS_PACKED(Z_ARRVAL_P(tmp))) {
				php_error_docref(NULL, E_ERROR, "'classmap' option must be an associative array");
			}
			service->class_map = zend_array_dup(Z_ARRVAL_P(tmp));
		}

		if ((tmp = zend_hash_str_find(ht, "typemap", sizeof("typemap")-1)) != NULL &&
			Z_TYPE_P(tmp) == IS_ARRAY &&
			zend_hash_num_elements(Z_ARRVAL_P(tmp)) > 0) {
			typemap_ht = Z_ARRVAL_P(tmp);
		}

		if ((tmp = zend_hash_str_find(ht, "features", sizeof("features")-1)) != NULL &&
			Z_TYPE_P(tmp) == IS_LONG) {
			service->features = Z_LVAL_P(tmp);
		}

		if ((tmp = zend_hash_str_find(ht, "cache_wsdl", sizeof("cache_wsdl")-1)) != NULL &&
		    Z_TYPE_P(tmp) == IS_LONG) {
			cache_wsdl = Z_LVAL_P(tmp);
		}

		if ((tmp = zend_hash_str_find(ht, "send_errors", sizeof("send_errors")-1)) != NULL) {
			if (Z_TYPE_P(tmp) == IS_FALSE) {
				service->send_errors = 0;
			} else if (Z_TYPE_P(tmp) == IS_TRUE) {
				service->send_errors = 1;
			} else if (Z_TYPE_P(tmp) == IS_LONG) {
				service->send_errors = Z_LVAL_P(tmp);
			}
		}

		if ((tmp = zend_hash_find(ht, ZSTR_KNOWN(ZEND_STR_TRACE))) != NULL &&
		    (Z_TYPE_P(tmp) == IS_TRUE ||
		     (Z_TYPE_P(tmp) == IS_LONG && Z_LVAL_P(tmp) == 1))) {
			service->trace = true;
		}
	} else if (!wsdl) {
		php_error_docref(NULL, E_ERROR, "'uri' option is required in nonWSDL mode");
	}

	service->version = version;
	service->type = SOAP_FUNCTIONS;
	service->soap_functions.functions_all = FALSE;
	service->soap_functions.ft = zend_new_array(0);

	if (wsdl) {
		service->sdl = get_sdl(ZEND_THIS, ZSTR_VAL(wsdl), cache_wsdl);
		if (service->uri == NULL) {
			if (service->sdl->target_ns) {
				service->uri = estrdup(service->sdl->target_ns);
			} else {
				/*FIXME*/
				service->uri = estrdup("http://unknown-uri/");
			}
		}
	}

	if (typemap_ht) {
		service->typemap = soap_create_typemap(service->sdl, typemap_ht);
	}

	soap_server_object *server_obj = soap_server_object_fetch(Z_OBJ_P(ZEND_THIS));
	server_obj->service = service;

	SOAP_SERVER_END_CODE();
}
/* }}} */


/* {{{ Sets persistence mode of SoapServer */
PHP_METHOD(SoapServer, setPersistence)
{
	soapServicePtr service;
	zend_long value;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &value) == FAILURE) {
		RETURN_THROWS();
	}

	FETCH_THIS_SERVICE_NO_BAILOUT(service);

	if (service->type == SOAP_CLASS) {
		if (value == SOAP_PERSISTENCE_SESSION ||
			value == SOAP_PERSISTENCE_REQUEST) {
			if (value == SOAP_PERSISTENCE_SESSION && !zend_hash_str_exists(&module_registry, "session", sizeof("session")-1)) {
				zend_throw_error(NULL, "SoapServer::setPersistence(): Session persistence cannot be enabled because the session module is not enabled");
				RETURN_THROWS();
			}

			service->soap_class.persistence = value;
		} else {
			zend_argument_value_error(
				1, "must be either SOAP_PERSISTENCE_SESSION or SOAP_PERSISTENCE_REQUEST when the SOAP server is used in class mode"
			);
		}
	} else {
		zend_throw_error(NULL, "SoapServer::setPersistence(): Persistence cannot be set when the SOAP server is used in function mode");
	}
}
/* }}} */


/* {{{ Sets class which will handle SOAP requests */
PHP_METHOD(SoapServer, setClass)
{
	soapServicePtr service;
	zend_class_entry *ce = NULL;
	uint32_t num_args = 0;
	zval *argv = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "C*", &ce, &argv, &num_args) == FAILURE) {
		RETURN_THROWS();
	}

	FETCH_THIS_SERVICE_NO_BAILOUT(service);

	service->type = SOAP_CLASS;
	service->soap_class.ce = ce;

	delete_argv(&service->soap_class);

	service->soap_class.persistence = SOAP_PERSISTENCE_REQUEST;
	service->soap_class.argc = num_args;
	if (service->soap_class.argc > 0) {
		service->soap_class.argv = safe_emalloc(service->soap_class.argc, sizeof(zval), 0);
		for (uint32_t i = 0; i < service->soap_class.argc; i++) {
			ZVAL_COPY(&service->soap_class.argv[i], &argv[i]);
		}
	}
}
/* }}} */


/* {{{ Sets object which will handle SOAP requests */
PHP_METHOD(SoapServer, setObject)
{
	soapServicePtr service;
	zval *obj;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "o", &obj) == FAILURE) {
		RETURN_THROWS();
	}

	FETCH_THIS_SERVICE_NO_BAILOUT(service);

	service->type = SOAP_OBJECT;

	zval_ptr_dtor(&service->soap_object);
	ZVAL_OBJ_COPY(&service->soap_object, Z_OBJ_P(obj));
}
/* }}} */


/* {{{ Returns list of defined functions */
PHP_METHOD(SoapServer, getFunctions)
{
	soapServicePtr  service;
	HashTable      *ft = NULL;

	if (zend_parse_parameters_none() == FAILURE) {
		RETURN_THROWS();
	}

	FETCH_THIS_SERVICE_NO_BAILOUT(service);

	array_init(return_value);
	if (service->type == SOAP_OBJECT) {
		ft = &(Z_OBJCE(service->soap_object)->function_table);
	} else if (service->type == SOAP_CLASS) {
		ft = &service->soap_class.ce->function_table;
	} else if (service->soap_functions.functions_all == TRUE) {
		ft = EG(function_table);
	} else if (service->soap_functions.ft != NULL) {
		zval *name;

		ZEND_HASH_MAP_FOREACH_VAL(service->soap_functions.ft, name) {
			add_next_index_str(return_value, zend_string_copy(Z_STR_P(name)));
		} ZEND_HASH_FOREACH_END();
	}
	if (ft != NULL) {
		zend_function *f;

		ZEND_HASH_MAP_FOREACH_PTR(ft, f) {
			if ((service->type != SOAP_OBJECT && service->type != SOAP_CLASS) || (f->common.fn_flags & ZEND_ACC_PUBLIC)) {
				add_next_index_str(return_value, zend_string_copy(f->common.function_name));
			}
		} ZEND_HASH_FOREACH_END();
	}
}
/* }}} */


/* {{{ Adds one or several functions those will handle SOAP requests */
PHP_METHOD(SoapServer, addFunction)
{
	soapServicePtr service;
	zval *function_name, function_copy;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "z", &function_name) == FAILURE) {
		RETURN_THROWS();
	}

	FETCH_THIS_SERVICE_NO_BAILOUT(service);

	/* TODO: could use zend_is_callable here */

	if (Z_TYPE_P(function_name) == IS_ARRAY) {
		if (service->type == SOAP_FUNCTIONS) {
			zval *tmp_function;

			if (service->soap_functions.ft == NULL) {
				service->soap_functions.functions_all = FALSE;
				service->soap_functions.ft = zend_new_array(zend_hash_num_elements(Z_ARRVAL_P(function_name)));
			}

			ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(function_name), tmp_function) {
				zend_string *key;
				zend_function *f;

				if (Z_TYPE_P(tmp_function) != IS_STRING) {
					zend_argument_type_error(1, "must contain only strings");
					RETURN_THROWS();
				}

				key = zend_string_tolower(Z_STR_P(tmp_function));

				if ((f = zend_hash_find_ptr(EG(function_table), key)) == NULL) {
					zend_string_release_ex(key, false);
					zend_type_error("SoapServer::addFunction(): Function \"%s\" not found", Z_STRVAL_P(tmp_function));
					RETURN_THROWS();
				}

				ZVAL_STR_COPY(&function_copy, f->common.function_name);
				zend_hash_update(service->soap_functions.ft, key, &function_copy);

				zend_string_release_ex(key, 0);
			} ZEND_HASH_FOREACH_END();
		}
	} else if (Z_TYPE_P(function_name) == IS_STRING) {
		zend_string *key;
		zend_function *f;

		key = zend_string_tolower(Z_STR_P(function_name));

		if ((f = zend_hash_find_ptr(EG(function_table), key)) == NULL) {
			zend_string_release_ex(key, false);
			zend_argument_type_error(1, "must be a valid function name, function \"%s\" not found", Z_STRVAL_P(function_name));
			RETURN_THROWS();
		}
		if (service->soap_functions.ft == NULL) {
			service->soap_functions.functions_all = FALSE;
			service->soap_functions.ft = zend_new_array(0);
		}

		ZVAL_STR_COPY(&function_copy, f->common.function_name);
		zend_hash_update(service->soap_functions.ft, key, &function_copy);
		zend_string_release_ex(key, 0);
	} else if (Z_TYPE_P(function_name) == IS_LONG) {
		if (Z_LVAL_P(function_name) == SOAP_FUNCTIONS_ALL) {
			php_error_docref(NULL, E_DEPRECATED, "Enabling all functions via SOAP_FUNCTIONS_ALL is deprecated since 8.4, due to possible security concerns."
				" If all PHP functions should be enabled, the flattened return value of get_defined_functions() can be used");
			if (UNEXPECTED(EG(exception))) {
				RETURN_THROWS();
			}
			if (service->soap_functions.ft != NULL) {
				zend_hash_destroy(service->soap_functions.ft);
				efree(service->soap_functions.ft);
				service->soap_functions.ft = NULL;
			}
			service->soap_functions.functions_all = TRUE;
		} else {
			zend_argument_value_error(1, "must be SOAP_FUNCTIONS_ALL when an integer is passed");
		}
	} else {
		zend_argument_type_error(1, "must be of type array|string|int, %s given", zend_zval_value_name(function_name));
	}
}
/* }}} */

static void _soap_server_exception(soapServicePtr service, sdlFunctionPtr function, zval *this_ptr) /* {{{ */
{
	zval exception_object;

	ZVAL_OBJ(&exception_object, EG(exception));
	if (instanceof_function(Z_OBJCE(exception_object), soap_fault_class_entry)) {
		soap_server_fault_ex(function, &exception_object, NULL);
	} else if (instanceof_function(Z_OBJCE(exception_object), zend_ce_error)) {
		if (service->send_errors) {
			zval rv;
			zend_string *msg = zval_get_string(zend_read_property_ex(zend_ce_error, Z_OBJ(exception_object), ZSTR_KNOWN(ZEND_STR_MESSAGE), /* silent */ false, &rv));
			add_soap_fault_ex_en(&exception_object, this_ptr, "Server", ZSTR_VAL(msg), NULL, NULL);
			zend_string_release_ex(msg, 0);
		} else {
			add_soap_fault_ex_en(&exception_object, this_ptr, "Server", "Internal Error", NULL, NULL);
		}
		soap_server_fault_ex(function, &exception_object, NULL);
	}
}
/* }}} */

/* {{{ Handles a SOAP request */
PHP_METHOD(SoapServer, handle)
{
	int soap_version, old_soap_version;
	sdlPtr old_sdl = NULL;
	soapServicePtr service;
	xmlDocPtr doc_request = NULL, doc_return = NULL;
	zval function_name, *params, *soap_obj, retval;
	char cont_len[30];
	uint32_t num_params = 0;
	int size, i, call_status = 0;
	xmlChar *buf;
	HashTable *function_table;
	soapHeader *soap_headers = NULL;
	sdlFunctionPtr function;
	char *arg = NULL;
	size_t arg_len = 0;
	xmlCharEncodingHandlerPtr old_encoding;
	HashTable *old_class_map, *old_typemap;
	int old_features;
	zval tmp_soap;
	const char *soap_action = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "|s!", &arg, &arg_len) == FAILURE) {
		RETURN_THROWS();
	}

	SOAP_SERVER_BEGIN_CODE();

	FETCH_THIS_SERVICE(service);
	SOAP_GLOBAL(soap_version) = service->version;

	if (arg && ZEND_SIZE_T_INT_OVFL(arg_len)) {
		soap_server_fault_en("Server", "Input string is too long", NULL, NULL, NULL);
		SOAP_SERVER_END_CODE();
		return;
	}

	if (SG(request_info).request_method &&
	    strcmp(SG(request_info).request_method, "GET") == 0 &&
	    SG(request_info).query_string &&
	    stricmp(SG(request_info).query_string, "wsdl") == 0) {

		if (service->sdl) {
/*
			char *hdr = emalloc(sizeof("Location: ")+strlen(service->sdl->source));
			strcpy(hdr,"Location: ");
			strcat(hdr,service->sdl->source);
			sapi_add_header(hdr, sizeof("Location: ")+strlen(service->sdl->source)-1, 1);
			efree(hdr);
*/

			sapi_add_header("Content-Type: text/xml; charset=utf-8", sizeof("Content-Type: text/xml; charset=utf-8")-1, 1);
			php_stream_context *context = php_stream_context_from_zval(NULL, false);
			php_stream *stream = php_stream_open_wrapper_ex(service->sdl->source, "rb", REPORT_ERRORS, NULL, context);
			if (stream) {
				php_stream_passthru(stream);
				php_stream_close(stream);
			} else {
				soap_server_fault_en("Server", "Couldn't find WSDL", NULL, NULL, NULL);
			}

			SOAP_SERVER_END_CODE();
			return;
		} else {
			soap_server_fault_en("Server", "WSDL generation is not supported yet", NULL, NULL, NULL);
/*
			sapi_add_header("Content-Type: text/xml; charset=utf-8", sizeof("Content-Type: text/xml; charset=utf-8"), 1);
			PUTS("<?xml version=\"1.0\" ?>\n<definitions\n");
			PUTS("    xmlns=\"http://schemas.xmlsoap.org/wsdl/\"\n");
			PUTS("    targetNamespace=\"");
			PUTS(service->uri);
			PUTS("\">\n");
			PUTS("</definitions>");
*/
			SOAP_SERVER_END_CODE();
			return;
		}
	}

	if (php_output_start_default() != SUCCESS) {
		php_error_docref(NULL, E_ERROR,"ob_start failed");
	}

	if (!arg) {
		if (SG(request_info).request_body && 0 == php_stream_rewind(SG(request_info).request_body)) {
			zval *server_vars, *encoding, *soap_action_z;
			php_stream_filter *zf = NULL;
			zend_string *server = ZSTR_KNOWN(ZEND_STR_AUTOGLOBAL_SERVER);

			zend_is_auto_global(server);
			if ((server_vars = zend_hash_find(&EG(symbol_table), server)) != NULL && Z_TYPE_P(server_vars) == IS_ARRAY) {
			    if ((encoding = zend_hash_str_find(Z_ARRVAL_P(server_vars), "HTTP_CONTENT_ENCODING", sizeof("HTTP_CONTENT_ENCODING")-1)) != NULL && Z_TYPE_P(encoding) == IS_STRING) {
					if (zend_string_equals_literal(Z_STR_P(encoding), "gzip")
						|| zend_string_equals_literal(Z_STR_P(encoding), "x-gzip")
						|| zend_string_equals_literal(Z_STR_P(encoding), "deflate")
					) {
						zval filter_params;

						array_init_size(&filter_params, 1);
						add_assoc_long_ex(&filter_params, "window", sizeof("window")-1, 0x2f); /* ANY WBITS */

						zf = php_stream_filter_create("zlib.inflate", &filter_params, 0);
						zend_array_destroy(Z_ARR(filter_params));

						if (zf) {
							php_stream_filter_append(&SG(request_info).request_body->readfilters, zf);
						} else {
							php_error_docref(NULL, E_WARNING,"Can't uncompress compressed request");
							SOAP_SERVER_END_CODE();
							return;
						}
					} else {
						php_error_docref(NULL, E_WARNING,"Request is compressed with unknown compression '%s'",Z_STRVAL_P(encoding));
						SOAP_SERVER_END_CODE();
						return;
					}
				}
			}

			if ((soap_action_z = zend_hash_str_find(Z_ARRVAL_P(server_vars), ZEND_STRL("HTTP_SOAPACTION"))) != NULL && Z_TYPE_P(soap_action_z) == IS_STRING) {
				soap_action = Z_STRVAL_P(soap_action_z);
			}

			doc_request = soap_xmlParseFile("php://input");

			if (zf) {
				php_stream_filter_remove(zf, 1);
			}
		} else {
			SOAP_SERVER_END_CODE();
			return;
		}
	} else {
		doc_request = soap_xmlParseMemory(arg,arg_len);
	}

	if (doc_request == NULL) {
		soap_server_fault_en("Client", "Bad Request", NULL, NULL, NULL);
	}
	if (xmlGetIntSubset(doc_request) != NULL) {
		xmlNodePtr env = get_node(doc_request->children,"Envelope");
		if (env && env->ns) {
			if (strcmp((char*)env->ns->href, SOAP_1_1_ENV_NAMESPACE) == 0) {
				SOAP_GLOBAL(soap_version) = SOAP_1_1;
			} else if (strcmp((char*)env->ns->href,SOAP_1_2_ENV_NAMESPACE) == 0) {
				SOAP_GLOBAL(soap_version) = SOAP_1_2;
			}
		}
		xmlFreeDoc(doc_request);
		soap_server_fault_en("Server", "DTD are not supported by SOAP", NULL, NULL, NULL);
	}

	old_sdl = SOAP_GLOBAL(sdl);
	SOAP_GLOBAL(sdl) = service->sdl;
	old_encoding = SOAP_GLOBAL(encoding);
	SOAP_GLOBAL(encoding) = service->encoding;
	old_class_map = SOAP_GLOBAL(class_map);
	SOAP_GLOBAL(class_map) = service->class_map;
	old_typemap = SOAP_GLOBAL(typemap);
	SOAP_GLOBAL(typemap) = service->typemap;
	old_features = SOAP_GLOBAL(features);
	SOAP_GLOBAL(features) = service->features;
	old_soap_version = SOAP_GLOBAL(soap_version);

	zend_try {
		function = deserialize_function_call(service->sdl, doc_request, service->actor, soap_action, &function_name, &num_params, &params, &soap_version, &soap_headers);
	} zend_catch {
		/* Avoid leaking persistent memory */
		xmlFreeDoc(doc_request);
		zend_bailout();
	} zend_end_try();

	xmlFreeDoc(doc_request);

	ZVAL_NULL(&retval);

	if (EG(exception)) {
		if (!zend_is_unwind_exit(EG(exception))) {
			php_output_discard();
			_soap_server_exception(service, function, ZEND_THIS);
		}
		goto fail;
	}

	service->soap_headers_ptr = &soap_headers;

	soap_obj = NULL;
	if (service->type == SOAP_OBJECT) {
		soap_obj = &service->soap_object;
		function_table = &((Z_OBJCE_P(soap_obj))->function_table);
	} else if (service->type == SOAP_CLASS) {
		/* If persistent then set soap_obj from the previous created session (if available) */
#ifdef SOAP_HAS_SESSION_SUPPORT
		if (service->soap_class.persistence == SOAP_PERSISTENCE_SESSION) {
			php_session_status session_status = php_get_session_status();
			if (session_status != php_session_active &&
				session_status != php_session_disabled) {
				php_session_start();
			}

			/* Find the soap object and assign */
			zval *tmp_soap_p = php_get_session_var_str("_bogus_session_name", sizeof("_bogus_session_name")-1);
			if (tmp_soap_p != NULL && Z_TYPE_P(tmp_soap_p) == IS_OBJECT) {
				if (EXPECTED(Z_OBJCE_P(tmp_soap_p) == service->soap_class.ce)) {
					soap_obj = tmp_soap_p;
				} else if (Z_OBJCE_P(tmp_soap_p) == php_ce_incomplete_class) {
					/* See #51561, communicate limitation to user */
					soap_server_fault_en("Server", "SoapServer class was deserialized from the session prior to loading the class passed to SoapServer::setClass(). Start the session after loading all classes to resolve this issue.", NULL, NULL, NULL);
				}
			}
		}
#endif

		/* If new session or something weird happned */
		if (soap_obj == NULL) {
			object_init_ex(&tmp_soap, service->soap_class.ce);

			/* Call constructor */
			if (service->soap_class.ce->constructor) {
				zend_call_known_instance_method(
					service->soap_class.ce->constructor, Z_OBJ(tmp_soap), NULL,
					service->soap_class.argc, service->soap_class.argv);
				if (EG(exception)) {
					php_output_discard();
					_soap_server_exception(service, function, ZEND_THIS);
					zval_ptr_dtor(&tmp_soap);
					goto fail;
				}
			}

			/* If session then update session hash with new object */
#ifdef SOAP_HAS_SESSION_SUPPORT
			if (service->soap_class.persistence == SOAP_PERSISTENCE_SESSION) {
				zend_string *session_var_name = ZSTR_INIT_LITERAL("_bogus_session_name", false);
				zval *tmp_soap_p = php_set_session_var(session_var_name, &tmp_soap, NULL);
				if (tmp_soap_p != NULL) {
					soap_obj = tmp_soap_p;
				} else {
					soap_obj = &tmp_soap;
				}
				zend_string_release_ex(session_var_name, false);
			} else
#endif
			{
				soap_obj = &tmp_soap;
			}
		}
		function_table = &((Z_OBJCE_P(soap_obj))->function_table);
	} else {
		if (service->soap_functions.functions_all == TRUE) {
			function_table = EG(function_table);
		} else {
			function_table = service->soap_functions.ft;
		}
	}

	/* Process soap headers */
	if (soap_headers != NULL) {
		soapHeader *header = soap_headers;
		while (header != NULL) {
			soapHeader *h = header;

			header = header->next;
#if 0
			if (service->sdl && !h->function && !h->hdr) {
				if (h->mustUnderstand) {
					soap_server_fault_en("MustUnderstand","Header not understood", NULL, NULL, NULL);
				} else {
					continue;
				}
			}
#endif
			if (zend_hash_find_ptr_lc(function_table, Z_STR(h->function_name)) != NULL ||
			    ((service->type == SOAP_CLASS || service->type == SOAP_OBJECT) &&
			     zend_hash_str_exists(function_table, ZEND_CALL_FUNC_NAME, sizeof(ZEND_CALL_FUNC_NAME)-1))) {
				if (service->type == SOAP_CLASS || service->type == SOAP_OBJECT) {
					call_status = call_user_function(NULL, soap_obj, &h->function_name, &h->retval, h->num_params, h->parameters);
				} else {
					call_status = call_user_function(EG(function_table), NULL, &h->function_name, &h->retval, h->num_params, h->parameters);
				}
				if (call_status != SUCCESS) {
					php_error_docref(NULL, E_WARNING, "Function '%s' call failed", Z_STRVAL(h->function_name));
					return;
				}
				if (Z_TYPE(h->retval) == IS_OBJECT &&
				    instanceof_function(Z_OBJCE(h->retval), soap_fault_class_entry)) {
					php_output_discard();
					soap_server_fault_ex(function, &h->retval, h);
					if (service->type == SOAP_CLASS && soap_obj) {zval_ptr_dtor(soap_obj);}
					goto fail;
				} else if (EG(exception)) {
					php_output_discard();
					_soap_server_exception(service, function, ZEND_THIS);
					if (service->type == SOAP_CLASS && soap_obj) {zval_ptr_dtor(soap_obj);}
					goto fail;
				}
			} else if (h->mustUnderstand) {
				soap_server_fault_en("MustUnderstand","Header not understood", NULL, NULL, NULL);
			}
		}
	}

	if (zend_hash_find_ptr_lc(function_table, Z_STR(function_name)) != NULL ||
	    ((service->type == SOAP_CLASS || service->type == SOAP_OBJECT) &&
	     zend_hash_str_exists(function_table, ZEND_CALL_FUNC_NAME, sizeof(ZEND_CALL_FUNC_NAME)-1))) {
		if (service->type == SOAP_CLASS || service->type == SOAP_OBJECT) {
			call_status = call_user_function(NULL, soap_obj, &function_name, &retval, num_params, params);
			if (service->type == SOAP_CLASS) {
				if (service->soap_class.persistence != SOAP_PERSISTENCE_SESSION) {
					zval_ptr_dtor(soap_obj);
					soap_obj = NULL;
				}
			}
		} else {
			call_status = call_user_function(EG(function_table), NULL, &function_name, &retval, num_params, params);
		}
	} else {
		php_error(E_ERROR, "Function '%s' doesn't exist", Z_STRVAL(function_name));
	}

	if (EG(exception)) {
		if (!zend_is_unwind_exit(EG(exception))) {
			php_output_discard();
			_soap_server_exception(service, function, ZEND_THIS);
			if (service->type == SOAP_CLASS) {
				if (soap_obj && service->soap_class.persistence != SOAP_PERSISTENCE_SESSION) {
					zval_ptr_dtor(soap_obj);
				}
			}
		}
		goto fail;
	}

	if (call_status == SUCCESS) {
		char *response_name;

		if (Z_TYPE(retval) == IS_OBJECT &&
		    instanceof_function(Z_OBJCE(retval), soap_fault_class_entry)) {
			php_output_discard();
			soap_server_fault_ex(function, &retval, NULL);
			goto fail;
		}

		bool has_response_name = function && function->responseName;
		if (has_response_name) {
			response_name = function->responseName;
		} else {
			response_name = emalloc(Z_STRLEN(function_name) + sizeof("Response"));
			memcpy(response_name,Z_STRVAL(function_name),Z_STRLEN(function_name));
			memcpy(response_name+Z_STRLEN(function_name),"Response",sizeof("Response"));
		}
		doc_return = serialize_response_call(function, response_name, service->uri, &retval, soap_headers, soap_version);

		if (!has_response_name) {
			efree(response_name);
		}
	} else {
		php_error_docref(NULL, E_WARNING, "Function '%s' call failed", Z_STRVAL(function_name));
		return;
	}

	if (EG(exception)) {
		php_output_discard();
		_soap_server_exception(service, function, ZEND_THIS);
		if (service->type == SOAP_CLASS) {
			if (soap_obj && service->soap_class.persistence != SOAP_PERSISTENCE_SESSION) {
				zval_ptr_dtor(soap_obj);
			}
		}
		goto fail;
	}

	/* Flush buffer */
	php_output_discard();

	if (doc_return) {
		/* xmlDocDumpMemoryEnc(doc_return, &buf, &size, XML_CHAR_ENCODING_UTF8); */
		xmlDocDumpMemory(doc_return, &buf, &size);

		if (size == 0) {
			php_error_docref(NULL, E_ERROR, "Dump memory failed");
		}

		if (soap_version == SOAP_1_2) {
			sapi_add_header("Content-Type: application/soap+xml; charset=utf-8", sizeof("Content-Type: application/soap+xml; charset=utf-8")-1, 1);
		} else {
			sapi_add_header("Content-Type: text/xml; charset=utf-8", sizeof("Content-Type: text/xml; charset=utf-8")-1, 1);
		}

		if (INI_INT("zlib.output_compression")) {
			sapi_add_header("Connection: close", sizeof("Connection: close")-1, 1);
		} else {
			snprintf(cont_len, sizeof(cont_len), "Content-Length: %d", size);
			sapi_add_header(cont_len, strlen(cont_len), 1);
		}
		php_write(buf, size);
		if (service->trace) {
			if (service->last_response_body) {
				zend_string_release_ex(service->last_response_body, false);
			}
			service->last_response_body = zend_string_init((const char *) buf, size, false);
		}
		xmlFree(buf);
	} else {
		sapi_add_header("HTTP/1.1 202 Accepted", sizeof("HTTP/1.1 202 Accepted")-1, 1);
		sapi_add_header("Content-Length: 0", sizeof("Content-Length: 0")-1, 1);
	}

fail:
	SOAP_GLOBAL(soap_version) = old_soap_version;
	SOAP_GLOBAL(encoding) = old_encoding;
	SOAP_GLOBAL(sdl) = old_sdl;
	SOAP_GLOBAL(class_map) = old_class_map;
	SOAP_GLOBAL(typemap) = old_typemap;
	SOAP_GLOBAL(features) = old_features;

	if (doc_return) {
		xmlFreeDoc(doc_return);
	}

	/* Free soap headers */
	zval_ptr_dtor(&retval);
	while (soap_headers != NULL) {
		soapHeader *h = soap_headers;
		uint32_t i;

		soap_headers = soap_headers->next;
		if (h->parameters) {
			i = h->num_params;
			while (i > 0) {
				zval_ptr_dtor(&h->parameters[--i]);
			}
			efree(h->parameters);
		}
		zval_ptr_dtor_str(&h->function_name);
		zval_ptr_dtor(&h->retval);
		efree(h);
	}
	service->soap_headers_ptr = NULL;

	/* Free Memory */
	if (num_params > 0) {
		for (i = 0; i < num_params;i++) {
			zval_ptr_dtor(&params[i]);
		}
		efree(params);
	}
	zval_ptr_dtor_str(&function_name);

	SOAP_SERVER_END_CODE();
}
/* }}} */

/* {{{ Issue SoapFault indicating an error */
PHP_METHOD(SoapServer, fault)
{
	char *code, *string, *actor=NULL;
	size_t code_len, string_len, actor_len = 0;
	zval* details = NULL;
	zend_string *name = NULL;
	zend_string *lang = ZSTR_EMPTY_ALLOC();
	soapServicePtr service;
	xmlCharEncodingHandlerPtr old_encoding;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "ss|szSP",
	    &code, &code_len, &string, &string_len, &actor, &actor_len, &details,
	    &name, &lang) == FAILURE) {
		RETURN_THROWS();
	}

	SOAP_SERVER_BEGIN_CODE();
	FETCH_THIS_SERVICE(service);
	old_encoding = SOAP_GLOBAL(encoding);
	SOAP_GLOBAL(encoding) = service->encoding;

	soap_server_fault(code, string, actor, details, name, lang);

	SOAP_GLOBAL(encoding) = old_encoding;
	SOAP_SERVER_END_CODE();
}
/* }}} */

/* {{{ */
PHP_METHOD(SoapServer, addSoapHeader)
{
	soapServicePtr service;
	zval *fault;
	soapHeader **p;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "O", &fault, soap_header_class_entry) == FAILURE) {
		RETURN_THROWS();
	}

	FETCH_THIS_SERVICE_NO_BAILOUT(service);

	if (!service || !service->soap_headers_ptr) {
		zend_throw_error(NULL, "SoapServer::addSoapHeader() may be called only during SOAP request processing");
		RETURN_THROWS();
	}

	p = service->soap_headers_ptr;
	while (*p != NULL) {
		p = &(*p)->next;
	}
	*p = emalloc(sizeof(soapHeader));
	memset(*p, 0, sizeof(soapHeader));
	ZVAL_NULL(&(*p)->function_name);
	ZVAL_OBJ_COPY(&(*p)->retval, Z_OBJ_P(fault));
}
/* }}} */

PHP_METHOD(SoapServer, __getLastResponse)
{
	soapServicePtr service;
	ZEND_PARSE_PARAMETERS_NONE();
	FETCH_THIS_SERVICE_NO_BAILOUT(service);
	if (service->last_response_body) {
		RETURN_STR_COPY(service->last_response_body);
	}
}

static void soap_server_fault_ex(sdlFunctionPtr function, zval* fault, soapHeader *hdr) /* {{{ */
{
	int soap_version;
	xmlChar *buf;
	char cont_len[30];
	int size;
	xmlDocPtr doc_return;
	zval *agent_name;
	int use_http_error_status = 1;

	soap_version = SOAP_GLOBAL(soap_version);

	doc_return = serialize_response_call(function, NULL, NULL, fault, hdr, soap_version);

	xmlDocDumpMemory(doc_return, &buf, &size);

	if ((Z_TYPE(PG(http_globals)[TRACK_VARS_SERVER]) == IS_ARRAY || zend_is_auto_global(ZSTR_KNOWN(ZEND_STR_AUTOGLOBAL_SERVER))) &&
		(agent_name = zend_hash_str_find(Z_ARRVAL(PG(http_globals)[TRACK_VARS_SERVER]), "HTTP_USER_AGENT", sizeof("HTTP_USER_AGENT")-1)) != NULL &&
		Z_TYPE_P(agent_name) == IS_STRING) {
		if (zend_string_equals_literal(Z_STR_P(agent_name), "Shockwave Flash")) {
			use_http_error_status = 0;
		}
	}
	/*
	   Want to return HTTP 500 but apache wants to over write
	   our fault code with their own handling... Figure this out later
	*/
	if (use_http_error_status) {
		sapi_add_header("HTTP/1.1 500 Internal Server Error", sizeof("HTTP/1.1 500 Internal Server Error")-1, 1);
	}
	if (INI_INT("zlib.output_compression")) {
		sapi_add_header("Connection: close", sizeof("Connection: close")-1, 1);
	} else {
		snprintf(cont_len, sizeof(cont_len), "Content-Length: %d", size);
		sapi_add_header(cont_len, strlen(cont_len), 1);
	}
	if (soap_version == SOAP_1_2) {
		sapi_add_header("Content-Type: application/soap+xml; charset=utf-8", sizeof("Content-Type: application/soap+xml; charset=utf-8")-1, 1);
	} else {
		sapi_add_header("Content-Type: text/xml; charset=utf-8", sizeof("Content-Type: text/xml; charset=utf-8")-1, 1);
	}

	php_write(buf, size);

	xmlFreeDoc(doc_return);
	xmlFree(buf);
	zend_clear_exception();
}
/* }}} */

static ZEND_NORETURN void soap_server_fault(char* code, char* string, char *actor, zval* details, zend_string* name, zend_string *lang) /* {{{ */
{
	zval ret;

	ZVAL_NULL(&ret);
	set_soap_fault(&ret, NULL, code, string, actor, details, name, lang);
	/* TODO: Which function */
	soap_server_fault_ex(NULL, &ret, NULL);
	zend_bailout();
}
/* }}} */

static ZEND_NORETURN void soap_server_fault_en(char* code, char* string, char *actor, zval* details, zend_string* name)
{
	soap_server_fault(code, string, actor, details, name, SOAP_GLOBAL(lang_en));
}

static zend_never_inline ZEND_COLD void soap_real_error_handler(int error_num, zend_string *error_filename, const uint32_t error_lineno, zend_string *message) /* {{{ */
{
	bool _old_in_compilation;
	zend_execute_data *_old_current_execute_data;
	int _old_http_response_code;
	char *_old_http_status_line;

	_old_in_compilation = CG(in_compilation);
	_old_current_execute_data = EG(current_execute_data);
	_old_http_response_code = SG(sapi_headers).http_response_code;
	_old_http_status_line = SG(sapi_headers).http_status_line;

	if (Z_OBJ(SOAP_GLOBAL(error_object)) &&
	    instanceof_function(Z_OBJCE(SOAP_GLOBAL(error_object)), soap_class_entry)) {
		bool use_exceptions =
			Z_TYPE_P(Z_CLIENT_EXCEPTIONS_P(&SOAP_GLOBAL(error_object))) != IS_FALSE;
		if ((error_num & E_FATAL_ERRORS) && use_exceptions) {
			zval fault;
			char *code = SOAP_GLOBAL(error_code);
			if (code == NULL) {
				code = "Client";
			}

			add_soap_fault_ex_en(&fault, &SOAP_GLOBAL(error_object), code, ZSTR_VAL(message), NULL, NULL);
			Z_ADDREF(fault);
			zend_throw_exception_object(&fault);
			zend_bailout();
		} else if (!use_exceptions ||
		           !SOAP_GLOBAL(error_code) ||
		           strcmp(SOAP_GLOBAL(error_code),"WSDL") != 0) {
			/* Ignore libxml warnings during WSDL parsing */
			old_error_handler(error_num, error_filename, error_lineno, message);
		}
	} else {
		int old = PG(display_errors);
		int fault = 0;
		zval fault_obj;

		if (error_num & E_FATAL_ERRORS) {
			char *code = SOAP_GLOBAL(error_code);
			zval *error_object = &SOAP_GLOBAL(error_object);
			zend_string *buffer;
			zval outbuf;
			soapServicePtr service;

			ZVAL_UNDEF(&outbuf);
			if (code == NULL) {
				code = "Server";
			}
			if (Z_OBJ_P(error_object) &&
			    instanceof_function(Z_OBJCE_P(error_object), soap_server_class_entry) &&
				(service = soap_server_object_fetch(Z_OBJ_P(error_object))->service) &&
				!service->send_errors) {
				buffer = ZSTR_INIT_LITERAL("Internal Error", 0);
			} else {
				buffer = zend_string_copy(message);

				/* Get output buffer and send as fault detials */
				zval outbuflen;
				if (php_output_get_length(&outbuflen) != FAILURE && Z_LVAL(outbuflen) != 0) {
					php_output_get_contents(&outbuf);
				}
				php_output_discard();

			}
			ZVAL_NULL(&fault_obj);
			set_soap_fault(&fault_obj, NULL, code, ZSTR_VAL(buffer), NULL, &outbuf, NULL, SOAP_GLOBAL(lang_en));
			zend_string_release(buffer);
			fault = 1;
		}

		PG(display_errors) = 0;
		SG(sapi_headers).http_status_line = NULL;
		zend_try {
			old_error_handler(error_num, error_filename, error_lineno, message);
		} zend_catch {
			CG(in_compilation) = _old_in_compilation;
			EG(current_execute_data) = _old_current_execute_data;
			if (SG(sapi_headers).http_status_line) {
				efree(SG(sapi_headers).http_status_line);
			}
			SG(sapi_headers).http_status_line = _old_http_status_line;
			SG(sapi_headers).http_response_code = _old_http_response_code;
		} zend_end_try();
		PG(display_errors) = old;

		if (fault) {
			soap_server_fault_ex(NULL, &fault_obj, NULL);
			zend_bailout();
		}
	}
}
/* }}} */

static void soap_error_handler(int error_num, zend_string *error_filename, const uint32_t error_lineno, zend_string *message) /* {{{ */
{
	if (EXPECTED(!SOAP_GLOBAL(use_soap_error_handler))) {
		old_error_handler(error_num, error_filename, error_lineno, message);
	} else {
		soap_real_error_handler(error_num, error_filename, error_lineno, message);
	}
}
/* }}} */

/* {{{ */
PHP_FUNCTION(use_soap_error_handler)
{
	bool handler = 1;

	ZVAL_BOOL(return_value, SOAP_GLOBAL(use_soap_error_handler));
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "|b", &handler) == SUCCESS) {
		SOAP_GLOBAL(use_soap_error_handler) = handler;
	}
}
/* }}} */

/* {{{ */
PHP_FUNCTION(is_soap_fault)
{
	zval *fault;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "z", &fault) == SUCCESS &&
	    Z_TYPE_P(fault) == IS_OBJECT &&
	    instanceof_function(Z_OBJCE_P(fault), soap_fault_class_entry)) {
		RETURN_TRUE;
	}
	RETURN_FALSE;
}
/* }}} */

/* SoapClient functions */

/* {{{ SoapClient constructor */
PHP_METHOD(SoapClient, __construct)
{

	zval *options = NULL;
	zend_string *wsdl;
	int  soap_version = SOAP_1_1;
	php_stream_context *context = NULL;
	zend_long cache_wsdl;
	sdlPtr sdl = NULL;
	HashTable *typemap_ht = NULL;
	zval *this_ptr = ZEND_THIS;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "S!|a", &wsdl, &options) == FAILURE) {
		RETURN_THROWS();
	}

	SOAP_CLIENT_BEGIN_CODE();

	cache_wsdl = SOAP_GLOBAL(cache_enabled) ? SOAP_GLOBAL(cache_mode) : 0;

	if (options != NULL) {
		HashTable *ht = Z_ARRVAL_P(options);
		zval *tmp, tmp2;

		if (!wsdl) {
			/* Fetching non-WSDL mode options */
			if ((tmp = zend_hash_str_find(ht, "uri", sizeof("uri")-1)) != NULL &&
			    Z_TYPE_P(tmp) == IS_STRING) {
				ZVAL_STR_COPY(Z_CLIENT_URI_P(this_ptr), Z_STR_P(tmp));
			} else {
				php_error_docref(NULL, E_ERROR, "'uri' option is required in nonWSDL mode");
			}

			if ((tmp = zend_hash_str_find(ht, "style", sizeof("style")-1)) != NULL &&
					Z_TYPE_P(tmp) == IS_LONG &&
					(Z_LVAL_P(tmp) == SOAP_RPC || Z_LVAL_P(tmp) == SOAP_DOCUMENT)) {
				ZVAL_LONG(Z_CLIENT_STYLE_P(this_ptr), Z_LVAL_P(tmp));
			}

			if ((tmp = zend_hash_str_find(ht, "use", sizeof("use")-1)) != NULL &&
					Z_TYPE_P(tmp) == IS_LONG &&
					(Z_LVAL_P(tmp) == SOAP_LITERAL || Z_LVAL_P(tmp) == SOAP_ENCODED)) {
				ZVAL_LONG(Z_CLIENT_USE_P(this_ptr), Z_LVAL_P(tmp));
			}
		}

		if ((tmp = zend_hash_str_find(ht, "stream_context", sizeof("stream_context")-1)) != NULL &&
				Z_TYPE_P(tmp) == IS_RESOURCE) {
			context = php_stream_context_from_zval(tmp, 1);
			Z_ADDREF_P(tmp);
		} else {
			context = php_stream_context_alloc();
		}

		if ((tmp = zend_hash_str_find(ht, "location", sizeof("location")-1)) != NULL &&
		    Z_TYPE_P(tmp) == IS_STRING) {
			ZVAL_STR_COPY(Z_CLIENT_LOCATION_P(this_ptr), Z_STR_P(tmp));
		} else if (!wsdl) {
			php_error_docref(NULL, E_ERROR, "'location' option is required in nonWSDL mode");
		}

		if ((tmp = zend_hash_str_find(ht, "soap_version", sizeof("soap_version")-1)) != NULL) {
			if (Z_TYPE_P(tmp) == IS_LONG &&
			    (Z_LVAL_P(tmp) == SOAP_1_1 || Z_LVAL_P(tmp) == SOAP_1_2)) {
				soap_version = Z_LVAL_P(tmp);
			}
		}
		if ((tmp = zend_hash_str_find(ht, "login", sizeof("login")-1)) != NULL &&
		    Z_TYPE_P(tmp) == IS_STRING) {
			ZVAL_STR_COPY(Z_CLIENT_LOGIN_P(this_ptr), Z_STR_P(tmp));
			if ((tmp = zend_hash_str_find(ht, "password", sizeof("password")-1)) != NULL &&
			    Z_TYPE_P(tmp) == IS_STRING) {
				ZVAL_STR_COPY(Z_CLIENT_PASSWORD_P(this_ptr), Z_STR_P(tmp));
			}
			if ((tmp = zend_hash_str_find(ht, "authentication", sizeof("authentication")-1)) != NULL &&
			    Z_TYPE_P(tmp) == IS_LONG &&
			    Z_LVAL_P(tmp) == SOAP_AUTHENTICATION_DIGEST) {
				ZVAL_TRUE(Z_CLIENT_USE_DIGEST_P(this_ptr));
			}
		}
		if ((tmp = zend_hash_str_find(ht, "proxy_host", sizeof("proxy_host")-1)) != NULL &&
		    Z_TYPE_P(tmp) == IS_STRING) {
			ZVAL_STR_COPY(Z_CLIENT_PROXY_HOST_P(this_ptr), Z_STR_P(tmp));
			if ((tmp = zend_hash_str_find(ht, "proxy_port", sizeof("proxy_port")-1)) != NULL) {
				if (Z_TYPE_P(tmp) != IS_LONG) {
					ZVAL_LONG(&tmp2, zval_get_long(tmp));
					tmp = &tmp2;
				}
				ZVAL_LONG(Z_CLIENT_PROXY_PORT_P(this_ptr), Z_LVAL_P(tmp));
			}
			if ((tmp = zend_hash_str_find(ht, "proxy_login", sizeof("proxy_login")-1)) != NULL &&
			    Z_TYPE_P(tmp) == IS_STRING) {
				ZVAL_STR_COPY(Z_CLIENT_PROXY_LOGIN_P(this_ptr), Z_STR_P(tmp));
				if ((tmp = zend_hash_str_find(ht, "proxy_password", sizeof("proxy_password")-1)) != NULL &&
				    Z_TYPE_P(tmp) == IS_STRING) {
					ZVAL_STR_COPY(Z_CLIENT_PROXY_PASSWORD_P(this_ptr), Z_STR_P(tmp));
				}
			}
		}
		if ((tmp = zend_hash_str_find(ht, "local_cert", sizeof("local_cert")-1)) != NULL &&
		    Z_TYPE_P(tmp) == IS_STRING) {
			if (!context) {
				context = php_stream_context_alloc();
			}
			php_stream_context_set_option(context, "ssl", "local_cert", tmp);
			if ((tmp = zend_hash_str_find(ht, "passphrase", sizeof("passphrase")-1)) != NULL &&
			    Z_TYPE_P(tmp) == IS_STRING) {
				php_stream_context_set_option(context, "ssl", "passphrase", tmp);
			}
		}
		if ((tmp = zend_hash_find(ht, ZSTR_KNOWN(ZEND_STR_TRACE))) != NULL &&
		    (Z_TYPE_P(tmp) == IS_TRUE ||
		     (Z_TYPE_P(tmp) == IS_LONG && Z_LVAL_P(tmp) == 1))) {
			ZVAL_TRUE(Z_CLIENT_TRACE_P(this_ptr));
		}

		if ((tmp = zend_hash_str_find(ht, "exceptions", sizeof("exceptions")-1)) != NULL &&
		    (Z_TYPE_P(tmp) == IS_FALSE ||
		     (Z_TYPE_P(tmp) == IS_LONG && Z_LVAL_P(tmp) == 0))) {
			ZVAL_FALSE(Z_CLIENT_EXCEPTIONS_P(this_ptr));
		}

		if ((tmp = zend_hash_str_find(ht, "compression", sizeof("compression")-1)) != NULL &&
		    Z_TYPE_P(tmp) == IS_LONG &&
	      zend_hash_str_exists(EG(function_table), "gzinflate", sizeof("gzinflate")-1) &&
	      zend_hash_str_exists(EG(function_table), "gzdeflate", sizeof("gzdeflate")-1) &&
	      zend_hash_str_exists(EG(function_table), "gzuncompress", sizeof("gzuncompress")-1) &&
	      zend_hash_str_exists(EG(function_table), "gzcompress", sizeof("gzcompress")-1) &&
	      zend_hash_str_exists(EG(function_table), "gzencode", sizeof("gzencode")-1)) {
			ZVAL_LONG(Z_CLIENT_COMPRESSION_P(this_ptr), Z_LVAL_P(tmp));
		}
		if ((tmp = zend_hash_str_find(ht, "encoding", sizeof("encoding")-1)) != NULL &&
		    Z_TYPE_P(tmp) == IS_STRING) {
			xmlCharEncodingHandlerPtr encoding;

			encoding = xmlFindCharEncodingHandler(Z_STRVAL_P(tmp));
			if (encoding == NULL) {
				php_error_docref(NULL, E_ERROR, "Invalid 'encoding' option - '%s'", Z_STRVAL_P(tmp));
			} else {
				xmlCharEncCloseFunc(encoding);
				ZVAL_STR_COPY(Z_CLIENT_ENCODING_P(this_ptr), Z_STR_P(tmp));
			}
		}
		if ((tmp = zend_hash_str_find(ht, "classmap", sizeof("classmap")-1)) != NULL &&
			Z_TYPE_P(tmp) == IS_ARRAY) {
			if (HT_IS_PACKED(Z_ARRVAL_P(tmp))) {
				php_error_docref(NULL, E_ERROR, "'classmap' option must be an associative array");
			}
			ZVAL_COPY(Z_CLIENT_CLASSMAP_P(this_ptr), tmp);
		}

		if ((tmp = zend_hash_str_find(ht, "typemap", sizeof("typemap")-1)) != NULL &&
			Z_TYPE_P(tmp) == IS_ARRAY &&
			zend_hash_num_elements(Z_ARRVAL_P(tmp)) > 0) {
			typemap_ht = Z_ARRVAL_P(tmp);
		}

		if ((tmp = zend_hash_str_find(ht, "features", sizeof("features")-1)) != NULL &&
			Z_TYPE_P(tmp) == IS_LONG) {
			ZVAL_LONG(Z_CLIENT_FEATURES_P(this_ptr), Z_LVAL_P(tmp));
		}

		if ((tmp = zend_hash_str_find(ht, "connection_timeout", sizeof("connection_timeout")-1)) != NULL) {
			zend_long lval = zval_get_long(tmp);
			if (lval > 0) {
				ZVAL_LONG(Z_CLIENT_CONNECTION_TIMEOUT_P(this_ptr), lval);
			}
		}

		if (context) {
			ZVAL_RES(Z_CLIENT_STREAM_CONTEXT_P(this_ptr), context->res);
		}

		if ((tmp = zend_hash_str_find(ht, "cache_wsdl", sizeof("cache_wsdl")-1)) != NULL &&
		    Z_TYPE_P(tmp) == IS_LONG) {
			cache_wsdl = Z_LVAL_P(tmp);
		}

		if ((tmp = zend_hash_str_find(ht, "user_agent", sizeof("user_agent")-1)) != NULL &&
		    Z_TYPE_P(tmp) == IS_STRING) {
			ZVAL_STR_COPY(Z_CLIENT_USER_AGENT_P(this_ptr), Z_STR_P(tmp));
		}

		if ((tmp = zend_hash_str_find(ht, "keep_alive", sizeof("keep_alive")-1)) != NULL &&
				(Z_TYPE_P(tmp) == IS_FALSE ||
				 (Z_TYPE_P(tmp) == IS_LONG && Z_LVAL_P(tmp) == 0))) {
			ZVAL_FALSE(Z_CLIENT_KEEP_ALIVE_P(this_ptr));
		}

		if ((tmp = zend_hash_str_find(ht, "ssl_method", sizeof("ssl_method")-1)) != NULL &&
			Z_TYPE_P(tmp) == IS_LONG) {
			ZVAL_LONG(Z_CLIENT_SSL_METHOD_P(this_ptr), Z_LVAL_P(tmp));
			php_error_docref(NULL, E_DEPRECATED,
				"The \"ssl_method\" option is deprecated. "
				"Use \"ssl\" stream context options instead");
		}
	} else if (!wsdl) {
		php_error_docref(NULL, E_ERROR, "'location' and 'uri' options are required in nonWSDL mode");
	}

	ZVAL_LONG(Z_CLIENT_SOAP_VERSION_P(this_ptr), soap_version);

	if (wsdl) {
		int    old_soap_version;

		old_soap_version = SOAP_GLOBAL(soap_version);
		SOAP_GLOBAL(soap_version) = soap_version;

		sdl = get_sdl(this_ptr, ZSTR_VAL(wsdl), cache_wsdl);

		zval *sdl_zval = Z_CLIENT_SDL_P(this_ptr);
		if (Z_TYPE_P(sdl_zval) == IS_OBJECT) {
			zval_ptr_dtor(sdl_zval);
		}

		object_init_ex(sdl_zval, soap_sdl_class_entry);
		soap_sdl_object *sdl_object = Z_SOAP_SDL_P(sdl_zval);
		sdl_object->sdl = sdl;

		SOAP_GLOBAL(soap_version) = old_soap_version;
	}

	if (typemap_ht) {
		HashTable *typemap = soap_create_typemap(sdl, typemap_ht);
		if (typemap) {
			ZVAL_ARR(Z_CLIENT_TYPEMAP_P(this_ptr), typemap);
		}
	}
	SOAP_CLIENT_END_CODE();
}
/* }}} */

static bool do_request(zval *this_ptr, xmlDoc *request, const char *location, const char *action, int version, bool one_way, zval *response) /* {{{ */
{
	bool   ret = true;
	char  *buf;
	int    buf_size;
	zval  params[5];
	bool  _bailout = false;

	ZVAL_NULL(response);

	xmlDocDumpMemory(request, (xmlChar**)&buf, &buf_size);
	if (!buf) {
		add_soap_fault_en(this_ptr, "HTTP", "Error build soap request", NULL, NULL);
		return false;
	}

	zend_try {
		ZVAL_STRINGL(&params[0], buf, buf_size);
		ZVAL_STRING(&params[1], location);
		if (action == NULL) {
			ZVAL_EMPTY_STRING(&params[2]);
		} else {
			ZVAL_STRING(&params[2], action);
		}
		ZVAL_LONG(&params[3], version);
		ZVAL_BOOL(&params[4], one_way);

		zval *trace = Z_CLIENT_TRACE_P(this_ptr);
		if (Z_TYPE_P(trace) == IS_TRUE) {
			zval_ptr_dtor(Z_CLIENT_LAST_REQUEST_P(this_ptr));
			ZVAL_COPY(Z_CLIENT_LAST_REQUEST_P(this_ptr), &params[0]);
		}

		zend_function *func = zend_hash_str_find_ptr(&Z_OBJCE_P(this_ptr)->function_table, ZEND_STRL("__dorequest"));
		ZEND_ASSERT(func != NULL);

		zend_call_known_instance_method(func, Z_OBJ_P(this_ptr), response, 5, params);

		if (Z_TYPE_P(response) != IS_STRING) {
			if (EG(exception) && instanceof_function(EG(exception)->ce, zend_ce_error)) {
				/* Programmer error in __doRequest() implementation, let it bubble up. */
			} else if (Z_TYPE_P(Z_CLIENT_SOAP_FAULT_P(this_ptr)) != IS_OBJECT) {
				add_soap_fault_en(this_ptr, "Client", "SoapClient::__doRequest() returned non string value", NULL, NULL);
			}
			ret = false;
		} else if (Z_TYPE_P(trace) == IS_TRUE) {
			zval_ptr_dtor(Z_CLIENT_LAST_RESPONSE_P(this_ptr));
			ZVAL_STR_COPY(Z_CLIENT_LAST_RESPONSE_P(this_ptr), Z_STR_P(response));
		}
	} zend_catch {
		_bailout = true;
	} zend_end_try();
	zval_ptr_dtor(&params[2]);
	zval_ptr_dtor(&params[1]);
	zval_ptr_dtor(&params[0]);
	xmlFree(buf);
	if (_bailout) {
		zend_bailout();
	}
	if (ret && Z_TYPE_P(Z_CLIENT_SOAP_FAULT_P(this_ptr)) == IS_OBJECT) {
		ret = false;
	}
	return ret;
}
/* }}} */

static void do_soap_call(zend_execute_data *execute_data,
                         zval* this_ptr,
                         const zend_string *function,
                         uint32_t arg_count,
                         zval* real_args,
                         zval* return_value,
                         const zend_string* location,
                         const zend_string* soap_action,
                         const zend_string* call_uri,
                         HashTable* soap_headers,
                         zval* output_headers
                        ) /* {{{ */
{
	sdlPtr sdl = NULL;
	sdlPtr old_sdl = NULL;
	sdlFunctionPtr fn;
	xmlDocPtr request = NULL;
	bool ret = false;
	int soap_version;
	zval response;
	xmlCharEncodingHandlerPtr old_encoding;
	HashTable *old_class_map;
	int old_features;
	HashTable *old_typemap, *typemap = NULL;
	smart_str action = {0};
	int bailout = 0;

	SOAP_CLIENT_BEGIN_CODE();

	if (Z_TYPE_P(Z_CLIENT_TRACE_P(this_ptr)) == IS_TRUE) {
		convert_to_null(Z_CLIENT_LAST_REQUEST_P(this_ptr));
		convert_to_null(Z_CLIENT_LAST_RESPONSE_P(this_ptr));
	}

	zval *tmp = Z_CLIENT_SOAP_VERSION_P(this_ptr);
	if (Z_TYPE_P(tmp) == IS_LONG && Z_LVAL_P(tmp) == SOAP_1_2) {
		soap_version = SOAP_1_2;
	} else {
		soap_version = SOAP_1_1;
	}

	if (location == NULL) {
		tmp = Z_CLIENT_LOCATION_P(this_ptr);
		if (Z_TYPE_P(tmp) == IS_STRING) {
			location = Z_STR_P(tmp);
		}
	}

	tmp = Z_CLIENT_SDL_P(this_ptr);
	if (Z_TYPE_P(tmp) == IS_OBJECT) {
#if ZEND_DEBUG
		ZEND_ASSERT(instanceof_function(Z_OBJCE_P(tmp), soap_sdl_class_entry));
#endif
		sdl = Z_SOAP_SDL_P(tmp)->sdl;
	}

	tmp = Z_CLIENT_TYPEMAP_P(this_ptr);
	if (Z_TYPE_P(tmp) == IS_ARRAY) {
		typemap = Z_ARR_P(tmp);
	}

	clear_soap_fault(this_ptr);

	SOAP_GLOBAL(soap_version) = soap_version;
	old_sdl = SOAP_GLOBAL(sdl);
	SOAP_GLOBAL(sdl) = sdl;
	old_encoding = SOAP_GLOBAL(encoding);
	tmp = Z_CLIENT_ENCODING_P(this_ptr);
	if (Z_TYPE_P(tmp) == IS_STRING) {
		SOAP_GLOBAL(encoding) = xmlFindCharEncodingHandler(Z_STRVAL_P(tmp));
	} else {
		SOAP_GLOBAL(encoding) = NULL;
	}
	old_class_map = SOAP_GLOBAL(class_map);
	tmp = Z_CLIENT_CLASSMAP_P(this_ptr);
	if (Z_TYPE_P(tmp) == IS_ARRAY) {
		SOAP_GLOBAL(class_map) = Z_ARRVAL_P(tmp);
	} else {
		SOAP_GLOBAL(class_map) = NULL;
	}
	old_typemap = SOAP_GLOBAL(typemap);
	SOAP_GLOBAL(typemap) = typemap;
	old_features = SOAP_GLOBAL(features);
	tmp = Z_CLIENT_FEATURES_P(this_ptr);
	if (Z_TYPE_P(tmp) == IS_LONG) {
		SOAP_GLOBAL(features) = Z_LVAL_P(tmp);
	} else {
		SOAP_GLOBAL(features) = 0;
	}

	zend_try {
	 	if (sdl != NULL) {
			fn = get_function(sdl, ZSTR_VAL(function), ZSTR_LEN(function));
			if (fn != NULL) {
				sdlBindingPtr binding = fn->binding;
				bool one_way = 0;

				if (fn->responseName == NULL &&
				    fn->responseParameters == NULL &&
				    soap_headers == NULL) {
					one_way = 1;
				}

				const char *location_c_str;
				if (location == NULL) {
					location_c_str = binding->location;
					ZEND_ASSERT(location_c_str);
				} else {
					location_c_str = ZSTR_VAL(location);
				}
				if (binding->bindingType == BINDING_SOAP) {
					sdlSoapBindingFunctionPtr fnb = (sdlSoapBindingFunctionPtr)fn->bindingAttributes;
					request = serialize_function_call(this_ptr, fn, NULL, fnb->input.ns, real_args, arg_count, soap_version, soap_headers);
					ret = do_request(this_ptr, request, location_c_str, fnb->soapAction, soap_version, one_way, &response);
				} else {
	 				request = serialize_function_call(this_ptr, fn, NULL, sdl->target_ns, real_args, arg_count, soap_version, soap_headers);
					ret = do_request(this_ptr, request, location_c_str, NULL, soap_version, one_way, &response);
				}

				xmlFreeDoc(request);
				request = NULL;

				if (ret && Z_TYPE(response) == IS_STRING) {
					encode_reset_ns();
					ret = parse_packet_soap(this_ptr, Z_STRVAL(response), Z_STRLEN(response), fn, NULL, return_value, output_headers);
					encode_finish();
				}

				zval_ptr_dtor(&response);

	 		} else {
	 			smart_str error = {0};
	 			smart_str_appends(&error,"Function (\"");
				smart_str_append(&error,function);
	 			smart_str_appends(&error,"\") is not a valid method for this service");
	 			smart_str_0(&error);
				add_soap_fault_en(this_ptr, "Client", ZSTR_VAL(error.s), NULL, NULL);
				smart_str_free(&error);
			}
		} else {
			zval *uri = Z_CLIENT_URI_P(this_ptr);
			if (Z_TYPE_P(uri) != IS_STRING) {
				add_soap_fault_en(this_ptr, "Client", "Error finding \"uri\" property", NULL, NULL);
			} else if (location == NULL) {
				add_soap_fault_en(this_ptr, "Client", "Error could not find \"location\" property", NULL, NULL);
			} else {
				if (call_uri == NULL) {
					call_uri = Z_STR_P(uri);
				}
				request = serialize_function_call(this_ptr, NULL, ZSTR_VAL(function), ZSTR_VAL(call_uri), real_args, arg_count, soap_version, soap_headers);

		 		if (soap_action == NULL) {
					smart_str_append(&action, call_uri);
					smart_str_appendc(&action, '#');
					smart_str_append(&action, function);
				} else {
					smart_str_append(&action, soap_action);
				}
				smart_str_0(&action);

				ret = do_request(this_ptr, request, ZSTR_VAL(location), ZSTR_VAL(action.s), soap_version, 0, &response);

		 		smart_str_free(&action);
				xmlFreeDoc(request);
				request = NULL;

				if (ret && Z_TYPE(response) == IS_STRING) {
					encode_reset_ns();
					ret = parse_packet_soap(this_ptr, Z_STRVAL(response), Z_STRLEN(response), NULL, NULL, return_value, output_headers);
					encode_finish();
				}

				zval_ptr_dtor(&response);
			}
	 	}

		zval *fault = Z_CLIENT_SOAP_FAULT_P(this_ptr);
		if (!ret) {
			if (Z_TYPE_P(fault) == IS_OBJECT) {
				ZVAL_COPY(return_value, fault);
			} else {
				add_soap_fault_ex_en(return_value, this_ptr, "Client", "Unknown Error", NULL, NULL);
				Z_ADDREF_P(return_value);
			}
		} else {
			if (Z_TYPE_P(fault) == IS_OBJECT) {
				ZVAL_COPY(return_value, fault);
			}
		}

		if (!EG(exception) &&
		    Z_TYPE_P(return_value) == IS_OBJECT &&
		    instanceof_function(Z_OBJCE_P(return_value), soap_fault_class_entry) &&
		    Z_TYPE_P(Z_CLIENT_EXCEPTIONS_P(this_ptr)) != IS_FALSE) {
			Z_ADDREF_P(return_value);
			zend_throw_exception_object(return_value);
		}

	} zend_catch {
		bailout = 1;
	} zend_end_try();

	if (SOAP_GLOBAL(encoding) != NULL) {
		xmlCharEncCloseFunc(SOAP_GLOBAL(encoding));
	}

	SOAP_GLOBAL(features) = old_features;
	SOAP_GLOBAL(typemap) = old_typemap;
	SOAP_GLOBAL(class_map) = old_class_map;
	SOAP_GLOBAL(encoding) = old_encoding;
	SOAP_GLOBAL(sdl) = old_sdl;
	if (bailout) {
		smart_str_free(&action);
		if (request) {
			xmlFreeDoc(request);
		}
		zend_bailout();
	}
	SOAP_CLIENT_END_CODE();
}
/* }}} */

static bool verify_soap_headers_array(const HashTable *ht) /* {{{ */
{
	zval *tmp;

	ZEND_HASH_FOREACH_VAL(ht, tmp) {
		if (Z_TYPE_P(tmp) != IS_OBJECT ||
		    !instanceof_function(Z_OBJCE_P(tmp), soap_header_class_entry)) {
			return false;
		}
	} ZEND_HASH_FOREACH_END();
	return true;
}
/* }}} */

/* {{{ Calls a SOAP function */
static void soap_client_call_common(
	zval *this_ptr,
	const zend_string *function,
	HashTable *args,
	const zend_string *location,
	const zend_string *soap_action,
	const zend_string *uri,
	HashTable* soap_headers,
	bool free_soap_headers,
	zval *output_headers,
	zend_execute_data *execute_data,
	zval *return_value
) {
	/* Add default headers */
	this_ptr = ZEND_THIS;
	zval *tmp = Z_CLIENT_DEFAULT_HEADERS_P(this_ptr);
	if (Z_TYPE_P(tmp) == IS_ARRAY) {
		HashTable *default_headers = Z_ARRVAL_P(tmp);
		if (soap_headers) {
			if (!free_soap_headers) {
				soap_headers = zend_array_dup(soap_headers);
				free_soap_headers = 1;
			}
			ZEND_HASH_FOREACH_VAL(default_headers, tmp) {
				if(Z_TYPE_P(tmp) == IS_OBJECT) {
					Z_ADDREF_P(tmp);
					zend_hash_next_index_insert(soap_headers, tmp);
				}
			} ZEND_HASH_FOREACH_END();
		} else {
			soap_headers = Z_ARRVAL_P(tmp);
			free_soap_headers = 0;
		}
	}

	uint32_t arg_count = zend_hash_num_elements(args);
	zval *real_args = NULL;
	if (arg_count > 0) {
		zval *param;
		int i = 0;

		real_args = safe_emalloc(sizeof(zval), arg_count, 0);
		ZEND_HASH_FOREACH_VAL(args, param) {
			/*zval_add_ref(param);*/
			ZVAL_DEREF(param);
			ZVAL_COPY_VALUE(&real_args[i], param);
			i++;
		} ZEND_HASH_FOREACH_END();
	}

	do_soap_call(execute_data, this_ptr, function, arg_count, real_args, return_value, location, soap_action, uri, soap_headers, output_headers);

	if (arg_count > 0) {
		efree(real_args);
	}
	if (soap_headers && free_soap_headers) {
		zend_hash_destroy(soap_headers);
		efree(soap_headers);
	}
}
/* }}} */

PHP_METHOD(SoapClient, __call)
{
	zend_string *function = NULL;
	HashTable *args = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "Sh", &function, &args) == FAILURE) {
		RETURN_THROWS();
	}
	soap_client_call_common(
		ZEND_THIS,
		function,
		args,
		/* location */ NULL,
		/* soap_action */ NULL,
		/* uri */ NULL,
		/* soap_headers */ NULL,
		/* free_soap_headers */ false,
		/* output_headers */ NULL,
		execute_data,
		return_value
	);
}

PHP_METHOD(SoapClient, __soapCall)
{
	zend_string *function = NULL;
	HashTable *args = NULL;
	HashTable *options = NULL;
	zval *headers = NULL;
	zval *output_headers = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "Sh|h!zz", &function, &args, &options, &headers, &output_headers) == FAILURE) {
		RETURN_THROWS();
	}


	zend_string *location = NULL;
	zend_string *soap_action = NULL;
	zend_string *uri = NULL;
	if (options) {
		zval *tmp;
		if ((tmp = zend_hash_str_find(options, "location", sizeof("location")-1)) != NULL && Z_TYPE_P(tmp) == IS_STRING) {
			location = Z_STR_P(tmp);
		}

		if ((tmp = zend_hash_str_find(options, "soapaction", sizeof("soapaction")-1)) != NULL && Z_TYPE_P(tmp) == IS_STRING) {
			soap_action = Z_STR_P(tmp);
		}

		if ((tmp = zend_hash_str_find(options, "uri", sizeof("uri")-1)) != NULL && Z_TYPE_P(tmp) == IS_STRING) {
			uri = Z_STR_P(tmp);
		}
	}

	if (output_headers) {
		output_headers = zend_try_array_init(output_headers);
		if (output_headers == NULL) {
			RETURN_THROWS();
		}
	}

	HashTable* soap_headers = NULL;
	bool free_soap_headers = false;
	if (headers == NULL || Z_TYPE_P(headers) == IS_NULL) {
	} else if (Z_TYPE_P(headers) == IS_ARRAY) {
		soap_headers = Z_ARRVAL_P(headers);
		if (!verify_soap_headers_array(soap_headers)) {
			php_error_docref(NULL, E_ERROR, "Invalid SOAP header");
		}
		free_soap_headers = false;
	} else if (Z_TYPE_P(headers) == IS_OBJECT && instanceof_function(Z_OBJCE_P(headers), soap_header_class_entry)) {
		soap_headers = zend_new_array(0);
		zend_hash_next_index_insert(soap_headers, headers);
		Z_ADDREF_P(headers);
		free_soap_headers = true;
	} else {
		zend_argument_type_error(4, "must be of type SoapHeader|array|null, %s given", zend_zval_value_name(headers));
		RETURN_THROWS();
	}

	soap_client_call_common(
		ZEND_THIS,
		function,
		args,
		location,
		soap_action,
		uri,
		soap_headers,
		free_soap_headers,
		output_headers,
		execute_data,
		return_value
	);
}

/* {{{ Returns list of SOAP functions */
PHP_METHOD(SoapClient, __getFunctions)
{
	if (zend_parse_parameters_none() == FAILURE) {
		RETURN_THROWS();
	}

	sdl *sdl;
	FETCH_THIS_SDL(sdl);

	if (sdl) {
		smart_str buf = {0};
		sdlFunctionPtr function;

		array_init(return_value);
		ZEND_HASH_MAP_FOREACH_PTR(&sdl->functions, function) {
			function_to_string(function, &buf);
			add_next_index_stringl(return_value, ZSTR_VAL(buf.s), ZSTR_LEN(buf.s));
			smart_str_free(&buf);
		} ZEND_HASH_FOREACH_END();
	}
}
/* }}} */


/* {{{ Returns list of SOAP types */
PHP_METHOD(SoapClient, __getTypes)
{
	if (zend_parse_parameters_none() == FAILURE) {
		RETURN_THROWS();
	}

	sdl *sdl;
	FETCH_THIS_SDL(sdl);

	if (sdl) {
		sdlTypePtr type;
		smart_str buf = {0};

		array_init(return_value);
		if (sdl->types) {
			ZEND_HASH_FOREACH_PTR(sdl->types, type) {
				type_to_string(type, &buf, 0);
				add_next_index_stringl(return_value, ZSTR_VAL(buf.s), ZSTR_LEN(buf.s));
				smart_str_free(&buf);
			} ZEND_HASH_FOREACH_END();
		}
	}
}
/* }}} */


/* {{{ Returns last SOAP request */
PHP_METHOD(SoapClient, __getLastRequest)
{
	if (zend_parse_parameters_none() == FAILURE) {
		RETURN_THROWS();
	}

	RETURN_COPY(Z_CLIENT_LAST_REQUEST_P(ZEND_THIS));
}
/* }}} */


/* {{{ Returns last SOAP response */
PHP_METHOD(SoapClient, __getLastResponse)
{
	if (zend_parse_parameters_none() == FAILURE) {
		RETURN_THROWS();
	}

	RETURN_COPY(Z_CLIENT_LAST_RESPONSE_P(ZEND_THIS));
}
/* }}} */


/* {{{ Returns last SOAP request headers */
PHP_METHOD(SoapClient, __getLastRequestHeaders)
{
	if (zend_parse_parameters_none() == FAILURE) {
		RETURN_THROWS();
	}

	RETURN_COPY(Z_CLIENT_LAST_REQUEST_HEADERS_P(ZEND_THIS));
}
/* }}} */


/* {{{ Returns last SOAP response headers */
PHP_METHOD(SoapClient, __getLastResponseHeaders)
{
	if (zend_parse_parameters_none() == FAILURE) {
		RETURN_THROWS();
	}

	RETURN_COPY(Z_CLIENT_LAST_RESPONSE_HEADERS_P(ZEND_THIS));
}
/* }}} */


/* {{{ SoapClient::__doRequest() */
PHP_METHOD(SoapClient, __doRequest)
{
	zend_string *buf;
	char      *location, *action;
	size_t     location_size, action_size;
	zend_long  version;
	bool  one_way = 0;
	zval      *this_ptr = ZEND_THIS;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "Sssl|b",
	    &buf,
	    &location, &location_size,
	    &action, &action_size,
	    &version, &one_way) == FAILURE) {
		RETURN_THROWS();
	}
	if (SOAP_GLOBAL(features) & SOAP_WAIT_ONE_WAY_CALLS) {
		one_way = 0;
	}
	if (one_way) {
		if (make_http_soap_request(this_ptr, buf, location, action, version, NULL)) {
			RETURN_EMPTY_STRING();
		}
	} else if (make_http_soap_request(this_ptr, buf, location, action, version,
	    return_value)) {
		return;
	}
	RETURN_NULL();
}
/* }}} */

/* {{{ Sets cookie thet will sent with SOAP request.
   The call to this function will effect all following calls of SOAP methods.
   If value is not specified cookie is removed. */
PHP_METHOD(SoapClient, __setCookie)
{
	zend_string *name, *val = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "S|S!", &name, &val) == FAILURE) {
		RETURN_THROWS();
	}

	zval *cookies = Z_CLIENT_COOKIES_P(ZEND_THIS);
	SEPARATE_ARRAY(cookies);
	if (val == NULL) {
		zend_hash_del(Z_ARRVAL_P(cookies), name);
	} else {
		zval zcookie;
		array_init(&zcookie);
		add_index_str(&zcookie, 0, zend_string_copy(val));
		zend_hash_update(Z_ARRVAL_P(cookies), name, &zcookie);
	}
}
/* }}} */

/* {{{ Returns list of cookies */
PHP_METHOD(SoapClient, __getCookies)
{
	if (zend_parse_parameters_none() == FAILURE) {
		RETURN_THROWS();
	}

	RETURN_COPY(Z_CLIENT_COOKIES_P(ZEND_THIS));
}
/* }}} */

/* {{{ Sets SOAP headers for subsequent calls (replaces any previous
   values).
   If no value is specified, all of the headers are removed. */
PHP_METHOD(SoapClient, __setSoapHeaders)
{
	zval *headers = NULL;
	zval *this_ptr = ZEND_THIS;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "|z", &headers) == FAILURE) {
		RETURN_THROWS();
	}

	if (headers == NULL || Z_TYPE_P(headers) == IS_NULL) {
		convert_to_null(Z_CLIENT_DEFAULT_HEADERS_P(this_ptr));
	} else if (Z_TYPE_P(headers) == IS_ARRAY) {
		if (!verify_soap_headers_array(Z_ARRVAL_P(headers))) {
			php_error_docref(NULL, E_ERROR, "Invalid SOAP header");
		}
		zval_ptr_dtor(Z_CLIENT_DEFAULT_HEADERS_P(this_ptr));
		ZVAL_COPY(Z_CLIENT_DEFAULT_HEADERS_P(this_ptr), headers);
	} else if (Z_TYPE_P(headers) == IS_OBJECT &&
	           instanceof_function(Z_OBJCE_P(headers), soap_header_class_entry)) {
		zval default_headers;
		array_init(&default_headers);
		Z_ADDREF_P(headers);
		add_next_index_zval(&default_headers, headers);

		zval_ptr_dtor(Z_CLIENT_DEFAULT_HEADERS_P(this_ptr));
		ZVAL_COPY_VALUE(Z_CLIENT_DEFAULT_HEADERS_P(this_ptr), &default_headers);
	} else {
		zend_argument_type_error(1, "must be of type SoapHeader|array|null, %s given", zend_zval_value_name(headers));
		RETURN_THROWS();
	}
	RETURN_TRUE;
}
/* }}} */

/* {{{ Sets the location option (the endpoint URL that will be touched by the
   following SOAP requests).
   If new_location is not specified or null then SoapClient will use endpoint
   from WSDL file.
   The function returns old value of location options. */
PHP_METHOD(SoapClient, __setLocation)
{
	zend_string *location = NULL;
	zval *this_ptr = ZEND_THIS;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "|S!", &location) == FAILURE) {
		RETURN_THROWS();
	}

	RETVAL_COPY_VALUE(Z_CLIENT_LOCATION_P(this_ptr));

	if (location && ZSTR_LEN(location) != 0) {
		ZVAL_STR_COPY(Z_CLIENT_LOCATION_P(this_ptr), location);
	} else {
		ZVAL_NULL(Z_CLIENT_LOCATION_P(this_ptr));
	}
}
/* }}} */

static void clear_soap_fault(zval *obj) /* {{{ */
{
	ZEND_ASSERT(instanceof_function(Z_OBJCE_P(obj), soap_class_entry));
	convert_to_null(Z_CLIENT_SOAP_FAULT_P(obj));
}
/* }}} */

static void add_soap_fault_ex(zval *fault, zval *obj, char *fault_code, char *fault_string, char *fault_actor, zval *fault_detail, zend_string *lang) /* {{{ */
{
	ZVAL_NULL(fault);
	set_soap_fault(fault, NULL, fault_code, fault_string, fault_actor, fault_detail, NULL, lang);
	zval *target;
	if (instanceof_function(Z_OBJCE_P(obj), soap_class_entry)) {
		target = Z_CLIENT_SOAP_FAULT_P(obj);
	} else if (instanceof_function(Z_OBJCE_P(obj), soap_server_class_entry)) {
		target = Z_SERVER_SOAP_FAULT_P(obj);
	} else {
		ZEND_UNREACHABLE();
	}
	zval_ptr_dtor(target);
	ZVAL_COPY_VALUE(target, fault);
}
/* }}} */

static void add_soap_fault_ex_en(zval *fault, zval *obj, char *fault_code, char *fault_string, char *fault_actor, zval *fault_detail)
{
	add_soap_fault_ex(fault, obj, fault_code, fault_string, fault_actor, fault_detail, SOAP_GLOBAL(lang_en));
}

void add_soap_fault(zval *obj, char *fault_code, char *fault_string, char *fault_actor, zval *fault_detail, zend_string *lang) /* {{{ */
{
	zval fault;
	add_soap_fault_ex(&fault, obj, fault_code, fault_string, fault_actor, fault_detail, lang);
}
/* }}} */

static void add_soap_fault_en(zval *obj, char *fault_code, char *fault_string, char *fault_actor, zval *fault_detail)
{
	add_soap_fault(obj, fault_code, fault_string, fault_actor, fault_detail, SOAP_GLOBAL(lang_en));
}

static void set_soap_fault(zval *obj, const char *fault_code_ns, const char *fault_code, const char *fault_string, const char *fault_actor, zval *fault_detail, zend_string *name, zend_string *lang) /* {{{ */
{
	if (Z_TYPE_P(obj) != IS_OBJECT) {
		object_init_ex(obj, soap_fault_class_entry);
	}

	ZVAL_STRING(Z_FAULT_STRING_P(obj), fault_string ? fault_string : "");
	zend_update_property_ex(zend_ce_exception, Z_OBJ_P(obj), ZSTR_KNOWN(ZEND_STR_MESSAGE), Z_FAULT_STRING_P(obj));

	if (fault_code != NULL) {
		int soap_version = SOAP_GLOBAL(soap_version);

		if (fault_code_ns) {
			ZVAL_STRING(Z_FAULT_CODE_P(obj), fault_code);
			ZVAL_STRING(Z_FAULT_CODENS_P(obj), fault_code_ns);
		} else {
			if (soap_version == SOAP_1_1) {
				ZVAL_STRING(Z_FAULT_CODE_P(obj), fault_code);
				if (strcmp(fault_code,"Client") == 0 ||
				    strcmp(fault_code,"Server") == 0 ||
				    strcmp(fault_code,"VersionMismatch") == 0 ||
			  	  strcmp(fault_code,"MustUnderstand") == 0) {
					ZVAL_STRING(Z_FAULT_CODENS_P(obj), SOAP_1_1_ENV_NAMESPACE);
				}
			} else if (soap_version == SOAP_1_2) {
				if (strcmp(fault_code,"Client") == 0) {
					ZVAL_STRING(Z_FAULT_CODE_P(obj), "Sender");
					ZVAL_STRING(Z_FAULT_CODENS_P(obj), SOAP_1_2_ENV_NAMESPACE);
				} else if (strcmp(fault_code,"Server") == 0) {
					ZVAL_STRING(Z_FAULT_CODE_P(obj), "Receiver");
					ZVAL_STRING(Z_FAULT_CODENS_P(obj), SOAP_1_2_ENV_NAMESPACE);
				} else if (strcmp(fault_code,"VersionMismatch") == 0 ||
				           strcmp(fault_code,"MustUnderstand") == 0 ||
				           strcmp(fault_code,"DataEncodingUnknown") == 0) {
					ZVAL_STRING(Z_FAULT_CODE_P(obj), fault_code);
					ZVAL_STRING(Z_FAULT_CODENS_P(obj), SOAP_1_2_ENV_NAMESPACE);
				} else {
					ZVAL_STRING(Z_FAULT_CODE_P(obj), fault_code);
				}
			}
		}
	}
	if (fault_actor != NULL) {
		ZVAL_STRING(Z_FAULT_ACTOR_P(obj), fault_actor);
	}
	if (fault_detail != NULL && Z_TYPE_P(fault_detail) != IS_UNDEF) {
		ZVAL_COPY(Z_FAULT_DETAIL_P(obj), fault_detail);
	}
	if (name != NULL) {
		ZVAL_STR_COPY(Z_FAULT_NAME_P(obj), name);
	}
	ZVAL_STR_COPY(Z_FAULT_LANG_P(obj), lang);
}
/* }}} */

static void deserialize_parameters(xmlNodePtr params, sdlFunctionPtr function, uint32_t *num_params, zval **parameters) /* {{{ */
{
	uint32_t cur_param = 0,num_of_params = 0;
	zval *tmp_parameters = NULL;

	if (function != NULL) {
		sdlParamPtr param;
		xmlNodePtr val;
		int	use_names = 0;

		if (function->requestParameters == NULL) {
			return;
		}
		num_of_params = zend_hash_num_elements(function->requestParameters);
		ZEND_HASH_FOREACH_PTR(function->requestParameters, param) {
			if (get_node(params, param->paramName) != NULL) {
				use_names = 1;
			}
		} ZEND_HASH_FOREACH_END();
		if (use_names) {
			tmp_parameters = safe_emalloc(num_of_params, sizeof(zval), 0);
			ZEND_HASH_FOREACH_PTR(function->requestParameters, param) {
				val = get_node(params, param->paramName);
				if (!val) {
					/* TODO: may be "nil" is not OK? */
					ZVAL_NULL(&tmp_parameters[cur_param]);
				} else {
					master_to_zval(&tmp_parameters[cur_param], param->encode, val);
				}
				cur_param++;
			} ZEND_HASH_FOREACH_END();
			*parameters = tmp_parameters;
			*num_params = num_of_params;
			return;
		}
	}
	if (params) {
		xmlNodePtr trav;

		num_of_params = 0;
		trav = params;
		while (trav != NULL) {
			if (trav->type == XML_ELEMENT_NODE) {
				num_of_params++;
			}
			trav = trav->next;
		}

		if (num_of_params == 1 &&
		    function &&
		    function->binding &&
		    function->binding->bindingType == BINDING_SOAP &&
		    ((sdlSoapBindingFunctionPtr)function->bindingAttributes)->style == SOAP_DOCUMENT &&
		    (function->requestParameters == NULL ||
		     zend_hash_num_elements(function->requestParameters) == 0) &&
		    strcmp((char *)params->name, function->functionName) == 0) {
			num_of_params = 0;
		} else if (num_of_params > 0) {
			tmp_parameters = safe_emalloc(num_of_params, sizeof(zval), 0);

			trav = params;
			while (trav != 0 && cur_param < num_of_params) {
				if (trav->type == XML_ELEMENT_NODE) {
					encodePtr enc;
					sdlParamPtr param = NULL;
					if (function != NULL &&
					    (param = zend_hash_index_find_ptr(function->requestParameters, cur_param)) == NULL) {
											soap_server_fault_en("Client", "Error cannot find parameter", NULL, NULL, NULL);
					}
					if (param == NULL) {
						enc = NULL;
					} else {
						enc = param->encode;
					}
					master_to_zval(&tmp_parameters[cur_param], enc, trav);
					cur_param++;
				}
				trav = trav->next;
			}
		}
	}
	if (num_of_params > cur_param) {
		soap_server_fault_en("Client","Missing parameter", NULL, NULL, NULL);
	}
	(*parameters) = tmp_parameters;
	(*num_params) = num_of_params;
}
/* }}} */

/* This function attempts to find the right function name based on the first node's name in the soap body.
 * If that doesn't work it falls back to get_doc_function(). */
static sdlFunctionPtr find_function(sdlPtr sdl, xmlNodePtr func, zval* function_name) /* {{{ */
{
	sdlFunctionPtr function;

	size_t xml_func_name_length = strlen((char*)func->name);
	function = get_function(sdl, (char*)func->name, xml_func_name_length);
	if (function && function->binding && function->binding->bindingType == BINDING_SOAP) {
		sdlSoapBindingFunctionPtr fnb = (sdlSoapBindingFunctionPtr)function->bindingAttributes;
		if (fnb->style == SOAP_DOCUMENT) {
			if (func->children != NULL ||
			    (function->requestParameters != NULL &&
			     zend_hash_num_elements(function->requestParameters) > 0)) {
				function = NULL;
			}
		}
	}
	if (sdl != NULL && function == NULL) {
		function = get_doc_function(sdl, func);
	}

	if (function != NULL) {
		ZVAL_STRING(function_name, (char *)function->functionName);
	} else {
		ZVAL_STRINGL(function_name, (char *)func->name, xml_func_name_length);
	}

	return function;
}
/* }}} */

static sdlFunctionPtr find_function_using_soap_action(const sdl *sdl, const char *soap_action, zval* function_name)
{
	if (!sdl) {
		return NULL;
	}

	/* The soap action may be a http-quoted string, in which case we're removing the quotes here. */
	size_t soap_action_length = strlen(soap_action);
	if (soap_action[0] == '"') {
		if (soap_action_length < 2 || soap_action[soap_action_length - 1] != '"') {
			return NULL;
		}
		soap_action++;
		soap_action_length -= 2;
	}

	/* TODO: This may depend on a particular target namespace, in which case this won't find a match when multiple different
	 *       target namespaces are used until #45282 is resolved. */
	sdlFunctionPtr function;
	ZEND_HASH_FOREACH_PTR(&sdl->functions, function) {
		if (function->binding && function->binding->bindingType == BINDING_SOAP) {
			sdlSoapBindingFunctionPtr fnb = function->bindingAttributes;
			if (fnb && fnb->soapAction && strncmp(fnb->soapAction, soap_action, soap_action_length) == 0 && fnb->soapAction[soap_action_length] == '\0') {
				ZVAL_STRING(function_name, function->functionName);
				return function;
			}
		}
	} ZEND_HASH_FOREACH_END();
	return NULL;
}

static xmlNodePtr get_envelope(xmlNodePtr trav, int *version, char **envelope_ns) {
	while (trav != NULL) {
		if (trav->type == XML_ELEMENT_NODE) {
			if (node_is_equal_ex(trav,"Envelope",SOAP_1_1_ENV_NAMESPACE)) {
				*version = SOAP_1_1;
				*envelope_ns = SOAP_1_1_ENV_NAMESPACE;
				SOAP_GLOBAL(soap_version) = SOAP_1_1;
				return trav;
			}

			if (node_is_equal_ex(trav,"Envelope",SOAP_1_2_ENV_NAMESPACE)) {
				*version = SOAP_1_2;
				*envelope_ns = SOAP_1_2_ENV_NAMESPACE;
				SOAP_GLOBAL(soap_version) = SOAP_1_2;
				return trav;
			}

			soap_server_fault_en("VersionMismatch", "Wrong Version", NULL, NULL, NULL);
		}
		trav = trav->next;
	}

	return NULL;
}

static sdlFunctionPtr deserialize_function_call(sdlPtr sdl, xmlDocPtr request, const char* actor, const char *soap_action, zval *function_name, uint32_t *num_params, zval **parameters, int *version, soapHeader **headers) /* {{{ */
{
	char* envelope_ns = NULL;
	xmlNodePtr trav,env,head,body,func;
	xmlAttrPtr attr;
	sdlFunctionPtr function = NULL;

	encode_reset_ns();

	/* Get <Envelope> element */
	env = get_envelope(request->children, version, &envelope_ns);
	if (!env) {
		soap_server_fault_en("Client", "looks like we got XML without \"Envelope\" element", NULL, NULL, NULL);
	}

	attr = env->properties;
	while (attr != NULL) {
		if (attr->ns == NULL) {
			soap_server_fault_en("Client", "A SOAP Envelope element cannot have non Namespace qualified attributes", NULL, NULL, NULL);
		} else if (attr_is_equal_ex(attr,"encodingStyle",SOAP_1_2_ENV_NAMESPACE)) {
			if (*version == SOAP_1_2) {
				soap_server_fault_en("Client", "encodingStyle cannot be specified on the Envelope", NULL, NULL, NULL);
			} else if (strcmp((char*)attr->children->content,SOAP_1_1_ENC_NAMESPACE) != 0) {
				soap_server_fault_en("Client", "Unknown data encoding style", NULL, NULL, NULL);
			}
		}
		attr = attr->next;
	}

	/* Get <Header> element */
	head = NULL;
	trav = env->children;
	while (trav != NULL && trav->type != XML_ELEMENT_NODE) {
		trav = trav->next;
	}
	if (trav != NULL && node_is_equal_ex(trav,"Header",envelope_ns)) {
		head = trav;
		trav = trav->next;
	}

	/* Get <Body> element */
	body = NULL;
	while (trav != NULL && trav->type != XML_ELEMENT_NODE) {
		trav = trav->next;
	}
	if (trav != NULL && node_is_equal_ex(trav,"Body",envelope_ns)) {
		body = trav;
		trav = trav->next;
	}
	while (trav != NULL && trav->type != XML_ELEMENT_NODE) {
		trav = trav->next;
	}
	if (body == NULL) {
		soap_server_fault_en("Client", "Body must be present in a SOAP envelope", NULL, NULL, NULL);
	}
	attr = body->properties;
	while (attr != NULL) {
		if (attr->ns == NULL) {
			if (*version == SOAP_1_2) {
				soap_server_fault_en("Client", "A SOAP Body element cannot have non Namespace qualified attributes", NULL, NULL, NULL);
			}
		} else if (attr_is_equal_ex(attr,"encodingStyle",SOAP_1_2_ENV_NAMESPACE)) {
			if (*version == SOAP_1_2) {
				soap_server_fault_en("Client", "encodingStyle cannot be specified on the Body", NULL, NULL, NULL);
			} else if (strcmp((char*)attr->children->content,SOAP_1_1_ENC_NAMESPACE) != 0) {
				soap_server_fault_en("Client", "Unknown data encoding style", NULL, NULL, NULL);
			}
		}
		attr = attr->next;
	}

	if (trav != NULL && *version == SOAP_1_2) {
		soap_server_fault_en("Client", "A SOAP 1.2 envelope can contain only Header and Body", NULL, NULL, NULL);
	}

	func = NULL;
	trav = body->children;
	while (trav != NULL) {
		if (trav->type == XML_ELEMENT_NODE) {
/*
			if (func != NULL) {
				soap_server_fault_en("Client", "looks like we got \"Body\" with several functions call", NULL, NULL, NULL);
			}
*/
			func = trav;
			break; /* FIXME: the rest of body is ignored */
		}
		trav = trav->next;
	}
	if (soap_action) {
		function = find_function_using_soap_action(sdl, soap_action, function_name);
	}
	if (func == NULL) {
		if (!function) {
			function = get_doc_function(sdl, NULL);
			if (function) {
				ZVAL_STRING(function_name, (char *)function->functionName);
			} else {
				soap_server_fault_en("Client", "looks like we got \"Body\" without function call", NULL, NULL, NULL);
			}
		}
	} else {
		if (*version == SOAP_1_1) {
			attr = get_attribute_ex(func->properties,"encodingStyle",SOAP_1_1_ENV_NAMESPACE);
			if (attr && strcmp((char*)attr->children->content,SOAP_1_1_ENC_NAMESPACE) != 0) {
				soap_server_fault_en("Client","Unknown Data Encoding Style", NULL, NULL, NULL);
			}
		} else {
			attr = get_attribute_ex(func->properties,"encodingStyle",SOAP_1_2_ENV_NAMESPACE);
			if (attr && strcmp((char*)attr->children->content,SOAP_1_2_ENC_NAMESPACE) != 0) {
				soap_server_fault_en("DataEncodingUnknown","Unknown Data Encoding Style", NULL, NULL, NULL);
			}
		}
		if (!function) {
			function = find_function(sdl, func, function_name);
		}
		if (sdl != NULL && function == NULL) {
			if (*version == SOAP_1_2) {
				soap_server_fault_en("rpc:ProcedureNotPresent","Procedure not present", NULL, NULL, NULL);
			} else {
				php_error(E_ERROR, "Procedure '%s' not present", func->name);
			}
		}
	}

	*headers = NULL;
	if (head) {
		soapHeader *h, *last = NULL;

		attr = head->properties;
		while (attr != NULL) {
			if (attr->ns == NULL) {
				soap_server_fault_en("Client", "A SOAP Header element cannot have non Namespace qualified attributes", NULL, NULL, NULL);
			} else if (attr_is_equal_ex(attr,"encodingStyle",SOAP_1_2_ENV_NAMESPACE)) {
				if (*version == SOAP_1_2) {
					soap_server_fault_en("Client", "encodingStyle cannot be specified on the Header", NULL, NULL, NULL);
				} else if (strcmp((char*)attr->children->content,SOAP_1_1_ENC_NAMESPACE) != 0) {
					soap_server_fault_en("Client", "Unknown data encoding style", NULL, NULL, NULL);
				}
			}
			attr = attr->next;
		}
		trav = head->children;
		while (trav != NULL) {
			if (trav->type == XML_ELEMENT_NODE) {
				xmlNodePtr hdr_func = trav;
				int mustUnderstand = 0;

				if (*version == SOAP_1_1) {
					attr = get_attribute_ex(hdr_func->properties,"encodingStyle",SOAP_1_1_ENV_NAMESPACE);
					if (attr && strcmp((char*)attr->children->content,SOAP_1_1_ENC_NAMESPACE) != 0) {
						soap_server_fault_en("Client","Unknown Data Encoding Style", NULL, NULL, NULL);
					}
					attr = get_attribute_ex(hdr_func->properties,"actor",envelope_ns);
					if (attr != NULL) {
						if (strcmp((char*)attr->children->content,SOAP_1_1_ACTOR_NEXT) != 0 &&
						    (actor == NULL || strcmp((char*)attr->children->content,actor) != 0)) {
						  goto ignore_header;
						}
					}
				} else if (*version == SOAP_1_2) {
					attr = get_attribute_ex(hdr_func->properties,"encodingStyle",SOAP_1_2_ENV_NAMESPACE);
					if (attr && strcmp((char*)attr->children->content,SOAP_1_2_ENC_NAMESPACE) != 0) {
						soap_server_fault_en("DataEncodingUnknown","Unknown Data Encoding Style", NULL, NULL, NULL);
					}
					attr = get_attribute_ex(hdr_func->properties,"role",envelope_ns);
					if (attr != NULL) {
						if (strcmp((char*)attr->children->content,SOAP_1_2_ACTOR_UNLIMATERECEIVER) != 0 &&
						    strcmp((char*)attr->children->content,SOAP_1_2_ACTOR_NEXT) != 0 &&
						    (actor == NULL || strcmp((char*)attr->children->content,actor) != 0)) {
						  goto ignore_header;
						}
					}
				}
				attr = get_attribute_ex(hdr_func->properties,"mustUnderstand",envelope_ns);
				if (attr) {
					if (strcmp((char*)attr->children->content,"1") == 0 ||
					    strcmp((char*)attr->children->content,"true") == 0) {
						mustUnderstand = 1;
					} else if (strcmp((char*)attr->children->content,"0") == 0 ||
					           strcmp((char*)attr->children->content,"false") == 0) {
						mustUnderstand = 0;
					} else {
						soap_server_fault_en("Client","mustUnderstand value is not boolean", NULL, NULL, NULL);
					}
				}
				h = emalloc(sizeof(soapHeader));
				memset(h, 0, sizeof(soapHeader));
				h->mustUnderstand = mustUnderstand;
				h->function = find_function(sdl, hdr_func, &h->function_name);
				if (!h->function && sdl && function && function->binding && function->binding->bindingType == BINDING_SOAP) {
					sdlSoapBindingFunctionHeaderPtr hdr;
					sdlSoapBindingFunctionPtr fnb = (sdlSoapBindingFunctionPtr)function->bindingAttributes;
					if (fnb->input.headers) {
						smart_str key = {0};

						if (hdr_func->ns) {
							smart_str_appends(&key, (char*)hdr_func->ns->href);
							smart_str_appendc(&key, ':');
						}
						smart_str_appendl(&key, Z_STRVAL(h->function_name), Z_STRLEN(h->function_name));
						smart_str_0(&key);
						if ((hdr = zend_hash_find_ptr(fnb->input.headers, key.s)) != NULL) {
							h->hdr = hdr;
						}
						smart_str_free(&key);
					}
				}
				if (h->hdr) {
					h->num_params = 1;
					h->parameters = emalloc(sizeof(zval));
					master_to_zval(&h->parameters[0], h->hdr->encode, hdr_func);
				} else {
					if (h->function && h->function->binding && h->function->binding->bindingType == BINDING_SOAP) {
						sdlSoapBindingFunctionPtr fnb = (sdlSoapBindingFunctionPtr)h->function->bindingAttributes;
						if (fnb->style == SOAP_RPC) {
							hdr_func = hdr_func->children;
						}
					}
					deserialize_parameters(hdr_func, h->function, &h->num_params, &h->parameters);
				}
				ZVAL_NULL(&h->retval);
				if (last == NULL) {
					*headers = h;
				} else {
					last->next = h;
				}
				last = h;
			}
ignore_header:
			trav = trav->next;
		}
	}

	if (function && function->binding && function->binding->bindingType == BINDING_SOAP) {
		sdlSoapBindingFunctionPtr fnb = (sdlSoapBindingFunctionPtr)function->bindingAttributes;
		if (fnb->style == SOAP_RPC) {
			func = func->children;
		}
	} else {
		func = func->children;
	}
	deserialize_parameters(func, function, num_params, parameters);

	encode_finish();

	return function;
}
/* }}} */

static void set_soap_header_attributes(xmlNodePtr h, zval *header, int version) /* {{{ */
{
	if (Z_TYPE_P(Z_HEADER_MUST_UNDERSTAND_P(header)) == IS_TRUE) {
		if (version == SOAP_1_1) {
			xmlSetProp(h, BAD_CAST(SOAP_1_1_ENV_NS_PREFIX":mustUnderstand"), BAD_CAST("1"));
		} else {
			xmlSetProp(h, BAD_CAST(SOAP_1_2_ENV_NS_PREFIX":mustUnderstand"), BAD_CAST("true"));
		}
	}

	zval *tmp = Z_HEADER_ACTOR_P(header);
	if (Z_TYPE_P(tmp) == IS_STRING) {
		if (version == SOAP_1_1) {
			xmlSetProp(h, BAD_CAST(SOAP_1_1_ENV_NS_PREFIX":actor"), BAD_CAST(Z_STRVAL_P(tmp)));
		} else {
			xmlSetProp(h, BAD_CAST(SOAP_1_2_ENV_NS_PREFIX":role"), BAD_CAST(Z_STRVAL_P(tmp)));
		}
	} else if (Z_TYPE_P(tmp) == IS_LONG) {
		if (version == SOAP_1_1) {
			if (Z_LVAL_P(tmp) == SOAP_ACTOR_NEXT) {
				xmlSetProp(h, BAD_CAST(SOAP_1_1_ENV_NS_PREFIX":actor"), BAD_CAST(SOAP_1_1_ACTOR_NEXT));
			}
		} else {
			if (Z_LVAL_P(tmp) == SOAP_ACTOR_NEXT) {
				xmlSetProp(h, BAD_CAST(SOAP_1_2_ENV_NS_PREFIX":role"), BAD_CAST(SOAP_1_2_ACTOR_NEXT));
			} else if (Z_LVAL_P(tmp) == SOAP_ACTOR_NONE) {
				xmlSetProp(h, BAD_CAST(SOAP_1_2_ENV_NS_PREFIX":role"), BAD_CAST(SOAP_1_2_ACTOR_NONE));
			} else if (Z_LVAL_P(tmp) == SOAP_ACTOR_UNLIMATERECEIVER) {
				xmlSetProp(h, BAD_CAST(SOAP_1_2_ENV_NS_PREFIX":role"), BAD_CAST(SOAP_1_2_ACTOR_UNLIMATERECEIVER));
			}
		}
	}
}
/* }}} */

static int serialize_response_call2(xmlNodePtr body, sdlFunctionPtr function, const char *function_name, const char *uri, zval *ret, int version, int main, xmlNodePtr *node) /* {{{ */
{
	xmlNodePtr method = NULL, param;
	sdlParamPtr parameter = NULL;
	uint32_t param_count;
	int style, use;
	xmlNsPtr ns = NULL;

	if (function != NULL && function->binding->bindingType == BINDING_SOAP) {
		sdlSoapBindingFunctionPtr fnb = (sdlSoapBindingFunctionPtr)function->bindingAttributes;

		style = fnb->style;
		use = fnb->output.use;
		if (style == SOAP_RPC) {
			ns = encode_add_ns(body, fnb->output.ns);
			if (function->responseName) {
				method = xmlNewChild(body, ns, BAD_CAST(function->responseName), NULL);
			} else if (function->responseParameters) {
				method = xmlNewChild(body, ns, BAD_CAST(function->functionName), NULL);
			}
		}
	} else {
		style = main?SOAP_RPC:SOAP_DOCUMENT;
		use = main?SOAP_ENCODED:SOAP_LITERAL;
		if (style == SOAP_RPC) {
			ns = encode_add_ns(body, uri);
			method = xmlNewChild(body, ns, BAD_CAST(function_name), NULL);
		}
	}

	if (function != NULL) {
		if (function->responseParameters) {
			param_count = zend_hash_num_elements(function->responseParameters);
		} else {
		  param_count = 0;
		}
	} else {
	  param_count = 1;
	}

	if (param_count == 1) {
		parameter = get_param(function, NULL, 0, TRUE);

		if (style == SOAP_RPC) {
		  xmlNode *rpc_result;
			if (main && version == SOAP_1_2) {
				xmlNs *rpc_ns = xmlNewNs(body, BAD_CAST(RPC_SOAP12_NAMESPACE), BAD_CAST(RPC_SOAP12_NS_PREFIX));
				rpc_result = xmlNewChild(method, rpc_ns, BAD_CAST("result"), NULL);
				param = serialize_parameter(parameter, ret, 0, "return", use, method);
				xmlNodeSetContent(rpc_result,param->name);
			} else {
				param = serialize_parameter(parameter, ret, 0, "return", use, method);
			}
		} else {
			param = serialize_parameter(parameter, ret, 0, "return", use, body);
			if (function && function->binding->bindingType == BINDING_SOAP) {
				if (parameter && parameter->element) {
					ns = encode_add_ns(param, parameter->element->namens);
					xmlNodeSetName(param, BAD_CAST(parameter->element->name));
					xmlSetNs(param, ns);
				}
			} else if (strcmp((char*)param->name,"return") == 0) {
				ns = encode_add_ns(param, uri);
				xmlNodeSetName(param, BAD_CAST(function_name));
				xmlSetNs(param, ns);
			}
		}
	} else if (param_count > 1 && Z_TYPE_P(ret) == IS_ARRAY) {
		zval *data;
		int i = 0;
		zend_string *param_name;
		zend_ulong param_index = i;

		ZEND_HASH_FOREACH_KEY_VAL(Z_ARRVAL_P(ret), param_index, param_name, data) {
			parameter = get_param(function, param_name ? ZSTR_VAL(param_name) : NULL, param_index, TRUE);
			if (style == SOAP_RPC) {
				param = serialize_parameter(parameter, data, i, param_name ? ZSTR_VAL(param_name) : NULL, use, method);
			} else {
				param = serialize_parameter(parameter, data, i, param_name ? ZSTR_VAL(param_name) : NULL, use, body);
				if (function && function->binding->bindingType == BINDING_SOAP) {
					if (parameter && parameter->element) {
						ns = encode_add_ns(param, parameter->element->namens);
						xmlNodeSetName(param, BAD_CAST(parameter->element->name));
						xmlSetNs(param, ns);
					}
				}
			}

			i++;
			param_index = i;
		} ZEND_HASH_FOREACH_END();
	}
	if (use == SOAP_ENCODED && version == SOAP_1_2 && method != NULL) {
		xmlSetNsProp(method, body->ns, BAD_CAST("encodingStyle"), BAD_CAST(SOAP_1_2_ENC_NAMESPACE));
	}
	if (node) {
		*node = method;
	}
	return use;
}
/* }}} */

static xmlDocPtr serialize_response_call(sdlFunctionPtr function, const char *function_name, const char *uri, zval *ret, soapHeader* headers, int version) /* {{{ */
{
	xmlDocPtr doc;
	xmlNodePtr envelope = NULL, body, param;
	xmlNsPtr ns = NULL;
	int use = SOAP_LITERAL;
	xmlNodePtr head = NULL;

	encode_reset_ns();

	doc = xmlNewDoc(BAD_CAST("1.0"));
	zend_try {

	doc->charset = XML_CHAR_ENCODING_UTF8;
	doc->encoding = xmlCharStrdup("UTF-8");

	if (version == SOAP_1_1) {
		envelope = xmlNewDocNode(doc, NULL, BAD_CAST("Envelope"), NULL);
		ns = xmlNewNs(envelope, BAD_CAST(SOAP_1_1_ENV_NAMESPACE), BAD_CAST(SOAP_1_1_ENV_NS_PREFIX));
		xmlSetNs(envelope,ns);
	} else if (version == SOAP_1_2) {
		envelope = xmlNewDocNode(doc, NULL, BAD_CAST("Envelope"), NULL);
		ns = xmlNewNs(envelope, BAD_CAST(SOAP_1_2_ENV_NAMESPACE), BAD_CAST(SOAP_1_2_ENV_NS_PREFIX));
		xmlSetNs(envelope,ns);
	} else {
		soap_server_fault_en("Server", "Unknown SOAP version", NULL, NULL, NULL);
	}
	xmlDocSetRootElement(doc, envelope);

	if (Z_TYPE_P(ret) == IS_OBJECT &&
	    instanceof_function(Z_OBJCE_P(ret), soap_fault_class_entry)) {
		char *detail_name;
		sdlFaultPtr fault = NULL;
		char *fault_ns = NULL;
		zval *tmp = Z_FAULT_HEADERFAULT_P(ret);
		if (headers && Z_TYPE_P(tmp) > IS_NULL) {
			encodePtr hdr_enc = NULL;
			int hdr_use = SOAP_LITERAL;
			zval *hdr_ret  = tmp;
			char *hdr_ns   = headers->hdr?headers->hdr->ns:NULL;
			char *hdr_name = Z_STRVAL(headers->function_name);

			head = xmlNewChild(envelope, ns, BAD_CAST("Header"), NULL);
			if (Z_TYPE_P(hdr_ret) == IS_OBJECT &&
			    instanceof_function(Z_OBJCE_P(hdr_ret), soap_header_class_entry)) {
				sdlSoapBindingFunctionHeaderPtr hdr;
				smart_str key = {0};

				tmp = Z_HEADER_NAMESPACE_P(hdr_ret);
				if (Z_TYPE_P(tmp) == IS_STRING) {
					smart_str_appendl(&key, Z_STRVAL_P(tmp), Z_STRLEN_P(tmp));
					smart_str_appendc(&key, ':');
					hdr_ns = Z_STRVAL_P(tmp);
				}

				tmp = Z_HEADER_NAME_P(hdr_ret);
				if (Z_TYPE_P(tmp) == IS_STRING) {
					smart_str_appendl(&key, Z_STRVAL_P(tmp), Z_STRLEN_P(tmp));
					hdr_name = Z_STRVAL_P(tmp);
				}
				smart_str_0(&key);
				if (headers->hdr && headers->hdr->headerfaults &&
				    (hdr = zend_hash_find_ptr(headers->hdr->headerfaults, key.s)) != NULL) {
					hdr_enc = hdr->encode;
					hdr_use = hdr->use;
				}
				smart_str_free(&key);
				tmp = Z_HEADER_DATA_P(hdr_ret);
				if (Z_TYPE_P(tmp) > IS_NULL) {
					hdr_ret = tmp;
				} else {
					hdr_ret = NULL;
				}
			}

			if (headers->function) {
				if (serialize_response_call2(head, headers->function, Z_STRVAL(headers->function_name), uri, hdr_ret, version, 0, NULL) == SOAP_ENCODED) {
					use = SOAP_ENCODED;
				}
			} else {
				xmlNodePtr xmlHdr = master_to_xml(hdr_enc, hdr_ret, hdr_use, head);
				if (hdr_name) {
					xmlNodeSetName(xmlHdr, BAD_CAST(hdr_name));
				}
				if (hdr_ns) {
					xmlNsPtr nsptr = encode_add_ns(xmlHdr, hdr_ns);
					xmlSetNs(xmlHdr, nsptr);
				}
			}
		}

		body = xmlNewChild(envelope, ns, BAD_CAST("Body"), NULL);
		param = xmlNewChild(body, ns, BAD_CAST("Fault"), NULL);

		tmp = Z_FAULT_CODENS_P(ret);
		if (Z_TYPE_P(tmp) == IS_STRING) {
			fault_ns = Z_STRVAL_P(tmp);
		}
		use = SOAP_LITERAL;

		tmp = Z_FAULT_NAME_P(ret);
		if (Z_TYPE_P(tmp) == IS_STRING) {
			sdlFaultPtr tmp_fault;
			if (function && function->faults &&
			    (tmp_fault = zend_hash_find_ptr(function->faults, Z_STR_P(tmp))) != NULL) {
				fault = tmp_fault;
				if (function->binding &&
				    function->binding->bindingType == BINDING_SOAP &&
				    fault->bindingAttributes) {
					sdlSoapBindingFunctionFaultPtr fb = (sdlSoapBindingFunctionFaultPtr)fault->bindingAttributes;
					use = fb->use;
					if (fault_ns == NULL) {
						fault_ns = fb->ns;
					}
				}
			}
		} else if (function && function->faults &&
		           zend_hash_num_elements(function->faults) == 1) {

			zend_hash_internal_pointer_reset(function->faults);
			fault = zend_hash_get_current_data_ptr(function->faults);
			if (function->binding &&
			    function->binding->bindingType == BINDING_SOAP &&
			    fault->bindingAttributes) {
				sdlSoapBindingFunctionFaultPtr fb = (sdlSoapBindingFunctionFaultPtr)fault->bindingAttributes;
				use = fb->use;
				if (fault_ns == NULL) {
					fault_ns = fb->ns;
				}
			}
		}

		if (fault_ns == NULL &&
		    fault &&
		    fault->details &&
		    zend_hash_num_elements(fault->details) == 1) {
			sdlParamPtr sparam;

			zend_hash_internal_pointer_reset(fault->details);
			sparam = zend_hash_get_current_data_ptr(fault->details);
			if (sparam->element) {
				fault_ns = sparam->element->namens;
			}
		}

		if (version == SOAP_1_1) {
			tmp = Z_FAULT_CODE_P(ret);
			if (Z_TYPE_P(tmp) == IS_STRING) {
				xmlNodePtr node = xmlNewDocNode(doc, NULL, BAD_CAST("faultcode"), NULL);
				zend_string *str = php_escape_html_entities((unsigned char*)Z_STRVAL_P(tmp), Z_STRLEN_P(tmp), 0, 0, NULL);
				xmlAddChild(param, node);
				if (fault_ns) {
					xmlNsPtr nsptr = encode_add_ns(node, fault_ns);
					xmlChar *code = xmlBuildQName(BAD_CAST(ZSTR_VAL(str)), nsptr->prefix, NULL, 0);
					xmlNodeSetContent(node, code);
					xmlFree(code);
				} else {
					xmlNodeSetContentLen(node, BAD_CAST(ZSTR_VAL(str)), (int)ZSTR_LEN(str));
				}
				zend_string_release_ex(str, 0);
			}
			tmp = Z_FAULT_STRING_P(ret);
			if (Z_TYPE_P(tmp) == IS_STRING) {
				xmlNodePtr node = master_to_xml(get_conversion(IS_STRING), tmp, SOAP_LITERAL, param);
				xmlNodeSetName(node, BAD_CAST("faultstring"));
			}
			tmp = Z_FAULT_ACTOR_P(ret);
			if (Z_TYPE_P(tmp) == IS_STRING) {
				xmlNodePtr node = master_to_xml(get_conversion(IS_STRING), tmp, SOAP_LITERAL, param);
				xmlNodeSetName(node, BAD_CAST("faultactor"));
			}
			detail_name = "detail";
		} else {
			tmp = Z_FAULT_CODE_P(ret);
			if (Z_TYPE_P(tmp) == IS_STRING) {
				xmlNodePtr node = xmlNewChild(param, ns, BAD_CAST("Code"), NULL);
				zend_string *str = php_escape_html_entities((unsigned char*)Z_STRVAL_P(tmp), Z_STRLEN_P(tmp), 0, 0, NULL);
				node = xmlNewChild(node, ns, BAD_CAST("Value"), NULL);
				if (fault_ns) {
					xmlNsPtr nsptr = encode_add_ns(node, fault_ns);
					xmlChar *code = xmlBuildQName(BAD_CAST(ZSTR_VAL(str)), nsptr->prefix, NULL, 0);
					xmlNodeSetContent(node, code);
					xmlFree(code);
				} else {
					xmlNodeSetContentLen(node, BAD_CAST(ZSTR_VAL(str)), (int)ZSTR_LEN(str));
				}
				zend_string_release_ex(str, 0);
			}
			tmp = Z_FAULT_STRING_P(ret);
			if (Z_TYPE_P(tmp) == IS_STRING) {
				xmlNodePtr node = xmlNewChild(param, ns, BAD_CAST("Reason"), NULL);
				node = master_to_xml(get_conversion(IS_STRING), tmp, SOAP_LITERAL, node);
				xmlNodeSetName(node, BAD_CAST("Text"));
				xmlSetNs(node, ns);

				/* xml:lang attribute is required for <Reason> in SOAP 1.2 */
				tmp = Z_FAULT_LANG_P(ret);
				zend_string *lang = Z_STR_P(tmp);
				xmlNodeSetLang(node, BAD_CAST ZSTR_VAL(lang));
			}
			detail_name = SOAP_1_2_ENV_NS_PREFIX":Detail";
		}
		if (fault && fault->details && zend_hash_num_elements(fault->details) == 1) {
			xmlNodePtr node;
			zval *detail = NULL;
			sdlParamPtr sparam;
			xmlNodePtr x;

			tmp = Z_FAULT_DETAIL_P(ret);
			if (Z_TYPE_P(tmp) > IS_NULL) {
				detail = tmp;
			}
			node = xmlNewDocNode(doc, NULL, BAD_CAST(detail_name), NULL);
			xmlAddChild(param, node);

			zend_hash_internal_pointer_reset(fault->details);
			sparam = zend_hash_get_current_data_ptr(fault->details);

			if (detail &&
			    Z_TYPE_P(detail) == IS_OBJECT &&
			    sparam->element &&
			    zend_hash_num_elements(Z_OBJPROP_P(detail)) == 1 &&
			    (tmp = zend_hash_str_find(Z_OBJPROP_P(detail), sparam->element->name, strlen(sparam->element->name))) != NULL) {
				detail = tmp;
			}

			x = serialize_parameter(sparam, detail, 1, NULL, use, node);

			if (function &&
			    function->binding &&
			    function->binding->bindingType == BINDING_SOAP &&
			    function->bindingAttributes) {
				sdlSoapBindingFunctionPtr fnb = (sdlSoapBindingFunctionPtr)function->bindingAttributes;
				if (fnb->style == SOAP_RPC && !sparam->element) {
				  if (fault->bindingAttributes) {
						sdlSoapBindingFunctionFaultPtr fb = (sdlSoapBindingFunctionFaultPtr)fault->bindingAttributes;
						if (fb->ns) {
							xmlNsPtr ns = encode_add_ns(x, fb->ns);
							xmlSetNs(x, ns);
						}
					}
				} else {
					if (sparam->element) {
						ns = encode_add_ns(x, sparam->element->namens);
						xmlNodeSetName(x, BAD_CAST(sparam->element->name));
						xmlSetNs(x, ns);
					}
				}
			}
			if (use == SOAP_ENCODED && version == SOAP_1_2) {
				xmlSetNsProp(x, envelope->ns, BAD_CAST("encodingStyle"), BAD_CAST(SOAP_1_2_ENC_NAMESPACE));
			}
		} else {
			tmp = Z_FAULT_DETAIL_P(ret);
			if (Z_TYPE_P(tmp) > IS_NULL) {
				serialize_zval(tmp, NULL, detail_name, use, param);
			}
		}
	} else {

		if (headers) {
			soapHeader *h;

			head = xmlNewChild(envelope, ns, BAD_CAST("Header"), NULL);
			h = headers;
			while (h != NULL) {
				if (Z_TYPE(h->retval) != IS_NULL) {
					encodePtr hdr_enc = NULL;
					int hdr_use = SOAP_LITERAL;
					zval *hdr_ret = &h->retval;
					char *hdr_ns   = h->hdr?h->hdr->ns:NULL;
					char *hdr_name = Z_TYPE(h->function_name) == IS_STRING
						? Z_STRVAL(h->function_name) : NULL;
					bool is_header_object = Z_TYPE(h->retval) == IS_OBJECT &&
					    instanceof_function(Z_OBJCE(h->retval), soap_header_class_entry);

					if (is_header_object) {
						zval *tmp;
						sdlSoapBindingFunctionHeaderPtr hdr;
						smart_str key = {0};

						tmp = Z_HEADER_NAMESPACE_P(&h->retval);
						if (Z_TYPE_P(tmp) == IS_STRING) {
							smart_str_appendl(&key, Z_STRVAL_P(tmp), Z_STRLEN_P(tmp));
							smart_str_appendc(&key, ':');
							hdr_ns = Z_STRVAL_P(tmp);
						}
						tmp = Z_HEADER_NAME_P(&h->retval);
						if (Z_TYPE_P(tmp) == IS_STRING) {
							smart_str_appendl(&key, Z_STRVAL_P(tmp), Z_STRLEN_P(tmp));
							hdr_name = Z_STRVAL_P(tmp);
						}
						smart_str_0(&key);
						if (function && function->binding && function->binding->bindingType == BINDING_SOAP) {
							sdlSoapBindingFunctionPtr fnb = (sdlSoapBindingFunctionPtr)function->bindingAttributes;

							if (fnb->output.headers &&
							    (hdr = zend_hash_find_ptr(fnb->output.headers, key.s)) != NULL) {
								hdr_enc = hdr->encode;
								hdr_use = hdr->use;
							}
						}
						smart_str_free(&key);
						tmp = Z_HEADER_DATA_P(&h->retval);
						if (Z_TYPE_P(tmp) > IS_NULL) {
							hdr_ret = tmp;
						} else {
							hdr_ret = NULL;
						}
					}

					if (h->function) {
						xmlNodePtr xmlHdr = NULL;

						if (serialize_response_call2(head, h->function, Z_STRVAL(h->function_name), uri, hdr_ret, version, 0, &xmlHdr) == SOAP_ENCODED) {
							use = SOAP_ENCODED;
						}
						if (is_header_object) {
							set_soap_header_attributes(xmlHdr, &h->retval, version);
						}
					} else {
						xmlNodePtr xmlHdr = master_to_xml(hdr_enc, hdr_ret, hdr_use, head);
						if (hdr_name) {
							xmlNodeSetName(xmlHdr, BAD_CAST(hdr_name));
						}
						if (hdr_ns) {
							xmlNsPtr nsptr = encode_add_ns(xmlHdr,hdr_ns);
							xmlSetNs(xmlHdr, nsptr);
						}
						if (is_header_object) {
							set_soap_header_attributes(xmlHdr, &h->retval, version);
						}
					}
				}
				h = h->next;
			}

			if (head->children == NULL) {
				xmlUnlinkNode(head);
				xmlFreeNode(head);
			}
		}

		body = xmlNewChild(envelope, ns, BAD_CAST("Body"), NULL);

		if (serialize_response_call2(body, function, function_name, uri, ret, version, 1, NULL) == SOAP_ENCODED) {
			use = SOAP_ENCODED;
		}

	}

	if (use == SOAP_ENCODED) {
		xmlNewNs(envelope, BAD_CAST(XSD_NAMESPACE), BAD_CAST(XSD_NS_PREFIX));
		if (version == SOAP_1_1) {
			xmlNewNs(envelope, BAD_CAST(SOAP_1_1_ENC_NAMESPACE), BAD_CAST(SOAP_1_1_ENC_NS_PREFIX));
			xmlSetNsProp(envelope, envelope->ns, BAD_CAST("encodingStyle"), BAD_CAST(SOAP_1_1_ENC_NAMESPACE));
		} else if (version == SOAP_1_2) {
			xmlNewNs(envelope, BAD_CAST(SOAP_1_2_ENC_NAMESPACE), BAD_CAST(SOAP_1_2_ENC_NS_PREFIX));
		}
	}

	encode_finish();

	} zend_catch {
		/* Avoid persistent memory leak. */
		xmlFreeDoc(doc);
		zend_bailout();
	} zend_end_try();

	if (function && function->responseName == NULL &&
	    body->children == NULL && head == NULL) {
		xmlFreeDoc(doc);
		return NULL;
	}
	return doc;
}
/* }}} */

static xmlDocPtr serialize_function_call(zval *this_ptr, sdlFunctionPtr function, const char *function_name, const char *uri, zval *arguments, uint32_t arg_count, int version, HashTable *soap_headers) /* {{{ */
{
	xmlDoc *doc;
	xmlNodePtr envelope = NULL, body, method = NULL, head = NULL;
	xmlNsPtr ns = NULL;
	zval *zstyle, *zuse;
	uint32_t i;
	int style, use;
	HashTable *hdrs = NULL;

	encode_reset_ns();

	doc = xmlNewDoc(BAD_CAST("1.0"));
	zend_try {

	doc->encoding = xmlCharStrdup("UTF-8");
	doc->charset = XML_CHAR_ENCODING_UTF8;
	if (version == SOAP_1_1) {
		envelope = xmlNewDocNode(doc, NULL, BAD_CAST("Envelope"), NULL);
		ns = xmlNewNs(envelope, BAD_CAST(SOAP_1_1_ENV_NAMESPACE), BAD_CAST(SOAP_1_1_ENV_NS_PREFIX));
		xmlSetNs(envelope, ns);
	} else if (version == SOAP_1_2) {
		envelope = xmlNewDocNode(doc, NULL, BAD_CAST("Envelope"), NULL);
		ns = xmlNewNs(envelope, BAD_CAST(SOAP_1_2_ENV_NAMESPACE), BAD_CAST(SOAP_1_2_ENV_NS_PREFIX));
		xmlSetNs(envelope, ns);
	} else {
		soap_error0(E_ERROR, "Unknown SOAP version");
	}
	xmlDocSetRootElement(doc, envelope);

	if (soap_headers) {
		head = xmlNewChild(envelope, ns, BAD_CAST("Header"), NULL);
	}

	body = xmlNewChild(envelope, ns, BAD_CAST("Body"), NULL);

	if (function && function->binding->bindingType == BINDING_SOAP) {
		sdlSoapBindingFunctionPtr fnb = (sdlSoapBindingFunctionPtr)function->bindingAttributes;

		hdrs = fnb->input.headers;
		style = fnb->style;
		/*FIXME: how to pass method name if style is SOAP_DOCUMENT */
		/*style = SOAP_RPC;*/
		use = fnb->input.use;
		if (style == SOAP_RPC) {
			ns = encode_add_ns(body, fnb->input.ns);
			if (function->requestName) {
				method = xmlNewChild(body, ns, BAD_CAST(function->requestName), NULL);
			} else {
				method = xmlNewChild(body, ns, BAD_CAST(function->functionName), NULL);
			}
		}
	} else {
		zstyle = Z_CLIENT_STYLE_P(this_ptr);
		if (Z_TYPE_P(zstyle) == IS_LONG) {
			style = Z_LVAL_P(zstyle);
		} else {
			style = SOAP_RPC;
		}
		/*FIXME: how to pass method name if style is SOAP_DOCUMENT */
		/*style = SOAP_RPC;*/
		if (style == SOAP_RPC) {
			ns = encode_add_ns(body, uri);
			if (function_name) {
				method = xmlNewChild(body, ns, BAD_CAST(function_name), NULL);
			} else if (function && function->requestName) {
				method = xmlNewChild(body, ns, BAD_CAST(function->requestName), NULL);
			} else if (function && function->functionName) {
				method = xmlNewChild(body, ns, BAD_CAST(function->functionName), NULL);
			} else {
				method = body;
			}
		} else {
			method = body;
		}

		zuse = Z_CLIENT_USE_P(this_ptr);
		if (Z_TYPE_P(zuse) == IS_LONG && Z_LVAL_P(zuse) == SOAP_LITERAL) {
			use = SOAP_LITERAL;
		} else {
			use = SOAP_ENCODED;
		}
	}

	for (i = 0;i < arg_count;i++) {
		xmlNodePtr param;
		sdlParamPtr parameter = get_param(function, NULL, i, FALSE);

		if (style == SOAP_RPC) {
			param = serialize_parameter(parameter, &arguments[i], i, NULL, use, method);
		} else if (style == SOAP_DOCUMENT) {
			param = serialize_parameter(parameter, &arguments[i], i, NULL, use, body);
			if (function && function->binding->bindingType == BINDING_SOAP) {
				if (parameter && parameter->element) {
					ns = encode_add_ns(param, parameter->element->namens);
					xmlNodeSetName(param, BAD_CAST(parameter->element->name));
					xmlSetNs(param, ns);
				}
			}
		}
	}

	if (function && function->requestParameters) {
		uint32_t n = zend_hash_num_elements(function->requestParameters);

		if (n > arg_count) {
			for (i = arg_count; i < n; i++) {
				xmlNodePtr param;
				sdlParamPtr parameter = get_param(function, NULL, i, FALSE);

				if (style == SOAP_RPC) {
					param = serialize_parameter(parameter, NULL, i, NULL, use, method);
				} else if (style == SOAP_DOCUMENT) {
					param = serialize_parameter(parameter, NULL, i, NULL, use, body);
					if (function && function->binding->bindingType == BINDING_SOAP) {
						if (parameter && parameter->element) {
							ns = encode_add_ns(param, parameter->element->namens);
							xmlNodeSetName(param, BAD_CAST(parameter->element->name));
							xmlSetNs(param, ns);
						}
					}
				}
			}
		}
	}

	if (head) {
		zval* header;

		ZEND_HASH_FOREACH_VAL(soap_headers, header) {
			if (Z_TYPE_P(header) != IS_OBJECT
					|| !instanceof_function(Z_OBJCE_P(header), soap_header_class_entry)) {
				continue;
			}

			zval *name = Z_HEADER_NAME_P(header);
			zval *ns = Z_HEADER_NAMESPACE_P(header);
			if (Z_TYPE_P(name) == IS_STRING && Z_TYPE_P(ns) == IS_STRING) {
				xmlNodePtr h;
				xmlNsPtr nsptr;
				int hdr_use = SOAP_LITERAL;
				encodePtr enc = NULL;

				if (hdrs) {
					smart_str key = {0};
					sdlSoapBindingFunctionHeaderPtr hdr;

					smart_str_appendl(&key, Z_STRVAL_P(ns), Z_STRLEN_P(ns));
					smart_str_appendc(&key, ':');
					smart_str_appendl(&key, Z_STRVAL_P(name), Z_STRLEN_P(name));
					smart_str_0(&key);
					if ((hdr = zend_hash_find_ptr(hdrs, key.s)) != NULL) {
						hdr_use = hdr->use;
						enc = hdr->encode;
						if (hdr_use == SOAP_ENCODED) {
							use = SOAP_ENCODED;
						}
					}
					smart_str_free(&key);
				}

				zval *data = Z_HEADER_DATA_P(header);
				if (Z_TYPE_P(data) > IS_NULL) {
					h = master_to_xml(enc, data, hdr_use, head);
					xmlNodeSetName(h, BAD_CAST(Z_STRVAL_P(name)));
				} else {
					h = xmlNewDocNode(doc, NULL, BAD_CAST(Z_STRVAL_P(name)), NULL);
					xmlAddChild(head, h);
				}
				nsptr = encode_add_ns(h, Z_STRVAL_P(ns));
				xmlSetNs(h, nsptr);
				set_soap_header_attributes(h, header, version);
			}
		} ZEND_HASH_FOREACH_END();
	}

	if (use == SOAP_ENCODED) {
		xmlNewNs(envelope, BAD_CAST(XSD_NAMESPACE), BAD_CAST(XSD_NS_PREFIX));
		if (version == SOAP_1_1) {
			xmlNewNs(envelope, BAD_CAST(SOAP_1_1_ENC_NAMESPACE), BAD_CAST(SOAP_1_1_ENC_NS_PREFIX));
			xmlSetNsProp(envelope, envelope->ns, BAD_CAST("encodingStyle"), BAD_CAST(SOAP_1_1_ENC_NAMESPACE));
		} else if (version == SOAP_1_2) {
			xmlNewNs(envelope, BAD_CAST(SOAP_1_2_ENC_NAMESPACE), BAD_CAST(SOAP_1_2_ENC_NS_PREFIX));
			if (method) {
				xmlSetNsProp(method, envelope->ns, BAD_CAST("encodingStyle"), BAD_CAST(SOAP_1_2_ENC_NAMESPACE));
			}
		}
	}

	encode_finish();

	} zend_catch {
		/* Avoid persistent memory leak. */
		xmlFreeDoc(doc);
		zend_bailout();
	} zend_end_try();

	return doc;
}
/* }}} */

static xmlNodePtr serialize_parameter(sdlParamPtr param, zval *param_val, uint32_t index, const char *name, int style, xmlNodePtr parent) /* {{{ */
{
	const char *paramName;
	xmlNodePtr xmlParam;
	char paramNameBuf[10];

	if (param_val && Z_TYPE_P(param_val) == IS_OBJECT
			&& Z_OBJCE_P(param_val) == soap_param_class_entry) {
		zval *param_name = Z_PARAM_NAME_P(param_val);
		zval *param_data = Z_PARAM_DATA_P(param_val);
		if (Z_TYPE_P(param_name) == IS_STRING && Z_TYPE_P(param_data) != IS_UNDEF) {
			param_val = param_data;
			name = Z_STRVAL_P(param_name);
		}
	}

	if (param != NULL && param->paramName != NULL) {
		paramName = param->paramName;
	} else {
		if (name == NULL) {
			snprintf(paramNameBuf, sizeof(paramNameBuf), "param%d",index);
			paramName = paramNameBuf;
		} else {
			paramName = name;
		}
	}

	xmlParam = serialize_zval(param_val, param, paramName, style, parent);

	return xmlParam;
}
/* }}} */

static xmlNodePtr serialize_zval(zval *val, sdlParamPtr param, const char *paramName, int style, xmlNodePtr parent) /* {{{ */
{
	xmlNodePtr xmlParam;
	encodePtr enc;
	zval defval;

	ZVAL_UNDEF(&defval);
	if (param != NULL) {
		enc = param->encode;
		if (val == NULL) {
			if (param->element) {
				if (param->element->fixed) {
					ZVAL_STRING(&defval, param->element->fixed);
					val = &defval;
				} else if (param->element->def && !param->element->nillable) {
					ZVAL_STRING(&defval, param->element->def);
					val = &defval;
				}
			}
		}
	} else {
		enc = NULL;
	}
	xmlParam = master_to_xml(enc, val, style, parent);
	zval_ptr_dtor(&defval);
	if (xmlParam != NULL) {
		if (xmlParam->name == NULL || strcmp((char*)xmlParam->name, "BOGUS") == 0) {
			xmlNodeSetName(xmlParam, BAD_CAST(paramName));
		}
	}
	return xmlParam;
}
/* }}} */

static sdlParamPtr get_param(sdlFunctionPtr function, const char *param_name, zend_ulong index, int response) /* {{{ */
{
	sdlParamPtr tmp;
	HashTable   *ht;

	if (function == NULL) {
		return NULL;
	}

	if (response == FALSE) {
		ht = function->requestParameters;
	} else {
		ht = function->responseParameters;
	}

	if (ht == NULL) {
	  return NULL;
	}

	if (param_name != NULL) {
		if ((tmp = zend_hash_str_find_ptr(ht, param_name, strlen(param_name))) != NULL) {
			return tmp;
		} else {
			ZEND_HASH_FOREACH_PTR(ht, tmp) {
				if (tmp->paramName && strcmp(param_name, tmp->paramName) == 0) {
					return tmp;
				}
			} ZEND_HASH_FOREACH_END();
		}
	} else {
		if ((tmp = zend_hash_index_find_ptr(ht, index)) != NULL) {
			return tmp;
		}
	}
	return NULL;
}
/* }}} */

static sdlFunctionPtr get_function(sdlPtr sdl, const char *function_name, size_t function_name_length) /* {{{ */
{
	sdlFunctionPtr tmp;

	if (sdl != NULL) {
		if ((tmp = zend_hash_str_find_ptr_lc(&sdl->functions, function_name, function_name_length)) != NULL) {
			return tmp;
		} else if (sdl->requests != NULL && (tmp = zend_hash_str_find_ptr_lc(sdl->requests, function_name, function_name_length)) != NULL) {
			return tmp;
		}
	}
	return NULL;
}
/* }}} */

/* This function tries to find the function that matches the given parameters.
 * If params is NULL, it will return the first function that has no parameters.
 * If params is not NULL, it will return the first function that has the same
 * parameters as the given XML node. */
static sdlFunctionPtr get_doc_function(sdlPtr sdl, xmlNodePtr params) /* {{{ */
{
	if (sdl) {
		sdlFunctionPtr tmp;
		sdlParamPtr    param;

		ZEND_HASH_MAP_FOREACH_PTR(&sdl->functions, tmp) {
			if (tmp->binding && tmp->binding->bindingType == BINDING_SOAP) {
				sdlSoapBindingFunctionPtr fnb = (sdlSoapBindingFunctionPtr)tmp->bindingAttributes;
				if (fnb->style == SOAP_DOCUMENT) {
					if (params == NULL) {
						if (tmp->requestParameters == NULL ||
						    zend_hash_num_elements(tmp->requestParameters) == 0) {
						  return tmp;
						}
					} else if (tmp->requestParameters != NULL &&
					           zend_hash_num_elements(tmp->requestParameters) > 0) {
						int ok = 1;
						xmlNodePtr node = params;

						ZEND_HASH_FOREACH_PTR(tmp->requestParameters, param) {
							if (param->element) {
								if (strcmp(param->element->name, (char*)node->name) != 0) {
									ok = 0;
									break;
								}
								if (param->element->namens != NULL && node->ns != NULL) {
									if (strcmp(param->element->namens, (char*)node->ns->href) != 0) {
										ok = 0;
										break;
									}
								} else if ((void*)param->element->namens != (void*)node->ns) {
									ok = 0;
									break;
								}
							} else if (strcmp(param->paramName, (char*)node->name) != 0) {
								ok = 0;
								break;
							}
							node = node->next;
						} ZEND_HASH_FOREACH_END();
						if (ok /*&& node == NULL*/) {
							return tmp;
						}
					}
				}
			}
		} ZEND_HASH_FOREACH_END();
	}
	return NULL;
}
/* }}} */

static void function_to_string(sdlFunctionPtr function, smart_str *buf) /* {{{ */
{
	int i = 0;
	sdlParamPtr param;

	if (function->responseParameters &&
	    zend_hash_num_elements(function->responseParameters) > 0) {
		if (zend_hash_num_elements(function->responseParameters) == 1) {
			zend_hash_internal_pointer_reset(function->responseParameters);
			param = zend_hash_get_current_data_ptr(function->responseParameters);
			if (param->encode && param->encode->details.type_str) {
				smart_str_appendl(buf, param->encode->details.type_str, strlen(param->encode->details.type_str));
				smart_str_appendc(buf, ' ');
			} else {
				smart_str_appendl(buf, "UNKNOWN ", 8);
			}
		} else {
			i = 0;
			smart_str_appendl(buf, "list(", 5);
			ZEND_HASH_FOREACH_PTR(function->responseParameters, param) {
				if (i > 0) {
					smart_str_appendl(buf, ", ", 2);
				}
				if (param->encode && param->encode->details.type_str) {
					smart_str_appendl(buf, param->encode->details.type_str, strlen(param->encode->details.type_str));
				} else {
					smart_str_appendl(buf, "UNKNOWN", 7);
				}
				smart_str_appendl(buf, " $", 2);
				smart_str_appendl(buf, param->paramName, strlen(param->paramName));
				i++;
			} ZEND_HASH_FOREACH_END();
			smart_str_appendl(buf, ") ", 2);
		}
	} else {
		smart_str_appendl(buf, "void ", 5);
	}

	smart_str_appendl(buf, function->functionName, strlen(function->functionName));

	smart_str_appendc(buf, '(');
	if (function->requestParameters) {
		i = 0;
		ZEND_HASH_FOREACH_PTR(function->requestParameters, param) {
			if (i > 0) {
				smart_str_appendl(buf, ", ", 2);
			}
			if (param->encode && param->encode->details.type_str) {
				smart_str_appendl(buf, param->encode->details.type_str, strlen(param->encode->details.type_str));
			} else {
				smart_str_appendl(buf, "UNKNOWN", 7);
			}
			smart_str_appendl(buf, " $", 2);
			smart_str_appendl(buf, param->paramName, strlen(param->paramName));
			i++;
		} ZEND_HASH_FOREACH_END();
	}
	smart_str_appendc(buf, ')');
	smart_str_0(buf);
}
/* }}} */

static void model_to_string(sdlContentModelPtr model, smart_str *buf, int level) /* {{{ */
{
	int i;

	switch (model->kind) {
		case XSD_CONTENT_ELEMENT:
			type_to_string(model->u.element, buf, level);
			smart_str_appendl(buf, ";\n", 2);
			break;
		case XSD_CONTENT_ANY:
			for (i = 0;i < level;i++) {
				smart_str_appendc(buf, ' ');
			}
			smart_str_appendl(buf, "<anyXML> any;\n", sizeof("<anyXML> any;\n")-1);
			break;
		case XSD_CONTENT_SEQUENCE:
		case XSD_CONTENT_ALL:
		case XSD_CONTENT_CHOICE: {
			sdlContentModelPtr tmp;

			ZEND_HASH_FOREACH_PTR(model->u.content, tmp) {
				model_to_string(tmp, buf, level);
			} ZEND_HASH_FOREACH_END();
			break;
		}
		case XSD_CONTENT_GROUP:
			model_to_string(model->u.group->model, buf, level);
		default:
		  break;
	}
}
/* }}} */

static void type_to_string(sdlTypePtr type, smart_str *buf, int level) /* {{{ */
{
	int i;
	smart_str spaces = {0};

	for (i = 0;i < level;i++) {
		smart_str_appendc(&spaces, ' ');
	}
	if (spaces.s) {
		smart_str_appendl(buf, ZSTR_VAL(spaces.s), ZSTR_LEN(spaces.s));
	}
	switch (type->kind) {
		case XSD_TYPEKIND_SIMPLE:
			if (type->encode) {
				smart_str_appendl(buf, type->encode->details.type_str, strlen(type->encode->details.type_str));
				smart_str_appendc(buf, ' ');
			} else {
				smart_str_appendl(buf, "anyType ", sizeof("anyType ")-1);
			}
			smart_str_appendl(buf, type->name, strlen(type->name));

			if (type->restrictions && type->restrictions->enumeration) {
				zend_string *key;
				bool first = true;

				smart_str_appends(buf, " {");
				ZEND_HASH_MAP_FOREACH_STR_KEY(type->restrictions->enumeration, key) {
					if (first) {
						first = false;
					} else {
						smart_str_appends(buf, ", ");
					}
					smart_str_append(buf, key);
				} ZEND_HASH_FOREACH_END();
				smart_str_appendc(buf, '}');
			}
			break;
		case XSD_TYPEKIND_LIST:
			smart_str_appendl(buf, "list ", 5);
			smart_str_appendl(buf, type->name, strlen(type->name));
			if (type->elements) {
				sdlTypePtr item_type;

				smart_str_appendl(buf, " {", 2);
				ZEND_HASH_FOREACH_PTR(type->elements, item_type) {
					smart_str_appendl(buf, item_type->name, strlen(item_type->name));
				} ZEND_HASH_FOREACH_END();
				smart_str_appendc(buf, '}');
			}
			break;
		case XSD_TYPEKIND_UNION:
			smart_str_appendl(buf, "union ", 6);
			smart_str_appendl(buf, type->name, strlen(type->name));
			if (type->elements) {
				sdlTypePtr item_type;
				int first = 0;

				smart_str_appendl(buf, " {", 2);
				ZEND_HASH_FOREACH_PTR(type->elements, item_type) {
					if (!first) {
						smart_str_appendc(buf, ',');
						first = 0;
					}
					smart_str_appendl(buf, item_type->name, strlen(item_type->name));
				} ZEND_HASH_FOREACH_END();
				smart_str_appendc(buf, '}');
			}
			break;
		case XSD_TYPEKIND_COMPLEX:
		case XSD_TYPEKIND_RESTRICTION:
		case XSD_TYPEKIND_EXTENSION:
			if (type->encode &&
			    (type->encode->details.type == IS_ARRAY ||
			     type->encode->details.type == SOAP_ENC_ARRAY)) {
			  sdlAttributePtr attr;
			  sdlExtraAttributePtr ext;

				if (type->attributes &&
				    (attr = zend_hash_str_find_ptr(type->attributes, SOAP_1_1_ENC_NAMESPACE":arrayType",
				      sizeof(SOAP_1_1_ENC_NAMESPACE":arrayType")-1)) != NULL &&
					attr->extraAttributes &&
				    (ext = zend_hash_str_find_ptr(attr->extraAttributes, WSDL_NAMESPACE":arrayType", sizeof(WSDL_NAMESPACE":arrayType")-1)) != NULL) {
					char *end = strchr(ext->val, '[');
					size_t len;
					if (end == NULL) {
						len = strlen(ext->val);
					} else {
						len = end - ext->val;
					}
					if (len == 0) {
						smart_str_appendl(buf, "anyType", sizeof("anyType")-1);
					} else {
						smart_str_appendl(buf, ext->val, len);
					}
					smart_str_appendc(buf, ' ');
					smart_str_appendl(buf, type->name, strlen(type->name));
					if (end != NULL) {
						smart_str_appends(buf, end);
					}
				} else {
					sdlTypePtr elementType;
					if (type->attributes &&
					    (attr = zend_hash_str_find_ptr(type->attributes, SOAP_1_2_ENC_NAMESPACE":itemType",
					      sizeof(SOAP_1_2_ENC_NAMESPACE":itemType")-1)) != NULL &&
						attr->extraAttributes &&
				    (ext = zend_hash_str_find_ptr(attr->extraAttributes, WSDL_NAMESPACE":itemType", sizeof(WSDL_NAMESPACE":arrayType")-1)) != NULL) {
						smart_str_appends(buf, ext->val);
						smart_str_appendc(buf, ' ');
					} else if (type->elements &&
					           zend_hash_num_elements(type->elements) == 1 &&
					           (zend_hash_internal_pointer_reset(type->elements),
					            (elementType = zend_hash_get_current_data_ptr(type->elements)) != NULL) &&
					           elementType->encode && elementType->encode->details.type_str) {
						smart_str_appends(buf, elementType->encode->details.type_str);
						smart_str_appendc(buf, ' ');
					} else {
						smart_str_appendl(buf, "anyType ", 8);
					}
					smart_str_appendl(buf, type->name, strlen(type->name));
					if (type->attributes &&
					    (attr = zend_hash_str_find_ptr(type->attributes, SOAP_1_2_ENC_NAMESPACE":arraySize",
					      sizeof(SOAP_1_2_ENC_NAMESPACE":arraySize")-1)) != NULL &&
						attr->extraAttributes &&
					    (ext = zend_hash_str_find_ptr(attr->extraAttributes, WSDL_NAMESPACE":itemType", sizeof(WSDL_NAMESPACE":arraySize")-1)) != NULL) {
						smart_str_appendc(buf, '[');
						smart_str_appends(buf, ext->val);
						smart_str_appendc(buf, ']');
					} else {
						smart_str_appendl(buf, "[]", 2);
					}
				}
			} else {
				smart_str_appendl(buf, "struct ", 7);
				smart_str_appendl(buf, type->name, strlen(type->name));
				smart_str_appendc(buf, ' ');
				smart_str_appendl(buf, "{\n", 2);
				if ((type->kind == XSD_TYPEKIND_RESTRICTION ||
				     type->kind == XSD_TYPEKIND_EXTENSION) && type->encode) {
					encodePtr enc = type->encode;
					while (enc && enc->details.sdl_type &&
					       enc != enc->details.sdl_type->encode &&
					       enc->details.sdl_type->kind != XSD_TYPEKIND_SIMPLE &&
					       enc->details.sdl_type->kind != XSD_TYPEKIND_LIST &&
					       enc->details.sdl_type->kind != XSD_TYPEKIND_UNION) {
						enc = enc->details.sdl_type->encode;
					}
					if (enc) {
						if (spaces.s) {
							smart_str_appendl(buf, ZSTR_VAL(spaces.s), ZSTR_LEN(spaces.s));
						}
						smart_str_appendc(buf, ' ');
						smart_str_appendl(buf, type->encode->details.type_str, strlen(type->encode->details.type_str));
						smart_str_appendl(buf, " _;\n", 4);
					}
				}
				if (type->model) {
					model_to_string(type->model, buf, level+1);
				}
				if (type->attributes) {
					sdlAttributePtr attr;

					ZEND_HASH_FOREACH_PTR(type->attributes, attr) {
						if (spaces.s) {
							smart_str_appendl(buf, ZSTR_VAL(spaces.s), ZSTR_LEN(spaces.s));
						}
						smart_str_appendc(buf, ' ');
						if (attr->encode && attr->encode->details.type_str) {
							smart_str_appends(buf, attr->encode->details.type_str);
							smart_str_appendc(buf, ' ');
						} else {
							smart_str_appendl(buf, "UNKNOWN ", 8);
						}
						smart_str_appends(buf, attr->name);
						smart_str_appendl(buf, ";\n", 2);
					} ZEND_HASH_FOREACH_END();
				}
				if (spaces.s) {
					smart_str_appendl(buf, ZSTR_VAL(spaces.s), ZSTR_LEN(spaces.s));
				}
				smart_str_appendc(buf, '}');
			}
			break;
		default:
			break;
	}
	smart_str_free(&spaces);
	smart_str_0(buf);
}
/* }}} */

static void delete_argv(struct _soap_class *class)
{
	if (class->argc) {
		for (uint32_t i = 0; i < class->argc; i++) {
			zval_ptr_dtor(&class->argv[i]);
		}
		efree(class->argv);
	}
}

static void delete_service(soapServicePtr service) /* {{{ */
{
	if (service->soap_functions.ft) {
		zend_hash_destroy(service->soap_functions.ft);
		efree(service->soap_functions.ft);
	}

	if (service->typemap) {
		zend_hash_destroy(service->typemap);
		efree(service->typemap);
	}

	delete_argv(&service->soap_class);

	if (service->actor) {
		efree(service->actor);
	}
	if (service->uri) {
		efree(service->uri);
	}
	if (service->sdl) {
		delete_sdl(service->sdl);
	}
	if (service->encoding) {
		xmlCharEncCloseFunc(service->encoding);
	}
	if (service->class_map) {
		zend_hash_destroy(service->class_map);
		FREE_HASHTABLE(service->class_map);
	}
	if (service->last_response_body) {
		zend_string_release_ex(service->last_response_body, false);
	}
	zval_ptr_dtor(&service->soap_object);
	efree(service);
}
/* }}} */

/* }}} */
