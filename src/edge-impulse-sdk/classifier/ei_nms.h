/* The Clear BSD License
 *
 * Copyright (c) 2025 EdgeImpulse Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the disclaimer
 * below) provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *
 *   * Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 *
 *   * Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY
 * THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _EDGE_IMPULSE_NMS_H_
#define _EDGE_IMPULSE_NMS_H_

#include "model-parameters/model_metadata.h"
#include "edge-impulse-sdk/classifier/ei_model_types.h"
#include "edge-impulse-sdk/classifier/ei_classifier_types.h"
#include "edge-impulse-sdk/porting/ei_classifier_porting.h"

#if (EI_CLASSIFIER_OBJECT_DETECTION_LAST_LAYER == EI_CLASSIFIER_LAST_LAYER_YOLOV5) || (EI_CLASSIFIER_OBJECT_DETECTION_LAST_LAYER == EI_CLASSIFIER_LAST_LAYER_YOLOV5_V5_DRPAI) || (EI_CLASSIFIER_OBJECT_DETECTION_LAST_LAYER == EI_CLASSIFIER_LAST_LAYER_YOLOX) || (EI_CLASSIFIER_OBJECT_DETECTION_LAST_LAYER == EI_CLASSIFIER_LAST_LAYER_TAO_RETINANET) || (EI_CLASSIFIER_OBJECT_DETECTION_LAST_LAYER == EI_CLASSIFIER_LAST_LAYER_TAO_SSD) || (EI_CLASSIFIER_OBJECT_DETECTION_LAST_LAYER == EI_CLASSIFIER_LAST_LAYER_TAO_YOLOV3) || (EI_CLASSIFIER_OBJECT_DETECTION_LAST_LAYER == EI_CLASSIFIER_LAST_LAYER_TAO_YOLOV4) || (EI_CLASSIFIER_OBJECT_DETECTION_LAST_LAYER == EI_CLASSIFIER_LAST_LAYER_YOLOV2) || (EI_CLASSIFIER_OBJECT_DETECTION_LAST_LAYER == EI_CLASSIFIER_LAST_LAYER_YOLO_PRO) || (EI_CLASSIFIER_OBJECT_DETECTION_LAST_LAYER == EI_CLASSIFIER_LAST_LAYER_YOLOV11) || (EI_CLASSIFIER_OBJECT_DETECTION_LAST_LAYER == EI_CLASSIFIER_LAST_LAYER_YOLOV11_ABS)

// The code below comes from tensorflow/lite/kernels/internal/reference/non_max_suppression.h
// Copyright 2019 The TensorFlow Authors.  All rights reserved.
// Licensed under the Apache License, Version 2.0
#include <algorithm>
#include <cmath>
#include <deque>
#include <queue>

// A pair of diagonal corners of the box.
struct BoxCornerEncoding {
  float y1;
  float x1;
  float y2;
  float x2;
};

static inline float ComputeIntersectionOverUnion(const float* boxes, const int i,
                                          const int j) {
  auto& box_i = reinterpret_cast<const BoxCornerEncoding*>(boxes)[i];
  auto& box_j = reinterpret_cast<const BoxCornerEncoding*>(boxes)[j];
  const float box_i_y_min = std::min<float>(box_i.y1, box_i.y2);
  const float box_i_y_max = std::max<float>(box_i.y1, box_i.y2);
  const float box_i_x_min = std::min<float>(box_i.x1, box_i.x2);
  const float box_i_x_max = std::max<float>(box_i.x1, box_i.x2);
  const float box_j_y_min = std::min<float>(box_j.y1, box_j.y2);
  const float box_j_y_max = std::max<float>(box_j.y1, box_j.y2);
  const float box_j_x_min = std::min<float>(box_j.x1, box_j.x2);
  const float box_j_x_max = std::max<float>(box_j.x1, box_j.x2);

  const float area_i =
      (box_i_y_max - box_i_y_min) * (box_i_x_max - box_i_x_min);
  const float area_j =
      (box_j_y_max - box_j_y_min) * (box_j_x_max - box_j_x_min);
  if (area_i <= 0 || area_j <= 0) return 0.0;
  const float intersection_ymax = std::min<float>(box_i_y_max, box_j_y_max);
  const float intersection_xmax = std::min<float>(box_i_x_max, box_j_x_max);
  const float intersection_ymin = std::max<float>(box_i_y_min, box_j_y_min);
  const float intersection_xmin = std::max<float>(box_i_x_min, box_j_x_min);
  const float intersection_area =
      std::max<float>(intersection_ymax - intersection_ymin, 0.0) *
      std::max<float>(intersection_xmax - intersection_xmin, 0.0);
  return intersection_area / (area_i + area_j - intersection_area);
}

// Implements (Single-Class) Soft NMS (with Gaussian weighting).
// Supports functionality of TensorFlow ops NonMaxSuppressionV4 & V5.
// Reference: "Soft-NMS - Improving Object Detection With One Line of Code"
//            [Bodla et al, https://arxiv.org/abs/1704.04503]
// Implementation adapted from the TensorFlow NMS code at
// tensorflow/core/kernels/non_max_suppression_op.cc.
//
// Arguments:
//  boxes: box encodings in format [y1, x1, y2, x2], shape: [num_boxes, 4]
//  num_boxes: number of candidates
//  scores: scores for candidate boxes, in the same order. shape: [num_boxes]
//  max_output_size: the maximum number of selections.
//  iou_threshold: Intersection-over-Union (IoU) threshold for NMS
//  score_threshold: All candidate scores below this value are rejected
//  soft_nms_sigma: Soft NMS parameter, used for decaying scores
//
// Outputs:
//  selected_indices: all the selected indices. Underlying array must have
//    length >= max_output_size. Cannot be null.
//  selected_scores: scores of selected indices. Defer from original value for
//    Soft NMS. If not null, array must have length >= max_output_size.
//  num_selected_indices: Number of selections. Only these many elements are
//    set in selected_indices, selected_scores. Cannot be null.
//
// Assumes inputs are valid (for eg, iou_threshold must be >= 0).
static inline void NonMaxSuppression(const float* boxes, const int num_boxes,
                              const float* scores, const int max_output_size,
                              const float iou_threshold,
                              const float score_threshold,
                              const float soft_nms_sigma, int* selected_indices,
                              float* selected_scores,
                              int* num_selected_indices) {
  struct Candidate {
    int index;
    float score;
    int suppress_begin_index;
  };

  // Priority queue to hold candidates.
  auto cmp = [](const Candidate bs_i, const Candidate bs_j) {
    return bs_i.score < bs_j.score;
  };
  std::priority_queue<Candidate, std::deque<Candidate>, decltype(cmp)>
      candidate_priority_queue(cmp);
  // Populate queue with candidates above the score threshold.
  for (int i = 0; i < num_boxes; ++i) {
    if (scores[i] > score_threshold) {
      candidate_priority_queue.emplace(Candidate({i, scores[i], 0}));
    }
  }

  *num_selected_indices = 0;
  int num_outputs = std::min(static_cast<int>(candidate_priority_queue.size()),
                             max_output_size);
  if (num_outputs == 0) return;

  // NMS loop.
  float scale = 0;
  if (soft_nms_sigma > 0.0) {
    scale = -0.5 / soft_nms_sigma;
  }
  while (*num_selected_indices < num_outputs &&
         !candidate_priority_queue.empty()) {
    Candidate next_candidate = candidate_priority_queue.top();
    const float original_score = next_candidate.score;
    candidate_priority_queue.pop();

    // Overlapping boxes are likely to have similar scores, therefore we
    // iterate through the previously selected boxes backwards in order to
    // see if `next_candidate` should be suppressed. We also enforce a property
    // that a candidate can be suppressed by another candidate no more than
    // once via `suppress_begin_index` which tracks which previously selected
    // boxes have already been compared against next_candidate prior to a given
    // iteration.  These previous selected boxes are then skipped over in the
    // following loop.
    bool should_hard_suppress = false;
    for (int j = *num_selected_indices - 1;
         j >= next_candidate.suppress_begin_index; --j) {
      const float iou = ComputeIntersectionOverUnion(
          boxes, next_candidate.index, selected_indices[j]);

      // First decide whether to perform hard suppression.
      if (iou >= iou_threshold) {
        should_hard_suppress = true;
        break;
      }

      // Suppress score if NMS sigma > 0.
      if (soft_nms_sigma > 0.0) {
        next_candidate.score =
            next_candidate.score * std::exp(scale * iou * iou);
      }

      // If score has fallen below score_threshold, it won't be pushed back into
      // the queue.
      if (next_candidate.score <= score_threshold) break;
    }
    // If `next_candidate.score` has not dropped below `score_threshold`
    // by this point, then we know that we went through all of the previous
    // selections and can safely update `suppress_begin_index` to
    // `selected.size()`. If on the other hand `next_candidate.score`
    // *has* dropped below the score threshold, then since `suppress_weight`
    // always returns values in [0, 1], further suppression by items that were
    // not covered in the above for loop would not have caused the algorithm
    // to select this item. We thus do the same update to
    // `suppress_begin_index`, but really, this element will not be added back
    // into the priority queue.
    next_candidate.suppress_begin_index = *num_selected_indices;

    if (!should_hard_suppress) {
      if (next_candidate.score == original_score) {
        // Suppression has not occurred, so select next_candidate.
        selected_indices[*num_selected_indices] = next_candidate.index;
        if (selected_scores) {
          selected_scores[*num_selected_indices] = next_candidate.score;
        }
        ++*num_selected_indices;
      }
      if ((soft_nms_sigma > 0.0) && (next_candidate.score > score_threshold)) {
        // Soft suppression might have occurred and current score is still
        // greater than score_threshold; add next_candidate back onto priority
        // queue.
        candidate_priority_queue.push(next_candidate);
      }
    }
  }
}

/**
 * Run non-max suppression over the results array (for bounding boxes)
 */
EI_IMPULSE_ERROR ei_run_nms(
    const ei_impulse_t *impulse,
    std::vector<ei_impulse_result_bounding_box_t> *results,
    float *boxes,
    float *scores,
    int *classes,
    size_t bb_count,
    bool clip_boxes,
    const ei_object_detection_nms_config_t *nms_config) {

    if (bb_count < 1) {
        return EI_IMPULSE_OK;
    }

    int *selected_indices = (int*)ei_malloc(1 * bb_count * sizeof(int));
    float *selected_scores = (float*)ei_malloc(1 * bb_count * sizeof(float));

    if (!scores || !boxes || !selected_indices || !selected_scores || !classes) {
        ei_free(selected_indices);
        ei_free(selected_scores);
        return EI_IMPULSE_OUT_OF_MEMORY;
    }

    //  boxes: box encodings in format [y1, x1, y2, x2], shape: [num_boxes, 4]
    //  num_boxes: number of candidates
    //  scores: scores for candidate boxes, in the same order. shape: [num_boxes]
    //  max_output_size: the maximum number of selections.
    //  iou_threshold: Intersection-over-Union (IoU) threshold for NMS
    //  score_threshold: All candidate scores below this value are rejected
    //  soft_nms_sigma: Soft NMS parameter, used for decaying scores

    int num_selected_indices;

    NonMaxSuppression(
        (const float*)boxes, // boxes
        bb_count, // num_boxes
        (const float*)scores, // scores
        bb_count, // max_output_size
        nms_config->iou_threshold, // iou_threshold
        nms_config->confidence_threshold, // score_threshold
        0.0f, // soft_nms_sigma
        selected_indices,
        selected_scores,
        &num_selected_indices);

    std::vector<ei_impulse_result_bounding_box_t> new_results;

    for (size_t ix = 0; ix < (size_t)num_selected_indices; ix++) {

        int out_ix = selected_indices[ix];
        ei_impulse_result_bounding_box_t bb;
        bb.label  = impulse->categories[classes[out_ix]];
        bb.value  = selected_scores[ix];

        float ymin = boxes[(out_ix * 4) + 0];
        float xmin = boxes[(out_ix * 4) + 1];
        float ymax = boxes[(out_ix * 4) + 2];
        float xmax = boxes[(out_ix * 4) + 3];

        if (clip_boxes) {
            ymin = std::min(std::max(ymin, 0.0f), (float)impulse->input_height);
            xmin = std::min(std::max(xmin, 0.0f), (float)impulse->input_width);
            ymax = std::min(std::max(ymax, 0.0f), (float)impulse->input_height);
            xmax = std::min(std::max(xmax, 0.0f), (float)impulse->input_width);
        }

        bb.y      = static_cast<uint32_t>(ymin);
        bb.x      = static_cast<uint32_t>(xmin);
        bb.height = static_cast<uint32_t>(ymax) - bb.y;
        bb.width  = static_cast<uint32_t>(xmax) - bb.x;
        new_results.push_back(bb);

        EI_LOGD("Found bb with label %s\n", bb.label);
    }

    results->clear();

    for (size_t ix = 0; ix < new_results.size(); ix++) {
        results->push_back(new_results[ix]);
    }

    ei_free(selected_indices);
    ei_free(selected_scores);

    return EI_IMPULSE_OK;

}

/**
 * Run non-max suppression over the results array (for bounding boxes)
 */
EI_IMPULSE_ERROR ei_run_nms(
    const ei_impulse_t *impulse,
    const ei_object_detection_nms_config_t *nms_config,
    std::vector<ei_impulse_result_bounding_box_t> *results,
    bool clip_boxes = true
    ) {

    size_t bb_count = 0;
    for (size_t ix = 0; ix < results->size(); ix++) {
        auto bb = results->at(ix);
        if (bb.value == 0) {
            continue;
        }
        bb_count++;
    }

    if (bb_count < 1) {
        return EI_IMPULSE_OK;
    }

    float *boxes = (float*)ei_malloc(4 * bb_count * sizeof(float));
    float *scores = (float*)ei_malloc(1 * bb_count * sizeof(float));
    int *classes = (int*) ei_malloc(bb_count * sizeof(int));

    if (!scores || !boxes || !classes) {
        ei_free(boxes);
        ei_free(scores);
        ei_free(classes);
        return EI_IMPULSE_OUT_OF_MEMORY;
    }

    size_t box_ix = 0;
    for (size_t ix = 0; ix < results->size(); ix++) {
        auto bb = results->at(ix);
        if (bb.value == 0) {
            continue;
        }
        boxes[(box_ix * 4) + 0] = bb.y;
        boxes[(box_ix * 4) + 1] = bb.x;
        boxes[(box_ix * 4) + 2] = bb.y + bb.height;
        boxes[(box_ix * 4) + 3] = bb.x + bb.width;
        scores[box_ix] = bb.value;

        for (size_t j = 0; j < impulse->label_count; j++) {
          if (strcmp(impulse->categories[j], bb.label) == 0)
          classes[box_ix] = j;
        }

        box_ix++;
    }

    EI_IMPULSE_ERROR nms_res = ei_run_nms(impulse,
                                          results,
                                          boxes,
                                          scores,
                                          classes,
                                          bb_count,
                                          clip_boxes,
                                          nms_config);


    ei_free(boxes);
    ei_free(scores);
    ei_free(classes);

    return nms_res;

}

#endif // #if (EI_CLASSIFIER_OBJECT_DETECTION_LAST_LAYER == EI_CLASSIFIER_LAST_LAYER_YOLOV5) || (EI_CLASSIFIER_OBJECT_DETECTION_LAST_LAYER == EI_CLASSIFIER_LAST_LAYER_YOLOV5_V5_DRPAI) || (EI_CLASSIFIER_OBJECT_DETECTION_LAST_LAYER == EI_CLASSIFIER_LAST_LAYER_YOLOX) || (EI_CLASSIFIER_OBJECT_DETECTION_LAST_LAYER == EI_CLASSIFIER_LAST_LAYER_TAO_RETINANET) || (EI_CLASSIFIER_OBJECT_DETECTION_LAST_LAYER == EI_CLASSIFIER_LAST_LAYER_TAO_SSD) || (EI_CLASSIFIER_OBJECT_DETECTION_LAST_LAYER == EI_CLASSIFIER_LAST_LAYER_TAO_YOLOV3) || (EI_CLASSIFIER_OBJECT_DETECTION_LAST_LAYER == EI_CLASSIFIER_LAST_LAYER_TAO_YOLOV4) || (EI_CLASSIFIER_OBJECT_DETECTION_LAST_LAYER == EI_CLASSIFIER_LAST_LAYER_YOLOV2) || (EI_CLASSIFIER_OBJECT_DETECTION_LAST_LAYER == EI_CLASSIFIER_LAST_LAYER_YOLO_PRO) || (EI_CLASSIFIER_OBJECT_DETECTION_LAST_LAYER == EI_CLASSIFIER_LAST_LAYER_YOLOV11) || || (EI_CLASSIFIER_OBJECT_DETECTION_LAST_LAYER == EI_CLASSIFIER_LAST_LAYER_YOLOV11_ABS)
#endif // _EDGE_IMPULSE_NMS_H_
