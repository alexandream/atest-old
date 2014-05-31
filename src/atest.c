#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "atest.h"
/* Shared external state */

/* Internal state. */
typedef int (*comparator_t)(const void*, const void*);

static ATPointerList _suites = { 0, 0, NULL };


/* Auxiliary functions declaration. */

static void
_append_failure(ATResult* result, ATFailure* failure);

static ATFailure*
_create_failure(const char* file_name, int line, const char* message);

static ATResult*
_create_result(ATSuite* suite, ATCase* tcase);

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
grow_list(ATPointerList* list, int initial, int factor);

static void
_grow_result_pool(ATResultList* result);

static void
init_list(ATPointerList* list);

static void
insert_in_order(ATPointerList* list, void* elem, comparator_t compare);

static int
compare_case_placement(const void* a, const void* b);

static int
compare_suites(const void* a, const void* b);

static int
compare_suite_to_name(const void* a, const void* b);

/* Interface functions definition. */
void
at_add_case(ATSuite* suite, ATCase* tcase) {
	if (suite->cases.count == suite->cases.capacity) {
		grow_list(&suite->cases, 64, 2);
	}
	insert_in_order(&suite->cases, tcase, compare_case_placement);
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
	if (result_list->result_count == result_list->result_capacity) {
		_grow_result_pool(result_list);
	}
	result_list->results[result_list->result_count] = result;
	result_list->result_count++;
}


int
at_check_with_msg(ATResult* at_result,
                  const char* file_name,
                  int line_number,
                  int condition,
                  const char* message) {
	if (!condition) {
		ATFailure* failure = _create_failure(file_name, line_number, message);
		_append_failure(at_result, failure);
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
	return result_list->result_count;
}


int
at_count_suites() {
	return _suites.count;
}


ATResult*
at_execute_case(ATSuite* suite, ATCase* tcase) {
	ATResult* at_result = _create_result(suite, tcase);
	tcase->function(at_result);
	return at_result;
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
	return result_list->results[index];
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
	result_list->result_capacity = 0;
	result_list->result_count    = 0;
	result_list->results         = NULL;
	return result_list;
}

/* Auxiliary functions definition. */

static void
_append_failure(ATResult* result, ATFailure* failure) {
	if (result->failures.count == result->failures.capacity) {
		grow_list(&result->failures, 64, 2);
	}
	result->failures.pointers[result->failures.count] = failure;
	result->failures.count++;
}


static ATFailure*
_create_failure(const char* file_name, int line, const char* message) {
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
_create_result(ATSuite* suite, ATCase* tcase) {
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


static ATSuite*
find_suite(const char* name) {
	return find(&_suites, compare_suite_to_name, name);
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
get_new_suite(const char* name) {
	ATSuite* suite = create_suite(name);

	if (_suites.count == _suites.capacity) {
		grow_list(&_suites, 64, 2);
	}

	insert_in_order(&_suites, suite, compare_suites);
	return suite;
}

static void
grow_list(ATPointerList* list, int initial, int factor) {
	int capacity = (list->capacity == 0) ? initial : list->capacity * factor;
	list->pointers = realloc(list->pointers, capacity * sizeof(void*));
	if (list->pointers == NULL) {
		_die("Unable to allocate memory while growing pointer "
		"list capacity from %d to %d.\n",
		list->capacity, capacity);
	}
	list->capacity = capacity;
}


static void
_grow_result_pool(ATResultList* result_list) {
	int new_capacity =
		(result_list->result_capacity) ? result_list->result_capacity * 2
	                                   : 64;
	result_list->results =
		realloc(result_list->results, new_capacity * sizeof(ATResult*));
	if (result_list->results == NULL) {
		_die("Unable to allocate memory while growing result list "
		     "from %d to %d.\n",
		     result_list->result_capacity, new_capacity);
	}
	result_list->result_capacity = new_capacity;
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
