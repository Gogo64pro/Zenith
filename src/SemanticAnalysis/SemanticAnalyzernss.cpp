//#include "SemanticAnalyzer.hpp"

namespace zenith {

	SemanticAnalyzer::SemanticAnalyzer(ErrorReporter& errorReporter)
			: errorReporter(errorReporter), symbolTable(errorReporter) {}

	void SemanticAnalyzer::analyze(ProgramNode& program) {
		visit(program);
	}

	bool SemanticAnalyzer::areTypesCompatible(const TypeNode* targetType, const TypeNode* valueType) {
		// TODO: Implement actual type compatibility logic
		// - Check for null assignment
		// - Check primitive conversions (e.g., int to float)
		// - Check inheritance/subtyping
		// - Check dynamic types
		// - Handle template types
		if (!targetType || !valueType) return false; // Cannot compare if one is missing
		if (targetType->kind == TypeNode::DYNAMIC || valueType->kind == TypeNode::DYNAMIC) return true; // Assume dynamic compatibility for now
		if (targetType->kind != valueType->kind) return false; // Basic kind check

		// Add more specific checks based on kind...
		if (targetType->kind == TypeNode::PRIMITIVE) {
			auto* pTarget = dynamic_cast<const PrimitiveTypeNode*>(targetType);
			auto* pValue = dynamic_cast<const PrimitiveTypeNode*>(valueType);
			// Very basic check, needs refinement for implicit conversions
			return pTarget->type == pValue->type;
		}
		if (targetType->kind == TypeNode::OBJECT) {
			auto* nTarget = dynamic_cast<const NamedTypeNode*>(targetType);
			auto* nValue = dynamic_cast<const NamedTypeNode*>(valueType);
			// Basic name check, needs inheritance check later
			return nTarget->name == nValue->name;
		}
		// ... other kinds (ARRAY, FUNCTION, TEMPLATE)

		return false; // Default to incompatible
	}

	std::unique_ptr<TypeNode> SemanticAnalyzer::resolveType(TypeNode* typeNode) {
		// TODO: Handle type aliases, potentially look up named types in symbol table
		// For now, just clone it if it exists
		if (!typeNode) return nullptr;
		return typeNode->clone(); // Assuming TypeNode has a clone method
	}

	std::string SemanticAnalyzer::typeToString(const TypeNode* type) {
		if (!type) return "<unknown_type>";
		// A simple version, real one might need more context
		return type->toString();
	}

	void SemanticAnalyzer::visit(ASTNode& node) {
		// ProgramNode is handled by the public analyze function
		if (auto* p = dynamic_cast<ImportNode*>(&node)) { visit(*p); }
		else if (auto* p = dynamic_cast<BlockNode*>(&node)) { visit(*p); } // Handles ScopeBlockNode, UnsafeNode too
		else if (auto* p = dynamic_cast<VarDeclNode*>(&node)) { visit(*p); }
		else if (auto* p = dynamic_cast<MultiVarDeclNode*>(&node)) { visit(*p); }
		else if (auto* p = dynamic_cast<FunctionDeclNode*>(&node)) { visit(*p); } // Handles LambdaNode too
		else if (auto* p = dynamic_cast<LambdaExprNode*>(&node)) { visit(*p); }
		else if (auto* p = dynamic_cast<ObjectDeclNode*>(&node)) { visit(*p); } // Handles ActorDeclNode
		else if (auto* p = dynamic_cast<UnionDeclNode*>(&node)) { visit(*p); }
		else if (auto* p = dynamic_cast<IfNode*>(&node)) { visit(*p); }
		else if (auto* p = dynamic_cast<WhileNode*>(&node)) { visit(*p); }
		else if (auto* p = dynamic_cast<DoWhileNode*>(&node)) { visit(*p); }
		else if (auto* p = dynamic_cast<ForNode*>(&node)) { visit(*p); }
		else if (auto* p = dynamic_cast<ExprStmtNode*>(&node)) { visit(*p); }
		else if (auto* p = dynamic_cast<ReturnStmtNode*>(&node)) { visit(*p); }
		else if (auto* p = dynamic_cast<EmptyStmtNode*>(&node)) { visit(*p); }
		else if (auto* p = dynamic_cast<AnnotationNode*>(&node)) { visit(*p); } // Might do validation later
		else if (auto* p = dynamic_cast<TemplateDeclNode*>(&node)) { visit(*p); }
			// Add other statement/declaration types as needed
			// Expressions are visited when needed by statements/other expressions
		else if (dynamic_cast<ExprNode*>(&node)) {
			// Expressions themselves aren't visited directly at the top level,
			// but their visit method returning a type will be called by containing statements/expressions.
			// We might add a warning here if an expression is somehow a top-level statement without ExprStmtNode.
			errorReporter.reportWarning(node.loc, "Expression result unused?");
			visitExpression(dynamic_cast<ExprNode&>(node)); // Evaluate it anyway for errors
		}
		else if (dynamic_cast<TypeNode*>(&node)) { /* Types are usually handled during declarations */ }
		else if (dynamic_cast<ErrorNode*>(&node)) { /* Parser error, already reported */ }
		// else { errorReporter.report(node.loc, "Internal Error: Unhandled AST node type in semantic analysis."); }
	}

	std::unique_ptr<TypeNode> SemanticAnalyzer::visitExpression(ExprNode& expr) {
		if (auto* p = dynamic_cast<LiteralNode*>(&expr)) { return visit(*p); }
		else if (auto* p = dynamic_cast<VarNode*>(&expr)) { return visit(*p); }
		else if (auto* p = dynamic_cast<BinaryOpNode*>(&expr)) { return visit(*p); }
		else if (auto* p = dynamic_cast<UnaryOpNode*>(&expr)) { return visit(*p); }
		else if (auto* p = dynamic_cast<CallNode*>(&expr)) { return visit(*p); }
		else if (auto* p = dynamic_cast<MemberAccessNode*>(&expr)) { return visit(*p); }
		else if (auto* p = dynamic_cast<ArrayAccessNode*>(&expr)) { return visit(*p); }
		else if (auto* p = dynamic_cast<NewExprNode*>(&expr)) { return visit(*p); }
		else if (auto* p = dynamic_cast<ThisNode*>(&expr)) { return visit(*p); }
		else if (auto* p = dynamic_cast<LambdaExprNode*>(&expr)) { return visit(*p); }
		else if (auto* p = dynamic_cast<FreeObjectNode*>(&expr)) { return visit(*p); }
		else if (auto* p = dynamic_cast<TemplateStringNode*>(&expr)) { return visit(*p); }
		else if (auto* p = dynamic_cast<StructInitializerNode*>(&expr)) { return visit(*p); }
			// Add other expression types
		else {
			errorReporter.report(expr.loc, "Internal Error: Unhandled expression type in semantic analysis.");
			// Return a dummy "error" type or nullptr
			return nullptr; // Or perhaps a specific error type node
		}
	}

	void SemanticAnalyzer::visit(ProgramNode& node) {
		symbolTable.enterScope(); // Global scope
		// TODO: Add built-in types/functions to the global scope here
		for (auto& decl : node.declarations) {
			if (decl) {
				visit(*decl);
			}
		}
		symbolTable.exitScope();
	}

	void SemanticAnalyzer::visit(ImportNode& node) {
		// TODO: Handle imports - check path validity, potentially load symbols
		// For now, just accept it.
	}

	void SemanticAnalyzer::visit(BlockNode& node) {
		// Determine if this block creates a new scope (most do, except maybe some switch cases)
		bool newScope = true; // Default to creating a scope
		if (dynamic_cast<UnsafeNode*>(&node)) {
			// Handle unsafe block specifics if any
		}
		else if (dynamic_cast<ScopeBlockNode*>(&node)) {
			// Already explicit
		} // Other block types?

		if (newScope) symbolTable.enterScope();

		for (auto& stmt : node.statements) {
			if (stmt) {
				visit(*stmt);
			}
		}

		if (newScope) symbolTable.exitScope();
	}

	void SemanticAnalyzer::visit(VarDeclNode& node) {
		std::unique_ptr<TypeNode> declaredType = resolveType(node.type.get());
		std::unique_ptr<TypeNode> initializerType = nullptr;

		if (node.initializer) {
			initializerType = visitExpression(*node.initializer);
		}

		// Type check assignment
		if (declaredType && initializerType) {
			if (!areTypesCompatible(declaredType.get(), initializerType.get())) {
				errorReporter.report(node.initializer->loc,
				                          "Initializer type '" + typeToString(initializerType.get()) +
				                          "' is not compatible with declared variable type '" + typeToString(declaredType.get()) + "'.");
				// Continue analysis, but mark the variable's type potentially based on declaration
			}
		}
		else if (!declaredType && !initializerType) {
			// This might be allowed if the language supports implicit 'dynamic' or requires later assignment
			// For now, let's require either a type or an initializer
			errorReporter.report(node.loc, "Variable '" + node.name + "' must have a type or an initializer.");
			return; // Cannot declare symbol without type
		}

		// Determine the final type for the symbol table
		std::unique_ptr<TypeNode> finalType = nullptr;
		if (declaredType) {
			finalType = std::move(declaredType); // Prefer declared type
		}
		else {
			finalType = std::move(initializerType); // Infer from initializer
		}

		// Declare in symbol table
		symbolTable.declare(node.name, SymbolInfo(SymbolInfo::VARIABLE, std::move(finalType), &node, node.isConst));
	}

	void SemanticAnalyzer::visit(MultiVarDeclNode& node) {
		for (auto& var : node.vars) {
			if (var) visit(*var);
		}
	}

	void SemanticAnalyzer::visit(FunctionDeclNode& node) {
		// TODO: Handle function overloading if supported
		// TODO: Handle annotations

		// Resolve return type
		auto returnType = resolveType(node.returnType.get());

		// --- Pre-declare function in the *outer* scope for recursion ---
		// Need to build the function type first
		std::vector<std::unique_ptr<TypeNode>> paramTypes;
		paramTypes.reserve(node.params.size());
		for (const auto& param : node.params) {
			auto pType = resolveType(param.second.get());
			if (!pType) {
				errorReporter.report(param.second ? param.second->loc : node.loc, "Invalid type for parameter '" + param.first + "' in function '" + node.name + "'.");
				// Add a dummy type or skip? For now, let's add a placeholder to avoid crashing later lookups.
				// This requires a way to represent an "error type" or using nullptr carefully.
				// Using nullptr for now.
				paramTypes.push_back(nullptr);
			}
			else {
				paramTypes.push_back(std::move(pType));
			}
		}
		// Create a FunctionTypeNode (assuming it exists or we create one)
		// std::unique_ptr<FunctionTypeNode> funcType = std::make_unique<FunctionTypeNode>(node.loc, std::move(paramTypes), std::move(returnType));
		// For now, just use the resolved return type directly for symbol table.
		// A real FunctionTypeNode would be better.
		SymbolInfo funcInfo(SymbolInfo::FUNCTION, std::move(returnType), &node);
		// funcInfo.parameters = ... // Store parameter types too
		if (!node.name.empty()) { // Lambdas might have empty names here
			if (!symbolTable.declare(node.name, std::move(funcInfo))) {
				// Redeclaration error already reported by symbolTable.declare
				// Might want to stop processing this function? For now, continue.
			}
		}
		// --- End Pre-declaration ---

		FunctionDeclNode* previousFunction = currentFunction;
		currentFunction = &node; // Set context

		symbolTable.enterScope(); // Function body scope

		// Declare parameters in the function scope
		for (size_t i = 0; i < node.params.size(); ++i) {
			const auto& param = node.params[i];
			// We already resolved types above, retrieve them (or re-resolve if needed)
			// If we stored a full FunctionTypeNode, retrieve param types from it.
			// For simplicity now, re-resolve (less efficient but simpler structure)
			auto pType = resolveType(param.second.get());
			if (!pType) {
				// Error already reported during pre-declaration type resolution
				pType = std::make_unique<TypeNode>(param.second ? param.second->loc : node.loc, TypeNode::Kind::DYNAMIC); // Assign dynamic as fallback?
			}

			// TODO: Handle default values - visit expression and check compatibility with pType
			if (i < node.defaultValues.size() && node.defaultValues[i]) {
				auto defaultValueType = visitExpression(*node.defaultValues[i]);
				if (pType && defaultValueType && !areTypesCompatible(pType.get(), defaultValueType.get())) {
					errorReporter.report(node.defaultValues[i]->loc, "Default value type '" + typeToString(defaultValueType.get()) + "' is not compatible with parameter type '" + typeToString(pType.get()) + "'.");
				}
			}

			symbolTable.declare(param.first, SymbolInfo(SymbolInfo::VARIABLE, std::move(pType), param.second.get() /* Use type node as declaration approx */));
		}

		// Visit the function body
		if (node.body) {
			visit(*node.body);
			// TODO: Check for return paths if returnType is not void/null
		}

		symbolTable.exitScope(); // Exit function body scope
		currentFunction = previousFunction; // Restore context
	}


	std::unique_ptr<TypeNode> SemanticAnalyzer::visit(LambdaExprNode& node) {
		// Lambdas are essentially anonymous functions treated as expressions.
		// We need to determine the *type* of the lambda itself.

		// 1. Resolve parameter and return types (similar to FunctionDeclNode)
		std::vector<std::unique_ptr<TypeNode>> paramTypes;
		paramTypes.reserve(node.lambda->params.size());
		for (const auto& param : node.lambda->params) {
			paramTypes.push_back(resolveType(param.second.get()));
			if (!paramTypes.back()) {
				errorReporter.report(param.second ? param.second->loc : node.loc, "Invalid type for lambda parameter '" + param.first + "'.");
				// Handle error - maybe return nullptr for the whole lambda type?
				return nullptr;
			}
		}
		auto returnType = resolveType(node.lambda->returnType.get());
		if (!returnType && !node.lambda->body) { // If no return type specified AND no body, it's ill-formed
			errorReporter.report(node.loc, "Lambda must have an explicit return type or a body for inference.");
			return nullptr;
		}

		// TODO: Return type inference if returnType is null but body exists.
		// Requires analyzing all return statements in the body.

		// 2. Perform semantic analysis on the lambda body *within its own scope*
		FunctionDeclNode* previousFunction = currentFunction;
		// Treat the LambdaNode itself as the current function context temporarily
		currentFunction = node.lambda.get();

		symbolTable.enterScope();
		for (size_t i = 0; i < node.lambda->params.size(); ++i) {
			// Use the types resolved in step 1
			auto pType = resolveType(node.lambda->params[i].second.get()); // Re-resolve for ownership transfer
			symbolTable.declare(node.lambda->params[i].first, SymbolInfo(SymbolInfo::VARIABLE, std::move(pType), node.lambda->params[i].second.get()));
		}

		if (node.lambda->body) {
			visit(*node.lambda->body);
			// TODO: Return type checks within the body against 'returnType' or inferred type.
		}

		symbolTable.exitScope();
		currentFunction = previousFunction;

		// 3. Construct and return the FunctionTypeNode representing the lambda signature
		// Assuming FunctionTypeNode exists:
		// return std::make_unique<FunctionTypeNode>(node.loc, std::move(paramTypes), std::move(returnType));

		// Placeholder: return a generic 'function' type or the resolved return type?
		// Returning the resolved return type is *incorrect*. Needs a FunctionType.
		// For now, return a dynamic type as a placeholder for a function type.
		return std::make_unique<TypeNode>(node.loc, TypeNode::Kind::FUNCTION); // Placeholder!
	}

	void SemanticAnalyzer::visit(ReturnStmtNode& node) {
		if (!currentFunction) {
			errorReporter.report(node.loc, "'return' statement outside of function.");
			return;
		}

		auto expectedReturnType = resolveType(currentFunction->returnType.get()); // Resolve expected type each time

		if (node.value) {
			// Return with value
			auto actualReturnType = visitExpression(*node.value);

			if (!expectedReturnType) {
				// Returning a value from a function declared void (or implicitly void)
				errorReporter.report(node.loc, "Cannot return a value from a function with no return type (implicitly void).");
			}
			else if (!actualReturnType) {
				// Expression had an error, cannot check compatibility
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
				// Not returning a value from a function that expects one
				// Note: Some languages might allow this if the return type is implicitly nullable or void*.
				// Assuming void must be explicitly specified or no return type means void.
				// A more precise check would be needed if null/void* are distinct.
				bool isVoid = false;
				if (auto* prim = dynamic_cast<PrimitiveTypeNode*>(expectedReturnType.get())) {
					// Check if it's a specific 'void' primitive if your language has one
					// isVoid = (prim->type == PrimitiveTypeNode::VOID);
				}
				if (!isVoid) { // If expected type is not void
					errorReporter.report(node.loc, "Must return a value from a function with declared return type '" + typeToString(expectedReturnType.get()) + "'.");
				}
			}
			// Else: Returning nothing from a void function - OK.
		}
	}

	void SemanticAnalyzer::visit(IfNode& node) {
		auto condType = visitExpression(*node.condition);
		if (condType) {
			// TODO: Check if condType is convertible to bool
			// bool isBoolCompatible = checkBooleanCompatibility(condType.get());
			// if (!isBoolCompatible) {
			//     errorReporter.report(node.condition->loc, "If condition must be a boolean type, found '" + typeToString(condType.get()) + "'.");
			// }
		}

		visit(*node.thenBranch);

		if (node.elseBranch) {
			visit(*node.elseBranch);
		}
	}

	void SemanticAnalyzer::visit(WhileNode& node) {
		auto condType = visitExpression(*node.condition);
		if (condType) {
			// TODO: Check if condType is convertible to bool
		}

		bool previousInLoop = inLoop;
		inLoop = true;
		visit(*node.body);
		inLoop = previousInLoop;
	}

	void SemanticAnalyzer::visit(DoWhileNode& node) {
		bool previousInLoop = inLoop;
		inLoop = true;
		visit(*node.body);
		inLoop = previousInLoop; // Condition is outside the loop body scope for break/continue

		auto condType = visitExpression(*node.condition);
		if (condType) {
			// TODO: Check if condType is convertible to bool
		}
	}


	void SemanticAnalyzer::visit(ForNode& node) {
		symbolTable.enterScope(); // Scope for initializer

		if (node.initializer) {
			visit(*node.initializer);
		}

		if (node.condition) {
			auto condType = visitExpression(*node.condition);
			if (condType) {
				// TODO: Check if condType is convertible to bool
			}
		}
		else {
			// No condition means infinite loop `for(;;)` - usually ok.
		}

		if (node.increment) {
			visitExpression(*node.increment); // Evaluate for type errors, result ignored
		}

		bool previousInLoop = inLoop;
		inLoop = true;
		visit(*node.body);
		inLoop = previousInLoop;

		symbolTable.exitScope();
	}


	void SemanticAnalyzer::visit(ExprStmtNode& node) {
		visitExpression(*node.expr); // Evaluate expression, ignore result type but catch errors
	}

	void SemanticAnalyzer::visit(EmptyStmtNode& node) {
		// Nothing to do semantically
	}

	void SemanticAnalyzer::visit(ObjectDeclNode& node) {
		// TODO: Handle annotations, base class lookup, inheritance cycle checks

		// Pre-declare the class/struct/actor type itself
		auto selfType = std::make_unique<NamedTypeNode>(node.loc, node.name);
		SymbolInfo::Kind symbolKind;
		switch (node.kind) {
			case ObjectDeclNode::Kind::CLASS: symbolKind = SymbolInfo::CLASS; break;
			case ObjectDeclNode::Kind::STRUCT: symbolKind = SymbolInfo::STRUCT; break;
			case ObjectDeclNode::Kind::ACTOR: symbolKind = SymbolInfo::ACTOR; break;
		}
		if (!symbolTable.declare(node.name, SymbolInfo(symbolKind, std::move(selfType), &node))) {
			// Redeclaration error already reported
			return; // Don't process members if class declaration failed
		}

		ObjectDeclNode* previousClass = currentClass;
		currentClass = &node;

		symbolTable.enterScope(); // Class scope

		// TODO: Add 'this' symbol to the scope
		// symbolTable.declare("this", SymbolInfo(SymbolInfo::VARIABLE, std::make_unique<NamedTypeNode>(node.loc, node.name), &node, true)); // 'this' is implicitly const?

		// TODO: Add base class members if inheritance exists

		// Visit members
		for (auto& member : node.members) {
			visitMember(*member);
		}

		// TODO: Visit operator overloads
		for (auto& op : node.operators) {
			visitOperatorOverload(*op);
		}

		// TODO: Check for constructor presence/validity if needed by language rules

		symbolTable.exitScope(); // Exit class scope
		currentClass = previousClass;
	}

	void SemanticAnalyzer::visitMember(MemberDeclNode& node) {
		// Check within the *class* scope (needs access to class definition)
		// TODO: Check for duplicate member names within the class context
		// TODO: Handle access modifiers based on current context

		if (node.flags.kind == MemberDeclNode::Kind::FIELD) {
			auto fieldType = resolveType(node.type.get());
			if (!fieldType) {
				errorReporter.report(node.type ? node.type->loc : node.loc, "Invalid type for field '" + node.name + "'.");
				return;
			}

			// Handle initializers (assuming unified in 'initializers' vector)
			if (!node.initializers.empty()) {
				// For fields, expect at most one initializer (without a name)
				if (node.initializers.size() > 1 || !node.initializers[0].first.empty()) {
					errorReporter.report(node.initializers[0].second->loc, "Field '" + node.name + "' allows only a single direct initializer.");
				}
				else {
					auto initExpr = node.initializers[0].second.get();
					auto initType = visitExpression(*initExpr);
					if (initType && !areTypesCompatible(fieldType.get(), initType.get())) {
						errorReporter.report(initExpr->loc, "Initializer type '" + typeToString(initType.get()) + "' is not compatible with field type '" + typeToString(fieldType.get()) + "'.");
					}
				}
			}
			// TODO: Store field info (type, access, const, static) in the class definition's symbol table entry
			// symbolTable.addClassMember(currentClass->name, node.name, SymbolInfo(...));

		}
		else if (node.flags.kind == MemberDeclNode::Kind::METHOD || node.flags.kind == MemberDeclNode::Kind::METHOD_CONSTRUCTOR) {
			// Very similar to FunctionDeclNode visit, but within class context
			// TODO: Handle 'static' methods (no 'this')
			// TODO: Constructors: check name matches class, no return type allowed

			FunctionDeclNode* previousFunction = currentFunction;
			// currentFunction = ??? // Need a way to represent the method context
			// Maybe pass 'node' itself or create a temporary FunctionDeclNode-like structure?

			symbolTable.enterScope(); // Method scope

			// TODO: Add parameters to scope
			// TODO: Visit method body
			if (node.body) visit(*node.body);

			symbolTable.exitScope(); // Exit method scope
			currentFunction = previousFunction;

			// TODO: Store method info (signature, access, static) in the class definition
		}
		else if (node.flags.kind == MemberDeclNode::Kind::MESSAGE_HANDLER) {
			// TODO: Specific checks for actor message handlers
		}
	}

	void SemanticAnalyzer::visitOperatorOverload(OperatorOverloadNode& node) {
		// TODO: Implement checks for operator overloading
		// - Check valid operator syntax (node.op)
		// - Check number/types of parameters based on operator arity (unary/binary)
		// - Check if operator can be overloaded for the current class type
		// - Resolve parameter and return types
		// - Visit body like a function/method
	}

	void SemanticAnalyzer::visit(UnionDeclNode& node) {
		// TODO: Implement checks for union declaration
		// - Pre-declare the union type name
		// - Resolve member types
		// - Store union definition
	}


	// --- Expression Visitors (Return Type) ---

	std::unique_ptr<TypeNode> SemanticAnalyzer::visit(LiteralNode& node) {
		// Determine type from literal kind
		PrimitiveTypeNode::Type primType;
		switch (node.type) {
			case LiteralNode::NUMBER:  primType = PrimitiveTypeNode::NUMBER; break; // Or infer INT/FLOAT
			case LiteralNode::STRING:  primType = PrimitiveTypeNode::STRING; break;
			case LiteralNode::BOOL:    primType = PrimitiveTypeNode::BOOL; break;
			case LiteralNode::NIL:     /* TODO: Handle nil type */ return nullptr; // Or a specific NilTypeNode
			default: /* Should not happen */ return nullptr;
		}
		return std::make_unique<PrimitiveTypeNode>(node.loc, primType);
	}

	std::unique_ptr<TypeNode> SemanticAnalyzer::visit(VarNode& node) {
		SymbolInfo* symbol = symbolTable.lookup(node.name);
		if (!symbol) {
			errorReporter.report(node.loc, "Undeclared identifier '" + node.name + "'.");
			return nullptr;
		}
		if (symbol->kind != SymbolInfo::VARIABLE && symbol->kind != SymbolInfo::FUNCTION /* maybe others? */) {
			errorReporter.report(node.loc, "'" + node.name + "' is not a variable or function (it's a " + std::to_string(symbol->kind) + ").");
			return nullptr; // Or return symbol->type if appropriate? Depends on context.
		}

		// Return a *copy* of the type from the symbol table
		return symbol->type ? symbol->type->clone() : nullptr;
	}

	std::unique_ptr<TypeNode> SemanticAnalyzer::visit(BinaryOpNode& node) {
		auto leftType = visitExpression(*node.left);
		auto rightType = visitExpression(*node.right);

		if (!leftType || !rightType) {
			return nullptr; // Error occurred in operand(s)
		}

		// --- Assignment Check ---
		if (node.op == BinaryOpNode::ASN || node.op == BinaryOpNode::ADD_ASN || node.op == BinaryOpNode::SUB_ASN || /* etc. */ false) {
			// 1. Check if left is assignable (l-value)
			bool isLValue = dynamic_cast<VarNode*>(node.left.get()) ||
			                dynamic_cast<MemberAccessNode*>(node.left.get()) ||
			                dynamic_cast<ArrayAccessNode*>(node.left.get());
			if (!isLValue) {
				errorReporter.report(node.left->loc, "Left side of assignment must be an assignable variable, member, or array element.");
				return nullptr;
			}

			// 2. Check for const assignment
			if (isLValue) {
				// Need to get the symbol info for the VarNode/Member/etc. to check isConst
				// This requires enhancing visit(VarNode) etc. or a helper function.
				// Simplified check:
				if (auto* var = dynamic_cast<VarNode*>(node.left.get())) {
					SymbolInfo* sym = symbolTable.lookup(var->name);
					if (sym && sym->isConst) {
						errorReporter.report(node.loc, "Cannot assign to constant variable '" + var->name + "'.");
						// Still return type for further analysis, but error reported
					}
				}
				// TODO: Add checks for const members/array elements
			}


			// 3. Check type compatibility
			if (!areTypesCompatible(leftType.get(), rightType.get())) {
				errorReporter.report(node.right->loc, "Type mismatch: cannot assign '" + typeToString(rightType.get()) + "' to '" + typeToString(leftType.get()) + "'.");
				// Return left type despite error? Or null? Let's return left type.
			}

			// Assignment result type is the type of the left operand
			return leftType;
		}

		// --- Other Binary Operations ---
		// TODO: Implement type checking logic for each operator (+, -, *, /, ==, !=, <, >, etc.)
		// This involves checking if leftType and rightType are valid for the 'op',
		// and determining the result type.
		// Example: Arithmetic ops need numeric types, comparison ops return bool.

		// Placeholder: Assume result type is same as left for now (INCORRECT!)
		if (node.op >= BinaryOpNode::EQ && node.op <= BinaryOpNode::GTE) {
			// Comparison operators return Bool
			return std::make_unique<PrimitiveTypeNode>(node.loc, PrimitiveTypeNode::BOOL);
		}
		else {
			// Placeholder: Arithmetic/Modulo - need proper numeric check & promotion
			// For now, just return left type as a placeholder
			return leftType;
		}

		// return nullptr; // If operation is invalid for the types
	}

	std::unique_ptr<TypeNode> SemanticAnalyzer::visit(UnaryOpNode& node) {
		auto rightType = visitExpression(*node.right);
		if (!rightType) return nullptr;

		// Check if operand is valid for INC/DEC (usually numeric)
		// TODO: Proper numeric check needed
		bool isNumeric = false;
		if (auto* prim = dynamic_cast<PrimitiveTypeNode*>(rightType.get())) {
			isNumeric = (prim->type == PrimitiveTypeNode::INT || prim->type == PrimitiveTypeNode::FLOAT || /* ... */ prim->type == PrimitiveTypeNode::NUMBER);
		}
		if (!isNumeric) {
			errorReporter.report(node.right->loc, "Operator '" + std::string(node.op == UnaryOpNode::INC ? "++" : "--") + "' requires a numeric operand, found '" + typeToString(rightType.get()) + "'.");
			return nullptr;
		}

		// Check if operand is an l-value
		bool isLValue = dynamic_cast<VarNode*>(node.right.get()) ||
		                dynamic_cast<MemberAccessNode*>(node.right.get()) ||
		                dynamic_cast<ArrayAccessNode*>(node.right.get());
		if (!isLValue) {
			errorReporter.report(node.right->loc, "Operand for '" + std::string(node.op == UnaryOpNode::INC ? "++" : "--") + "' must be an assignable variable, member, or array element.");
			return nullptr;
		}
		// TODO: Check for const assignment attempt (similar to BinaryOpNode assignment)


		// Result type is the same as the operand type
		return rightType;
	}


	std::unique_ptr<TypeNode> SemanticAnalyzer::visit(CallNode& node) {
		auto calleeType = visitExpression(*node.callee);
		if (!calleeType) return nullptr;

		// TODO: Check if calleeType is actually a function type
		// This requires a proper FunctionTypeNode or similar mechanism.
		bool isFunction = (calleeType->kind == TypeNode::FUNCTION); // Basic check
		// If calleeType is VarNode, look up symbol to get Function info
		SymbolInfo* funcSymbol = nullptr;
		if (auto* var = dynamic_cast<VarNode*>(node.callee.get())) {
			funcSymbol = symbolTable.lookup(var->name);
			if (funcSymbol && funcSymbol->kind == SymbolInfo::FUNCTION) {
				isFunction = true;
			} else {
				isFunction = false; // Overwrite basic check if it's a var but not a function symbol
			}
		}
		// TODO: Handle MemberAccessNode for method calls

		if (!isFunction) {
			errorReporter.report(node.callee->loc, "Expression is not callable (not a function or method).");
			return nullptr;
		}

		// TODO: Get expected parameter types from funcSymbol or calleeType (if FunctionTypeNode)
		// std::vector<TypeNode*> expectedParamTypes = funcSymbol->parameters; // Or similar

		// Visit arguments and get their types
		std::vector<std::unique_ptr<TypeNode>> argTypes;
		argTypes.reserve(node.arguments.size());
		for (auto& arg : node.arguments) {
			argTypes.push_back(visitExpression(*arg));
			if (!argTypes.back()) {
				// Error in argument, stop checking this call?
				return nullptr;
			}
		}

		// TODO: Check argument count against expected parameter count (handle varargs)
		// TODO: Check type compatibility for each argument vs parameter (using areTypesCompatible)
		// Example check (needs expectedParamTypes):
		// if (argTypes.size() != expectedParamTypes.size()) { /* Error */ }
		// for (size_t i = 0; i < argTypes.size(); ++i) {
		//     if (!areTypesCompatible(expectedParamTypes[i], argTypes[i].get())) { /* Error */ }
		// }

		// TODO: Return the function's return type
		// return funcSymbol->type->clone(); // Assuming symbol's type *is* the return type (or get from FunctionTypeNode)
		// Placeholder:
		return std::make_unique<TypeNode>(node.loc, TypeNode::DYNAMIC); // Needs actual return type!
	}

	std::unique_ptr<TypeNode> SemanticAnalyzer::visit(MemberAccessNode& node) {
		auto objectType = visitExpression(*node.object);
		if (!objectType) return nullptr;

		// TODO: Check if objectType is a class/struct/actor type that has members
		// This requires looking up the type definition (e.g., from the symbol table if it's a NamedTypeNode)
		// Example:
		// if (auto* named = dynamic_cast<NamedTypeNode*>(objectType.get())) {
		//     SymbolInfo* classSymbol = symbolTable.lookup(named->name);
		//     if (!classSymbol || (classSymbol->kind != SymbolInfo::CLASS && ...)) { /* Error: not an object type */ }
		//
		//     // Look up the member within the class definition
		//     SymbolInfo* memberSymbol = symbolTable.lookupClassMember(named->name, node.member);
		//     if (!memberSymbol) { /* Error: member not found */ }
		//
		//     // TODO: Check access modifiers (public/protected/private) based on currentClass context
		//
		//     return memberSymbol->type->clone(); // Return member's type
		// } else {
		//     errorReporter.report(node.object->loc, "Cannot access member '" + node.member + "' on non-object type '" + typeToString(objectType.get()) + "'.");
		//     return nullptr;
		// }

		// Placeholder:
		return std::make_unique<TypeNode>(node.loc, TypeNode::DYNAMIC);
	}


	std::unique_ptr<TypeNode> SemanticAnalyzer::visit(ArrayAccessNode& node) {
		auto arrayType = visitExpression(*node.array);
		auto indexType = visitExpression(*node.index);

		if (!arrayType || !indexType) return nullptr;

		// Check if index is an integer type
		bool isIndexInt = false;
		if (auto* prim = dynamic_cast<PrimitiveTypeNode*>(indexType.get())) {
			isIndexInt = (prim->type == PrimitiveTypeNode::INT || prim->type == PrimitiveTypeNode::NUMBER /* or other integrals */);
		}
		if (!isIndexInt) {
			errorReporter.report(node.index->loc, "Array index must be an integer type, found '" + typeToString(indexType.get()) + "'.");
			// Continue to check array type, but result will be uncertain
		}

		// Check if arrayType is actually an array and get element type
		if (auto* arrT = dynamic_cast<ArrayTypeNode*>(arrayType.get())) {
			return arrT->elementType->clone(); // Return element type
		} else {
			// TODO: Check for types that overload operator[]
			errorReporter.report(node.array->loc, "Type '" + typeToString(arrayType.get()) + "' does not support array access ([]).");
			return nullptr;
		}
	}

	std::unique_ptr<TypeNode> SemanticAnalyzer::visit(NewExprNode& node) {
		// Look up the class name
		SymbolInfo* classSymbol = symbolTable.lookup(node.className);
		if (!classSymbol || (classSymbol->kind != SymbolInfo::CLASS && classSymbol->kind != SymbolInfo::STRUCT && classSymbol->kind != SymbolInfo::ACTOR)) {
			errorReporter.report(node.loc, "Cannot find class, struct, or actor named '" + node.className + "' to instantiate.");
			return nullptr;
		}

		// TODO: Find the appropriate constructor based on argument types
		// This involves:
		// 1. Getting the list of constructors for the classSymbol.
		// 2. Visiting argument expressions to get their types.
		// 3. Performing overload resolution to find the best matching constructor.
		// 4. Checking type compatibility for arguments vs constructor parameters.

		// For now, assume a default constructor or skip detailed check

		// Return the type of the class being instantiated
		// The type is stored in the classSymbol itself (or create new NamedTypeNode)
		return std::make_unique<NamedTypeNode>(node.loc, node.className);
	}

	std::unique_ptr<TypeNode> SemanticAnalyzer::visit(ThisNode& node) {
		if (!currentClass) {
			errorReporter.report(node.loc, "'this' keyword used outside of a class method context.");
			return nullptr;
		}
		// TODO: Check if inside a *static* method if applicable

		// Return the type of the current class
		return std::make_unique<NamedTypeNode>(node.loc, currentClass->name);
	}

	std::unique_ptr<TypeNode> SemanticAnalyzer::visit(FreeObjectNode& node) {
		// Type checking for free objects (like JS objects or dictionaries)
		// - Visit each property value expression.
		// - Check for duplicate property names? (Maybe allowed)
		// The resulting type is often dynamic or a specific "object/map" type.
		for(auto& prop : node.properties) {
			if(prop.second) visitExpression(*prop.second); // Check for errors in values
		}
		// TODO: Return a specific Map/Dictionary/Object type if the language has one
		return std::make_unique<TypeNode>(node.loc, TypeNode::Kind::DYNAMIC); // Placeholder
	}

	std::unique_ptr<TypeNode> SemanticAnalyzer::visit(TemplateStringNode& node) {
		// Visit each embedded expression part
		for(auto& part : node.parts) {
			if(part) {
				auto partType = visitExpression(*part);
				// Optional: Check if partType is convertible to string if needed
			}
		}
		// Result is always a string
		return std::make_unique<PrimitiveTypeNode>(node.loc, PrimitiveTypeNode::STRING);
	}

	std::unique_ptr<TypeNode> SemanticAnalyzer::visit(StructInitializerNode& node) {
		// This usually appears where a specific struct type is expected (e.g., assignment, function call).
		// The type checking needs context from the expected type.
		// Example: `MyStruct s = { x: 10, y: "hi" };` or `foo({ a: 1, b: 2 })`
		// 1. Determine the expected struct type from context.
		// 2. Look up the struct definition.
		// 3. Visit initializer expressions.
		// 4. Match initializers to fields (by name or position).
		// 5. Check type compatibility for each field.
		// 6. Check if all required fields are initialized.

		// Without context, we can only check the expressions themselves.
		for(auto& field : node.fields) {
			if(field.value) visitExpression(*field.value);
		}

		// TODO: Return the specific struct type IF context allows inference, otherwise maybe dynamic/error?
		errorReporter.reportWarning(node.loc, "Context-dependent struct initializer - full type check requires assignment or function call context.");
		return std::make_unique<TypeNode>(node.loc, TypeNode::Kind::DYNAMIC); // Placeholder
	}


	// --- Others ---
	void SemanticAnalyzer::visit(AnnotationNode& node) {
		// TODO: Validate annotation usage
		// - Check if annotation name exists / is applicable in this context
		// - Visit argument expressions and check types against annotation definition
	}

	void SemanticAnalyzer::visit(TemplateDeclNode& node) {
		// TODO: Handle template declarations
		// - Store the template definition associated with its name.
		// - Don't fully analyze the inner 'declaration' yet, only when instantiated.
		// - Might do basic syntax checks on parameters.
	}
	// class SemanticAnalyzer

} // namespace zenith