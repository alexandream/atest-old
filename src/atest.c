#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "atest.h"

/* Internal state. */

static ATSuite** _suites         = NULL;
static unsigned  _suite_capacity = 0;
static unsigned  _suite_count    = 0;


/* Auxiliary functions declaration. */

static void
_add_failure(ATResult* result,
             const char* file_name,
             int line_number,
             const char* message);

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


int
at_check_with_msg(const char* file_name,
                  int line_number,
                  ATResult* result,
                  int condition,
                  const char* message) {
	if (!condition) {
		_add_failure(result, file_name, line_number, message);
	}
	return condition;
}

unsigned
at_count_suites() {
	return _suite_count;
}


ATSuite*
at_get_nth_suite(unsigned index) {
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



/* Auxiliary functions definition. */

static void
_add_failure(ATResult* result,
             const char* file_name,
             int line_number,
             const char* message) {

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
	unsigned i;

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
		     "suite capacity from %u to %u.\n",
		     _suite_capacity, _suite_capacity*2);
	}
}


static void
_grow_case_pool(ATSuite* suite) {
	unsigned new_capacity =
		(suite->case_capacity == 0) ? 64 : suite->case_capacity * 2;
	suite->cases = realloc(suite->cases, new_capacity * sizeof(ATCase*));
	if (suite->cases == NULL) {
		_die("Unable_to_allocate memory while growing "
		     "suite \"%s\"'s case capacity from %u to %u.\n",
		     suite->case_capacity, new_capacity);
	}
	suite->case_capacity = new_capacity;
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
	printf("_suite_capacity: %u\n", _suite_capacity);
	printf("_suite_count: %u\n", _suite_count);
	puts("----------");
}


void
_print_suite(ATSuite* suite) {
	puts("Begin Suite:");
	printf(" suite addr: %p\n", suite);
	if (suite != NULL) {
		printf(" suite->name: %s\n", suite->name);
		printf(" suite->case_count: %u\n", suite->case_count);
		printf(" suite->case_capacity: %u\n", suite->case_capacity);
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
