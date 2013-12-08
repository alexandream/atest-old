#ifndef __ATEST__ATEST_H__
#define __ATEST__ATEST_H__

/* Type Definitions. */

typedef struct ATCase ATCase;
typedef struct ATResult ATResult;
typedef struct ATSuite ATSuite;

typedef void (*ATFunction)(ATResult* result);

struct ATCase {
	const char* name;
	ATFunction function;
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

ATSuite*
at_get_suite(const char* name);

ATCase*
at_new_case(const char* name, ATFunction func);

#endif
