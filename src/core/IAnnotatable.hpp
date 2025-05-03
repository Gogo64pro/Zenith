#include <vector>
#include "../ast/Other.hpp"
#include "../utils/small_vector.hpp"

namespace zenith{
	struct IAnnotatable{
		small_vector<std_P3019_modified::polymorphic<AnnotationNode>, 2> annotations;
		virtual ~IAnnotatable() = default;
		//virtual void setAnnotations(std::vector<polymorphic<AnnotationNode>> annotations) = 0;
		void setAnnotations(std::vector<std_P3019_modified::polymorphic<AnnotationNode>> ann){
			annotations = std::move(ann);
		};
	};
}