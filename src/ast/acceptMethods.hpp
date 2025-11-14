#pragma once
#include <type_traits>

#define ACCEPT_METHODS \
void accept(Visitor& visitor) override; \
void accept(PolymorphicVisitor& visitor, std_P3019_modified::polymorphic<ASTNode> x) override;