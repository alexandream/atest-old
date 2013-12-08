#ifndef __ATEST__ATEST_H__
#define __ATEST__ATEST_H__

/* Type Definitions. */

typedef struct ATCase ATCase;
typedef struct ATFailure ATFailure;
typedef struct ATResult ATResult;
typedef struct ATSuite ATSuite;

typedef void (*ATFunction)(ATResult* result);

struct ATCase {
	const char* name;
	ATFunction function;
};

struct ATFailure {
	const char* file_name;
	int line_number;
	const char* message;
};

struct ATResult {
	ATSuite* suite;
	ATCase* tcase;
	ATFailure* failures;
	unsigned failure_count;
};

struct ATSuite {
	const char* name;
	unsigned case_count;
	unsigned case_capacity;
	ATCase** cases;
};


/* Function Definitions. */

void
at_add_case(ATSuite* suite, ATCase* tcase);

char*
at_allocf(const char* format, ...);

int
at_check_with_msg(const char*, int, ATResult*, int, const char*);

unsigned
at_count_suites();

ATSuite*
at_get_nth_suite(unsigned index);

ATSuite*
at_get_suite(const char* name);

ATCase*
at_new_case(const char* name, ATFunction func);

ATResult*
at_run_suite(ATSuite* suite);


#define _at_check_msg(cond, ...) \
at_check_with_msg(__FILE__,__LINE__,_at_result,at_allocf(cond,__VA_ARGS__))

#define _at_check(cond) \
_at_check_msg(cond, #cond)

#define at_assert_msg(cond, ...) \
if (_at_check_msg(cond, __VA_ARGS__)) return 0;

#define at_assert(cond) \
if (_at_check(cond)) return 0;

#define at_expect_msg(cond, ...) \
_at_check_msg(cond, __VA_ARGS__)

#define at_expect(cond) \
_at_check(cond)

void _print_state();
void _print_suite(ATSuite* suite);

#endif
