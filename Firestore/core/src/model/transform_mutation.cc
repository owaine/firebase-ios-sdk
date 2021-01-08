/*
 * Copyright 2019 Google
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "Firestore/core/src/model/transform_mutation.h"

#include <memory>
#include <set>
#include <utility>

#include "Firestore/core/src/model/document.h"
#include "Firestore/core/src/model/field_path.h"
#include "Firestore/core/src/model/field_value.h"
#include "Firestore/core/src/model/no_document.h"
#include "Firestore/core/src/model/transform_operation.h"
#include "Firestore/core/src/model/unknown_document.h"
#include "Firestore/core/src/util/hard_assert.h"
#include "Firestore/core/src/util/to_string.h"
#include "absl/strings/str_cat.h"

namespace firebase {
namespace firestore {
namespace model {

static_assert(sizeof(Mutation) == sizeof(TransformMutation),
              "TransformMutation may not have additional members (everything "
              "goes in Rep)");

TransformMutation::TransformMutation(
    DocumentKey key, std::vector<FieldTransform> field_transforms)
    : Mutation(
          std::make_shared<Rep>(std::move(key), std::move(field_transforms))) {
}

TransformMutation::TransformMutation(const Mutation& mutation)
    : Mutation(mutation) {
  HARD_ASSERT(type() == Type::Transform);
}

TransformMutation::Rep::Rep(DocumentKey&& key,
                            std::vector<FieldTransform>&& field_transforms)
    : Mutation::Rep(std::move(key),
                    Precondition::Exists(true),
                    std::move(field_transforms)) {
  //  std::set<FieldPath> fields;
  //  for (const auto& transform : set_rep().field_transforms()) {
  //    fields.insert(transform.path());
  //  }
  //
  //  field_mask_ = FieldMask(std::move(fields));
}

MaybeDocument TransformMutation::Rep::ApplyToRemoteDocument(
    const absl::optional<MaybeDocument>& maybe_doc,
    const MutationResult& mutation_result) const {
  VerifyKeyMatches(maybe_doc);

  HARD_ASSERT(mutation_result.transform_results() != absl::nullopt,
              "Transform results missing from TransformMutation.");

  if (!precondition().IsValidFor(maybe_doc)) {
    // Since the mutation was not rejected, we know that the precondition
    // matched on the backend. We therefore must not have the expected version
    // of the document in our cache and return an UnknownDocument with the
    // known update_time.
    return UnknownDocument(key(), mutation_result.version());
  }

  // We only support transforms with precondition exists, so we can only apply
  // it to an existing document
  HARD_ASSERT(maybe_doc && maybe_doc->is_document(),
              "Unknown MaybeDocument type %s", maybe_doc->type());
  Document doc(*maybe_doc);

  HARD_ASSERT(mutation_result.transform_results() != absl::nullopt);

  std::vector<FieldValue> transform_results =
      ServerTransformResults(maybe_doc, *mutation_result.transform_results());

  ObjectValue new_data = TransformObject(doc.data(), transform_results);

  return Document(std::move(new_data), key(), mutation_result.version(),
                  DocumentState::kCommittedMutations);
}

absl::optional<MaybeDocument> TransformMutation::Rep::ApplyToLocalView(
    const absl::optional<MaybeDocument>& maybe_doc,
    const absl::optional<MaybeDocument>& base_doc,
    const Timestamp& local_write_time) const {
  VerifyKeyMatches(maybe_doc);

  if (!precondition().IsValidFor(maybe_doc)) {
    return maybe_doc;
  }

  // We only support transforms with precondition exists, so we can only apply
  // it to an existing document
  HARD_ASSERT(maybe_doc && maybe_doc->is_document(),
              "Unknown MaybeDocument type %s", maybe_doc->type());
  Document doc(*maybe_doc);

  std::vector<FieldValue> transform_results =
      LocalTransformResults(maybe_doc, base_doc, local_write_time);
  ObjectValue new_data = TransformObject(doc.data(), transform_results);

  return Document(std::move(new_data), doc.key(), doc.version(),
                  DocumentState::kLocalMutations);
}

bool TransformMutation::Rep::Equals(const Mutation::Rep& other) const {
  if (!Mutation::Rep::Equals(other)) return false;
  return true;
}

std::string TransformMutation::Rep::ToString() const {
  return absl::StrCat("TransformMutation(key=", key().ToString(),
                      ", transforms=", util::ToString(field_transforms()), ")");
}

}  // namespace model
}  // namespace firestore
}  // namespace firebase
