
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
	const struct traceeval_type	*def_keys;
	const struct traceeval_type	*def_vals;
	struct hist_table		*hist;
};

/** Iterate over results of histogram */
struct traceeval_iterator {};  // TODO

/**
 * Return 0 if @orig and @copy are the same, 1 otherwise.
 */
int compare_traceeval_type(const struct traceeval_type *orig,
		const struct traceeval_type *copy)
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
		if (orig[i].flags != copy[i].flags)
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
		union traceeval_data *copy, const struct traceeval_type *type)
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
		if (orig->number == copy->number)
			return 0;
		if (orig->number > copy->number)
			return 1;
		return -1;

	case TRACEEVAL_TYPE_NUMBER_64:
		if (orig->number_64 == copy->number_64)
			return 0;
		if (orig->number_64 > copy->number_64)
			return 1;
		return -1;

	case TRACEEVAL_TYPE_NUMBER_32:
		if (orig->number_32 == copy->number_32)
			return 0;
		if (orig->number_32 > copy->number_32)
			return 1;
		return -1;

	case TRACEEVAL_TYPE_NUMBER_16:
		if (orig->number_16 == copy->number_16)
			return 0;
		if (orig->number_16 > copy->number_16)
			return 1;
		return -1;

	case TRACEEVAL_TYPE_NUMBER_8:
		if (orig->number_8 == copy->number_8)
			return 0;
		if (orig->number_8 > copy->number_8)
			return 1;
		return -1;

	case TRACEEVAL_TYPE_DYNAMIC:
		return type->dyn_cmp(orig, copy);

	default:
		fprintf(stderr, "%d is out of range of enum traceeval_data_type\n", type->type);
		return -2;
	}
}

/**
 * Return 0 if @orig and @copy are the same, 1 if not, and -1 on error.
 */
static int compare_entries(struct entry *orig, struct entry *copy,
		struct traceeval *eval)
{
	const struct traceeval_type *type;
	int i = 0;
	int check;

	// compare keys
	do {
		type = &eval->def_keys[i];
		if ((check = compare_traceeval_data(&orig->keys[i], &copy->keys[i], type)))
			goto entries_not_equal;
		i++;
	} while (type->type != TRACEEVAL_TYPE_NONE);

	// compare values
	i = 0;
	do {
		type = &eval->def_vals[i];
		if ((check = compare_traceeval_data(&orig->vals[i], &copy->vals[i], type)))
			goto entries_not_equal;
		i++;
	} while (type->type != TRACEEVAL_TYPE_NONE);

	return 0;
entries_not_equal:
	if (check == -2)
		return -1;
	return 1;
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
 * Return 0 if @orig and @copy are the same, 1 otherwise.
 */
int traceeval_compare(struct traceeval *orig, struct traceeval *copy)
{
	int keys = compare_traceeval_type(orig->def_keys, copy->def_keys);
	int vals = compare_traceeval_type(orig->def_vals, copy->def_vals);
	int hists = compare_hist(orig, copy);

	return (keys || vals || hists);
}

/**
 * Clone traceeval_type array @defs to the heap. Must be terminated with
 * an instance of type TRACEEVAL_TYPE_NONE.
 * Returns NULL if @defs is NULL, or a name is not null terminated.
 */
static const struct traceeval_type *type_alloc(const struct traceeval_type *defs)
{
	if (defs == NULL)
		return NULL;

	struct traceeval_type *new_defs = NULL;
	char *name;
	size_t size = 0;

	do {
		// Resize heap defs and clone
		new_defs = realloc(new_defs,
				(size + 1) * sizeof(struct traceeval_type));
		if (!new_defs)
			goto fail_type_alloc;

		// copy null terminated name to heap if it's not NULL
		new_defs[size].name = NULL;
		name = NULL;
		if (defs[size].name) {
			name = strdup(defs[size].name);
			if (!name)
				goto fail_type_alloc_name;
			new_defs[size].name = name;
		}

		new_defs[size].type = defs[size].type;
		new_defs[size].flags = defs[size].flags;
		new_defs[size].dyn_release = defs[size].dyn_release;
		new_defs[size].dyn_cmp = defs[size].dyn_cmp;

	} while (defs[size++].type != TRACEEVAL_TYPE_NONE);

	return new_defs;
fail_type_alloc:
	if (defs[size].name)
		fprintf(stderr, "failed to allocate memory for traceeval_type %s\n", defs[size].name);
	fprintf(stderr, "failed to allocate memory for traceeval_type index %zu\n",
			size);

	return NULL;

fail_type_alloc_name:
	if (defs[size].name)
		fprintf(stderr, "failed to allocate name for traceeval_type %s\n", defs[size].name);

	fprintf(stderr, "failed to allocate name for traceeval_type index %zu\n",
			size);
	for (int i = 0; i < size; i++) {
		if (new_defs[i].name)
			free(new_defs[i].name);
	}
	free(new_defs);
	return NULL;
}

/**
 * Create a new histogram over the given keys and values.
 */
struct traceeval *traceeval_init(const struct traceeval_type *keys,
				 const struct traceeval_type *vals)
{
	struct traceeval *eval;
	char *err_msg;

	eval = calloc(1, sizeof(struct traceeval));
	if (!eval)
		goto fail_eval_init_alloc;

	eval->def_keys = type_alloc(keys);
	if (!eval->def_keys) {
		err_msg = "failed to allocate user defined keys";
		goto fail_eval_init;
	}

	eval->def_vals = type_alloc(vals);
	if (!eval->def_vals) {
		err_msg = "failed to allocate user defined values";
		goto fail_eval_init;
	}

	eval->hist = calloc(1, sizeof(struct hist_table));
	if (!eval->hist) {
		err_msg = "failed to allocate memory for histogram";
		goto fail_eval_init;
	}
	eval->hist->nr_entries = 0;

	return eval;
fail_eval_init_alloc:
	fprintf(stderr, "failed to allocate memory for traceeval instance\n");
	return NULL;

fail_eval_init:
	fprintf(stderr, "%s\n", err_msg);
	traceeval_release(eval);
	return NULL;
}

/**
 * Deallocate array of traceeval_type's, which must be terminated by
 * TRACEEVAL_TYPE_NONE.
 *
 * Returns 0 on success, -1 if @defs is NULL.
 */
static int type_release(struct traceeval_type *defs)
{
	enum traceeval_data_type type;
	size_t i = 0;

	if (!defs)
		return -1;

	do {
		type = defs[i].type;
		if (defs[i].name) {
			free(defs[i].name);
			defs[i].name = NULL;
		}
		i++;
	} while (type != TRACEEVAL_TYPE_NONE);

	free(defs);
	return 0;
}

/**
 * Deallocate any specified dynamic data in @data.
 *
 * Returns 0 on success, -1 on error.
 */
static int clean_data(union traceeval_data *data, const struct traceeval_type *def)
{
	int result = 0;
	size_t i = -1;

	if (!data || !def)
		goto fail_clean_data;
	
	while (def[++i].type != TRACEEVAL_TYPE_NONE) {
		switch (def[i].type) {
		case TRACEEVAL_TYPE_STRING:
			if (data[i].string)
				free(data[i].string);
			break;
		case TRACEEVAL_TYPE_DYNAMIC:
			if (result |= def[i].dyn_release(&data[i])) {
				fprintf(stderr, "dyn_release function returned non-zero value for traceeval_data %s\n",
						def[i].name);
			}
			break;
		default:
			break;
		}
	}

	if (result)
		return -1;
	return 0;
fail_clean_data:
	fprintf(stderr, "null pointer received for traceeval_data or traceeval_type\n");
	return -1;
}

/**
 * Deallocate all possible data stored within the entry.
 *
 * Returns 0 on success, -1 on error.
 */
static int clean_entry(struct entry *entry, struct traceeval *eval)
{
	int keys = 0;
	int vals = 0;

	if (!entry)
		return -1;

	// deallocate dynamic traceeval_data
	if ((keys = clean_data(entry->keys, eval->def_keys)))
		fprintf(stderr, "unable to deallocate data within an entries keys\n");
	if ((vals = clean_data(entry->vals, eval->def_vals)))
		fprintf(stderr, "unable to deallocate data within an entries vals\n");
	free(entry->keys);
	free(entry->vals);

	if (keys || vals)
		return -1;
	return 0;
}

/**
 * Deallocate the hist_table allocated to a traceeval instance.
 *
 * Returns 0 on success, -1 on error.
 */
static int hist_table_release(struct traceeval *eval)
{
	int result = 0;
	struct hist_table *hist = eval->hist;

	if (!hist)
		return -1;

	for (size_t i = 0; i < hist->nr_entries; i++) {
		result |= clean_entry(&hist->map[i], eval);
	}
	free(hist->map);
	free(hist);

	if (result)
		return -1;
	return 0;
}

/**
 * Deallocate a traceeval instance.
 *
 * Returns 0 on success, -1 on error.
 */
int traceeval_release(struct traceeval *eval)
{
	int result = 0;

	if (!eval)
		return -1;

	result |= hist_table_release(eval);
	result |= type_release(eval->def_keys);
	result |= type_release(eval->def_vals);
	free(eval);

	if (result)
		return -1;
	return 0;
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
