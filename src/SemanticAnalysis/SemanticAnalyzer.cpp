module;
#include <ranges>
#include "exceptions/ErrorReporter.hpp"
#include <string>
#include <vector>
#include <unordered_set>
#define CREATE_ERROR_TYPE(loc) std::move(make_polymorphic_ref_owns<TypeNode>(loc, TypeNode::Kind::ERROR))
import zenith.core.polymorphic_ref;
import zenith.core.polymorphic;
import zenith.ast;
module zenith.semantic;
namespace zenith {


	SymbolTable &&SemanticAnalyzer::analyze(polymorphic_ref<ProgramNode> program) {
		program->accept(*this);
		return std::move(symbolTable);
	}

	void SemanticAnalyzer::visit(ProgramNode& node) {
		for (auto &x: node.declarations) {
			x->accept(*this);
		}
		symbolTable.exitScope();
	}

	void SemanticAnalyzer::visit(ASTNode& node) {
		Visitor::visit(node);
	}

	void SemanticAnalyzer::visit(ImportNode& node) {
		errorReporter.warning(node.loc, "Import are not handled");
	}

	void SemanticAnalyzer::visit(BlockNode& node) {
		symbolTable.enterScope();
		for (auto &x: node.statements) {
			x->accept(*this);
		}
		symbolTable.exitScope();
	}

	void SemanticAnalyzer::visit(VarDeclNode& node) {
		polymorphic_ref<TypeNode> declaredType = node.type ? resolveType(node.type) : nullptr;
		polymorphic_ref<TypeNode> initializerType = nullptr;

		if (node.initializer) {
			initializerType = visitExpression(node.initializer);
			if (!initializerType) {
				errorReporter.error(node.initializer->loc, "Could not evaluate initializer");
			}
		}

		polymorphic_ref<TypeNode> finalType = nullptr;


		// Handle dynamic variables
		if (node.kind == VarDeclNode::DYNAMIC) {
			finalType = make_polymorphic_ref_owns<TypeNode>(node.loc, TypeNode::Kind::DYNAMIC);

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
					                     std::string("Initializer type '") + typeToString(initializerType) +
					                     std::string("' is not compatible with declared variable type '") +
					                     typeToString(declaredType) + "'");
					finalType = std::move(declaredType); // Keep declared type despite error
				}
				else {
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
				errorReporter.report(node.loc,
				                     "Variable '" + node.name +
				                     "' must have a type or an initializer for static declaration");
				finalType = make_polymorphic_ref_owns<TypeNode>(node.loc, TypeNode::Kind::ERROR);
			}
		}

		// Declare in symbol table
		symbolTable.declare(node.name,
		                    SymbolInfo(SymbolInfo::VARIABLE, std::move(finalType), node, node.isConst));
	}

	void SemanticAnalyzer::visit(MultiVarDeclNode& node) {
		for (auto &x: node.vars) {
			x->accept(*this);
		}
	}

	void SemanticAnalyzer::visit(FunctionDeclNode& node) {
		auto returnType = resolveType(node.returnType);

		std::vector<polymorphic_ref<TypeNode>> paramTypes;
		paramTypes.reserve(node.params.size());
		for (auto &[name, type]: node.params) {
			auto pType = resolveType(type);
			if (!pType) {
				errorReporter.report(type ? type->loc : node.loc,
				                     "Unresolved type for parameter '" + name + "' in function '" + node.name);
				paramTypes.push_back(make_polymorphic_ref_owns<TypeNode>(type ? type->loc : node.loc, TypeNode::Kind::ERROR));
			}
			else {
				paramTypes.push_back(std::move(pType));
			}
		}
		SymbolInfo funcInfo(SymbolInfo::FUNCTION, std::move(returnType), node);

		if (!node.name.empty()) {
			// Redeclaration error already reported by symbolTable.declare
			// Might want to stop processing this function? For now, continue
			errorReporter.
					report(node.loc, "[Overloading is unimplemented]Redeclaration of function " + node.name);
		}
		auto previousFunction = std::move(currentFunction);
		currentFunction = node;

		symbolTable.enterScope();

		for (size_t i = 0; i < node.params.size(); ++i) {
			auto &[name, type] = node.params[i];
			auto pType = resolveType(type);
			if (!pType) {
				// Error already reported during pre-declaration type resolution
				pType = make_polymorphic_ref_owns<TypeNode>(
					type ? type->loc : node.loc, TypeNode::Kind::DYNAMIC);;
			}

			if (i < node.defaultValues.size() && node.defaultValues[i]) {
				auto defaultValueType = visitExpression(node.defaultValues[i]);
				if (defaultValueType && !areTypesCompatible(pType, defaultValueType)) {
					errorReporter.report(node.defaultValues[i]->loc,
					                     "Default value type '" + typeToString(defaultValueType) +
					                     "' is not compatible with parameter type '" + typeToString(pType));
				}
			}

			symbolTable.declare(name, SymbolInfo(SymbolInfo::VARIABLE, std::move(pType), type));
		}
		if (node.body) {
			node.body->accept(*this);
			//TODO When in current function scope make sure a ReturnNode is found
		}
		symbolTable.exitScope();
		currentFunction = std::move(previousFunction);
	}

	void SemanticAnalyzer::visit(LambdaExprNode& node) {
		exprVR = nullptr;

		std::vector<polymorphic_ref<TypeNode>> paramTypes;
		bool paramError = false;
		paramTypes.reserve(node.lambda->params.size());
		for (auto &[name, param]: node.lambda->params) {
			if (auto resolvedParamType = resolveType(param); !resolvedParamType) {
				errorReporter.report(param ? param->loc : node.loc,
				                     "Could not resolve type for lambda parameter '" + name);
				paramError = true;
				paramTypes.push_back(make_polymorphic_ref_owns<TypeNode>(
					param ? param->loc : node.loc, TypeNode::Kind::ERROR));
			}
			else {
				paramTypes.push_back(std::move(resolvedParamType));
			}
		}

		if (paramError) {
			exprVR = make_polymorphic_ref_owns<TypeNode>(
					node.loc, TypeNode::Kind::ERROR);
			return;
		}

		polymorphic_ref<TypeNode> returnType = nullptr;
		if (node.lambda->returnType) {
			returnType = resolveType(node.lambda->returnType);
			if (!returnType) {
				errorReporter.report(node.lambda->returnType->loc,
				                     "Could not resolve explicit return type for lambda");
				exprVR = make_polymorphic_ref_owns<TypeNode>(node.lambda->returnType->loc, TypeNode::Kind::ERROR);
				return; // Early exit
			}
		}
		else if (node.lambda->body) {
			// TODO: Implement robust return type inference.
			errorReporter.report(
				node.loc, "Lambda return type inference not yet implemented. Please provide an explicit return type.");
			returnType = CREATE_ERROR_TYPE(node.loc); // Use error type for now
		}
		else {
			errorReporter.report(node.loc, "Lambda must have an explicit return type or a body for inference.");
			exprVR = CREATE_ERROR_TYPE(node.loc);
			return;
		}

		auto previousFunction = std::move(currentFunction);
		currentFunction = node.lambda;
		bool analysisError = false;

		symbolTable.enterScope();

		for (auto&& [i, pair] : std::views::enumerate(node.lambda->params)) {
			auto& [name, param] = pair;

			polymorphic_ref<TypeNode> paramTypeForSymbol = paramTypes[i];
			if (!paramTypeForSymbol) {
				errorReporter.internalError(node.loc, "Failed to clone type for lambda parameter symbol");
				analysisError = true;
				paramTypeForSymbol = CREATE_ERROR_TYPE(node.loc);
			}

			const polymorphic_ref<ASTNode> paramDeclNode = param;
			symbolTable.declare(
				name,
				SymbolInfo(SymbolInfo::VARIABLE,
						   paramTypeForSymbol,
						   paramDeclNode)
			);
		}

		if (auto expectedReturnType = returnType; expectedReturnType && expectedReturnType->kind == TypeNode::Kind::ERROR) {
			analysisError = true;
		}

		if (node.lambda->body) {
			node.lambda->body->accept(*this);
			// TODO: Check error flags after visiting the body
			// if (/* check error state */) { analysisError = true; }
		}

		symbolTable.exitScope();
		currentFunction = std::move(previousFunction);

		if (analysisError) {
			exprVR = CREATE_ERROR_TYPE(node.loc);
		}
		else {
			exprVR = make_polymorphic_ref_owns<FunctionTypeNode>(
				node.loc, std::move(paramTypes), std::move(returnType));
		}
	}

	polymorphic_ref<TypeNode> SemanticAnalyzer::visitExpression(
		polymorphic_ref<ExprNode> expr) {
		expr->accept(*this);
		return std::move(exprVR);
	}

	void SemanticAnalyzer::visit(ObjectDeclNode& node) {
		// Save current class context
		auto previousClass = std::move(currentClass);
		currentClass = node;

		// Enter class scope
		symbolTable.enterScope();

		// Handle inheritance if base class is specified
		if (!node.base.empty()) {
			// Look up base class in symbol table
			const SymbolInfo *baseInfo = symbolTable.lookup(node.base, SymbolInfo::OBJECT);
			if (!baseInfo) {
				errorReporter.report(node.loc, "Base class '" + node.base + "' not found");
			}
		}

		// Process all members
		for (auto &member: node.members) {
			member->accept(*this);
		}

		// Process operator overloads
		for (auto &op: node.operators) {
			op->accept(*this);
		}

		// Exit class scope
		symbolTable.exitScope();

		// Restore previous class context
		currentClass = std::move(previousClass);
	}

	void SemanticAnalyzer::visit(ReturnStmtNode& node) {
		if (!currentFunction) {
			errorReporter.report(node.loc, "'return' statement outside of function.");
			return;
		}

		auto expectedReturnType = resolveType(currentFunction->returnType);
		if (node.value) {
			auto actualReturnType = visitExpression(node.value);

			if (!expectedReturnType) {
				errorReporter.report(
					node.loc, "Cannot return a value from a function with no return type (implicitly void).");
			}
			else if (!actualReturnType) {
				// Expression had an error, cannot check compatibility
				errorReporter.report(node.loc, "Expression had an error, cannot check compatibility");
			}
			else if (!areTypesCompatible(expectedReturnType, actualReturnType)) {
				errorReporter.report(node.value->loc,
				                     "Return type '" + typeToString(actualReturnType) +
				                     "' is not compatible with function's declared return type '" + typeToString(
					                     expectedReturnType));
			}
		}
		else {
			// Return without value
			if (expectedReturnType) {
				bool isVoid = false;
				if (const auto prim = expectedReturnType.cast().non_throwing().as_ptr<PrimitiveTypeNode>()) {
					isVoid = (prim->type == PrimitiveTypeNode::Type::VOID);
				}
				if (!isVoid) {
					errorReporter.report(
						node.loc,
						"Must return a value from a function with declared return type '" + typeToString(
							expectedReturnType));
				}
			}
		}
	}

	bool SemanticAnalyzer::areTypesCompatible(polymorphic_ref<TypeNode> targetType,
	                                          polymorphic_ref<TypeNode> valueType) {
		if (!targetType || !valueType) return false;
		if (targetType->kind == TypeNode::Kind::ERROR || valueType->kind == TypeNode::Kind::ERROR) return false;
		if (targetType->isDynamic() || valueType->isDynamic()) return true;

		if (targetType->kind == valueType->kind) {
			switch (targetType->kind) {
				case TypeNode::Kind::PRIMITIVE: {
					auto targetPrim = targetType.cast().to<PrimitiveTypeNode>();
					auto valuePrim = valueType.cast().to<PrimitiveTypeNode>();
					if (!targetPrim || !valuePrim) return false;

					if (targetPrim->type == valuePrim->type) return true;

					static const std::unordered_set numericTypes = {
						PrimitiveTypeNode::Type::INT, PrimitiveTypeNode::Type::FLOAT, PrimitiveTypeNode::Type::DOUBLE,
						PrimitiveTypeNode::Type::SHORT, PrimitiveTypeNode::Type::LONG, PrimitiveTypeNode::Type::BYTE,
						PrimitiveTypeNode::Type::NUMBER, PrimitiveTypeNode::Type::BIGINT, PrimitiveTypeNode::Type::BIGNUMBER
					};

					if (numericTypes.contains(targetPrim->type) && numericTypes.contains(valuePrim->type)) {
						// Allow widening conversions
						if (targetPrim->type == PrimitiveTypeNode::Type::FLOAT && valuePrim->type ==
						    PrimitiveTypeNode::Type::INT) {
							return true;
						}
						if (targetPrim->type == PrimitiveTypeNode::Type::DOUBLE &&
						    (valuePrim->type == PrimitiveTypeNode::Type::INT || valuePrim->type ==
						     PrimitiveTypeNode::Type::FLOAT)) {
							return true;
						}
						if (targetPrim->type == PrimitiveTypeNode::Type::INT &&
						    (valuePrim->type == PrimitiveTypeNode::Type::SHORT || valuePrim->type ==
						     PrimitiveTypeNode::Type::BYTE)) {
							return true;
						}
						if (targetPrim->type == PrimitiveTypeNode::Type::LONG &&
						    (valuePrim->type == PrimitiveTypeNode::Type::INT || valuePrim->type ==
						     PrimitiveTypeNode::Type::SHORT ||
						     valuePrim->type == PrimitiveTypeNode::Type::BYTE)) {
							return true;
						}
					}
					return false;
				}

				case TypeNode::Kind::OBJECT: {
					auto targetObj = targetType.cast().non_throwing().to<NamedTypeNode>();
					auto valueObj = valueType.cast().non_throwing().to<NamedTypeNode>();
					if (!targetObj || !valueObj) return false;

					// Exact same name
					if (targetObj->name == valueObj->name) return true;

					// Check inheritance
					if (const SymbolInfo *valueSymbol = symbolTable.lookup(valueObj->name, SymbolInfo::OBJECT); valueSymbol && valueSymbol->declarationNode) {
						if (auto valueDecl = valueSymbol->declarationNode.cast().non_throwing().to<ObjectDeclNode>(); valueDecl && !valueDecl->base.empty()) {
							polymorphic_ref<TypeNode> baseType = make_polymorphic_ref_owns<
								NamedTypeNode>(valueObj->loc, valueDecl->base);
							return areTypesCompatible(targetType, baseType);
						}
					}
					return false;
				}

				case TypeNode::Kind::ARRAY: {
					auto targetArr = targetType.cast().non_throwing().to<ArrayTypeNode>();
					auto valueArr = valueType.cast().non_throwing().to<ArrayTypeNode>();
					if (!targetArr || !valueArr) return false;

					return areTypesCompatible(targetArr->elementType.get_ref(), valueArr->elementType.get_ref());
				}

				case TypeNode::Kind::FUNCTION: {
					auto targetFunc = targetType.cast().non_throwing().to<FunctionTypeNode>();
					auto valueFunc = valueType.cast().non_throwing().to<FunctionTypeNode>();
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

				case TypeNode::Kind::TEMPLATE: {
					auto targetTemp = targetType.cast().non_throwing().to<TemplateTypeNode>();
					auto valueTemp = valueType.cast().non_throwing().to<TemplateTypeNode>();
					if (!targetTemp || !valueTemp) return false;

					// Same base name and number of template arguments
					if (targetTemp->baseName != valueTemp->baseName ||
					    targetTemp->templateArgs.size() != valueTemp->templateArgs.size()) {
						return false;
					}

					// Check compatibility of each template argument
					for (size_t i = 0; i < targetTemp->templateArgs.size(); ++i) {
						if (!areTypesCompatible(targetTemp->templateArgs[i].get_ref(), valueTemp->templateArgs[i].get_ref())) {
							return false;
						}
					}
					return true;
				}

				default:
					return false;
			}
		}

		if (targetType->kind == TypeNode::Kind::OBJECT && valueType->kind == TypeNode::Kind::PRIMITIVE) {
			if (auto valuePrim = valueType.cast().non_throwing().to<PrimitiveTypeNode>(); valuePrim && valuePrim->type == PrimitiveTypeNode::Type::NIL) {
				return true;
			}
		}

		return false;
	}

	std::string SemanticAnalyzer::typeToString(const polymorphic_ref<TypeNode> &type) {
		if (!type) return "<nullptr>";

		switch (type->kind) {
			case TypeNode::Kind::PRIMITIVE: {
				auto prim = type.cast().non_throwing().unchecked().to<PrimitiveTypeNode>();
				if (!prim) return "<invalid primitive>";

				static const char *typeNames[] = {
					"int", "float", "double", "string", "bool", "number",
					"bigint", "bignumber", "short", "long", "byte", "void", "nil"
				};
				return typeNames[static_cast<int>(prim->type)];
			}

			case TypeNode::Kind::OBJECT: {
				auto named = type.cast().non_throwing().unchecked().to<NamedTypeNode>();
				if (!named) return "<invalid object>";
				return named->name;
			}

			case TypeNode::Kind::ARRAY: {
				auto arr = type.cast().non_throwing().unchecked().to<ArrayTypeNode>();
				if (!arr || !arr->elementType) return "<invalid array>";
				return typeToString(arr->elementType.get_ref()) + "[]";
			}

			case TypeNode::Kind::FUNCTION: {
				auto func = type.cast().non_throwing().unchecked().to<FunctionTypeNode>();
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

			case TypeNode::Kind::TEMPLATE: {
				auto templ = type.cast().non_throwing().unchecked().to<TemplateTypeNode>();
				if (!templ) return "<invalid template>";

				std::string result = templ->baseName + "<";
				for (size_t i = 0; i < templ->templateArgs.size(); ++i) {
					if (i > 0) result += ", ";
					result += typeToString(templ->templateArgs[i].get_ref());
				}
				result += ">";
				return result;
			}

			case TypeNode::Kind::DYNAMIC:
				return "dynamic";

			case TypeNode::Kind::ERROR:
				return "<error>";

			default:
				return "<unknown type>";
		}
	}

	void SemanticAnalyzer::visit(LiteralNode& node) {
		switch (node.type) {
			case LiteralNode::NUMBER:
				exprVR = std::move(make_polymorphic_ref_owns<PrimitiveTypeNode>(node.loc, PrimitiveTypeNode::Type::NUMBER));
				break;
			case LiteralNode::STRING:
				exprVR = std::move(make_polymorphic_ref_owns<PrimitiveTypeNode>(node.loc, PrimitiveTypeNode::Type::STRING));
				break;
			case LiteralNode::BOOL:
				exprVR = std::move(make_polymorphic_ref_owns<PrimitiveTypeNode>(node.loc, PrimitiveTypeNode::Type::BOOL));
				break;
			case LiteralNode::NIL:
				exprVR = std::move(make_polymorphic_ref_owns<PrimitiveTypeNode>(node.loc, PrimitiveTypeNode::Type::NIL));
				break;
		}
	}

	void SemanticAnalyzer::visit(VarNode& node) {
		if (const auto symbol = symbolTable.lookup(node.name)) {
			exprVR = symbol->type;
		}
		else {
			errorReporter.error(node.loc, "Undeclared variable '" + node.name + "'");
			exprVR = CREATE_ERROR_TYPE(node.loc);
		}
	}

	void SemanticAnalyzer::visit(BinaryOpNode& node) {
		const auto leftType = visitExpression(node.left);
		const auto rightType = visitExpression(node.right);

		if (node.op >= BinaryOpNode::ASN && node.op <= BinaryOpNode::MOD_ASN) {
			if (!areTypesCompatible(leftType, rightType)) {
				errorReporter.report(node.loc,
				                     "Type mismatch in assignment. Left type: " +
				                     typeToString(leftType) + ", right type: " +
				                     typeToString(rightType));
			}
			exprVR = leftType;
			return;
		}

		if (!areTypesCompatible(leftType, rightType)) {
			errorReporter.report(node.loc,
			                     "Type mismatch in binary operation. Left type: " +
			                     typeToString(leftType) + ", right type: " +
			                     typeToString(rightType));
		}

		exprVR = leftType;
	}

	polymorphic_ref<TypeNode> SemanticAnalyzer::resolveType(polymorphic_ref<TypeNode> typeNode) {
		if (!typeNode) {
			errorReporter.error(SourceLocation{}, "Attempting to resolve null type");
			return make_polymorphic_ref_owns<PrimitiveTypeNode>(
				SourceLocation{}, PrimitiveTypeNode::Type::NIL);
		}

		switch (typeNode->kind) {
			case TypeNode::Kind::PRIMITIVE:
			case TypeNode::Kind::DYNAMIC:
			case TypeNode::Kind::ERROR:
				return typeNode;

			case TypeNode::Kind::OBJECT: {
				auto named = typeNode.cast().non_throwing().to<NamedTypeNode>();
				if (!named) {
					errorReporter.internalError(typeNode->loc, "TypeNode::OBJECT is not a NamedTypeNode");
					return typeNode;
				}

				auto sym = symbolTable.lookup(named->name);
				if (!sym || (sym->kind != SymbolInfo::TYPE_ALIAS && sym->kind != SymbolInfo::OBJECT)) {
					errorReporter.error(sym->declarationNode->loc,
					               "Unknown or non-type identifier used as type: '" + named->name + "'");
					return make_polymorphic_ref_owns<PrimitiveTypeNode>(
						named->loc, PrimitiveTypeNode::Type::NIL);
				}

				if (!sym->type) {
					errorReporter.internalError(named->loc, "Type symbol '" + named->name + "' has no associated type");
					return make_polymorphic_ref_owns<PrimitiveTypeNode>(
						named->loc, PrimitiveTypeNode::Type::NIL);
				}

				// Recursively resolve the underlying type
				return resolveType(sym->type);
			}

			case TypeNode::Kind::ARRAY: {
				auto arr = typeNode.cast().non_throwing().to<ArrayTypeNode>();
				if (!arr) goto invalid;

				auto resolvedElem = resolveType(arr->elementType.get_ref());

				return make_polymorphic_ref_owns<ArrayTypeNode>(
					arr->loc,
					std::move(resolvedElem),
					arr->sizeExpr ? arr->sizeExpr.copy_or_share() : nullptr
				);
			}

			case TypeNode::Kind::FUNCTION: {
				auto fn = typeNode.cast().non_throwing().to<FunctionTypeNode>();
				if (!fn) goto invalid;

				std::vector<polymorphic_ref<TypeNode>> resolvedParams;
				resolvedParams.reserve(fn->parameterTypes.size());

				for (auto &param: fn->parameterTypes) {
					resolvedParams.push_back(resolveType(param));
				}

				auto resolvedRet = fn->returnType
					                   ? resolveType(fn->returnType)
					                   : polymorphic_ref<TypeNode>{nullptr};

				return make_polymorphic_ref_owns<FunctionTypeNode>(
					fn->loc,
					std::move(resolvedParams),
					std::move(resolvedRet)
				);
			}

			case TypeNode::Kind::TEMPLATE: {
				auto tmpl = typeNode.cast().non_throwing().to<TemplateTypeNode>();
				if (!tmpl) goto invalid;

				std::vector<polymorphic_ref<TypeNode>> resolvedArgs;
				resolvedArgs.reserve(tmpl->templateArgs.size());

				for (auto &arg: tmpl->templateArgs) {
					resolvedArgs.push_back(resolveType(arg.get_ref()));
				}

				return make_polymorphic_ref_owns<TemplateTypeNode>(
					tmpl->loc,
					tmpl->baseName,
					std::move(resolvedArgs)
				);
			}

			invalid:
			default:
				errorReporter.internalError(typeNode->loc, "Invalid or corrupted TypeNode during resolution");
				return make_polymorphic_ref_owns<PrimitiveTypeNode>(
					typeNode->loc, PrimitiveTypeNode::Type::NIL);
		}
	}

	void SemanticAnalyzer::visit(UnaryOpNode& node) {
		errorReporter.warning(node.loc,"idk brotein shake");
	}
	void SemanticAnalyzer::visit(CallNode& node) {
		polymorphic_ref<TypeNode> calleeType = visitExpression(node.callee);

		if (!calleeType) {
			errorReporter.report(node.loc, "Cannot determine type of callee.");
			exprVR = nullptr;
			return;
		}
		if (calleeType->kind != TypeNode::Kind::FUNCTION) {
			errorReporter.report(node.loc, "Attempted to call a non-function type: " + typeToString(calleeType));
			exprVR = nullptr;
			return;
		}
		auto funcType = calleeType.cast().unchecked().to<FunctionTypeNode>();
		if (node.arguments.size() != funcType->parameterTypes.size()) {
			errorReporter.report(node.loc, "Incorrect number of arguments: expected " +
												 std::to_string(funcType->parameterTypes.size()) +
												 ", got " + std::to_string(node.arguments.size()));
		}
		for (auto [i, arg] : std::ranges::views::enumerate(node.arguments)) {
			if (polymorphic_ref<TypeNode> argType = visitExpression(arg); i < funcType->parameterTypes.size() &&
			                                                              !areTypesCompatible(funcType->parameterTypes[i], argType)) {
				errorReporter.report(arg->loc,
									 "Argument type mismatch: expected " +
									 typeToString(funcType->parameterTypes[i]) +
									 ", got " + typeToString(argType));
				}
		}

		exprVR = funcType->returnType;
	}
	void SemanticAnalyzer::visit(IfNode& node) {
		const polymorphic_ref<ExprNode> condition = node.condition;
		bool areCompatible = areTypesCompatible(visitExpression(condition), make_polymorphic_ref_owns<PrimitiveTypeNode>(condition->loc,PrimitiveTypeNode::Type::BOOL));
		if (!areCompatible) {
			errorReporter.error(condition->loc, "Expression is not convertible to bool");
		}
		node.thenBranch->accept(*this);
		if (node.elseBranch) node.elseBranch->accept(*this);
	}
} // namespace zenith
