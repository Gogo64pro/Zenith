module;
#include <ranges>

#include "utils/Colorize.hpp"
#include "exceptions/ErrorReporter.hpp"
#include <string>
#include <vector>
#include <unordered_set>
#define CREATE_ERROR_TYPE(loc) make_polymorphic_ref<TypeNode>(loc, TypeNode::Kind::ERROR)
import zenith.core.polymorphic_ref;
import zenith.ast;
module zenith.semantic;
namespace zenith {
	SymbolTable &&SemanticAnalyzer::analyze(ProgramNode &program) {
		visit(program);
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
			finalType = make_polymorphic_ref_owns<TypeNode>(node.loc, VarDeclNode::DYNAMIC);

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
					                     typeToString(declaredType));
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
			auto resolvedParamType = resolveType(param);
			if (!resolvedParamType) {
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
				exprVR = make_polymorphic_ref_owns<TypeNode>(node.lambda->returnType->loc);
				return; // Early exit
			}
		}
		else if (node.lambda->body) {
			// TODO: Implement robust return type inference.
			errorReporter.report(
				node.loc, "Lambda return type inference not yet implemented. Please provide an explicit return type.");
			returnType = getErrorTypeNode(node.loc); // Use error type for now
		}
		else {
			errorReporter.report(node.loc, "Lambda must have an explicit return type or a body for inference.");
			exprVR = getErrorTypeNode(node.loc);
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
				paramTypeForSymbol = getErrorTypeNode(node.loc);
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

		// --- Step 4: Assign Final Type to Member exprVR ---
		if (analysisError) {
			exprVR = getErrorTypeNode(node.loc);
		}
		else {
			exprVR = make_polymorphic<FunctionTypeNode>(
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
		currentClass = node.share();

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
			auto actualReturnType = visitExpression(node.value.share());

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
				                     "Return type '" + typeToString(actualReturnType.share()) +
				                     "' is not compatible with function's declared return type '" + typeToString(
					                     expectedReturnType.share()));
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
							expectedReturnType.share()));
				}
			}
		}
	}

	bool SemanticAnalyzer::areTypesCompatible(polymorphic<TypeNode> &targetType,
	                                          polymorphic<TypeNode> &valueType) {
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
							polymorphic<TypeNode> baseType = make_polymorphic<
								NamedTypeNode>(valueObj.loc, valueDecl->base);
							return areTypesCompatible(targetType, baseType);
						}
					}
					return false;
				}

				case TypeNode::Kind::ARRAY: {
					auto targetArr = targetType.cast().non_throwing().to<ArrayTypeNode>();
					auto valueArr = valueType.cast().non_throwing().to<ArrayTypeNode>();
					if (!targetArr || !valueArr) return false;

					return areTypesCompatible(targetArr->elementType, valueArr->elementType);
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
		if (targetType->kind == TypeNode::Kind::OBJECT && valueType->kind == TypeNode::Kind::PRIMITIVE) {
			if (auto valuePrim = valueType.cast().non_throwing().to<PrimitiveTypeNode>(); valuePrim && valuePrim->type == PrimitiveTypeNode::Type::NIL) {
				return true;
			}
		}

		return false;
	}

	std::string SemanticAnalyzer::typeToString(const polymorphic<TypeNode> &type) {
		if (!type) return "<nullptr>";

		switch (type->kind) {
			case TypeNode::Kind::PRIMITIVE: {
				auto prim = type.cast().non_throwing().unchecked().to<PrimitiveTypeNode>();
				if (!prim) return "<invalid primitive>";

				static const char *typeNames[] = {
					"int", "float", "double", "string", "bool", "number",
					"bigint", "bignumber", "short", "long", "byte", "void", "nil"
				};
				return typeNames[prim->type];
			}

			case TypeNode::Kind::OBJECT: {
				auto named = type.cast().non_throwing().unchecked().to<NamedTypeNode>();
				if (!named) return "<invalid object>";
				return named->name;
			}

			case TypeNode::Kind::ARRAY: {
				auto arr = type.cast().non_throwing().unchecked().to<ArrayTypeNode>();
				if (!arr || !arr->elementType) return "<invalid array>";
				return typeToString(std::move(arr->elementType)) + "[]";
			}

			case TypeNode::Kind::FUNCTION: {
				auto func = type.cast().non_throwing().unchecked().to<FunctionTypeNode>();
				if (!func) return "<invalid function>";

				std::string result = "(";
				for (size_t i = 0; i < func->parameterTypes.size(); ++i) {
					if (i > 0) result += ", ";
					result += typeToString(func->parameterTypes[i].share());
				}
				result += ") -> ";
				result += typeToString(std::move(func->returnType));
				return result;
			}

			case TypeNode::Kind::TEMPLATE: {
				auto templ = type.cast().non_throwing().unchecked().to<TemplateTypeNode>();
				if (!templ) return "<invalid template>";

				std::string result = templ->baseName + "<";
				for (size_t i = 0; i < templ->templateArgs.size(); ++i) {
					if (i > 0) result += ", ";
					result += typeToString(templ->templateArgs[i].share());
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
		const auto numberType = make_polymorphic<PrimitiveTypeNode>(node.loc, PrimitiveTypeNode::Type::NUMBER);
		const auto stringType = make_polymorphic<PrimitiveTypeNode>(node.loc, PrimitiveTypeNode::Type::STRING);
		const auto boolType = make_polymorphic<PrimitiveTypeNode>(node.loc, PrimitiveTypeNode::Type::BOOL);
		const auto nilType = make_polymorphic<PrimitiveTypeNode>(node.loc, PrimitiveTypeNode::Type::NIL);
		switch (node->type) {
			case LiteralNode::NUMBER:
				exprVR = numberType.share();
				break;
			case LiteralNode::STRING:
				exprVR = stringType.share();
				break;
			case LiteralNode::BOOL:
				exprVR = boolType.share();
				break;
			case LiteralNode::NIL:
				exprVR = nilType.share();
				break;
		}
	}

	void SemanticAnalyzer::visit(VarNode& node) {
		if (const auto symbol = symbolTable.lookup(node->name)) {
			exprVR = symbol->type.share();
		}
		else {
			errorReporter.report(node.loc, "Undeclared variable '" + node->name + "'");
			exprVR = getErrorTypeNode(node.loc);
		}
	}

	void SemanticAnalyzer::visit(BinaryOpNode& node) {
		auto leftType = visitExpression(node.left);
		auto rightType = visitExpression(node.right);

		// Check for assignment operations
		if (node.op >= BinaryOpNode::ASN && node.op <= BinaryOpNode::MOD_ASN) {
			// For assignments, left must be a lvalue
			if (!areTypesCompatible(leftType, rightType)) {
				errorReporter.report(node.loc,
				                     "Type mismatch in assignment. Left type: " +
				                     typeToString(leftType.share()) + ", right type: " +
				                     typeToString(rightType.share()));
			}
			exprVR = leftType;
			return;
		}

		// For other operations, types must be compatible
		if (!areTypesCompatible(leftType, rightType)) {
			errorReporter.report(node.loc,
			                     "Type mismatch in binary operation. Left type: " +
			                     typeToString(leftType.share()) + ", right type: " +
			                     typeToString(rightType.share()));
		}

		// Result type is the more general of the two
		exprVR = leftType.share();
	}

	polymorphic<TypeNode> resolveType(polymorphic<TypeNode> typeNode,
	                                                      SymbolTable &symbols, ErrorReporter &reporter,
	                                                      const SourceLocation &usageLoc) {
		if (!typeNode) {
			reporter.report(usageLoc, "Attempting to resolve null type");
			return make_polymorphic<PrimitiveTypeNode>(
				usageLoc, PrimitiveTypeNode::Type::NIL);
		}

		switch (typeNode->kind) {
			case TypeNode::Kind::PRIMITIVE:
			case TypeNode::Kind::DYNAMIC:
			case TypeNode::Kind::ERROR:
				return std::move(typeNode);

			case TypeNode::Kind::OBJECT: {
				auto named = typeNode.cast().non_throwing().to<NamedTypeNode>();
				if (!named) {
					reporter.report(typeNode.loc, "TypeNode::OBJECT is not a NamedTypeNode",
					                {"Internal Error", RED_TEXT});
					return std::move(typeNode);
				}

				const SymbolInfo *sym = symbols.lookup(named->name);
				if (!sym || (sym->kind != SymbolInfo::TYPE_ALIAS && sym->kind != SymbolInfo::OBJECT)) {
					reporter.error(usageLoc,
					               "Unknown or non-type identifier used as type: '" + named->name + "'");
					return make_polymorphic<PrimitiveTypeNode>(
						named.loc, PrimitiveTypeNode::Type::NIL);
				}

				if (!sym->type) {
					reporter.internalError(named.loc, "Type symbol '" + named->name + "' has no associated type");
					return make_polymorphic<PrimitiveTypeNode>(
						named.loc, PrimitiveTypeNode::Type::NIL);
				}

				// Recursively resolve the underlying type
				return resolveType(sym->type.share(), symbols, reporter,
				                   usageLoc);
			}

			case TypeNode::Kind::ARRAY: {
				auto arr = typeNode.cast().non_throwing().to<ArrayTypeNode>();
				if (!arr) goto invalid;

				auto resolvedElem = resolveType(arr->elementType.share(), symbols, reporter,
				                                usageLoc);

				return make_polymorphic<ArrayTypeNode>(
					arr.loc,
					std::move(resolvedElem),
					arr->sizeExpr ? arr->sizeExpr.share() : nullptr
				);
			}

			case TypeNode::Kind::FUNCTION: {
				auto fn = typeNode.cast().non_throwing().to<FunctionTypeNode>();
				if (!fn) goto invalid;

				std::vector<polymorphic<TypeNode> > resolvedParams;
				resolvedParams.reserve(fn->parameterTypes.size());

				for (auto &param: fn->parameterTypes) {
					resolvedParams.push_back(resolveType(param.share(), symbols, reporter,
					                                     usageLoc));
				}

				auto resolvedRet = fn->returnType
					                   ? resolveType(fn->returnType.share(), symbols, reporter,
					                                 usageLoc)
					                   : polymorphic<TypeNode>{nullptr};

				return make_polymorphic<FunctionTypeNode>(
					fn.loc,
					std::move(resolvedParams),
					std::move(resolvedRet)
				);
			}

			case TypeNode::Kind::TEMPLATE: {
				auto tmpl = typeNode.cast().non_throwing().to<TemplateTypeNode>();
				if (!tmpl) goto invalid;

				std::vector<polymorphic<TypeNode> > resolvedArgs;
				resolvedArgs.reserve(tmpl->templateArgs.size());

				for (auto &arg: tmpl->templateArgs) {
					resolvedArgs.push_back(resolveType(arg.share(), symbols, reporter,
					                                   usageLoc));
				}

				return make_polymorphic<TemplateTypeNode>(
					tmpl.loc,
					tmpl->baseName,
					std::move(resolvedArgs)
				);
			}

			invalid:
			default:
				reporter.internalError(typeNode.loc, "Invalid or corrupted TypeNode during resolution");
				return make_polymorphic<PrimitiveTypeNode>(
					typeNode.loc, PrimitiveTypeNode::Type::NIL);
		}
	}
} // namespace zenith
