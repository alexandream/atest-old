#ifndef __ATEST__ATEST_H__
#define __ATEST__ATEST_H__

#ifndef AT_SUITE_NAME
#define AT_SUITE_NAME Untitled
#endif

/* Type Definitions. */
typedef struct ATCase ATCase;
typedef struct ATFailure ATFailure;
typedef struct ATPointerList ATPointerList;
typedef struct ATResult ATResult;
typedef struct ATSuite ATSuite;

typedef ATPointerList ATResultList;

typedef void (*ATFunction)(ATResult*);

struct ATPointerList {
	int capacity;
	int count;
	void** pointers;
};


struct ATCase {
	ATFunction function;
	const char* name;
	const char* file_name;
	int line_number;
};


struct ATFailure {
	const char* file_name;
	int line_number;
	const char* message;
};

struct ATResult {
	ATPointerList failures;
	ATSuite* suite;
	ATCase* tcase;
};

struct ATSuite {
	ATPointerList cases;
	ATPointerList constructors;
	ATPointerList setups;
	ATPointerList teardowns;
	const char* name;
};


/* Function Definitions. */

void
at_add_constructor(ATSuite* suite, void (*constructor)(), int line);

void
at_add_setup(ATSuite* suite, ATFunction setup, int line);

void
at_add_teardown(ATSuite* suite, ATFunction teardown, int line);

void
at_add_case(ATSuite* suite, ATCase* tcase);

char*
at_allocf(const char* format, ...);

void
at_append_result(ATResultList* result_list, ATResult* result);

int
at_check_with_msg(ATResult* result_name,
                  const char* file_name,
                  int line_number,
                  int condition,
                  const char* message);

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

ATResultList*
at_execute_suite(ATSuite* suite, ATResultList* out_results);

const char*
at_get_full_name(ATResult* result);

ATCase*
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

ATCase*
at_new_case_with_location(const char* name,
                          ATFunction func,
                          const char* file_name,
                          int line_number);

ATResultList*
at_new_result_list();

/* Simple macros for asserts and expects */
#define at_check_msg(result, cond, msg) \
at_check_with_msg(result, __FILE__,__LINE__,cond,msg)


#if (__STDC_VERSION__ >= 199901L || USE_VARIADIC_MACROS)
	#define at_check_msgf(result, cond, ...) \
	at_check_with_msg(result, __FILE__,__LINE__,cond,at_allocf(__VA_ARGS__))
#endif

/* Crazy macros for auto registering constructors, setups, teardowns and tests */
/* SF: Stringify -- Indirect stringification to use expansion. */
#define AT_SF2(x) #x
#define AT_SF(x) AT_SF2(x)
/* PT: Paste Tokens */
#define AT_PT2(x, y) x ## y
#define AT_PT(x, y) AT_PT2(x, y)
/* UNIQUE: Uses the current line number to create a likely unique identifier */
#define AT_UNIQUE(identifier) AT_PT(identifier, __LINE__)

#ifndef AT_COMPILER_CONSTRUCTOR_DIRECTIVE
#define AT_COMPILER_CONSTRUCTOR_DIRECTIVE __attribute__((constructor))
#endif

/* Define and register a constructor function to be run before the suite. */
#define AT_CONSTRUCTOR(suite_name) \
static void AT_UNIQUE(AT_PT(at_g_constructor_,suite_name)) ();\
AT_COMPILER_CONSTRUCTOR_DIRECTIVE \
static void AT_UNIQUE(AT_PT(at_g_init_constructor_,suite_name)) () {\
	ATSuite* s = at_get_suite(AT_SF(suite_name));\
	at_add_constructor(s, \
	                   AT_UNIQUE(AT_PT(at_g_constructor_,suite_name)),\
					   __LINE__);\
}\
static void AT_UNIQUE(AT_PT(at_g_constructor_,suite_name))()


/* Define and register a setup function to be run before each case. */
#define AT_SETUP(suite_name, result_name)\
static void AT_UNIQUE(AT_PT(at_g_setup_,suite_name))(ATResult* result_name);\
AT_COMPILER_CONSTRUCTOR_DIRECTIVE \
static void AT_UNIQUE(AT_PT(at_g_init_setup_,suite_name))() {\
	ATSuite* s = at_get_suite(AT_SF(suite_name));\
	at_add_setup(s, AT_UNIQUE(AT_PT(at_g_setup_,suite_name)), __LINE__);\
}\
static void AT_UNIQUE(AT_PT(at_g_setup_,suite_name))()


/* Define and register a teardown function to be run after each case. */
#define AT_TEARDOWN(suite_name, result_name)\
static void AT_UNIQUE(AT_PT(at_g_teardown_,suite_name))(ATResult* result_name);\
AT_COMPILER_CONSTRUCTOR_DIRECTIVE \
static void AT_UNIQUE(AT_PT(at_g_init_teardown_,suite_name))() {\
	ATSuite* s = at_get_suite(AT_SF(suite_name));\
	at_add_teardown(s, AT_UNIQUE(AT_PT(at_g_teardown_,suite_name)), __LINE__);\
}\
static void AT_UNIQUE(AT_PT(at_g_teardown_,suite_name))()


/* Define and register a test case function. */
#define AT_TEST(suite_name, case_name, result_name) \
static void at_g_run_ ## case_name(ATResult* result_name);\
AT_COMPILER_CONSTRUCTOR_DIRECTIVE \
static void at_g_init_ ## case_name() {\
	ATSuite* s = at_get_suite(AT_SF(suite_name));\
	at_add_case(s, at_new_case_with_location(#case_name, at_g_run_ ## case_name, __FILE__, __LINE__));\
}\
static void at_g_run_ ## case_name(ATResult* result_name)


#endif
