
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_monitor.h"
#include <hiredis/hiredis.h>

//===========redis===========
redisContext* cache_client;
#define REDIS_EXE redisReply* cache_reply = (redisReply*) redisCommand(cache_client, command);
#define REDIS_FREE freeReplyObject(cache_reply);
#define REDIS_NIL if (cache_reply->type == REDIS_REPLY_NIL) {REDIS_FREE return NULL;}
#define REDIS_CHECK(X)  if (cache_reply->type != X) {printf("Failed to execute command[%s]\n", command);REDIS_FREE return;}

//============pipe===========
static int monitor_enable = 0;
int pipe_send;
//===========zend============
static struct zend_exe zend_backup = {NULL};

#ifdef EX
#undef EX
#define EX(element) ((execute_data)->element)
#endif
#define EX_T(offset) (*(temp_variable *)((char *) EX(Ts) + offset))
/* If you declare any globals in php_monitor.h uncomment this: */
ZEND_DECLARE_MODULE_GLOBALS(monitor)


/* True global resources - no need for thread safety here */
static int le_monitor;
static redisReply *listenFunction;
/* {{{ monitor_functions[]
 *
 * Every user visible function must have an entry in monitor_functions[].
 */
const zend_function_entry monitor_functions[] = {
    PHP_FE(confirm_monitor_compiled, NULL) /* For testing, remove later. */
    PHP_FE_END /* Must be the last line in monitor_functions[] */
};
/* }}} */

/* {{{ monitor_module_entry
 */
zend_module_entry monitor_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
    STANDARD_MODULE_HEADER,
#endif
    "monitor",
    monitor_functions,
    PHP_MINIT(monitor),
    PHP_MSHUTDOWN(monitor),
    PHP_RINIT(monitor), /* Replace with NULL if there's nothing to do at request start */
    PHP_RSHUTDOWN(monitor), /* Replace with NULL if there's nothing to do at request end */
    PHP_MINFO(monitor),
#if ZEND_MODULE_API_NO >= 20010901
    PHP_MONITOR_VERSION,
#endif
    STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_MONITOR

ZEND_GET_MODULE(monitor)
#endif

/* {{{ PHP_INI
 */

PHP_INI_BEGIN()
STD_PHP_INI_ENTRY("monitor.enable", "0", PHP_INI_ALL, OnUpdateString, enable, zend_monitor_globals, monitor_globals)
STD_PHP_INI_ENTRY("monitor.cache_host", "127.0.0.1", PHP_INI_ALL, OnUpdateString, cache_host, zend_monitor_globals, monitor_globals)
STD_PHP_INI_ENTRY("monitor.cache_port", "6379", PHP_INI_ALL, OnUpdateString, cache_port, zend_monitor_globals, monitor_globals)
STD_PHP_INI_ENTRY("monitor.cache_set_key", "_monitor_php_function", PHP_INI_ALL, OnUpdateString, cache_set_key, zend_monitor_globals, monitor_globals)
STD_PHP_INI_ENTRY("monitor.cache_list_key", "_monitor_php_function_info", PHP_INI_ALL, OnUpdateString, cache_list_key, zend_monitor_globals, monitor_globals)
PHP_INI_END()

/* }}} */

/* {{{ php_monitor_init_globals
 */
/* Uncomment this function if you have INI entries 
static void php_monitor_init_globals(zend_monitor_globals *monitor_globals)
{
monitor_globals->global_value = 0;
monitor_globals->global_string = NULL;
}
 */
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(monitor) {
    /* If you have INI entries, uncomment these lines */
    REGISTER_INI_ENTRIES();
    if (INI_INT("monitor.enable") == 1) {
        monitor_enable = 1;
        monitor_start();
    }
    return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(monitor) {
    /* uncomment this line if you have INI entries */
    UNREGISTER_INI_ENTRIES();

    return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request start */

/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(monitor) {
    if (monitor_enable == 1) {
        monitor_set();
        zend_backup.zend_execute_internal = zend_execute_internal;
        zend_execute_internal = monitor_execute_internal;
    }
    return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */

/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(monitor) {
    if (monitor_enable == 1) {
        freeReplyObject(listenFunction);
        zend_execute_internal = zend_backup.zend_execute_internal;
    }
    return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(monitor) {
    php_info_print_table_start();
    php_info_print_table_header(2, "monitor support", "enabled");
    php_info_print_table_end();

    /* Remove comments if you have entries in php.ini*/
    DISPLAY_INI_ENTRIES();

}
/* }}} */


/* Remove the following function when you have successfully modified config.m4
   so that your module can be compiled into PHP, it exists only for testing
   purposes. */

/* Every user-visible function in PHP should document itself in the source */

/* {{{ proto string confirm_monitor_compiled(string arg)
   Return a string to confirm that the module is compiled in */
PHP_FUNCTION(confirm_monitor_compiled) {
    char *arg = NULL;
    int arg_len, len;
    char *strg;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &arg, &arg_len) == FAILURE) {
        return;
    }

    len = spprintf(&strg, 0, "Congratulations! You have successfully modified ext/%.78s/config.m4. Module %.78s is now compiled into PHP.", "monitor", arg);
    RETURN_STRINGL(strg, len, 0);
}
/* }}} */

/* The previous line is meant for vim and emacs, so it can correctly fold and 
   unfold functions in source code. See the corresponding marks just before 
   function definition, where the functions purpose is also documented. Please 
   follow this convention for the convenience of others editing your code.
 */

void get_cache_client() {
    cache_client = redisConnect(INI_STR("globals.cache_host"), INI_INT("monitor.cache_port"));
    if (cache_client->err) {
        redisFree(cache_client);
        perror("can not client redis\n");
        return;
        //exit(EXIT_FAILURE);
    }
}

void cache_excute_set(char * command) {
    REDIS_EXE
    if (NULL == cache_reply) {
        printf("Execut command failure\n");
        return;
    }
    if (!(cache_reply->type == REDIS_REPLY_STATUS && strcasecmp(cache_reply->str, "OK") == 0)) {
        printf("Failed to execute command[%s]\n", command);
        REDIS_FREE
        return;
    }
    REDIS_FREE
}

void cache_excute_lpush(char* command) {
    REDIS_EXE
    if (!(cache_reply->type == REDIS_REPLY_INTEGER && cache_reply->integer > 0)) {
        printf("Failed to execute command[%s]\n", command);
        REDIS_FREE
        return;
    }
    REDIS_FREE
}

void monitor_start() {
    int fds[2];
    if (pipe(fds) < 0) {
        perror("create pipe error");
        return;
        //exit(EXIT_FAILURE);
    }
    get_cache_client();
    switch (fork()) {
        case 0:
            close(fds[1]);
            monitor_init(fds[0]);
            close(fds[0]);
            _exit(0);
        case -1:
            perror("fork child error");
            return;
            //exit(EXIT_FAILURE);
        default:
            close(fds[0]);
            pipe_send = fds[1];
            break;
    }

}

void monitor_init(int pipe_get) {
    char buf[MAX_BUF_SIZE];
    char *cache_list_key = INI_STR("monitor.cache_list_key");
    int len=strlen(cache_list_key) + MAX_BUF_SIZE + 6;
    char *command = (char *) emalloc(6);
    char *run = (char *) emalloc(len);
    strcpy(command, "lpush ");
    strcat(command, cache_list_key);
    strcat(command, " ");
    while (read(pipe_get, buf, MAX_BUF_SIZE) > 0) {
        strcpy(run,command);
        strcat(run, buf);
        cache_excute_lpush(run);
        memset(run,'\0',len);
    }
    return;
}

void monitor_set() {
    char *cache_set_key = INI_STR("monitor.cache_set_key");
    char *command = (char *) emalloc(strlen(cache_set_key) + 9);
    strcpy(command, "smembers ");
    strcat(command, cache_set_key);
    REDIS_EXE
    REDIS_CHECK(REDIS_REPLY_ARRAY)
    listenFunction = cache_reply;
    return;
}

ZEND_API void monitor_execute_internal(zend_execute_data *execute_data_ptr, zend_fcall_info *fci, int return_value_used TSRMLS_DC) {
    int i;
    if (listenFunction->elements == 0) {
        return;
    }
    zend_execute_data* data = EG(current_execute_data);
    for (i = 0; i < listenFunction->elements; i++) {
        char *p = NULL;
        char temp[100];
        sprintf(temp, "%s", listenFunction->element[i]->str);
        char* token = strtok_r(temp, split, &p);
        char* parms = strtok_r(NULL, split, &p);
        if (token != NULL && (strcmp(data->function_state.function->internal_function.function_name, token) == 0)) {
            char *arg = NULL, *buf = NULL;
            int arg_len;
            buf = monitor_func_info(data);
            //data->function_state.function->common.required_num_args
            zend_parse_parameters(1 TSRMLS_CC, "s", &arg, &arg_len);
            if (parms != NULL) {
                if (strstr(arg, parms) != NULL) {
                    strcat(buf, "|&|args:");
                    char src[] = " ";
                    char des[] = "\\s";
                    char *dest;
                    strcat(buf, strreplace(dest,arg,src,des,MAX_BUF_SIZE));
                }
            }
            printf("%s\n", buf);
            write(pipe_send, buf, MAX_BUF_SIZE);
            free(buf);
        }
    }
    if (fci != NULL) {
        ((zend_internal_function *) execute_data_ptr->function_state.function)->handler(fci->param_count, *fci->retval_ptr_ptr, fci->retval_ptr_ptr, fci->object_ptr, 1 TSRMLS_CC);
    } else {
        zval **return_value_ptr = &EX_TMP_VAR(execute_data_ptr, execute_data_ptr->opline->result.var)->var.ptr;
        ((zend_internal_function *) execute_data_ptr->function_state.function)->handler(execute_data_ptr->opline->extended_value, *return_value_ptr, (execute_data_ptr->function_state.function->common.fn_flags & ZEND_ACC_RETURN_REFERENCE) ? return_value_ptr : NULL, execute_data_ptr->object, return_value_used TSRMLS_CC);
    }
    monitor_fun_res(data);
}

void strcat_int(char* buf, int num) {
    char sz[10];
    sprintf(sz, "%d", num);
    strcat(buf, sz);
}

static char* monitor_func_info(const zend_execute_data* data TSRMLS_DC) {
    zend_function* func = data->function_state.function;
    if ((func == NULL) || (func->common.function_name == NULL)) {
        return NULL;
    }
    char *buf;
    buf = (char*) malloc(MAX_BUF_SIZE);
    strcpy(buf, "");
    strcat(buf, "filename:");
    strcat(buf, (func->type != ZEND_INTERNAL_FUNCTION) ? func->op_array.filename : "");
    strcat(buf, "|&|scope:");
    strcat(buf, (func->common.scope == NULL) ? "" : func->common.scope->name);
    strcat(buf, "|&|fun_name:");
    strcat(buf, func->common.function_name);
    strcat(buf, "|&|argc:");

    strcat_int(buf, func->common.num_args);
    strcat(buf, "|&|required:");
    strcat_int(buf, func->common.required_num_args);
    strcat(buf, "|&|start:");
    strcat_int(buf, (func->type != ZEND_INTERNAL_FUNCTION) ? func->op_array.line_start : 0);
    strcat(buf, "|&|end:");
    strcat_int(buf, (func->type != ZEND_INTERNAL_FUNCTION) ? func->op_array.line_end : 0);
    strcat(buf, "|&|run_line:");
    strcat_int(buf, (data->opline != NULL) ? data->opline->lineno : 0);

    int argc = (int) (zend_uintptr_t) data->function_state.arguments[0];
    for (int i = 0; i < argc; i++) {
        if (i < func->common.num_args) {
            strcat(buf, "|&|class_name:");
            strcat(buf, (func->common.arg_info[i].class_name != NULL) ? func->common.arg_info[i].class_name : "");
            strcat(buf, "|&|name:");
            strcat(buf, func->common.arg_info[i].name);
        }
    }
    return buf;
}

static void monitor_fun_res(const zend_execute_data* data TSRMLS_DC) {
    zend_function* func = data->function_state.function;
    if ((func == NULL) || (func->common.function_name == NULL)) {
        return;
    }
}

char *strreplace(char *dest, char *src, const char *oldstr, const char *newstr, size_t len) {
    if (strcmp(oldstr, newstr) == 0)
        return src;
    char *needle;
    char *tmp;
    dest = src;
    while ((needle = strstr(dest, oldstr)) && (needle - dest <= len)) {
        tmp = (char*) malloc(strlen(dest)+(strlen(newstr) - strlen(oldstr)) + 1);
        strncpy(tmp, dest, needle - dest);
        tmp[needle - dest] = '\0';
        strcat(tmp, newstr);
        strcat(tmp, needle + strlen(oldstr));
        dest = strdup(tmp);
        free(tmp);
    }
    return dest;
}
/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
