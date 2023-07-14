
/* SPDX-License-Identifier: MIT */
/*
 * libtraceeval histogram interface implementation.
 *
 * Copyright (C) 2023 Google Inc, Stevie Alvarez <stevie.6strings@gmail.com>
 */

#include <histograms.h>

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

struct traceeval {  // TODO
	const struct traceeval_type	*def_keys;
	const struct traceeval_type	*def_vals;
	struct hist_table		*hist;
};

struct traceeval_iterator {};  // TODO

// TODO
struct traceeval *traceeval_init(const struct traceeval_type *keys,
				 const struct traceeval_type *vals)
{
	return NULL;
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
