/*
Copyright (c) by respective owners including Yahoo!, Microsoft, and
individual contributors. All rights reserved.  Released under a BSD
license as described in the file LICENSE.
 */
#pragma once
#include <stdint.h>
#include "v_array.h"
#include "simple_label.h"
#include "multiclass.h"
#include "multilabel.h"
#include "cost_sensitive.h"
#include "cb.h"
#include "constant.h"

const size_t wap_ldf_namespace  = 126;
const size_t history_namespace  = 127;
const size_t constant_namespace = 128;
const size_t nn_output_namespace  = 129;
const size_t autolink_namespace  = 130;
const size_t neighbor_namespace  = 131;   // this is \x83 -- to do quadratic, say "-q a`printf "\x83"` on the command line
const size_t affix_namespace     = 132;   // this is \x84
const size_t spelling_namespace  = 133;   // this is \x85
const size_t conditioning_namespace = 134;// this is \x86
const size_t dictionary_namespace  = 135; // this is \x87

struct feature
{ float x;
  uint64_t weight_index;
  bool operator==(feature j) {return weight_index == j.weight_index;}
  feature(float x_=0., uint64_t weight_index_=0) : x(x_), weight_index(weight_index_) {}
};

struct audit_data
{ char* space;
  char* feature;
  uint64_t weight_index;
  float x;
};

typedef union
{ label_data simple;
  MULTICLASS::label_t multi;
  COST_SENSITIVE::label cs;
  CB::label cb;
  CB_EVAL::label cb_eval;
  MULTILABEL::labels multilabels;
} polylabel;

typedef union
{ float scalar;
  uint32_t multiclass;
  MULTILABEL::labels multilabels;
  float* probs; // for --probabilities --oaa
  float prob; // for --probabilities --csoaa_ldf=mc
} polyprediction;

struct example // core example datatype.
{ //output prediction
  polyprediction pred;

  // input fields
  polylabel l;

  float weight;//a relative importance weight for the example, default = 1
  v_array<char> tag;//An identifier for the example.
  size_t example_counter;
  v_array<unsigned char> indices;
  v_array<feature> atomics[256]; // raw parsed data
  uint64_t ft_offset;

  //helpers
  v_array<audit_data> audit_features[256];
  size_t num_features;//precomputed, cause it's fast&easy.
  float partial_prediction;//shared data for prediction.
  float updated_prediction;//estimated post-update prediction.
  v_array<float> topic_predictions;
  float loss;
  float example_t;//sum of importance weights so far.
  float sum_feat_sq[256];//helper for total_sum_feat_sq.
  float total_sum_feat_sq;//precomputed, cause it's kind of fast & easy.
  float confidence;
  v_array<feature>* passthrough; // if a higher-up reduction wants access to internal state of lower-down reductions, they go here

  bool test_only;
  bool end_pass;//special example indicating end of pass.
  bool sorted;//Are the features sorted or not?
  bool in_use; //in use or not (for the parser)
};

struct vw;

struct flat_example
{ polylabel l;

  size_t tag_len;
  char* tag;//An identifier for the example.

  size_t example_counter;
  uint32_t ft_offset;
  float global_weight;

  size_t num_features;//precomputed, cause it's fast&easy.
  float total_sum_feat_sq;//precomputed, cause it's kind of fast & easy.
  size_t feature_map_len;
  feature* feature_map; //map to store sparse feature vectors
};

flat_example* flatten_example(vw& all, example *ec);
flat_example* flatten_sort_example(vw& all, example *ec);
void free_flatten_example(flat_example* fec);

inline int example_is_newline(example& ec)
{ // if only index is constant namespace or no index
  return ((ec.indices.size() == 0) ||
          ((ec.indices.size() == 1) &&
           (ec.indices.last() == constant_namespace)));
}

inline bool valid_ns(char c)
{ return !(c == '|' || c == ':');
}

inline void add_passthrough_feature_magic(example& ec, uint32_t magic, uint32_t i, float x)
{ if (ec.passthrough)
    ec.passthrough->push_back( feature(x, (FNV_prime * magic) ^ i) );
}

#define add_passthrough_feature(ec, i, x) add_passthrough_feature_magic(ec, __FILE__[0]*483901+__FILE__[1]*3417+__FILE__[2]*8490177, i, x);
