#include "SemanticAnalyzer.hpp"

namespace zenith {

	SymbolTable&& SemanticAnalyzer::analyze(ProgramNode &program) {
		visit(program);
		return std::move(symbolTable);
	}

	void SemanticAnalyzer::visit(ProgramNode &node) {
		symbolTable.enterScope();
		for (auto &x: node.declarations) {
			x->accept(*this);
		}
		symbolTable.exitScope();
	}

	void SemanticAnalyzer::visit(ASTNode &node) {

	}

	void SemanticAnalyzer::visit(ImportNode &node) {
		errorReporter.report(node.loc, "Import are not handled until I extract entire process to blah, blah blah", {"warning", YELLOW_TEXT});
	}

	void SemanticAnalyzer::visit(BlockNode &node) {
		symbolTable.enterScope();
		for (auto &x: node.statements) {
			x->accept(*this);
		}
		symbolTable.exitScope();
	}

	void SemanticAnalyzer::visit(VarDeclNode &node) {
		std::unique_ptr<TypeNode> declaredType = nullptr;
		if (node.type) {
			declaredType = resolveType(node.type.get());
		}

		std::unique_ptr<TypeNode> initializerType = nullptr;
		if (node.initializer) {
			initializerType = visitExpression(*node.initializer);
			if (initializerType == nullptr) {
				errorReporter.report(node.initializer->loc, "Could not evaluate initializer");
			}
		}

		std::unique_ptr<TypeNode> finalType = nullptr;

		// --- Check the Kind ---
		if (node.kind == VarDeclNode::DYNAMIC) {
			finalType = std::make_unique<TypeNode>(node.loc, TypeNode::DYNAMIC);

			if (declaredType && !declaredType->isDynamic()) {
				errorReporter.report(node.type->loc,
				                     "Explicit type conflicts with 'dynamic' keyword.");
			}
			// No compatibility check needed for the initializer itself,
			// as dynamic vars can hold anything.

		}
		else { // STATIC or CLASS_INIT
			// Check compatibility if both exist
			if (declaredType && initializerType) {
				if (!areTypesCompatible(declaredType.get(), initializerType.get())) {
					errorReporter.report(node.initializer->loc,
					                     "Initializer type '" + typeToString(initializerType.get()) +
					                     "' is not compatible with declared variable type '" + typeToString(declaredType.get()) + "'.");
					// Handle error: maybe finalType stays declaredType, or becomes an error type?
					finalType = std::move(declaredType); // Tentatively keep declared type despite error
				}
				else {
					finalType = std::move(declaredType); // Types are compatible, use declared type
				}
			}
			else if (declaredType) {
				finalType = std::move(declaredType);
			}
			else if (initializerType) { // (type inference)
				finalType = std::move(initializerType);
			}
			else {
				// Neither type nor initializer - Error (unless language allows this for static?)
				errorReporter.report(node.loc, "Variable '" + node.name + "' must have a type or an initializer for static declaration.");
				return;
			}
		}

		if (!finalType) {
			errorReporter.report(node.loc, "Could not determine type for variable '" + node.name + "'.");
			finalType = getErrorTypeNode(node.loc);
		}


		// Declare in symbol table
		if (finalType) {
			symbolTable.declare(node.name, SymbolInfo(SymbolInfo::VARIABLE, std::move(finalType), &node, node.isConst));
		}
	}

	std::unique_ptr<TypeNode> SemanticAnalyzer::getErrorTypeNode(const SourceLocation &loc) {
		return std::make_unique<TypeNode>(loc, TypeNode::ERROR);
	}

	void SemanticAnalyzer::visit(MultiVarDeclNode &node) {
		for (const auto &x: node.vars) {
			x->accept(*this);
		}
	}

	void SemanticAnalyzer::visit(FunctionDeclNode &node) {
		auto returnType = resolveType(node.returnType.get());

		//Params
		std::vector<std::unique_ptr<TypeNode>> paramTypes;
		paramTypes.reserve(node.params.size());
		for (const auto &param: node.params) {
			auto pType = resolveType(param.second.get());
			if (!pType) {
				errorReporter.report(param.second ? param.second->loc : node.loc, "Invalid type for parameter '" + param.first + "' in function '" + node.name + "'.");
				paramTypes.push_back(getErrorTypeNode(param.second->loc));
			}
			else {
				paramTypes.push_back(std::move(pType));
			}
		}
		SymbolInfo funcInfo(SymbolInfo::FUNCTION, std::move(returnType), &node);

		if (!node.name.empty()) {
			if (!symbolTable.declare(node.name, std::move(funcInfo))) {
				// Redeclaration error already reported by symbolTable.declare
				// Might want to stop processing this function? For now, continue
			}
		}
		FunctionDeclNode *previousFunction = currentFunction;
		currentFunction = &node;

		symbolTable.enterScope();

		for (size_t i = 0; i < node.params.size(); ++i) {
			const auto &param = node.params[i];
			auto pType = resolveType(param.second.get());
			if (!pType) {
				// Error already reported during pre-declaration type resolution
				pType = std::make_unique<TypeNode>(param.second ? param.second->loc : node.loc, TypeNode::Kind::DYNAMIC); // Assign dynamic as fallback
			}

			if (i < node.defaultValues.size() && node.defaultValues[i]) {
				auto defaultValueType = visitExpression(*node.defaultValues[i]);
				if (pType && defaultValueType && !areTypesCompatible(pType.get(), defaultValueType.get())) {
					errorReporter.report(node.defaultValues[i]->loc, "Default value type '" + typeToString(defaultValueType.get()) + "' is not compatible with parameter type '" + typeToString(pType.get()) + "'.");
				}
			}

			symbolTable.declare(param.first, SymbolInfo(SymbolInfo::VARIABLE, std::move(pType), param.second.get()));
		}
		if (node.body) {
			visit(*node.body);
			//TODO When in current function scope make sure a ReturnNode is found
		}
		symbolTable.exitScope();
		currentFunction = previousFunction;
	}

	void SemanticAnalyzer::visit(LambdaExprNode &node) {
		exprVR = nullptr;


		std::vector<std::unique_ptr<TypeNode>> paramTypes;
		bool paramError = false;
		paramTypes.reserve(node.lambda->params.size());
		for (const auto &param: node.lambda->params) {
			auto resolvedParamType = resolveType(param.second.get());
			if (!resolvedParamType) {
				errorReporter.report(param.second ? param.second->loc : node.loc,
				                     "Could not resolve type for lambda parameter '" + param.first + "'.");
				paramError = true;
				paramTypes.push_back(getErrorTypeNode(param.second ? param.second->loc : node.loc));
			}
			else {
				paramTypes.push_back(std::move(resolvedParamType));
			}
		}

		if (paramError) {
			exprVR = getErrorTypeNode(node.loc);
			return;
		}


		std::unique_ptr<TypeNode> returnType = nullptr;
		if (node.lambda->returnType) {
			returnType = resolveType(node.lambda->returnType.get());
			if (!returnType) {
				errorReporter.report(node.lambda->returnType->loc, "Could not resolve explicit return type for lambda.");
				exprVR = getErrorTypeNode(node.lambda->returnType->loc);
				return; // Early exit
			}
		}
		else if (node.lambda->body) {
			// TODO: Implement robust return type inference.
			errorReporter.report(node.loc, "Lambda return type inference not yet implemented. Please provide an explicit return type.");
			returnType = getErrorTypeNode(node.loc); // Use error type for now
		}
		else {
			errorReporter.report(node.loc, "Lambda must have an explicit return type or a body for inference.");
			exprVR = getErrorTypeNode(node.loc);
			return;
		}


		FunctionDeclNode *previousFunction = currentFunction;
		currentFunction = node.lambda.get();
		bool analysisError = false;

		symbolTable.enterScope();

		for (size_t i = 0; i < node.lambda->params.size(); ++i) {
			auto paramTypeForSymbol = paramTypes[i];
			if (!paramTypeForSymbol) {
				errorReporter.report(node.loc, "Internal error: Failed to clone type for lambda parameter symbol.");
				analysisError = true;
				paramTypeForSymbol = getErrorTypeNode(node.loc);
			}
			ASTNode *paramDeclNode = node.lambda->params[i].second.get();
			symbolTable.declare(node.lambda->params[i].first, SymbolInfo(SymbolInfo::VARIABLE, std::move(paramTypeForSymbol), paramDeclNode));
		}

		TypeNode *expectedReturnType = returnType.get();
		if (expectedReturnType && expectedReturnType->kind == TypeNode::ERROR) {
			analysisError = true;
		}

		if (node.lambda->body) {
			visit(*node.lambda->body);
			// TODO: Check error flags after visiting the body
			// if (/* check error state */) { analysisError = true; }
		}

		symbolTable.exitScope();
		currentFunction = previousFunction;

		// --- Step 4: Assign Final Type to Member exprVR ---

		if (analysisError) {
			exprVR = getErrorTypeNode(node.loc);
		}
		else {
			exprVR = std::make_unique<FunctionTypeNode>(node.loc, std::move(paramTypes), std::move(returnType));
		}
	}

	std::unique_ptr<TypeNode> SemanticAnalyzer::visitExpression(ExprNode &expr) {
		expr.accept(*this);
		return std::move(exprVR);
	}

	void SemanticAnalyzer::visit(ObjectDeclNode &node) {
		Visitor::visit(node);
	}

	void SemanticAnalyzer::visit(ReturnStmtNode &node) {
		if (!currentFunction) {
			errorReporter.report(node.loc, "'return' statement outside of function.");
			return;
		}

		auto expectedReturnType = resolveType(currentFunction->returnType.get());
		if (node.value) {
			auto actualReturnType = visitExpression(*node.value);

			if (!expectedReturnType) {
				errorReporter.report(node.loc, "Cannot return a value from a function with no return type (implicitly void).");
			}
			else if (!actualReturnType) {
				// Expression had an error, cannot check compatibility
				errorReporter.report(node.loc, "Expression had an error, cannot check compatibility");
			}
			else if (!areTypesCompatible(expectedReturnType.get(), actualReturnType.get())) {
				errorReporter.report(node.value->loc,
				                     "Return type '" + typeToString(actualReturnType.get()) +
				                     "' is not compatible with function's declared return type '" + typeToString(expectedReturnType.get()) + "'.");
			}
		}
		else {
			// Return without value
			if (expectedReturnType) {
				bool isVoid = false;
				if (auto *prim = dynamic_cast<PrimitiveTypeNode *>(expectedReturnType.get())) {
					isVoid = (prim->type == PrimitiveTypeNode::VOID);
				}
				if (!isVoid) {
					errorReporter.report(node.loc, "Must return a value from a function with declared return type '" + typeToString(expectedReturnType.get()) + "'.");
				}
			}
		}
	}

	bool SemanticAnalyzer::areTypesCompatible(const TypeNode *targetType, const TypeNode *valueType) {
		if (!targetType || !valueType) return false; //Nullptrs
		if (targetType->kind == TypeNode::ERROR || valueType->kind == TypeNode::ERROR) return false;
		if (targetType->isDynamic() || valueType->isDynamic()) return true;
		if (targetType->kind == valueType->kind) {
			switch (targetType->kind) {
				case TypeNode::PRIMITIVE: {
					auto *targetPrim = dynamic_cast<const PrimitiveTypeNode *>(targetType);
					auto *valuePrim = dynamic_cast<const PrimitiveTypeNode *>(valueType);
					if (!targetPrim || !valuePrim) return false;
					if (targetPrim->type == valuePrim->type) return true;
					static const std::unordered_set<PrimitiveTypeNode::Type> numericTypes = {
							PrimitiveTypeNode::INT, PrimitiveTypeNode::FLOAT, PrimitiveTypeNode::DOUBLE,
							PrimitiveTypeNode::SHORT, PrimitiveTypeNode::LONG, PrimitiveTypeNode::BYTE,
							PrimitiveTypeNode::NUMBER, PrimitiveTypeNode::BIGINT, PrimitiveTypeNode::BIGNUMBER
					};
					if (numericTypes.contains(targetPrim->type) && numericTypes.contains(valuePrim->type)) {
						// Allow widening conversions (e.g., int to float, short to int)
						if (targetPrim->type == PrimitiveTypeNode::FLOAT && valuePrim->type == PrimitiveTypeNode::INT) {
							return true;
						}
						if (targetPrim->type == PrimitiveTypeNode::DOUBLE &&
						    (valuePrim->type == PrimitiveTypeNode::INT || valuePrim->type == PrimitiveTypeNode::FLOAT)) {
							return true;
						}
						if (targetPrim->type == PrimitiveTypeNode::INT &&
						    (valuePrim->type == PrimitiveTypeNode::SHORT || valuePrim->type == PrimitiveTypeNode::BYTE)) {
							return true;
						}
						if (targetPrim->type == PrimitiveTypeNode::LONG &&
						    (valuePrim->type == PrimitiveTypeNode::INT || valuePrim->type == PrimitiveTypeNode::SHORT ||
						     valuePrim->type == PrimitiveTypeNode::BYTE)) {
							return true;
						}
					}
					return false;
				}
				case TypeNode::OBJECT: {
					auto* targetObj = dynamic_cast<const NamedTypeNode*>(targetType);
					auto* valueObj = dynamic_cast<const NamedTypeNode*>(valueType);
					if (!targetObj || !valueObj) {
						return false;
					}

					// Exact same name
					if (targetObj->name == valueObj->name) {
						return true;
					}

					// Check inheritance (base class compatibility)
					const SymbolInfo* valueSymbol = symbolTable.lookup(valueObj->name, SymbolInfo::OBJECT);
					if (valueSymbol && valueSymbol->declarationNode) {
						// Ensure the declaration is an ObjectDeclNode
						auto* valueDecl = dynamic_cast<ObjectDeclNode*>(valueSymbol->declarationNode);
						if (valueDecl && !valueDecl->base.empty()) {
							// Recursively check if the base class is compatible
							auto baseType = std::make_unique<NamedTypeNode>(valueObj->loc, valueDecl->base);
							return areTypesCompatible(targetType, baseType.get());
						}
					}
					return false;
				}
				case TypeNode::ARRAY: {
					auto* targetArr = dynamic_cast<const ArrayTypeNode*>(targetType);
					auto* valueArr = dynamic_cast<const ArrayTypeNode*>(valueType);
					if (!targetArr || !valueArr) {
						return false;
					}
					// Element types must be compatible
					return areTypesCompatible(targetArr->elementType.get(), valueArr->elementType.get());
				}
				case TypeNode::FUNCTION: {
					auto* targetFunc = dynamic_cast<const FunctionTypeNode*>(targetType);
					auto* valueFunc = dynamic_cast<const FunctionTypeNode*>(valueType);
					if (!targetFunc || !valueFunc) {
						return false;
					}

					// Check return type compatibility
					if (!areTypesCompatible(targetFunc->returnType.get(), valueFunc->returnType.get())) {
						return false;
					}

					// Check parameter count
					if (targetFunc->parameterTypes.size() != valueFunc->parameterTypes.size()) {
						return false;
					}

					// Check parameter type compatibility
					for (size_t i = 0; i < targetFunc->parameterTypes.size(); ++i) {
						if (!areTypesCompatible(targetFunc->parameterTypes[i].get(), valueFunc->parameterTypes[i].get())) {
							return false;
						}
					}
					return true;
				}
				case TypeNode::TEMPLATE: {
					auto* targetTemp = dynamic_cast<const TemplateTypeNode*>(targetType);
					auto* valueTemp = dynamic_cast<const TemplateTypeNode*>(valueType);
					if (!targetTemp || !valueTemp) {
						return false;
					}

					// Same base name and number of template arguments
					if (targetTemp->baseName != valueTemp->baseName ||
					    targetTemp->templateArgs.size() != valueTemp->templateArgs.size()) {
						return false;
					}

					// Check compatibility of each template argument
					for (size_t i = 0; i < targetTemp->templateArgs.size(); ++i) {
						if (!areTypesCompatible(targetTemp->templateArgs[i].get(), valueTemp->templateArgs[i].get())) {
							return false;
						}
					}
					return true;
				}
				default:
					return false;
			}
		}
		// Special case: Allow null (NIL) to be assigned to object types
		if (targetType->kind == TypeNode::OBJECT && valueType->kind == TypeNode::PRIMITIVE) {
			auto* valuePrim = dynamic_cast<const PrimitiveTypeNode*>(valueType);
			if (valuePrim && valuePrim->type == PrimitiveTypeNode::NIL) {
				return true;
			}
		}
		return false;
	}

	std::string SemanticAnalyzer::typeToString(const TypeNode *type) {
		if (!type) return "<nullptr>";

		switch (type->kind) {
			case TypeNode::PRIMITIVE: {
				auto* prim = dynamic_cast<const PrimitiveTypeNode*>(type);
				if (!prim) return "<invalid primitive>";

				static const char* typeNames[] = {
						"int", "float", "double", "string", "bool", "number",
						"bigint", "bignumber", "short", "long", "byte", "void", "nil"
				};
				return typeNames[prim->type];
			}

			case TypeNode::OBJECT: {
				auto* named = dynamic_cast<const NamedTypeNode*>(type);
				if (!named) return "<invalid object>";
				return named->name;
			}

			case TypeNode::ARRAY: {
				auto* arr = dynamic_cast<const ArrayTypeNode*>(type);
				if (!arr || !arr->elementType) return "<invalid array>";
				return typeToString(arr->elementType.get()) + "[]";
			}

			case TypeNode::FUNCTION: {
				auto* func = dynamic_cast<const FunctionTypeNode*>(type);
				if (!func) return "<invalid function>";

				std::string result = "(";
				for (size_t i = 0; i < func->parameterTypes.size(); ++i) {
					if (i > 0) result += ", ";
					result += typeToString(func->parameterTypes[i].get());
				}
				result += ") -> ";
				result += typeToString(func->returnType.get());
				return result;
			}

			case TypeNode::TEMPLATE: {
				auto* templ = dynamic_cast<const TemplateTypeNode*>(type);
				if (!templ) return "<invalid template>";

				std::string result = templ->baseName + "<";
				for (size_t i = 0; i < templ->templateArgs.size(); ++i) {
					if (i > 0) result += ", ";
					result += typeToString(templ->templateArgs[i].get());
				}
				result += ">";
				return result;
			}

			case TypeNode::DYNAMIC:
				return "dynamic";

			case TypeNode::ERROR:
				return "<error>";

			default:
				return "<unknown type>";
		}
	}

	std::unique_ptr<TypeNode> SemanticAnalyzer::resolveType(TypeNode *typeNode) {
		if (!typeNode) {
			return nullptr;
		}

		switch (typeNode->kind) {
			case TypeNode::OBJECT: {
				// Handle named types (classes, structs, unions)
				auto* namedType = dynamic_cast<NamedTypeNode*>(typeNode);
				if (!namedType) {
					errorReporter.report(typeNode->loc, "Invalid object type node");
					return getErrorTypeNode(typeNode->loc);
				}

				// Look up the type in the symbol table
				const SymbolInfo* typeInfo = symbolTable.lookup(namedType->name, SymbolInfo::OBJECT);
				if (!typeInfo) {
					errorReporter.report(typeNode->loc, "Unknown type '" + namedType->name + "'");
					return getErrorTypeNode(typeNode->loc);
				}

				// Return a clone of the resolved type
				return typeInfo->type->clone();
			}

			case TypeNode::ARRAY: {
				// Recursively resolve element type
				auto* arrayType = dynamic_cast<ArrayTypeNode*>(typeNode);
				if (!arrayType) {
					errorReporter.report(typeNode->loc, "Invalid array type node");
					return getErrorTypeNode(typeNode->loc);
				}

				auto resolvedElement = resolveType(arrayType->elementType.get());
				if (!resolvedElement) {
					return getErrorTypeNode(typeNode->loc);
				}

				return std::make_unique<ArrayTypeNode>(
						arrayType->loc,
						std::move(resolvedElement),
						arrayType->sizeExpr ? arrayType->sizeExpr->clone() : nullptr
				);
			}

			case TypeNode::TEMPLATE: {
				// Resolve template arguments
				auto* templateType = dynamic_cast<TemplateTypeNode*>(typeNode);
				if (!templateType) {
					errorReporter.report(typeNode->loc, "Invalid template type node");
					return getErrorTypeNode(typeNode->loc);
				}

				std::vector<std::unique_ptr<TypeNode>> resolvedArgs;
				resolvedArgs.reserve(templateType->templateArgs.size());

				for (auto& arg : templateType->templateArgs) {
					auto resolvedArg = resolveType(arg.get());
					if (!resolvedArg) {
						return getErrorTypeNode(typeNode->loc);
					}
					resolvedArgs.push_back(std::move(resolvedArg));
				}

				return std::make_unique<TemplateTypeNode>(
						templateType->loc,
						templateType->baseName,
						std::move(resolvedArgs)
				);
			}

			case TypeNode::FUNCTION: {
				// Resolve function parameter and return types
				auto* funcType = dynamic_cast<FunctionTypeNode*>(typeNode);
				if (!funcType) {
					errorReporter.report(typeNode->loc, "Invalid function type node");
					return getErrorTypeNode(typeNode->loc);
				}

				std::vector<std::unique_ptr<TypeNode>> resolvedParams;
				resolvedParams.reserve(funcType->parameterTypes.size());

				for (auto& param : funcType->parameterTypes) {
					auto resolvedParam = resolveType(param.get());
					if (!resolvedParam) {
						return getErrorTypeNode(typeNode->loc);
					}
					resolvedParams.push_back(std::move(resolvedParam));
				}

				auto resolvedReturn = funcType->returnType ?
				                      resolveType(funcType->returnType.get()) :
				                      std::make_unique<PrimitiveTypeNode>(funcType->loc, PrimitiveTypeNode::VOID);

				return std::make_unique<FunctionTypeNode>(
						funcType->loc,
						std::move(resolvedParams),
						std::move(resolvedReturn)
				);
			}

				// For primitive and dynamic types, just clone them
			case TypeNode::PRIMITIVE:
			case TypeNode::DYNAMIC:
				return typeNode->clone();

			case TypeNode::ERROR:
				return getErrorTypeNode(typeNode->loc);

			default:
				errorReporter.report(typeNode->loc, "Unsupported type kind in resolveType");
				return getErrorTypeNode(typeNode->loc);
		}
	}

} // namespace zenith