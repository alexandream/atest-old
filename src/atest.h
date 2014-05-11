#ifndef __ATEST__ATEST_H__
#define __ATEST__ATEST_H__

#ifndef AT_SUITE_NAME
#define AT_SUITE_NAME Untitled
#endif

/* Type Definitions. */

typedef struct ATCase ATCase;
typedef struct ATFailure ATFailure;
typedef struct ATExecution ATExecution;
typedef struct ATResult ATResult;
typedef struct ATResultList ATResultList;
typedef struct ATSuite ATSuite;

typedef void (*ATFunction)();

struct ATCase {
	ATFunction function;
	const char* name;
};

struct ATExecution {
	int placeholder;
};

struct ATFailure {
	const char* file_name;
	int line_number;
	const char* message;
};

struct ATResult {
	int failure_capacity;
	int failure_count;
	ATFailure** failures;

	ATSuite* suite;
	ATCase* tcase;
};

struct ATResultList {
	int result_capacity;
	int result_count;
	ATResult** results;
};

struct ATSuite {
	int case_capacity;
	int case_count;
	ATCase** cases;

	const char* name;
};

/* Data definitions */

extern ATResult* _at_result;

/* Function Definitions. */

void
at_add_case(ATSuite* suite, ATCase* tcase);

char*
at_allocf(const char* format, ...);

void
at_append_result(ATResultList* result_list, ATResult* result);

int
at_check_with_msg(const char*, int, int, const char*);

int
at_count_cases(ATSuite* suite);

int
at_count_failures(ATResult* result);

int
at_count_results(ATResultList* result_list);

int
at_count_suites();

ATResult*
at_execute_case(ATSuite* suite, ATCase* tcase);


const char*
at_get_full_name(ATResult* result);ATCase*
at_get_nth_case(ATSuite* suite, int index);

ATFailure*
at_get_nth_failure(ATResult* result, int index);

ATResult*
at_get_nth_result(ATResultList* result_list, int index);

ATSuite*
at_get_nth_suite(int index);

ATSuite*
at_get_suite(const char* name);

ATCase*
at_new_case(const char* name, ATFunction func);

ATResultList*
at_new_result_list();


#define _at_check_msg(cond, msg) \
at_check_with_msg(__FILE__,__LINE__,cond,msg)


#define _at_check(cond) \
_at_check_msg(cond, #cond)


#define at_assert(cond) \
if (_at_check(cond)) return;

#define at_expect(cond) \
_at_check(cond)

#define at_assert_msg(cond, msg) \
if (_at_check_msg(cond, msg)) return;

#define at_expect_msg(cond, msg) \
at_check_msg(cond, msg)

#if (__STDC_VERSION__ >= 199901L || USE_VARIADIC_MACROS)
	#define _at_check_msgf(cond, ...) \
	at_check_with_msg(__FILE__,__LINE__,cond,at_allocf(__VA_ARGS__))

	#define at_assert_msgf(cond, ...) \
	if (_at_check_msgf(cond, __VA_ARGS__)) return;

	#define at_expect_msgf(cond, ...) \
	_at_check_msgf(cond, __VA_ARGS__)
#endif

void _print_state();
void _print_suite(ATSuite* suite);

#define AT_STEST(suite_name, case_name) \
static void _at_run_ ## case_name(ATResult* _at_result);\
__attribute__((constructor))\
static void _at_init_ ## case_name() {\
	ATSuite* s = at_get_suite(#suite_name);\
	at_add_case(s, at_new_case(#case_name, _at_run_ ## case_name));\
}\
static void _at_run_ ## case_name(ATResult* _at_result)

#define _AT_STEST(suite_name, case_name) AT_STEST(suite_name, case_name)
#define AT_TEST(case_name) _AT_STEST(AT_SUITE_NAME, case_name)

#endif
