module;
#include <vector>
#include "../utils/small_vector.hpp"
import zenith.core.polymorphic;
export module zenith.ast:IAnnotatable;
import :other;

export namespace zenith{
	struct IAnnotatable{
		small_vector<polymorphic<AnnotationNode>, 2> annotations;
		virtual ~IAnnotatable() = default;
		//virtual void setAnnotations(std::vector<polymorphic<AnnotationNode>> annotations) = 0;
		void setAnnotations(std::vector<polymorphic<AnnotationNode>> ann) {
			annotations.clear();
			annotations.reserve(ann.size());
			for (auto&& item : ann) {
				annotations.emplace_back(std::move(item));
			}
		}
	};
}