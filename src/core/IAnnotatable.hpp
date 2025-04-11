#include <vector>
#include "../ast/Other.hpp"
#include "../utils/small_vector.hpp"

namespace zenith{
	struct IAnnotatable{
		small_vector<std::unique_ptr<AnnotationNode>, 2> annotations;
		virtual ~IAnnotatable() = default;
		//virtual void setAnnotations(std::vector<std::unique_ptr<AnnotationNode>> annotations) = 0;
		void setAnnotations(std::vector<std::unique_ptr<AnnotationNode>> ann){
			annotations = std::move(ann);
		};
	};
}