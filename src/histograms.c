
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
struct traceeval {  // TODO
	const struct traceeval_type	*def_keys;
	const struct traceeval_type	*def_vals;
	struct hist_table		*hist;
};

/** Iterate over results of histogram */
struct traceeval_iterator {};  // TODO

/**
 * Clone traceeval_type array @defs to the heap. Must be terminated with
 * an instance of type TRACEEVAL_TYPE_NONE.
 * Returns NULL if @defs is NULL, or a name is not null terminated.
 */
const struct traceeval_type *type_alloc(const struct traceeval_type *defs)
{
	struct traceeval_type *new_defs = calloc (1,
				sizeof(struct traceeval_type));
	char *name;
	size_t len_name;
	size_t check;
	size_t size = 0;

	if (defs == NULL)
		return NULL;

	do {
		// Resize heap defs and clone
		new_defs = realloc(new_defs,
				(size + 1) * sizeof(struct traceeval_type));

		// copy null terminated name to heap
		len_name = strlen(defs[size].name) + 1;
		name = calloc(len_name, sizeof(char));
		check = strlcpy(name, defs[size].name, len_name);
		if (check != len_name)
			goto fail_type_name;

		new_defs[size].type = defs[size].type;
		new_defs[size].name = name;
		new_defs[size].flags = defs[size].flags;
		new_defs[size].dyn_release = defs[size].dyn_release;

		name = NULL;
		size++;
	} while (defs[size].type != TRACEEVAL_TYPE_NONE);

	return new_defs;
fail_type_name:
	for (int i = 0; i < size; i++) {
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

	eval = calloc(1, sizeof(struct traceeval));
	eval->def_keys = type_alloc(keys);
	eval->def_vals = type_alloc(vals);
	eval->hist = calloc(1, sizeof(struct hist_table));
	eval->hist->nr_entries = 0;

	if ((eval->def_keys == NULL) || (eval->def_vals == NULL))
		goto fail_eval_init;

	return eval;
fail_eval_init:
	traceeval_release(eval);
	return NULL;
}

/**
 * Deallocate array of traceeval_type's, which must be terminated by
 * TRACEEVAL_TYPE_NONE.
 *
 * Returns 0 on success, -1 on error.
 */
static int type_release(struct traceeval_type *defs)
{
	enum traceeval_data_type type;
	size_t i = 0;

	if (!defs)
		return -1;

	do {
		type = defs[i].type;
		free(defs[i].name);
	} while (type != TRACEEVAL_TYPE_NONE);

	free(defs);
	return 0;
}

/**
 * Deallocate any specified dynamic data in @data.
 *
 * Returns 0 on success, -1 on error.
 */
static int clean_data(union traceeval_data *data, struct traceeval_type *def)
{
	int result = 0;
	size_t i = -1;

	if (!data || !def)
		return -1;
	
	while (def[++i].type != TRACEEVAL_TYPE_NONE) {
		switch (def[i].type) {
		case TRACEEVAL_TYPE_STRING:
			free(data[i].string);
			break;
		case TRACEEVAL_TYPE_DYNAMIC:
			result |= def[i].dyn_release(&data[i]);
			break;
		default:
			break;
		}
	}

	if (result)
		return -1;
	return 0;
}

/**
 * Deallocate all possible data stored within the entry.
 *
 * Returns 0 on success, -1 on error.
 */
static int clean_entry(struct entry *entry, struct traceeval *eval)
{
	int result = 0;

	if (!entry)
		return -1;

	// deallocate dynamic traceeval_data
	result |= clean_data(entry->keys, eval->def_keys);
	result |= clean_data(entry->vals, eval->def_vals);
	free(entry->keys);
	free(entry->vals);

	if (result)
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
