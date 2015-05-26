
#ifndef PHP_MONITOR_H
#define PHP_MONITOR_H

extern zend_module_entry monitor_module_entry;
#define phpext_monitor_ptr &monitor_module_entry

#define PHP_MONITOR_VERSION "0.1.0" /* Replace with version number for your extension */

#ifdef PHP_WIN32
#define PHP_MONITOR_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#define PHP_MONITOR_API __attribute__ ((visibility("default")))
#else
#define PHP_MONITOR_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

PHP_MINIT_FUNCTION(monitor);
PHP_MSHUTDOWN_FUNCTION(monitor);
PHP_RINIT_FUNCTION(monitor);
PHP_RSHUTDOWN_FUNCTION(monitor);
PHP_MINFO_FUNCTION(monitor);

PHP_FUNCTION(confirm_monitor_compiled); /* For testing, remove later. */

/* 
        Declare any global variables you may need between the BEGIN
        and END macros here:     
 */
ZEND_BEGIN_MODULE_GLOBALS(monitor)
int enable;
char *cache_host;
char *cache_port;
char *cache_set_key;
char *cache_list_key;
ZEND_END_MODULE_GLOBALS(monitor)

void get_cache_client();
void cache_excute_set(char *);
int cache_get_int(char *);
char* cache_get_str(char *);
void monitor_start();
void monitor_init(int);
void monitor_set();

char *strreplace(char *dest, char *src, const char *oldstr, const char *newstr, size_t len);

static char* monitor_func_info(const zend_execute_data* TSRMLS_DC);
static void monitor_fun_res(const zend_execute_data* TSRMLS_DC);
struct zend_exe {
    void (*zend_execute_internal)(zend_execute_data *, zend_fcall_info *, int TSRMLS_DC);
};

ZEND_API void monitor_execute_internal(zend_execute_data *, zend_fcall_info *, int TSRMLS_DC);

const char * split = "|"; 

typedef struct {
    char *c;
    size_t len;
    size_t a;
} smart_str;
/* In every utility function you add that needs to use variables 
   in php_monitor_globals, call TSRMLS_FETCH(); after declaring other 
   variables used by that function, or better yet, pass in TSRMLS_CC
   after the last function argument and declare your utility function
   with TSRMLS_DC after the last declared argument.  Always refer to
   the globals in your function as MONITOR_G(variable).  You are 
   encouraged to rename these macros something shorter, see
   examples in any other php module directory.
 */
#define MAX_BUF_SIZE 500
#ifdef ZTS
#define MONITOR_G(v) TSRMG(monitor_globals_id, zend_monitor_globals *, v)
#else
#define MONITOR_G(v) (monitor_globals.v)
#endif

#endif	/* PHP_MONITOR_H */

