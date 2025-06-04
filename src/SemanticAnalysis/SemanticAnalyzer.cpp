#include "SemanticAnalyzer.hpp"

namespace zenith {

	SymbolTable&& SemanticAnalyzer::analyze(ProgramNode &program) {
		visit(program);
		return std::move(symbolTable);
	}

	void SemanticAnalyzer::visit(std_P3019_modified::polymorphic<ProgramNode> node) {
		for (auto &x: node->declarations) {
			x->accept(*this);
		}
		symbolTable.exitScope();
	}

	void SemanticAnalyzer::visit(std_P3019_modified::polymorphic<ASTNode> node) {
		errorReporter.report(node->loc, "Unhandled node type",{"Internal error", RED_TEXT});
	}

	void SemanticAnalyzer::visit(std_P3019_modified::polymorphic<ImportNode> node) {
		errorReporter.report(node->loc, "Import are not handled until I extract entire process to blah, blah blah", {"warning", YELLOW_TEXT});
	}

	void SemanticAnalyzer::visit(std_P3019_modified::polymorphic<BlockNode> node) {
		symbolTable.enterScope();
		for (auto &x: node->statements) {
			x->accept(*this);
		}
		symbolTable.exitScope();
	}

	void SemanticAnalyzer::visit(std_P3019_modified::polymorphic<VarDeclNode> node) {
		std_P3019_modified::polymorphic<TypeNode> declaredType = node->type ? resolveType(node->type) : nullptr;
		std_P3019_modified::polymorphic<TypeNode> initializerType = nullptr;

		if (node->initializer) {
			initializerType = visitExpression(node->initializer);
			if (!initializerType) {
				errorReporter.report(node->initializer->loc, "Could not evaluate initializer");
			}
		}

		std_P3019_modified::polymorphic<TypeNode> finalType = nullptr;
		static auto dynamicType = std_P3019_modified::make_polymorphic<TypeNode>(node->loc, TypeNode::DYNAMIC);
		static auto errorType = std_P3019_modified::make_polymorphic<TypeNode>(node->loc, TypeNode::ERROR);

		// Handle dynamic variables
		if (node->kind == VarDeclNode::DYNAMIC) {
			finalType = dynamicType.share();

			if (declaredType && !declaredType->isDynamic()) {
				errorReporter.report(node->type->loc,
				                     "Explicit type conflicts with 'dynamic' keyword.");
			}
		}
			// Handle static/class_init variables
		else {
			if (declaredType && initializerType) {
				if (!areTypesCompatible(declaredType, initializerType)) {
					errorReporter.report(node->initializer->loc,
					                     "Initializer type '" + typeToString(initializerType) +
					                     "' is not compatible with declared variable type '" +
					                     typeToString(declaredType) + "'.");
					finalType = std::move(declaredType); // Keep declared type despite error
				} else {
					finalType = std::move(declaredType);
				}
			}
			else if (declaredType) {
				finalType = std::move(declaredType);
			}
			else if (initializerType) {
				finalType = std::move(initializerType);
			}
			else {
				errorReporter.report(node->loc,
				                     "Variable '" + node->name +
				                     "' must have a type or an initializer for static declaration.");
				finalType = errorType.share();
			}
		}

		// Declare in symbol table
		symbolTable.declare(node->name,
		                    SymbolInfo(SymbolInfo::VARIABLE, finalType, node.share(), node->isConst));
	}

	std_P3019_modified::polymorphic<TypeNode> SemanticAnalyzer::getErrorTypeNode(const SourceLocation &loc) {
		static auto errorNode = std_P3019_modified::make_polymorphic<TypeNode>(loc, TypeNode::ERROR);
		return errorNode.share();
	}

	void SemanticAnalyzer::visit(std_P3019_modified::polymorphic<MultiVarDeclNode> node) {
		for (auto &x: node->vars) {
			x->accept(*this);
		}
	}

	void SemanticAnalyzer::visit(std_P3019_modified::polymorphic<FunctionDeclNode> node) {
		auto returnType = resolveType(node->returnType);

		//Params
		std::vector<std_P3019_modified::polymorphic<TypeNode>> paramTypes;
		paramTypes.reserve(node->params.size());
		for (const auto &param: node->params) {
			auto pType = resolveType(param->second);
			if (!pType) {
				errorReporter.report(param.second ? param.second->loc : node->loc, "Invalid type for parameter '" + param.first + "' in function '" + node->name + "'.");
				paramTypes.push_back(getErrorTypeNode(param.second ? param.second->loc : node->loc));
			}
			else {
				paramTypes.push_back(pType);
			}
		}
		SymbolInfo funcInfo(SymbolInfo::FUNCTION, returnType, node.share());

		if (!node->name.empty()) {
			if (!symbolTable.declare(node->name, std::move(funcInfo))) {
				// Redeclaration error already reported by symbolTable.declare
				// Might want to stop processing this function? For now, continue
				errorReporter.report(node->loc,"[Overloading is unimplemented]Redeclaration of function " + node->name);
			}
		}
		auto previousFunction = std::move(currentFunction);
		currentFunction = node.share();

		symbolTable.enterScope();

		for (size_t i = 0; i < node->params.size(); ++i) {
			const auto &param = node->params[i];
			auto pType = resolveType(param.second);
			if (!pType) {
				// Error already reported during pre-declaration type resolution
				auto dynamicType = std_P3019_modified::make_polymorphic<TypeNode>(param.second ? param.second->loc : node->loc, TypeNode::DYNAMIC);
				pType = dynamicType.share();
			}

			if (i < node->defaultValues.size() && node->defaultValues[i]) {
				auto defaultValueType = visitExpression(*node->defaultValues[i]);
				if (defaultValueType && !areTypesCompatible(pType, defaultValueType)) {
					errorReporter.report(node->defaultValues[i]->loc, "Default value type '" + typeToString(defaultValueType) + "' is not compatible with parameter type '" + typeToString(pType) + "'.");
				}
			}

			symbolTable.declare(param.first, SymbolInfo(SymbolInfo::VARIABLE, std::move(pType), param.second.share()));
		}
		if (node->body) {
			visit(*node->body);
			//TODO When in current function scope make sure a ReturnNode is found
		}
		symbolTable.exitScope();
		currentFunction = std::move(previousFunction);
	}

	void SemanticAnalyzer::visit(std_P3019_modified::polymorphic<LambdaExprNode> node) {
		exprVR = nullptr;

		std::vector<std_P3019_modified::polymorphic<TypeNode>> paramTypes;
		bool paramError = false;
		paramTypes.reserve(node->lambda->params.size());
		for (const auto &param: node->lambda->params) {
			auto resolvedParamType = resolveType(param.second);
			if (!resolvedParamType) {
				errorReporter.report(param.second ? param.second->loc : node->loc,
				                     "Could not resolve type for lambda parameter '" + param.first + "'.");
				paramError = true;
				paramTypes.push_back(getErrorTypeNode(param.second ? param.second->loc : node->loc));
			}
			else {
				paramTypes.push_back(resolvedParamType);
			}
		}

		if (paramError) {
			exprVR = getErrorTypeNode(node->loc);
			return;
		}

		std_P3019_modified::polymorphic<TypeNode> returnType = nullptr;
		if (node->lambda->returnType) {
			returnType = resolveType(node->lambda->returnType);
			if (!returnType) {
				errorReporter.report(node->lambda->returnType->loc, "Could not resolve explicit return type for lambda.");
				exprVR = getErrorTypeNode(node->lambda->returnType->loc);
				return; // Early exit
			}
		}
		else if (node->lambda->body) {
			// TODO: Implement robust return type inference.
			errorReporter.report(node->loc, "Lambda return type inference not yet implemented. Please provide an explicit return type.");
			returnType = getErrorTypeNode(node->loc); // Use error type for now
		}
		else {
			errorReporter.report(node->loc, "Lambda must have an explicit return type or a body for inference.");
			exprVR = getErrorTypeNode(node->loc);
			return;
		}

		auto previousFunction = std::move(currentFunction);
		currentFunction = node->lambda.share();
		bool analysisError = false;

		symbolTable.enterScope();

		for (size_t i = 0; i < node->lambda->params.size(); ++i) {
			auto paramTypeForSymbol =std::move(paramTypes[i]);
			if (!paramTypeForSymbol) {
				errorReporter.report(node->loc, "Internal error: Failed to clone type for lambda parameter symbol.");
				analysisError = true;
				paramTypeForSymbol = getErrorTypeNode(node->loc);
			}
			auto paramDeclNode = node->lambda->params[i].second;
			symbolTable.declare(node->lambda->params[i].first, SymbolInfo(SymbolInfo::VARIABLE, std::move(paramTypeForSymbol), std::move(paramDeclNode)));
		}

		auto expectedReturnType = returnType;
		if (expectedReturnType && expectedReturnType->kind == TypeNode::ERROR) {
			analysisError = true;
		}

		if (node->lambda->body) {
			visit(*node->lambda->body);
			// TODO: Check error flags after visiting the body
			// if (/* check error state */) { analysisError = true; }
		}

		symbolTable.exitScope();
		currentFunction = previousFunction;

		// --- Step 4: Assign Final Type to Member exprVR ---
		if (analysisError) {
			exprVR = getErrorTypeNode(node->loc);
		}
		else {
			exprVR = std_P3019_modified::make_polymorphic<FunctionTypeNode>(node->loc, std::move(paramTypes), returnType);
		}
	}

	std_P3019_modified::polymorphic<TypeNode> SemanticAnalyzer::visitExpression(ExprNode &expr) {
		expr.accept(*this);
		return exprVR;
	}

	void SemanticAnalyzer::visit(std_P3019_modified::polymorphic<ObjectDeclNode> node) {
		// Save current class context
		auto previousClass = std::move(currentClass);
		currentClass = node.share();

		// Enter class scope
		symbolTable.enterScope();

		// Handle inheritance if base class is specified
		if (!node->base.empty()) {
			// Look up base class in symbol table
			const SymbolInfo* baseInfo = symbolTable.lookup(node->base, SymbolInfo::OBJECT);
			if (!baseInfo) {
				errorReporter.report(node->loc, "Base class '" + node->base + "' not found");
			}
		}

		// Process all members
		for (auto& member : node->members) {
			member->accept(*this);
		}

		// Process operator overloads
		for (auto& op : node->operators) {
			op->accept(*this);
		}

		// Exit class scope
		symbolTable.exitScope();

		// Restore previous class context
		currentClass = std::move(previousClass);
	}

	void SemanticAnalyzer::visit(std_P3019_modified::polymorphic<ReturnStmtNode> node) {
		if (!currentFunction) {
			errorReporter.report(node->loc, "'return' statement outside of function.");
			return;
		}

		auto expectedReturnType = resolveType(currentFunction->returnType);
		if (node->value) {
			auto actualReturnType = visitExpression(node->value);

			if (!expectedReturnType) {
				errorReporter.report(node->loc, "Cannot return a value from a function with no return type (implicitly void).");
			}
			else if (!actualReturnType) {
				// Expression had an error, cannot check compatibility
				errorReporter.report(node->loc, "Expression had an error, cannot check compatibility");
			}
			else if (!areTypesCompatible(expectedReturnType, actualReturnType)) {
				errorReporter.report(node->value->loc,
				                     "Return type '" + typeToString(actualReturnType) +
				                     "' is not compatible with function's declared return type '" + typeToString(expectedReturnType) + "'.");
			}
		}
		else {
			// Return without value
			if (expectedReturnType) {
				bool isVoid = false;
				if (auto prim = dynamic_cast<PrimitiveTypeNode*>(expectedReturnType.get())) {
					isVoid = (prim->type == PrimitiveTypeNode::VOID);
				}
				if (!isVoid) {
					errorReporter.report(node->loc, "Must return a value from a function with declared return type '" + typeToString(expectedReturnType) + "'.");
				}
			}
		}
	}

	bool SemanticAnalyzer::areTypesCompatible(std_P3019_modified::polymorphic<TypeNode>& targetType,
	                                          std_P3019_modified::polymorphic<TypeNode>& valueType) {
		if (!targetType || !valueType) return false; // Nullptrs
		if (targetType->kind == TypeNode::ERROR || valueType->kind == TypeNode::ERROR) return false;
		if (targetType->isDynamic() || valueType->isDynamic()) return true;

		if (targetType->kind == valueType->kind) {
			switch (targetType->kind) {
				case TypeNode::PRIMITIVE: {
					auto targetPrim = targetType.try_as_interface<PrimitiveTypeNode>();
					auto valuePrim = valueType.try_as_interface<PrimitiveTypeNode>();
					if (!targetPrim || !valuePrim) return false;

					if ((*targetPrim)->type == (*valuePrim)->type) return true;

					static const std::unordered_set<PrimitiveTypeNode::Type> numericTypes = {
							PrimitiveTypeNode::INT, PrimitiveTypeNode::FLOAT, PrimitiveTypeNode::DOUBLE,
							PrimitiveTypeNode::SHORT, PrimitiveTypeNode::LONG, PrimitiveTypeNode::BYTE,
							PrimitiveTypeNode::NUMBER, PrimitiveTypeNode::BIGINT, PrimitiveTypeNode::BIGNUMBER
					};

					if (numericTypes.contains((*targetPrim)->type) && numericTypes.contains((*valuePrim)->type)) {
						// Allow widening conversions
						if ((*targetPrim)->type == PrimitiveTypeNode::FLOAT && (*valuePrim)->type == PrimitiveTypeNode::INT) {
							return true;
						}
						if ((*targetPrim)->type == PrimitiveTypeNode::DOUBLE &&
						    ((*valuePrim)->type == PrimitiveTypeNode::INT || (*valuePrim)->type == PrimitiveTypeNode::FLOAT)) {
							return true;
						}
						if ((*targetPrim)->type == PrimitiveTypeNode::INT &&
						    ((*valuePrim)->type == PrimitiveTypeNode::SHORT || (*valuePrim)->type == PrimitiveTypeNode::BYTE)) {
							return true;
						}
						if ((*targetPrim)->type == PrimitiveTypeNode::LONG &&
						    ((*valuePrim)->type == PrimitiveTypeNode::INT || (*valuePrim)->type == PrimitiveTypeNode::SHORT ||
						     (*valuePrim)->type == PrimitiveTypeNode::BYTE)) {
							return true;
						}
					}
					return false;
				}

				case TypeNode::OBJECT: {
					auto targetObj = targetType.dynamic_get<NamedTypeNode>();
					auto valueObj = valueType.dynamic_get<NamedTypeNode>();
					if (!targetObj || !valueObj) return false;

					// Exact same name
					if (targetObj->name == valueObj->name) return true;

					// Check inheritance
					const SymbolInfo* valueSymbol = symbolTable.lookup(valueObj->name, SymbolInfo::OBJECT);
					if (valueSymbol && valueSymbol->declarationNode) {
						auto valueDecl = valueSymbol->declarationNode.dynamic_get<ObjectDeclNode>();
						if (valueDecl && !valueDecl->base.empty()) {
							auto baseType = std_P3019_modified::make_polymorphic<NamedTypeNode>(valueObj->loc, valueDecl->base);
							return areTypesCompatible(targetType, baseType);
						}
					}
					return false;
				}

				case TypeNode::ARRAY: {
					auto targetArr = targetType.dynamic_get<ArrayTypeNode>();
					auto valueArr = valueType.dynamic_get<ArrayTypeNode>();
					if (!targetArr || !valueArr) return false;

					return areTypesCompatible(targetArr->elementType, valueArr->elementType);
				}

				case TypeNode::FUNCTION: {
					auto targetFunc = targetType.dynamic_get<FunctionTypeNode>();
					auto valueFunc = valueType.dynamic_get<FunctionTypeNode>();
					if (!targetFunc || !valueFunc) return false;

					// Check return type compatibility
					if (!areTypesCompatible(targetFunc->returnType, valueFunc->returnType)) {
						return false;
					}

					// Check parameter count
					if (targetFunc->parameterTypes.size() != valueFunc->parameterTypes.size()) {
						return false;
					}

					// Check parameter type compatibility
					for (size_t i = 0; i < targetFunc->parameterTypes.size(); ++i) {
						if (!areTypesCompatible(targetFunc->parameterTypes[i], valueFunc->parameterTypes[i])) {
							return false;
						}
					}
					return true;
				}

				case TypeNode::TEMPLATE: {
					auto targetTemp = targetType.dynamic_get<TemplateTypeNode>();
					auto valueTemp = valueType.dynamic_get<TemplateTypeNode>();
					if (!targetTemp || !valueTemp) return false;

					// Same base name and number of template arguments
					if (targetTemp->baseName != valueTemp->baseName ||
					    targetTemp->templateArgs.size() != valueTemp->templateArgs.size()) {
						return false;
					}

					// Check compatibility of each template argument
					for (size_t i = 0; i < targetTemp->templateArgs.size(); ++i) {
						if (!areTypesCompatible(targetTemp->templateArgs[i], valueTemp->templateArgs[i])) {
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
			auto valuePrim = valueType.try_as_interface<PrimitiveTypeNode>();
			if (valuePrim && (*valuePrim)->type == PrimitiveTypeNode::NIL) {
				return true;
			}
		}

		return false;
	}

	std::string SemanticAnalyzer::typeToString(std_P3019_modified::polymorphic<TypeNode>& type) {
		if (!type) return "<nullptr>";

		switch (type->kind) {
			case TypeNode::PRIMITIVE: {
				auto prim = dynamic_cast<const PrimitiveTypeNode*>(type.get());
				if (!prim) return "<invalid primitive>";

				static const char* typeNames[] = {
						"int", "float", "double", "string", "bool", "number",
						"bigint", "bignumber", "short", "long", "byte", "void", "nil"
				};
				return typeNames[prim->type];
			}

			case TypeNode::OBJECT: {
				auto named = dynamic_cast<const NamedTypeNode*>(type.get());
				if (!named) return "<invalid object>";
				return named->name;
			}

			case TypeNode::ARRAY: {
				auto arr = dynamic_cast<const ArrayTypeNode*>(type.get());
				if (!arr || !arr->elementType) return "<invalid array>";
				return typeToString(arr->elementType) + "[]";
			}

			case TypeNode::FUNCTION: {
				auto func = dynamic_cast<const FunctionTypeNode*>(type.get());
				if (!func) return "<invalid function>";

				std::string result = "(";
				for (size_t i = 0; i < func->parameterTypes.size(); ++i) {
					if (i > 0) result += ", ";
					result += typeToString(func->parameterTypes[i]);
				}
				result += ") -> ";
				result += typeToString(func->returnType);
				return result;
			}

			case TypeNode::TEMPLATE: {
				auto templ = dynamic_cast<const TemplateTypeNode*>(type.get());
				if (!templ) return "<invalid template>";

				std::string result = templ->baseName + "<";
				for (size_t i = 0; i < templ->templateArgs.size(); ++i) {
					if (i > 0) result += ", ";
					result += typeToString(templ->templateArgs[i]);
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

	void SemanticAnalyzer::visit(std_P3019_modified::polymorphic<LiteralNode> node) {
		auto numberType = std_P3019_modified::make_polymorphic<PrimitiveTypeNode>(node->loc, PrimitiveTypeNode::NUMBER);
		auto stringType = std_P3019_modified::make_polymorphic<PrimitiveTypeNode>(node->loc, PrimitiveTypeNode::STRING);
		auto boolType = std_P3019_modified::make_polymorphic<PrimitiveTypeNode>(node->loc, PrimitiveTypeNode::BOOL);
		auto nilType = std_P3019_modified::make_polymorphic<PrimitiveTypeNode>(node->loc, PrimitiveTypeNode::NIL);
		switch (node->type) {
			case LiteralNode::NUMBER:
				exprVR = numberType.share(); break;
			case LiteralNode::STRING:
				exprVR = stringType.share(); break;
			case LiteralNode::BOOL:
				exprVR = boolType.share(); break;
			case LiteralNode::NIL:
				exprVR = nilType.share(); break;
		}
	}

	void SemanticAnalyzer::visit(std_P3019_modified::polymorphic<VarNode> node) {
		if (auto symbol = symbolTable.lookup(node->name)) {
			exprVR = symbol->type;
		} else {
			errorReporter.report(node->loc, "Undeclared variable '" + node->name + "'");
			exprVR = getErrorTypeNode(node->loc);
		}
	}

	void SemanticAnalyzer::visit(std_P3019_modified::polymorphic<BinaryOpNode> node) {
		auto leftType = visitExpression(*node.left);
		auto rightType = visitExpression(*node.right);

		// Check for assignment operations
		if (node.op >= BinaryOpNode::ASN && node.op <= BinaryOpNode::MOD_ASN) {
			// For assignments, left must be an lvalue
			if (!areTypesCompatible(leftType, rightType)) {
				errorReporter.report(node->loc,
				                     "Type mismatch in assignment. Left type: " +
				                     typeToString(leftType) + ", right type: " +
				                     typeToString(rightType));
			}
			exprVR = leftType;
			return;
		}

		// For other operations, types must be compatible
		if (!areTypesCompatible(leftType, rightType)) {
			errorReporter.report(node->loc,
			                     "Type mismatch in binary operation. Left type: " +
			                     typeToString(leftType) + ", right type: " +
			                     typeToString(rightType));
		}

		// Result type is the more general of the two
		exprVR = leftType;
	}

} // namespace zenith