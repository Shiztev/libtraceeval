/* SPDX-License-Identifier: MIT */
/*
 * libtraceeval histogram interface.
 *
 * Copyright (C) 2023 Google Inc, Steven Rostedt <rostedt@goodmis.org>
 * Copyright (C) 2023 Google Inc, Stevie Alvarez <stevie.6strings@gmail.com>
 */
#ifndef __LIBTRACEEVAL_HIST_H__
#define __LIBTRACEEVAL_HIST_H__

#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>

// Data definition interfaces

/** Data type distinguishers */
enum traceeval_data_type {
	TRACEEVAL_TYPE_NONE,
	TRACEEVAL_TYPE_STRING,
	TRACEEVAL_TYPE_NUMBER,
	TRACEEVAL_TYPE_NUMBER_64,
	TRACEEVAL_TYPE_NUMBER_32,
	TRACEEVAL_TYPE_NUMBER_16,
	TRACEEVAL_TYPE_NUMBER_8,
	TRACEEVAL_TYPE_DYNAMIC
};

/** Statistics specification flags */
enum traceeval_flags {
	TRACEEVAL_FL_SIGNED	= (1 << 0),
	TRACEEVAL_FL_STATS	= (1 << 1)
};

/**
 * Trace data entry for a traceeval histogram
 * Constitutes keys and values.
 */
union traceeval_data {
	char				*string;
	struct traceeval_dynamic	*dyn_data;
	unsigned long			number;
	unsigned char			number_8;
	unsigned short			number_16;
	unsigned int			number_32;
	unsigned long long		number_64;
};

/**
 * Describes a struct traceeval_data instance
 * Defines expectations for a corresponding traceeval_data instance for a
 * traceeval histogram instance. Used to describe both keys and values.
 *
 * The id field is an optional value in case the user has multiple struct
 * traceeval_type instances with type fields set to TRACEEVAL_TYPE_DYNAMIC,
 * which each relate to distinct user defined struct traceeval_dynamic
 * 'sub-types'.
 * For flexibility, dyn_cmp() and dyn_release() take a struct traceeval_type
 * instance. This allows the user to distingush between different sub-types of
 * struct traceeeval_dynamic within a single callback function by examining the
 * id field. This is not a required approach, merely one that is accomodated.
 *
 * dyn_cmp() is used to compare two struct traceeval_dynamic instances when a
 * corresponding struct traceeval_type is reached with its type field set to
 * TRACEEVAL_TYPE_DYNAMIC. It should return 0 on equality, 1 if the first
 * argument is greater than the second, -1 for the otherway around, and -2 on
 * error.
 *
 * dyn_release() is used during traceeval_release() to release a union
 * traceeval_data's struct traceeval_dynamic field when the corresponding
 * traceeval_type type is set to TRACEEVAL_TYPE_DYNAMIC.
 */
struct traceeval_type {
	enum traceeval_data_type	type;
	char				*name;
	size_t				flags;
	size_t				id;
	void (*dyn_release)(struct traceeval_dynamic *, struct traceeval_type *);
	int (*dyn_cmp)(struct traceeval_dynamic *, struct traceeval_dynamic *,
			struct traceeval_type *);
};

/** Storage for atypical data */
struct traceeval_dynamic {
	size_t		size;
	void		*data;
};

// Histogram interfaces

/** Histogram */
struct traceeval;

/**
 * traceeval_init - create a traceeval descriptor
 * @keys: Defines the keys of the "histogram"
 * @vals: Defines the vals of the "histogram"
 *
 * The caller still owns @keys and @vals. The caller is responsible for any
 * necessary clean up of @keys and @vals.
 * For any member of @keys or @vals that isn't of type TRACEEVAL_TYPE_NONE,
 * the name field must be either a null-terminated string or NULL. For
 * members of type TRACEEVAL_TYPE_NONE, the name is ignored.
 * The @keys and @vals define how the traceeval instance will be populated.
 * The @keys will be used by traceeval_query() to find an instance within
 * the "historgram". Note, both the @keys and @vals array must end with:
 * { .type = TRACEEVAL_TYPE_NONE }.
 *
 * @vals can be NULL or start with its type field as TRACEEVAL_TYPE_NONE to
 * define the values of the histogram to be empty. If @keys is NULL or starts
 * with { .type = TRACEEVAL_TYPE_NONE }, it is treated as an error to ensure 
 * the histogram is valid.
 *
 * Retruns the descriptor on success, or NULL on error.
 */
struct traceeval *traceeval_init(const struct traceeval_type *keys,
		const struct traceeval_type *vals);

/**
 * traceeval_release - release a traceeval descriptor
 * @teval: An instance of traceeval returned by traceeval_init()
 *
 * When the caller of traceeval_init() is done with the returned @teval,
 * it must call traceeval_release().
 * This does not release any dynamically allocated data inserted by
 * the user, although it will call any dyn_release() functions provided by
 * the user from traceeval_init().
 */
void traceeval_release(struct traceeval *teval);

/**
 * traceeval_insert - Insert an item into the traceeval descriptor
 * @teval: The descriptor to insert into
 * @keys: The list of keys that defines what is being inserted.
 * @vals: The list of values that defines what is being inserted.
 *
 * Any dynamically allocated data is still owned by the caller; the
 * responsibility of deallocating it lies on the caller.
 * For all elements of @keys and @vals that correspond to a struct
 * traceeval_type of type TRACEEVAL_TYPE_STRING, the string field must be set
 * to either a null-terminated string or NULL.
 * The @keys is an array that holds the data in the order of the keys
 * passed into traceeval_init(). That is, if traceeval_init() had
 * keys = { { .type = TRACEEVAL_STRING }, { .type = TRACEEVAL_NUMBER_8 },
 * { .type = TRACEEVAL_NONE } }; then the @keys array passed in must
 * be a string (char *) followed by a 8 byte number (char). The @keys
 * and @vals are only examined to where it expects data. That is,
 * if the traceeval_init() keys had 3 items where the first two was defining
 * data, and the last one was the TRACEEVAL_TYPE_NONE, then the @keys
 * here only needs to be an array of 2, inserting the two items defined
 * by traceeval_init(). The same goes for @vals.
 *
 * Returns 0 on success, and -1 on error.
 */
int traceeval_insert(struct traceeval *teval,
		const union traceeval_data *keys,
		const union traceeval_data *vals);

/**
 * traceeval_compare - Check equality between two traceeval instances
 * @orig: The first traceeval instance
 * @copy: The second traceeval instance
 *
 * This compares the values of the key definitions, value definitions, and
 * inserted data between @orig and @copy in order. It does not compare
 * by memory address, except for struct traceeval_type's dyn_release() and
 * dyn_cmp() fields.
 *
 * Returns 0 if @orig and @copy are the same, 1 if not, and -1 on error.
 */
int traceeval_compare(struct traceeval *orig, struct traceeval *copy);

// interface to queuery traceeval

/**
 * traceeval_query - Find the last instance defined by the keys
 * @teval: The descriptor to search from
 * @keys: A list of data to look for
 * @results: A pointer to where to place the results (if found)
 *
 * This does a lookup for an instance within the traceeval data.
 * The @keys is an array defined by the keys declared in traceeval_init().
 * The @keys will return an item that had the same keys when it was
 * inserted by traceeval_insert(). The @keys here follow the same rules
 * as the keys for traceeval_insert().
 *
 * Note, when the caller is done with @results, it must call
 * traceeval_results_release() on it.
 *
 * Returns 1 if found, 0 if not found, and -1 on error.
 */
int traceeval_query(struct traceeval *teval,
		const union traceeval_data *keys,
		union traceeval_data * const *results);

/** Field name/descriptor for number of hits */
#define TRACEEVAL_VAL_HITS ((const char *)(-1UL))

/**
 * traceeval_find_key - find the index of a key
 * @teval: The descriptor to find the key for
 * @field: The name of the key to return the index for
 *
 * As the order of how keys are defined by traceeval_init(), it
 * is important to know what index they are for when dealing with
 * the other functions.
 *
 * Returns the index of the key with @field as the name.
 * Return -1 if not found.
 */
int traceeval_find_key(struct traceeval *teval, const char *field);

/**
 * traceeval_find_val - find the index of a value
 * @teval: The descriptor to find the value for
 * @field: The name of the value to return the index for
 *
 * As the order of how values are defined by traceeval_init(), it
 * is important to know what index they are for when dealing with
 * the results array from traceeval_query(). In order to facilitate
 * this, traceeval_find_val() will return the index for a given
 * @field so that the caller does not have to keep track of it.
 *
 * Returns the index of the value with @field as the name that can be
 * used to index the @results array from traceeval_query().
 * Return -1 if not found.
 */
int traceeval_find_val(struct traceeval *teval, const char *field);

/**
 * traceeval_results_release - Release the results return by traceeval_query()
 * @teval: The descriptor used in traceeval_query()
 * @results: The results returned by traceeval_query()
 *
 * The @results returned by traceeval_query() is owned by @teval, and
 * how it manages it is implementation specific. The caller should not
 * worry about it. When the caller of traceeval_query() is done with
 * the @results, it must call traceeval_results_release() on it to
 * allow traceeval to clean up its references.
 */
void traceeval_results_release(struct traceeval *teval,
		const union traceeval_data *results);

// Result iterator/histogram dump interfaces

/** Iterator over aggregated data */
struct traceeval_iterator;

/**
 * traceeval_iterator_get - get an iterator to read the data from traceeval
 * @teval: The descriptor to read from
 *
 * This returns a descriptor that can be used to interate through all the
 * data within @teval.
 *
 * Returns the iterator on success, NULL on error.
 */
struct traceeval_iterator *traceeval_iterator_get(struct traceeval *teval);

/**
 * traceeval_iterator_sort - Set how the iterator is sorted
 * @iter: The iterator to set the sorting
 * @sort_field: The field (defined by traceeval_init) to sort by.
 * @level: The level of sorting.
 * @ascending: Set to true to sort ascending order, or false to descending
 *
 * Sets how the iterator shall be sorted. @sort_field is the field to sort
 * by and may be the name of either a key or a value.
 *
 * The @level should be zero the first time this is called to define the main sort
 * field. If secondary sorting is to be done, then this function should be called
 * again with @level as 1. For more levels of sorting just call this function
 * for each level incrementing level each time. Note, if a level is missed,
 * then this will return an error and sorting will not be done for that level.
 *
 * Returns 0 on success, -1 or error (including missing a level).
 */
int traceeval_iterator_sort(struct traceeval_iterator *iter,
		const char *sort_field, int level, bool ascending);

/**
 * traceeval_iterator_next - Iterate through the data of a traceeval descriptor
 * @iter: The iterator that holds the location and sorting of the data
 * @keys: The current set of keys of the traceeval the @iter is sorting through
 *
 * This will iterate through all the data of the traceeval descriptor held
 * by @iter in the sort pattern defined by traceeval_iterator_sort().
 * The @keys return is same as the data used to populate the data passed into
 * traceveal_insert(). When the caller is done with @keys, it should be passed
 * into traceeval_keys_release().
 *
 * Returns 1 if it returned an item, 0 if there's no more data to return,
 * and -1 on error.
 */
int traceeval_iterator_next(struct traceeval_iterator *iter,
		const union traceeval_data **keys);


/**
 * traceeval_keys_release - Release the keys return by traceeval_iterator_next()
 * @iter: The iterator used in traceeval_iterator_next().
 * @keys: The results returned by traceeval_iterator_next()
 *
 * The @keys returned by traceeval_iterator_next() is owned by @iter, and
 * how it manages it is implementation specific. The caller should not
 * worry about it. When the caller of traceeval_iterator_next() is done with
 * the @keys, it must call traceeval_keys_release() on it to
 * allow traceeval to clean up its references.
 */
void traceeval_keys_release(struct traceeval_iterator *iter,
		const union traceeval_data *keys);

// Statistic extraction

/** Statistics about a given field */
struct traceeval_stat {
	unsigned long long	max;
	unsigned long long	min;
	unsigned long long	total;
	unsigned long long	avg;
	unsigned long long	std;
};

/**
 * traceeval_stat - Extract stats from a field marke a TRACEEVAL_FL_STATS
 * @teval: The descriptor holding the traceeval information
 * @keys: The list of keys to find the instance to get the stats on
 * @field: The field to retreive the stats for
 * @stats: A structure to place the stats into.
 *
 * This returns the stats of the the given @field. Note, if @field was
 * not marked for recording stats with the TRACEEVAL_FL_STATS flag, or
 * if no instance is found that has @keys, this will return an error.
 *
 * Returns 0 on success, -1 on error.
 */
int traceeval_stat(struct traceeval *teval, const union traceeval_data *keys,
		const char *field, struct traceeval_stat *stat);

#endif /* __LIBTRACEEVAL_HIST_H__ */

