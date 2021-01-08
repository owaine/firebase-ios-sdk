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

#ifndef FIRESTORE_CORE_SRC_MODEL_TRANSFORM_MUTATION_H_
#define FIRESTORE_CORE_SRC_MODEL_TRANSFORM_MUTATION_H_

#include <string>
#include <vector>

#include "Firestore/core/src/model/field_mask.h"
#include "Firestore/core/src/model/field_transform.h"
#include "Firestore/core/src/model/mutation.h"
#include "absl/types/optional.h"

namespace firebase {
namespace firestore {
namespace model {

/**
 * A mutation that modifies specific fields of the document with transform
 * operations. Tranforms include operation like increment and server
 * timestamps. See tranform_operations.h for all supported operations.
 *
 * It is somewhat similar to a PatchMutation in that it patches specific fields
 * and has no effect when applied to nullopt or a DeletedDocument (see comments
 * on PatchMutation for more details).
 */
class TransformMutation : public Mutation {
 public:
  TransformMutation(DocumentKey key,
                    std::vector<FieldTransform> field_transforms);

  /**
   * Casts a Mutation to a TransformMutation. This is a checked operation that
   * will assert if the type of the Mutation isn't actually Type::Transform.
   */
  explicit TransformMutation(const Mutation& mutation);

  /** Creates an invalid TransformMutation instance. */
  TransformMutation() = default;

 private:
  class Rep : public Mutation::Rep {
   public:
    Rep(DocumentKey&& key, std::vector<FieldTransform>&& field_transforms);

    Type type() const override {
      return Type::Transform;
    }

    MaybeDocument ApplyToRemoteDocument(
        const absl::optional<MaybeDocument>& maybe_doc,
        const MutationResult& mutation_result) const override;

    absl::optional<MaybeDocument> ApplyToLocalView(
        const absl::optional<MaybeDocument>& maybe_doc,
        const absl::optional<MaybeDocument>&,
        const Timestamp&) const override;

    absl::optional<ObjectValue> ExtractBaseValue(
        const absl::optional<MaybeDocument>& maybe_doc) const;

    bool Equals(const Mutation::Rep& other) const override;

    std::string ToString() const override;

   private:
    FieldMask field_mask_;
  };

  const Rep& set_rep() const {
    return static_cast<const Rep&>(rep());
  }
};

}  // namespace model
}  // namespace firestore
}  // namespace firebase

#endif  // FIRESTORE_CORE_SRC_MODEL_TRANSFORM_MUTATION_H_
