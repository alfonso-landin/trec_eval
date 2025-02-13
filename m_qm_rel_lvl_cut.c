/*
 Copyright (c) 2017 - Daniel Valcarce.

 Permission is granted for use and modification of this file for
 research, non-commercial purposes.
 */

#include "common.h"
#include "sysfunc.h"
#include "trec_eval.h"
#include "functions.h"
#include "trec_format.h"

static int
te_calc_qm_rel_lvl_cut(const EPI *epi, const REL_INFO *rel_info,
		const RESULTS *results, const TREC_MEAS *tm, TREC_EVAL *eval);
static long long_cutoff_array[] = { 5, 10, 15, 20, 30, 100 };
static PARAMS default_qm_rel_lvl_cutoffs = {
NULL, sizeof(long_cutoff_array) / sizeof(long_cutoff_array[0]),
		&long_cutoff_array[0] };
static double get_gain(const long rel_level, const long min_lvl);

/* See trec_eval.h for definition of TREC_MEAS */
TREC_MEAS te_meas_qm_rel_lvl_cut =
		{ "qm_rel_lvl_cut",
				"    Q-measure.\n\
	Gains are adjusted by the minimum relevance level.\n\
	Cutoffs must be positive without duplicates\n\
	Default params: -m qm_cut_rel_lvl.5,10,15,20,30,100\n\
	Cite:  Tetsuya Sakai and Noriko Kando: On information retrieval metrics\n\
	designed for evaluation with incomplete relevance assessments. In\n\
	Information	Retrieval 11, 5 (2008), 447-470.\n\
	DOI=http://dx.doi.org/10.1007/s10791-008-9059-7\n",
				te_init_meas_a_float_cut_long, te_calc_qm_rel_lvl_cut,
				te_acc_meas_a_cut, te_calc_avg_meas_a_cut,
				te_print_single_meas_a_cut, te_print_final_meas_a_cut,
				(void *) &default_qm_rel_lvl_cutoffs, -1 };

static int te_calc_qm_rel_lvl_cut(const EPI *epi, const REL_INFO *rel_info,
		const RESULTS *results, const TREC_MEAS *tm, TREC_EVAL *eval) {

	long *cutoffs = (long *) tm->meas_params->param_values;
	long cutoff_index = 0;
	RES_RELS res_rels;
	long i;
	long rel_so_far = 0;
	long cur_lvl, lvl_count = 0;
	double q_measure = 0.0;
	double cum_gain = 0.0;
	double *cgi;
	double max_cgi;
	double gain;
	long total = 0.0;

	if (UNDEF == te_form_res_rels(epi, rel_info, results, &res_rels)) {
		return (UNDEF);
	}

	for (i = 1; i < res_rels.num_rel_levels; i++) {
		if (get_gain(i, epi->relevance_level) > 0.0) {
			total += res_rels.rel_levels[i];
		}
	}

	if (!total) {
		eval->values[tm->eval_index].value = 0.0;
		return (1);
	}

	/* Precompute ideal cumulative gains (cgi) at each position */
	if ((cgi = (double *) calloc(total, sizeof(double))) == NULL) {
		fprintf(stderr, "qm: Not enough memory");
		return (UNDEF);
	}
	cutoff_index = 0;
	lvl_count = 0;
	cur_lvl = res_rels.num_rel_levels - 1;
	for (i = 0; 1; i++) {
		lvl_count++;
		while (cur_lvl > 0 && lvl_count > res_rels.rel_levels[cur_lvl]) {
			cur_lvl--;
			lvl_count = 1;
		}
		if ((gain = get_gain(cur_lvl, epi->relevance_level)) == 0) {
			break;
		}
		cgi[i] = gain;
		if (i > 0) {
			cgi[i] += cgi[i - 1];
		}
	}
	max_cgi = cgi[total - 1];

	/* Compute Q-measure */
	for (i = 0; i < res_rels.num_ret; i++) {
		if (i == cutoffs[cutoff_index]) {
			/* Calculate previous cutoff threshold.
			 Note i guaranteed to be positive by init_meas */
			eval->values[tm->eval_index + cutoff_index].value = q_measure
					/ (double) total;
			if (++cutoff_index == tm->meas_params->num_params) {
				break;
			}
		}
		if ((gain = get_gain(res_rels.results_rel_list[i], epi->relevance_level)) > 0.0) {
			rel_so_far++;
			cum_gain += gain;
			q_measure += (cum_gain + rel_so_far)
					/ (i + 1 + (i >= total ? max_cgi : cgi[i]));
		}
	}

	/* calculate values for those cutoffs not achieved */
	q_measure /= (double) total;
	while (cutoff_index < tm->meas_params->num_params) {
		eval->values[tm->eval_index + cutoff_index].value = q_measure;
		cutoff_index++;
	}

	free(cgi);
	return (1);
}

static inline double get_gain(const long rel_level, const long min_lvl) {
    if (rel_level < min_lvl) {
	return 0.0;
    } else {
	return rel_level - min_lvl + 1;
    }
}
