#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "atest.h"
typedef struct ATAuxiliary ATAuxiliary;
typedef struct ATConstructor ATConstructor;

struct ATAuxiliary {
	ATFunction function;
	int index;
};
struct ATConstructor {
	void (*function)();
	int index;
};


typedef int (*comparator_t)(const void*, const void*);

/* Internal state. */
static ATPointerList _suites = { 0, 0, NULL };


/* Auxiliary functions declaration. */
static void
append(ATPointerList* list, void* elem);

static int
compare_auxiliary_placement(const void* a, const void* b);

static int
compare_case_placement(const void* a, const void* b);

static int
compare_constructor_placement(const void* a, const void* b);

static int
compare_suites(const void* a, const void* b);

static int
compare_suite_to_name(const void* a, const void* b);

static ATFailure*
create_failure(const char* file_name, int line, const char* message);

static ATResult*
create_result(ATSuite* suite, ATCase* tcase);

static ATSuite*
create_suite(const char* name);

static void
_die(const char* fmt, ...);

static int
_discover_formatted_length(const char* format, va_list ap);

static char*
_duplicate_string(const char* input);

static void*
find(ATPointerList* list, comparator_t compare, const void* argument);

static ATSuite*
find_suite(const char* name);

static ATSuite*
get_new_suite(const char* name);

static void
grow_list(ATPointerList* list);

static void
init_list(ATPointerList* list);

static void
insert_in_order(ATPointerList* list, void* elem, comparator_t compare);

/* Interface functions definition. */
void
at_add_case(ATSuite* suite, ATCase* tcase) {
	insert_in_order(&suite->cases, tcase, compare_case_placement);
}


void
at_add_constructor(ATSuite* suite, void(*function)(), int index) {
	ATConstructor* constructor = (ATConstructor*) malloc(sizeof(ATConstructor));
	constructor->function = function;
	constructor->index = index;

	insert_in_order(&suite->constructors,
	                constructor,
	                compare_constructor_placement);
}


void
at_add_setup(ATSuite* suite, ATFunction function, int index) {
	ATAuxiliary* setup = (ATAuxiliary*) malloc(sizeof(ATAuxiliary));
	setup->function = function;
	setup->index = index;

	insert_in_order(&suite->setups, setup, compare_auxiliary_placement);
}


void
at_add_teardown(ATSuite* suite, ATFunction function, int index) {
	ATAuxiliary* teardown = (ATAuxiliary*) malloc(sizeof(ATAuxiliary));
	teardown->function = function;
	teardown->index = index;

	insert_in_order(&suite->teardowns, teardown, compare_auxiliary_placement);
}

char*
at_allocf(const char* format, ...) {
	int total_length;
	va_list discovery_args;
	char* buffer = NULL;

	va_start(discovery_args, format);
	total_length = 1 + _discover_formatted_length(format, discovery_args);
	va_end(discovery_args);
	if (total_length <= 0) {
		_die("Unable to figure out size of formatted string.");
	}
	if (total_length > 0) {
		va_list args;
		buffer = malloc(sizeof(char) * total_length+1);
		if (buffer == NULL) {
			_die("Unable to allocate string of size %d.", total_length+1);
		}
		va_start(args, format);
		vsprintf(buffer, format, args);
		va_end(args);
	}
	return buffer;
}


void
at_append_result(ATResultList* result_list, ATResult* result) {
	append(result_list, result);
}


int
at_check_with_msg(ATResult* at_result,
                  const char* file_name,
                  int line_number,
                  int condition,
                  const char* message) {
	if (!condition) {
		ATFailure* failure = create_failure(file_name, line_number, message);
		append(&at_result->failures, failure);
	}
	return !condition;
}


int
at_count_cases(ATSuite* suite) {
	return suite->cases.count;
}


int
at_count_failures(ATResult* result) {
	return result->failures.count;
}


int
at_count_results(ATResultList* result_list) {
	return result_list->count;
}


int
at_count_suites() {
	return _suites.count;
}


ATResult*
at_execute_case(ATSuite* suite, ATCase* tcase) {
	ATResult* at_result = create_result(suite, tcase);
	int i;
	/* Execute all the setups in order before running the test.*/
	for (i = 0; i < suite->setups.count; i++) {
		ATAuxiliary* setup = (ATAuxiliary*) suite->setups.pointers[i];
		setup->function(at_result);
	}
	tcase->function(at_result);

	/* Execute all the teardowns in order before finishing the test. */
	for (i = 0; i < suite->teardowns.count; i++) {
		ATAuxiliary* teardown = (ATAuxiliary*) suite->teardowns.pointers[i];
		teardown->function(at_result);
	}
	return at_result;
}


ATResultList*
at_execute_suite(ATSuite* suite, ATResultList* out_results) {
	int i, case_count;
	ATResultList* results = (out_results != NULL) ? out_results :
	                                                at_new_result_list();
	ATPointerList* constructors = &suite->constructors;
	/* Execute all the constructors in order before starting the suite */
	for (i = 0; i < constructors->count; i++) {
		ATConstructor* constructor = (ATConstructor*) constructors->pointers[i];
		constructor->function();
	}

	case_count = at_count_cases(suite);
	for (i = 0; i < case_count; i++) {
		ATCase* tcase = at_get_nth_case(suite, i);
		ATResult* result = at_execute_case(suite, tcase);
		at_append_result(results, result);
	}
	return results;
}

const char*
at_get_full_name(ATResult* result) {
	return at_allocf("%s.%s", result->suite->name, result->tcase->name);
}


ATCase*
at_get_nth_case(ATSuite* suite, int index) {
	return suite->cases.pointers[index];
}


ATFailure*
at_get_nth_failure(ATResult* result, int index) {
	return result->failures.pointers[index];
}


ATResult*
at_get_nth_result(ATResultList* result_list, int index) {
	return result_list->pointers[index];
}


ATSuite*
at_get_nth_suite(int index) {
	return _suites.pointers[index];
}


ATSuite*
at_get_suite(const char* name) {
	ATSuite* result = find_suite(name);

	if (result == NULL) {
		result = get_new_suite(name);
	}

	return result;
}


ATCase*
at_new_case(const char* name, ATFunction func) {
	return at_new_case_with_location(name, func, "", 0);
}


ATCase*
at_new_case_with_location(const char* name,
                          ATFunction func,
                          const char* file_name,
                          int line_number) {
	ATCase* tcase = malloc(sizeof(ATCase));
	if (tcase == NULL) {
		_die("Unable to allocate test case \"%s\".\n", name);
	}

	tcase->name = _duplicate_string(name);
	tcase->file_name = _duplicate_string(file_name);
	tcase->line_number = line_number;
	tcase->function = func;

	return tcase;

}


ATResultList*
at_new_result_list() {
	ATResultList* result_list = malloc(sizeof(ATResultList));
	if (result_list == NULL) {
		_die("Unable to allocate result list.");
	}

	init_list(result_list);
	return result_list;
}


/* Auxiliary functions definition. */
static void
append(ATPointerList* list, void* elem) {
	if (list->count == list->capacity) {
		grow_list(list);
	}
	list->pointers[list->count] = elem;
	list->count++;
}


static int
compare_auxiliary_placement(const void* a, const void* b) {
	const ATAuxiliary* auxiliary1 = (const ATAuxiliary*) a;
	const ATAuxiliary* auxiliary2 = (const ATAuxiliary*) b;
	return (auxiliary1->index > auxiliary2->index)   ?  1 :
	       (auxiliary1 -> index < auxiliary2->index) ? -1
	                                     :  0;
}


static int
compare_case_placement(const void* a, const void* b) {
	const ATCase* case1 = (const ATCase*) a;
	const ATCase* case2 = (const ATCase*) b;
	int file_comparison = strcmp(case1->file_name, case2->file_name);
	if (file_comparison == 0) {
		return (case1->line_number > case2->line_number) ?  1 :
		       (case1->line_number < case2->line_number) ? -1 :
		                                                    0;
	}
	return file_comparison;
}


static int
compare_constructor_placement(const void* a, const void* b) {
	const ATConstructor* constructor1 = (const ATConstructor*) a;
	const ATConstructor* constructor2 = (const ATConstructor*) b;
	return (constructor1->index > constructor2->index)   ?  1 :
	       (constructor1 -> index < constructor2->index) ? -1
	                                     :  0;
}


static int
compare_suites(const void* a, const void* b) {
	const ATSuite* suite1 = (const ATSuite*) a;
	const ATSuite* suite2 = (const ATSuite*) b;
	return strcmp(suite1->name, suite2->name);
}


static int
compare_suite_to_name(const void* a, const void* b) {
	const ATSuite* suite = (const ATSuite*) a;
	const char* name = (const char*) b;
	return strcmp(suite->name, name);
}


static ATFailure*
create_failure(const char* file_name, int line, const char* message) {
	ATFailure* failure = malloc(sizeof(ATFailure));
	if (failure == NULL) {
		_die("Unable to allocate failure on %s[%d]: %s\n",
		     file_name, line, message);
	}
	failure->file_name   = _duplicate_string(file_name);
	failure->line_number = line;
	failure->message     = message;
	return failure;
}


static ATResult*
create_result(ATSuite* suite, ATCase* tcase) {
	ATResult* result = malloc(sizeof(ATResult));
	if (result == NULL) {
		_die("Unable to allocate result for ",
		     "suite \"%s\", test case \"%s\".",
		     suite->name, tcase->name);
	}
	result->suite = suite;
	result->tcase = tcase;
	init_list(&result->failures);
	return result;
}


static ATSuite*
create_suite(const char* name) {
	ATSuite* suite = malloc(sizeof(ATSuite));
	if (suite == NULL) {
		_die("Unable to allocate test suite \"%s\".\n", name);
	}
	suite->name = _duplicate_string(name);
	init_list(&suite->cases);
	init_list(&suite->constructors);
	return suite;
}


static void
_die(const char* format, ...) {
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	abort();
}


static int
_discover_formatted_length(const char* format, va_list args) {
	FILE* fd = fopen("/dev/null", "w");
	int result = -1;
	if (fd != NULL) {
		result = vfprintf(fd, format, args);
		fclose(fd);
	}
	return result;
}


static char*
_duplicate_string(const char* input) {
	size_t result_len = strlen(input) + 1;
	char* result = malloc(sizeof(char) * result_len);
	if (result == NULL) {
		_die("Unable to duplicate string %s, likely ran out of memory.\n",
		     input);
	}
	return memcpy(result, input, result_len);
}


static void*
find(ATPointerList* list, comparator_t compare, const void* argument) {
	int i;

	for (i = 0; i < list->count; i++) {
		if (compare(_suites.pointers[i], argument) == 0) {
			return _suites.pointers[i];
		}
	}
	return NULL;
}


static ATSuite*
find_suite(const char* name) {
	return find(&_suites, compare_suite_to_name, name);
}


static ATSuite*
get_new_suite(const char* name) {
	ATSuite* suite = create_suite(name);

	insert_in_order(&_suites, suite, compare_suites);
	return suite;
}


static void
grow_list(ATPointerList* list) {
	int capacity = (list->capacity == 0) ? 64 : list->capacity * 2;
	list->pointers = realloc(list->pointers, capacity * sizeof(void*));
	if (list->pointers == NULL) {
		_die("Unable to allocate memory while growing pointer "
		"list capacity from %d to %d.\n",
		list->capacity, capacity);
	}
	list->capacity = capacity;
}


static void
init_list(ATPointerList* list) {
	list->capacity = 0;
	list->count = 0;
	list->pointers = NULL;
}


static void
insert_in_order(ATPointerList* list, void* elem, comparator_t compare) {
	int i, j;
	/* If the list needs growing, do it here. */
	if (list->count == list->capacity) {
		grow_list(list);
	}

	/* Find the spot this element should be inserted into (final value of i) */
	for (i = 0; i < list->count; i++) {
		if (compare(list->pointers[i], elem) > 0) {
			break;
		}
	}

	/* Move every elem from i to the end one spot to the right */
	for (j = list->count -1; j >= i; j--) {
		list->pointers[j+1] = list->pointers[j];
	}

	list->pointers[i] = elem;
	list->count++;
}
