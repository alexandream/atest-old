#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "atest.h"

/* Internal state. */

static ATSuite** _suites    = NULL;
static int  _suite_capacity = 0;
static int  _suite_count    = 0;


/* Auxiliary functions declaration. */

static void
_append_failure(ATResult* result, ATFailure* failure);

static ATFailure*
_create_failure(const char* file_name, int line, const char* message);

static ATResult*
_create_result(ATSuite* suite, ATCase* tcase);

static ATSuite*
_create_suite(const char* name);

static void
_die(const char* fmt, ...);

static int
_discover_formatted_length(const char* format, va_list ap);

static char*
_duplicate_string(const char* input);

static ATSuite*
_find_suite(const char* name);

static ATSuite*
_get_new_suite(const char* name);

static void
_grow_case_pool(ATSuite* suite);

static void
_grow_failure_pool(ATResult* result);

static void
_grow_result_pool(ATResultList* result);

static void
_grow_suite_pool();

static void
_insert_case_in_order(ATSuite* suite, ATCase* tcase);

static void
_insert_suite_in_order(ATSuite* suite);


/* Interface functions definition. */

void
at_add_case(ATSuite* suite, ATCase* tcase) {
	if (suite->case_count == suite->case_capacity) {
		_grow_case_pool(suite);
	}
	_insert_case_in_order(suite, tcase);
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
at_check_with_msg(const char* file_name,
                  int line_number,
                  ATResult* result,
                  int condition,
                  const char* message) {
	if (!condition) {
		ATFailure* failure = _create_failure(file_name, line_number, message);
		_append_failure(result, failure);
	}
	return condition;
}


int
at_count_cases(ATSuite* suite) {
	return suite->case_count;
}


int
at_count_failures(ATResult* result) {
	return result->failure_count;
}


int
at_count_results(ATResultList* result_list) {
	return result_list->result_count;
}


int
at_count_suites() {
	return _suite_count;
}


ATResult*
at_execute_case(ATSuite* suite, ATCase* tcase) {
	ATResult* result = _create_result(suite, tcase);
	tcase->function(result);
	return result;
}

const char*
at_get_full_name(ATResult* result) {
	return at_allocf("%s.%s", result->suite->name, result->tcase->name);
}

ATCase*
at_get_nth_case(ATSuite* suite, int index) {
	return suite->cases[index];
}


ATFailure*
at_get_nth_failure(ATResult* result, int index) {
	return result->failures[index];
}


ATResult*
at_get_nth_result(ATResultList* result_list, int index) {
	return result_list->results[index];
}


ATSuite*
at_get_nth_suite(int index) {
	return _suites[index];
}


ATSuite*
at_get_suite(const char* name) {
	ATSuite* result = _find_suite(name);

	if (result == NULL) {
		result = _get_new_suite(name);
	}

	return result;
}


ATCase*
at_new_case(const char* name, ATFunction func) {
	ATCase* tcase = malloc(sizeof(ATCase));
	if (tcase == NULL) {
		_die("Unable to allocate test case \"%s\".\n", name);
	}

	tcase->name = _duplicate_string(name);
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
	if (result->failure_count == result->failure_capacity) {
		_grow_failure_pool(result);
	}
	result->failures[result->failure_count] = failure;
	result->failure_count++;
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
	result->failure_capacity = 0;
	result->failure_count = 0;
	result->failures = NULL;
	result->suite = suite;
	result->tcase = tcase;
	return result;
}


static ATSuite*
_create_suite(const char* name) {
	ATSuite* suite = malloc(sizeof(ATSuite));
	if (suite == NULL) {
		_die("Unable to allocate test suite \"%s\".\n", name);
	}
	suite->name = _duplicate_string(name);
	suite->case_count = 0;
	suite->case_capacity = 0;
	suite->cases = NULL;
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
_find_suite(const char* name) {
	int i;

	for (i = 0; i < _suite_count; i++) {
		if (strcmp(_suites[i]->name, name) == 0) {
			return _suites[i];
		}
	}
	return NULL;
}


static ATSuite*
_get_new_suite(const char* name) {
	ATSuite* suite = _create_suite(name);

	if (_suite_count == _suite_capacity) {
		_grow_suite_pool();
	}

	_insert_suite_in_order(suite);
	return suite;
}


static void
_grow_suite_pool() {
	_suite_capacity = (_suite_capacity == 0) ? 64 : _suite_capacity * 2;
	_suites = realloc(_suites, _suite_capacity * sizeof(ATSuite*));
	if (_suites == NULL) {
		_die("Unable to allocate memory while growing "
		     "suite capacity from %d to %d.\n",
		     _suite_capacity, _suite_capacity*2);
	}
}


static void
_grow_case_pool(ATSuite* suite) {
	int new_capacity =
		(suite->case_capacity == 0) ? 64 : suite->case_capacity * 2;
	suite->cases = realloc(suite->cases, new_capacity * sizeof(ATCase*));
	if (suite->cases == NULL) {
		_die("Unable to allocate memory while growing "
		     "suite \"%s\"'s case capacity from %d to %d.\n",
		     suite->case_capacity, new_capacity);
	}
	suite->case_capacity = new_capacity;
}


static void
_grow_failure_pool(ATResult* result) {
	int new_capacity =
		(result->failure_capacity == 0) ? 8 : result->failure_capacity * 2;
	result->failures =
		realloc(result->failures, new_capacity * sizeof(ATFailure*));
	if (result->failures == NULL) {
		_die("Unable to allocate memory while growing "
		     "failure list of results for suite \"%s\", test \"%s\" "
		     "from %d to %d.\n",
		     result->suite->name, result->tcase->name,
		     result->failure_capacity, new_capacity);
	}
	result->failure_capacity = new_capacity;
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
_insert_case_in_order(ATSuite* suite, ATCase* tcase) {
	int i, j;
	ATCase* other;
	/* Find the spot this suite should be inserted into (final value of i) */
	for (i = 0; i < suite->case_count; i++) {
		other = suite->cases[i];
		if (strcmp(other->name, tcase->name) > 0) {
			break;
		}
	}
	/* Move every suite from i to the end one spot to the right */
	for (j = suite->case_count -1; j >= i; j--) {
		suite->cases[j+1] = suite->cases[j];
	}

	suite->cases[i] = tcase;
	suite->case_count++;
}


static void
_insert_suite_in_order(ATSuite* suite) {
	int i, j;
	ATSuite* other;
	/* Find the spot this suite should be inserted into (final value of i) */
	for (i = 0; i < _suite_count; i++) {
		other = _suites[i];
		if (strcmp(other->name, suite->name) > 0) {
			break;
		}
	}
	/* Move every suite from i to the end one spot to the right */
	for (j = _suite_count -1; j >= i; j--) {
		_suites[j+1] = _suites[j];
	}

	_suites[i] = suite;
	_suite_count++;
}


void
_print_state() {
	printf("_suites: %p\n", _suites);
	printf("_suite_capacity: %d\n", _suite_capacity);
	printf("_suite_count: %d\n", _suite_count);
	puts("----------");
}


void
_print_suite(ATSuite* suite) {
	puts("Begin Suite:");
	printf(" suite addr: %p\n", suite);
	if (suite != NULL) {
		printf(" suite->name: %s\n", suite->name);
		printf(" suite->case_count: %d\n", suite->case_count);
		printf(" suite->case_capacity: %d\n", suite->case_capacity);
		printf(" suite->cases: %p\n", suite->cases);
		if (suite->case_count > 0) {
			int i;
			ATCase** cases = suite->cases;
			puts(" Begin Cases: ");
			for (i = 0; i < suite->case_count; i++) {
				printf("  case[%d]: %s [%p].\n",
				       i, cases[i]->name, cases[i]->function);
			}
		}
	}
	puts("----------");
}
