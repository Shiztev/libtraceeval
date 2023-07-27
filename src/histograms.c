
/* SPDX-License-Identifier: MIT */
/*
 * libtraceeval histogram interface implementation.
 *
 * Copyright (C) 2023 Google Inc, Stevie Alvarez <stevie.6strings@gmail.com>
 */

#include <histograms.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>

/**
 * Iterate over @keys, which should be an array of struct traceeval_type's,
 * until reaching an instance of type TRACEEVAL_TYPE_NONE.
 * @i should be a declared integer type.
 */
#define for_each_key(i, keys)	\
	for (i = 0; (keys)[(i)].type != TRACEEVAL_TYPE_NONE; (i)++)

/** A key-value pair */
struct entry {
	union traceeval_data	*keys;
	union traceeval_data	*vals;
};

/** A table of key-value entries */
struct hist_table {
	struct entry	*map;
	size_t		nr_entries;
};

/** Histogram */
struct traceeval {
	struct traceeval_type		*def_keys;
	struct traceeval_type		*def_vals;
	struct hist_table		*hist;
};

/** Iterate over results of histogram */
struct traceeval_iterator {};  // TODO

/**
 * Print error message.
 * Additional arguments are used with respect to fmt.
 */
static void print_err(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fprintf(stderr, "\n");
}

// TODO
int traceeval_compare(struct traceeval *orig, struct traceeval *copy)
{
	return -1;
}

/**
 * Resize a struct traceeval_type array to a size of @size + 1.
 *
 * Returns a pointer to the resized array, or NULL if the provided pointer was
 * freed to due lack of memory.
 */
static struct traceeval_type *type_realloc(struct traceeval_type *defs,
		size_t size)
{
	struct traceeval_type *tmp_defs = NULL;
	tmp_defs = realloc(defs,
			(size + 1) * sizeof(struct traceeval_type));
	if (!tmp_defs)
		goto fail_type_realloc;
	return tmp_defs;

fail_type_realloc:
	for (int i = 0; i < size; i++) {
		if (defs[i].name)
			free(defs[i].name);
	}
	free(defs);
	return NULL;
}

/**
 * Clone traceeval_type array @defs to the heap. Must be terminated with
 * an instance of type TRACEEVAL_TYPE_NONE.
 * Returns NULL if @defs is NULL, or a name is not null terminated.
 */
static struct traceeval_type *type_alloc(const struct traceeval_type *defs)
{
	struct traceeval_type *new_defs = NULL;
	char *name;
	size_t size = 0;

	// Empty def is represented with single TRACEEVAL_TYPE_NONE
	if (defs == NULL) {
		if (!(new_defs = calloc(1, sizeof(struct traceeval_type))))
			goto fail_type_alloc;
		new_defs->type = TRACEEVAL_TYPE_NONE;
		return new_defs;
	}

	for_each_key(size, defs) {
		// Resize heap defs and clone
		new_defs = type_realloc(new_defs, size);
		if (!new_defs)
			goto fail_type_alloc;

		// copy current def data to new_def
		new_defs[size] = defs[size];
		new_defs[size].name = NULL;
		// copy name to heap if it's not NULL or type NONE
		if (defs[size].type != TRACEEVAL_TYPE_NONE) {
			name = NULL;
			if (!defs[size].name)
				goto fail_type_alloc_name;

			name = strdup(defs[size].name);
			if (!name)
				goto fail_type_alloc_name;
			new_defs[size].name = name;
		}
	}

	// append array terminator
	new_defs = type_realloc(new_defs, size);
	if (!new_defs)
		goto fail_type_alloc;
	new_defs[size].type = TRACEEVAL_TYPE_NONE;

	return new_defs;
fail_type_alloc:
	if (defs[size].name)
		print_err("failed to allocate memory for traceeval_type %s", defs[size].name);
	print_err("failed to allocate memory for traceeval_type index %zu", size);
	return NULL;

fail_type_alloc_name:
	if (defs[size].name)
		print_err("failed to allocate name for traceeval_type %s", defs[size].name);

	print_err("failed to allocate name for traceeval_type index %zu", size);
	return NULL;
}

/**
 * Create a new histogram over the given keys and values.
 */
struct traceeval *traceeval_init(const struct traceeval_type *keys,
				 const struct traceeval_type *vals)
{
	struct traceeval *teval;
	char *err_msg;
	struct traceeval_type type = {
		.type = TRACEEVAL_TYPE_NONE
	};

	if (!keys)
		return NULL;

	if (keys->type == TRACEEVAL_TYPE_NONE) {
		err_msg = "keys cannot start with type TRACEEVAL_TYPE_NONE";
		goto fail_eval_init_unalloced;
	}

	teval = calloc(1, sizeof(struct traceeval));
	if (!teval) {
		err_msg = "failed to allocate memory for traceeval instance";
		goto fail_eval_init_unalloced;
	}

	teval->def_keys = type_alloc(keys);
	if (!teval->def_keys) {
		err_msg = "failed to allocate user defined keys";
		goto fail_eval_init;
	}

	// if vals is NULL, alloc single type NONE
	if (vals)
		teval->def_vals = type_alloc(vals);
	else
		teval->def_vals = type_alloc(&type);

	if (!teval->def_vals) {
		err_msg = "failed to allocate user defined values";
		goto fail_eval_init;
	}

	teval->hist = calloc(1, sizeof(struct hist_table));
	if (!teval->hist) {
		err_msg = "failed to allocate memory for histogram";
		goto fail_eval_init;
	}
	teval->hist->nr_entries = 0;

	return teval;
fail_eval_init:
	traceeval_release(teval);
	/* fall-through */

fail_eval_init_unalloced:
	print_err(err_msg);
	return NULL;
}

// TODO
void traceeval_release(struct traceeval *teval)
{

}

// TODO
int traceeval_insert(struct traceeval *teval,
		     const union traceeval_data *keys,
		     const union traceeval_data *vals)
{
	return -1;
}

// TODO
int traceeval_query(struct traceeval *teval,
		    const union traceeval_data *keys,
		    union traceeval_data * const *results)
{
	return 0;
}

// TODO
int traceeval_find_key(struct traceeval *teval, const char *field)
{
	return -1;
}

// TODO
int traceeval_find_val(struct traceeval *teval, const char *field)
{
	return -1;
}

// TODO
void traceeval_results_release(struct traceeval *teval,
			       const union traceeval_data *results)
{

}

// TODO
struct traceeval_iterator *traceeval_iterator_get(struct traceeval *teval)
{
	return NULL;
}

// TODO
int traceeval_iterator_sort(struct traceeval_iterator *iter,
			    const char *sort_field, int level, bool ascending)
{
	return -1;
}

// TODO
int traceeval_iterator_next(struct traceeval_iterator *iter,
			    const union traceeval_data **keys)
{
	return 0;
}

// TODO
void traceeval_keys_release(struct traceeval_iterator *iter,
			    const union traceeval_data *keys)
{

}

// TODO
int traceeval_stat(struct traceeval *teval, const union traceeval_data *keys,
		   const char *field, struct traceeval_stat *stat)
{
	return -1;
}
