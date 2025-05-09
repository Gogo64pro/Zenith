#include "SemanticAnalyzer.hpp"

namespace zenith {

	SymbolTable&& SemanticAnalyzer::analyze(ProgramNode &program) {
		visit(program);
		return std::move(symbolTable);
	}

	void SemanticAnalyzer::visit(ProgramNode &node) {
		for (auto &x: node.declarations) {
			x->accept(*this);
		}
		symbolTable.exitScope();
	}

	void SemanticAnalyzer::visit(ASTNode &node) {
		errorReporter.report(node.loc, "Unhandled node type",{"Internal error", RED_TEXT});
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

	void SemanticAnalyzer::visit(VarDeclNode& node) {
		TypeNode* declaredType = node.type ? resolveType(node.type.get()) : nullptr;
		TypeNode* initializerType = nullptr;

		if (node.initializer) {
			initializerType = visitExpression(*node.initializer).first;
			if (!initializerType) {
				errorReporter.report(node.initializer->loc, "Could not evaluate initializer");
			}
		}

		TypeNode* finalType = nullptr;
		static TypeNode dynamicType(node.loc, TypeNode::DYNAMIC);
		static TypeNode errorType(node.loc, TypeNode::ERROR);

		// Handle dynamic variables
		if (node.kind == VarDeclNode::DYNAMIC) {
			finalType = &dynamicType;

			if (declaredType && !declaredType->isDynamic()) {
				errorReporter.report(node.type->loc,
				                     "Explicit type conflicts with 'dynamic' keyword.");
			}
		}
			// Handle static/class_init variables
		else {
			if (declaredType && initializerType) {
				if (!areTypesCompatible(declaredType, initializerType)) {
					errorReporter.report(node.initializer->loc,
					                     "Initializer type '" + typeToString(initializerType) +
					                     "' is not compatible with declared variable type '" +
					                     typeToString(declaredType) + "'.");
					finalType = declaredType; // Keep declared type despite error
				} else {
					finalType = declaredType;
				}
			}
			else if (declaredType) {
				finalType = declaredType;
			}
			else if (initializerType) {
				finalType = initializerType;
			}
			else {
				errorReporter.report(node.loc,
				                     "Variable '" + node.name +
				                     "' must have a type or an initializer for static declaration.");
				finalType = &errorType;
			}
		}

		// Declare in symbol table
		symbolTable.declare(node.name,
		                    SymbolInfo(SymbolInfo::VARIABLE, finalType, &node, node.isConst));
	}

	TypeNode* SemanticAnalyzer::getErrorTypeNode(const SourceLocation &loc) {
		static TypeNode errorNode = TypeNode(loc, TypeNode::ERROR);
		return &errorNode;
	}

	void SemanticAnalyzer::visit(MultiVarDeclNode &node) {
		for (const auto &x: node.vars) {
			x->accept(*this);
		}
	}

	void SemanticAnalyzer::visit(FunctionDeclNode &node) {
		auto returnType = resolveType(node.returnType.get());

		//Params
		std::vector<TypeNode*> paramTypes;
		paramTypes.reserve(node.params.size());
		for (const auto &param: node.params) {
			auto pType = resolveType(param.second.get());
			if (!pType) {
				errorReporter.report(param.second ? param.second->loc : node.loc, "Invalid type for parameter '" + param.first + "' in function '" + node.name + "'.");
				paramTypes.push_back(getErrorTypeNode(param.second->loc));
			}
			else {
				paramTypes.push_back(pType);
			}
		}
		SymbolInfo funcInfo(SymbolInfo::FUNCTION, returnType, &node);

		if (!node.name.empty()) {
			if (!symbolTable.declare(node.name, std::move(funcInfo))) {
				// Redeclaration error already reported by symbolTable.declare
				// Might want to stop processing this function? For now, continue
				errorReporter.report(node.loc,"[Overloading is unimplemented]Redeclaration of function " + node.name);
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
				static TypeNode dynamicType(param.second ? param.second->loc : node.loc,TypeNode::DYNAMIC);
				pType = &dynamicType;
			}

			if (i < node.defaultValues.size() && node.defaultValues[i]) {
				auto [defaultValueType, dynamicH] = visitExpression(*node.defaultValues[i]);
				if (defaultValueType && !areTypesCompatible(pType, defaultValueType)) {
					errorReporter.report(node.defaultValues[i]->loc, "Default value type '" + typeToString(defaultValueType) + "' is not compatible with parameter type '" + typeToString(pType) + "'.");
				}
			}

			symbolTable.declare(param.first, SymbolInfo(SymbolInfo::VARIABLE, pType, param.second.get()));
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


		std::vector<TypeNode*> paramTypes;
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
				paramTypes.push_back(resolvedParamType);
			}
		}

		if (paramError) {
			exprVR = getErrorTypeNode(node.loc);
			return;
		}


		TypeNode* returnType = nullptr;
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
			symbolTable.declare(node.lambda->params[i].first, SymbolInfo(SymbolInfo::VARIABLE, paramTypeForSymbol, paramDeclNode));
		}

		TypeNode *expectedReturnType = returnType;
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
			dynamicType = std::make_unique<FunctionTypeNode>(node.loc, std::move(paramTypes), returnType);
			exprVR = dynamicType.get();
		}
	}

	std::pair<TypeNode*, std::unique_ptr<TypeNode>> SemanticAnalyzer::visitExpression(ExprNode &expr) {
		expr.accept(*this);
		return {exprVR,std::move(dynamicType)};
	}

	void SemanticAnalyzer::visit(ObjectDeclNode &node) {
		// Save current class context
		ObjectDeclNode* previousClass = currentClass;
		currentClass = &node;

		// Enter class scope
		symbolTable.enterScope();

		// Handle inheritance if base class is specified
		if (!node.base.empty()) {
			// Look up base class in symbol table
			const SymbolInfo* baseInfo = symbolTable.lookup(node.base, SymbolInfo::OBJECT);
			if (!baseInfo) {
				errorReporter.report(node.loc, "Base class '" + node.base + "' not found");
			}
		}

		// Process all members
		for (auto& member : node.members) {
			member->accept(*this);
		}

		// Process operator overloads
		for (auto& op : node.operators) {
			op->accept(*this);
		}

		// Exit class scope
		symbolTable.exitScope();

		// Restore previous class context
		currentClass = previousClass;
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
			else if (!areTypesCompatible(expectedReturnType, actualReturnType)) {
				errorReporter.report(node.value->loc,
				                     "Return type '" + typeToString(actualReturnType) +
				                     "' is not compatible with function's declared return type '" + typeToString(expectedReturnType) + "'.");
			}
		}
		else {
			// Return without value
			if (expectedReturnType) {
				bool isVoid = false;
				if (auto *prim = dynamic_cast<PrimitiveTypeNode *>(expectedReturnType)) {
					isVoid = (prim->type == PrimitiveTypeNode::VOID);
				}
				if (!isVoid) {
					errorReporter.report(node.loc, "Must return a value from a function with declared return type '" + typeToString(expectedReturnType) + "'.");
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

	void SemanticAnalyzer::visit(LiteralNode &node) {
		static PrimitiveTypeNode numberType(node.loc, PrimitiveTypeNode::NUMBER);
		static PrimitiveTypeNode stringType(node.loc, PrimitiveTypeNode::STRING);
		static PrimitiveTypeNode boolType(node.loc, PrimitiveTypeNode::BOOL);
		static PrimitiveTypeNode nilType(node.loc, PrimitiveTypeNode::NIL);
		switch (node.type) {
			case LiteralNode::NUMBER:
				exprVR = &numberType; break;
			case LiteralNode::STRING:
				exprVR = &stringType; break;
			case LiteralNode::BOOL:
				exprVR = &boolType; break;
			case LiteralNode::NIL:
				exprVR = &nilType; break;
		}
	}

	void SemanticAnalyzer::visit(VarNode &node) {
		if (auto symbol = symbolTable.lookup(node.name)) {
			exprVR = symbol->type;
		} else {
			errorReporter.report(node.loc, "Undeclared variable '" + node.name + "'");
			exprVR = getErrorTypeNode(node.loc);
		}
	}

	void SemanticAnalyzer::visit(BinaryOpNode &node) {
		auto leftType = visitExpression(*node.left);
		auto rightType = visitExpression(*node.right);

		// Check for assignment operations
		if (node.op >= BinaryOpNode::ASN && node.op <= BinaryOpNode::MOD_ASN) {
			// For assignments, left must be an lvalue
			if (!areTypesCompatible(leftType, rightType)) {
				errorReporter.report(node.loc,
				                     "Type mismatch in assignment. Left type: " +
				                     typeToString(leftType) + ", right type: " +
				                     typeToString(rightType));
			}
			exprVR = std::move(leftType);
			return;
		}

		// For other operations, types must be compatible
		if (!areTypesCompatible(leftType, rightType)) {
			errorReporter.report(node.loc,
			                     "Type mismatch in binary operation. Left type: " +
			                     typeToString(leftType) + ", right type: " +
			                     typeToString(rightType));
		}

		// Result type is the more general of the two
		exprVR = leftType;
	}

} // namespace zenith