#ifndef __ATEST__ATEST_H__
#define __ATEST__ATEST_H__

/* Type Definitions. */

typedef struct ATCase ATCase;
typedef struct ATFailure ATFailure;
typedef struct ATExecution ATExecution;
typedef struct ATResult ATResult;
typedef struct ATSuite ATSuite;

typedef void (*ATFunction)(ATResult* result);

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

struct ATSuite {
	int case_capacity;
	int case_count;
	ATCase** cases;

	const char* name;
};


/* Function Definitions. */

void
at_add_case(ATSuite* suite, ATCase* tcase);

char*
at_allocf(const char* format, ...);

int
at_check_with_msg(const char*, int, ATResult*, int, const char*);

int
at_count_cases(ATSuite* suite);

int
at_count_failures(ATResult* result);

int
at_count_suites();

ATResult*
at_execute_case(ATSuite* suite, ATCase* tcase);

ATCase*
at_get_nth_case(ATSuite* suite, int index);

ATFailure*
at_get_nth_failure(ATResult* result, int index);

ATSuite*
at_get_nth_suite(int index);

ATSuite*
at_get_suite(const char* name);

ATCase*
at_new_case(const char* name, ATFunction func);

ATResult*
at_run_suite(ATSuite* suite);


#define _at_check_msg(cond, ...) \
at_check_with_msg(__FILE__,__LINE__,_at_result,cond,at_allocf(__VA_ARGS__))

#define _at_check(cond) \
_at_check_msg(cond, #cond)

#define at_assert_msg(cond, ...) \
if (_at_check_msg(cond, __VA_ARGS__)) return;

#define at_assert(cond) \
if (_at_check(cond)) return;

#define at_expect_msg(cond, ...) \
_at_check_msg(cond, __VA_ARGS__)

#define at_expect(cond) \
_at_check(cond)

void _print_state();
void _print_suite(ATSuite* suite);

#endif
