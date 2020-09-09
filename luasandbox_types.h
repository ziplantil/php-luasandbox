#ifndef LUASANDBOX_TYPES_H
#define LUASANDBOX_TYPES_H

#include <semaphore.h>
#include "php.h"
#include "luasandbox_version.h"

#ifdef CLOCK_REALTIME

struct _php_luasandbox_obj;

typedef struct _luasandbox_timer {
	struct _php_luasandbox_obj * sandbox;
	timer_t timer;
	clockid_t clock_id;
	int type;
	sem_t semaphore;
	int id;
} luasandbox_timer;

typedef struct {
	luasandbox_timer *limiter_timer;
	luasandbox_timer *profiler_timer;
	struct timespec limiter_limit, limiter_remaining;
	struct timespec usage_start, usage;
	struct timespec pause_start, pause_delta;
	struct timespec limiter_expired_at;
	struct timespec profiler_period;
	struct _php_luasandbox_obj * sandbox;
	int is_running;
	int limiter_running;
	int profiler_running;

	// A HashTable storing the number of times each function was hit by the
	// profiler. The data is a size_t because that hits a special case in
	// zend_hash which avoids the need to allocate separate space for the data
	// on the heap.
	HashTable * function_counts;

	// The total number of samples recorded in function_counts
	long total_count;

	// The number of timer expirations that have occurred since the profiler hook
	// was last run
	volatile long profiler_signal_count;

	volatile long overrun_count;
} luasandbox_timer_set;

#else /*CLOCK_REALTIME*/

typedef struct {} luasandbox_timer;
typedef struct {
	struct timespec profiler_period;
	HashTable * function_counts;
	long total_count;
	int is_paused;
} luasandbox_timer_set;

#endif /*CLOCK_REALTIME*/

ZEND_BEGIN_MODULE_GLOBALS(luasandbox)
	HashTable * allowed_globals;
	long active_count;
ZEND_END_MODULE_GLOBALS(luasandbox)

typedef struct {
	lua_Alloc old_alloc;
	void * old_alloc_ud;
	size_t memory_limit;
	size_t memory_usage;
	size_t peak_memory_usage;
} php_luasandbox_alloc;

struct _php_luasandbox_obj {
#if PHP_VERSION_ID < 70000
	zend_object std;
#endif
	lua_State * state;
	php_luasandbox_alloc alloc;
	int in_php;
	int in_lua;
#if PHP_VERSION_ID < 70000
	zval * current_zval; /* The zval for the LuaSandbox which is currently executing Lua code */
#else
	zval current_zval; /* The zval for the LuaSandbox which is currently executing Lua code */
#endif
	volatile int timed_out;
	int is_cpu_limited;
	luasandbox_timer_set timer;
	int function_index;
#if LUASANDBOX_MEMORY_PROFILING
        char * current_fname;
        HashTable * function_memory;
#endif
	unsigned int random_seed;
	int allow_pause;
#if PHP_VERSION_ID >= 70000
	zend_object std;
#endif
};
typedef struct _php_luasandbox_obj php_luasandbox_obj;

struct _php_luasandboxfunction_obj {
#if PHP_VERSION_ID < 70000
	zend_object std;
	zval * sandbox;
	int index;
#else
	zval sandbox;
	int index;
	zend_object std;
#endif
};
typedef struct _php_luasandboxfunction_obj php_luasandboxfunction_obj;

// Accessor macros
#if PHP_VERSION_ID < 70000

static inline php_luasandbox_obj *php_luasandbox_fetch_object(zend_object *obj) {
	return (php_luasandbox_obj *)obj;
}

static inline php_luasandboxfunction_obj *php_luasandboxfunction_fetch_object(zend_object *obj) {
	return (php_luasandboxfunction_obj *)obj;
}

#define GET_LUASANDBOX_OBJ(z) (php_luasandbox_obj *)zend_object_store_get_object(z TSRMLS_CC)
#define GET_LUASANDBOXFUNCTION_OBJ(z) (php_luasandboxfunction_obj *)zend_object_store_get_object(z TSRMLS_CC)
#define LUASANDBOXFUNCTION_SANDBOX_IS_OK(pfunc) (pfunc)->sandbox
#define LUASANDBOXFUNCTION_GET_SANDBOX_ZVALPTR(pfunc) (pfunc)->sandbox
#define LUASANDBOX_GET_CURRENT_ZVAL_PTR(psandbox) (psandbox)->current_zval

#else

static inline php_luasandbox_obj *php_luasandbox_fetch_object(zend_object *obj) {
	return (php_luasandbox_obj *)((char*)(obj) - XtOffsetOf(php_luasandbox_obj, std));
}

static inline php_luasandboxfunction_obj *php_luasandboxfunction_fetch_object(zend_object *obj) {
	return (php_luasandboxfunction_obj *)((char*)(obj) - XtOffsetOf(php_luasandboxfunction_obj, std));
}

#define GET_LUASANDBOX_OBJ(z) php_luasandbox_fetch_object(Z_OBJ_P(z))
#define GET_LUASANDBOXFUNCTION_OBJ(z) php_luasandboxfunction_fetch_object(Z_OBJ_P(z))
#define LUASANDBOXFUNCTION_SANDBOX_IS_OK(pfunc) !Z_ISUNDEF((pfunc)->sandbox)
#define LUASANDBOXFUNCTION_GET_SANDBOX_ZVALPTR(pfunc) &((pfunc)->sandbox)
#define LUASANDBOX_GET_CURRENT_ZVAL_PTR(psandbox) &((psandbox)->current_zval)

#endif

#endif /*LUASANDBOX_TYPES_H*/

