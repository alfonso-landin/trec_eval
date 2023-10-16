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
te_calc_err_rel_lvl(const EPI *epi, const REL_INFO *rel_info, const RESULTS *results,
		const TREC_MEAS *tm, TREC_EVAL *eval);

/* See trec_eval.h for definition of TREC_MEAS */
TREC_MEAS te_meas_err_rel_lvl =
		{ "err_rel_lvl",
				"    Expected Reciprocal Rank of the first relevant retrieved doc.\n\
		Measure is most useful for tasks in which there is only one relevant\n\
		doc, or the user only wants one relevant doc using graded relevance.\n\
		Gain values are adjusted by the minimum relvance level, eg.g if the minimum\n\
                level is 4, gain for relevance level 4 is 1, for relevance level 5 is 2, etc.\n\
		Cite: Olivier Chapelle, Donald Metlzer, Ya Zhang, and Pierre Grinspan.\n\
		2009. Expected Reciprocal Rank for Graded Relevance. In Proceedings of\n\
		the 18th ACM Conference on Information and Knowledge Management\n\
		(CIKM '09). ACM, New York, NY, USA, 621-630.\n\
		DOI=http://dx.doi.org/10.1145/1645953.1646033 \n",
				te_init_meas_s_float, te_calc_err_rel_lvl, te_acc_meas_s,
				te_calc_avg_meas_s, te_print_single_meas_s_float,
				te_print_final_meas_s_float, NULL, -1 };

static double compute_gain(const long rel_level, const long max_rel,
			   const long min_rel);

static int te_calc_err_rel_lvl(const EPI *epi, const REL_INFO *rel_info,
		const RESULTS *results, const TREC_MEAS *tm, TREC_EVAL *eval) {

	RES_RELS res_rels;
	double p;
	double err;
	double gain;
	long r;

	if (UNDEF == te_form_res_rels(epi, rel_info, results, &res_rels))
		return (UNDEF);

	p = 1.0;
	err = 0.0;
	for (r = 0; r < res_rels.num_ret; r++) {
		gain = compute_gain(res_rels.results_rel_list[r],
				res_rels.num_rel_levels,
				epi->relevance_level);
		err += p * gain / (double) (r + 1);
		p *= 1.0 - gain;
	}
	eval->values[tm->eval_index].value = err;

	return (1);

}

static inline double compute_gain(const long rel_level, const long max_rel,
				  const long min_rel) {

	return rel_level >= min_rel ?
			(pow(2.0, rel_level - min_rel + 1) - 1.0) / pow(2.0, (max_rel - 1)) : 0.0;
}
