
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

/**
 * Compare two integers of variable length.
 *
 * Return 0 if @a and @b are the same, 1 if @a is greater than @b, and -1
 * if @b is greater than @a.
 */
#define compare_numbers_return(a, b)	\
do {					\
	if ((a) < (b))			\
		return -1;		\
	return (a) != (b);		\
} while (0)				\

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
void print_err(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fprintf(stderr, "\n");
}

/**
 * Return 0 if @orig and @copy are the same, 1 otherwise.
 */
static int compare_traceeval_type(struct traceeval_type *orig,
		struct traceeval_type *copy)
{
	int o_name_null;
	int c_name_null;

	// same memory/null
	if (orig == copy)
		return 0;

	size_t i = 0;
	do {
		if (orig[i].type != copy[i].type)
			return 1;
		if (orig[i].type == TRACEEVAL_TYPE_NONE)
			return 0;
		if (orig[i].flags != copy[i].flags)
			return 1;
		if (orig[i].id != copy[i].id)
			return 1;
		if (orig[i].dyn_release != copy[i].dyn_release)
			return 1;
		if (orig[i].dyn_cmp != copy[i].dyn_cmp)
			return 1;

		// make sure both names are same type
		o_name_null = !orig[i].name;
		c_name_null = !copy[i].name;
		if (o_name_null != c_name_null)
			return 1;
		if (o_name_null)
			continue;
		if (strcmp(orig[i].name, copy[i].name) != 0)
			return 1;
	} while (orig[i++].type != TRACEEVAL_TYPE_NONE);

	return 0;
}

/**
 * Return 0 if @orig and @copy are the same, 1 if @orig is greater than @copy,
 * -1 for the other way around, and -2 on error.
 */
static int compare_traceeval_data(union traceeval_data *orig,
		const union traceeval_data *copy, struct traceeval_type *type)
{
	if (!orig || !copy)
		return 1;

	switch (type->type) {
	case TRACEEVAL_TYPE_NONE:
		/* There is no corresponding traceeval_data for TRACEEVAL_TYPE_NONE */
		return -2;

	case TRACEEVAL_TYPE_STRING:
		int i = strcmp(orig->string, copy->string);
		if (!i)
			return 0;
		if (i > 0)
			return 1;
		return -1;

	case TRACEEVAL_TYPE_NUMBER:
		compare_numbers_return(orig->number, copy->number);

	case TRACEEVAL_TYPE_NUMBER_64:
		compare_numbers_return(orig->number_64, copy->number_64);

	case TRACEEVAL_TYPE_NUMBER_32:
		compare_numbers_return(orig->number_32, copy->number_32);

	case TRACEEVAL_TYPE_NUMBER_16:
		compare_numbers_return(orig->number_16, copy->number_16);

	case TRACEEVAL_TYPE_NUMBER_8:
		compare_numbers_return(orig->number_8, copy->number_8);

	case TRACEEVAL_TYPE_DYNAMIC:
		return type->dyn_cmp(orig->dyn_data, copy->dyn_data, type);

	default:
		print_err("%d is out of range of enum traceeval_data_type", type->type);
		return -2;
	}
}

/**
 * Compare arrays fo union traceeval_data's with respect to @def.
 *
 * Return 0 if @orig and @copy are the same, 1 if not, and -1 on error.
 */
static int compare_traceeval_data_set(union traceeval_data *orig,
		const union traceeval_data *copy, struct traceeval_type *def)
{
	int i = 0;
	int check;

	// compare data arrays
	for_each_key(i, def) {
		if ((check = compare_traceeval_data(&orig[i], &copy[i], &def[i])))
			goto fail_compare_data_set;
	}

	return 0;
fail_compare_data_set:
	if (check == -2)
		return -1;
	return 1;
}

/**
 * Return 0 if @orig and @copy are the same, 1 if not, and -1 on error.
 */
static int compare_entries(struct entry *orig, struct entry *copy,
		struct traceeval *teval)
{
	int check;

	// compare keys
	check = compare_traceeval_data_set(orig->keys, copy->keys,
			teval->def_keys);
	if (check)
		return check;

	// compare values
	check = compare_traceeval_data_set(orig->vals, copy->vals,
			teval->def_vals);
	return check;
}

/**
 * Return 0 if struct hist_table of @orig and @copy are the same, 1 if not,
 * and -1 on error.
 */
static int compare_hist(struct traceeval *orig, struct traceeval *copy)
{
	struct hist_table *o_hist = orig->hist;
	struct hist_table *c_hist = copy->hist;
	int cnt = !(o_hist->nr_entries == c_hist->nr_entries);
	if (cnt)
		return 1;

	for (size_t i = 0; i < o_hist->nr_entries; i++) {
		// cmp each entry
		compare_entries(&o_hist->map[i], &c_hist->map[i], orig);
	}
	return 0;	
}

/**
 * Return 0 if @orig and @copy are the same, 1 if not, -1 if error.
 */
int traceeval_compare(struct traceeval *orig, struct traceeval *copy)
{
	if ((!orig) || (!copy))
		return -1;

	int keys = compare_traceeval_type(orig->def_keys, copy->def_keys);
	int vals = compare_traceeval_type(orig->def_vals, copy->def_vals);
	int hists = compare_hist(orig, copy);

	return (keys || vals || hists);
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

/**
 * Deallocate array of traceeval_type's, which must be terminated by
 * TRACEEVAL_TYPE_NONE.
 */
static void type_release(struct traceeval_type *defs)
{
	size_t i = 0;

	if (!defs)
		return;

	for_each_key(i, defs) {
		if (defs[i].name)
			free(defs[i].name);
	}

	free(defs);
}

/**
 * Deallocate any specified dynamic data in @data.
 */
static void clean_data(union traceeval_data *data, struct traceeval_type *def)
{
	size_t i = 0;

	if (!data || !def)
		return;

	for_each_key(i, def) {
		switch (def[i].type) {
		case TRACEEVAL_TYPE_STRING:
			if (data[i].string)
				free(data[i].string);
			break;
		case TRACEEVAL_TYPE_DYNAMIC:
			def[i].dyn_release(data[i].dyn_data, &def[i]);
			break;
		default:
			break;
		}
	}
}

/**
 * Deallocate all possible data stored within the entry.
 */
static void clean_entry(struct entry *entry, struct traceeval *teval)
{
	if (!entry)
		return;

	// deallocate dynamic traceeval_data
	clean_data(entry->keys, teval->def_keys);
	clean_data(entry->vals, teval->def_vals);
	free(entry->keys);
	free(entry->vals);
}

/**
 * Deallocate the hist_table allocated to a traceeval instance.
 */
static void hist_table_release(struct traceeval *teval)
{
	struct hist_table *hist = teval->hist;

	if (!hist)
		return;

	for (size_t i = 0; i < hist->nr_entries; i++) {
		clean_entry(&hist->map[i], teval);
	}
	free(hist->map);
	free(hist);
}

/**
 * Deallocate a traceeval instance.
 */
void traceeval_release(struct traceeval *teval)
{
	if (teval) {
		hist_table_release(teval);
		type_release(teval->def_keys);
		type_release(teval->def_vals);
		free(teval);
	}
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
