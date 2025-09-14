/*
 * Copyright (c) 2023 Christopher Taylor
 *
 * SPDX-License-Identifier: BSL-1.0
 * Distributed under the Boost Software License, Version 1.0. *(See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#pragma once

#ifndef __CHPLX_PROGRAMTREE_HPP__
#define __CHPLX_PROGRAMTREE_HPP__

#include "chpl/uast/AstNode.h"

#include "symboltypes.hpp"

#include <string>
#include <variant>
#include <utility>
#include <tuple>
#include <vector>
#include <ostream>

using namespace chpl;
using namespace chpl::ast::visitors::hpx;

namespace chplx { namespace ast { namespace hpx {

struct ExpressionBase {
   std::size_t scopeId;
};

struct VariableDeclarationExpression : public ExpressionBase {
   std::string identifier;
   kind_types kind;
   std::string chplLine;
   int qualifier;
   bool config;
};

struct ScalarDeclarationExpression : public VariableDeclarationExpression {
   void emit(std::ostream & os) const;
};

struct ScalarDeclarationLiteralExpression : public VariableDeclarationExpression {
   std::vector<uast::AstNode const*> literalValue;
   void emit(std::ostream & os) const;
};

struct ScalarDeclarationLiteralExpressionVisitor {
    template<typename T>
    void operator()(T const&) {}

    void operator()(bool_kind const&);
    void operator()(byte_kind const&);
    void operator()(int_kind const&);
    void operator()(real_kind const&);
    void operator()(string_kind const&);
    void operator()(std::shared_ptr<array_kind> const&);

    uast::AstNode const* ast;
    std::ostream & os;
};

struct ArrayDeclarationExpression : public VariableDeclarationExpression {
   void emit(std::ostream & os) const;
};

struct ArrayDeclarationLiteralExpression : public VariableDeclarationExpression {
   std::vector<Symbol*> literalValues;
   void emit(std::ostream & os) const;
};

struct TupleDeclarationExpression : public VariableDeclarationExpression {
   void emit(std::ostream & os) const;
};

struct ArithmeticOpExpression : public ExpressionBase {
   std::string op;
   uast::AstNode const *ast;
};

struct LiteralExpression {
   kind_types kind;
   uast::AstNode const * value;
   void emit(std::ostream & os) const;
};

struct TupleDeclarationLiteralExpression : public VariableDeclarationExpression {
   std::vector<Symbol> literalValues;
   void emit(std::ostream & os) const;
};

struct VariableExpression {
   std::shared_ptr<Symbol> sym;
   void emit(std::ostream & os) const;
};

struct OpExpression {
   std::shared_ptr<Symbol> sym;
};

struct UnaryOpExpression;
struct BinaryOpExpression;
struct TernaryOpExpression;

struct ReturnExpression;
struct ScopeExpression;
struct FunctionDeclarationExpression;
struct FunctionCallExpression;
struct ConditionalExpression;

struct ForLoopExpression;
struct ForallLoopExpression;
struct CoforallLoopExpression;

struct StatementList;
struct ScalarDeclarationExprExpression;
struct ArrayDeclarationExprExpression;
struct TupleDeclarationExprExpression;

struct RecordDeclarationExpression;
struct ClassDeclarationExpression;
struct ModuleDeclarationExpression;
struct OnExpression;

using Statement = std::variant<std::monostate,           // 0
    std::shared_ptr<StatementList>,                      // 1
    ScalarDeclarationExpression,                         // 2
    ScalarDeclarationLiteralExpression,                  // 3
    std::shared_ptr<ScalarDeclarationExprExpression>,    // 4
    ArrayDeclarationExpression,                          // 5
    ArrayDeclarationLiteralExpression,                   // 6
    std::shared_ptr<ArrayDeclarationExprExpression>,     // 7
    TupleDeclarationExpression,                          // 8
    TupleDeclarationLiteralExpression,                   // 9
    std::shared_ptr<TupleDeclarationExprExpression>,     // 10
    LiteralExpression,                                   // 11
    VariableExpression,                                  // 12
    std::shared_ptr<UnaryOpExpression>,                  // 13
    std::shared_ptr<BinaryOpExpression>,                 // 14
    std::shared_ptr<TernaryOpExpression>,                // 15
    std::shared_ptr<ReturnExpression>,                   // 16
    std::shared_ptr<ScopeExpression>,                    // 17
    std::shared_ptr<FunctionDeclarationExpression>,      // 18
    std::shared_ptr<FunctionCallExpression>,             // 19
    std::shared_ptr<ConditionalExpression>,              // 20
    std::shared_ptr<ForLoopExpression>,                  // 21
    std::shared_ptr<ForallLoopExpression>,               // 22
    std::shared_ptr<CoforallLoopExpression>,             // 23
    std::shared_ptr<OnExpression>,                       // 24
    std::shared_ptr<RecordDeclarationExpression>,        // 25
    std::shared_ptr<ClassDeclarationExpression>,         // 26
    std::shared_ptr<ModuleDeclarationExpression>         // 27
    >;

struct StatementList {
   std::vector<Statement> statements;
};

struct UnaryOpExpression : public ArithmeticOpExpression {
   std::vector<Statement> statements;

   void emit(std::ostream & os) const;
};

struct BinaryOpExpression : public ArithmeticOpExpression {
   std::vector<Statement> statements;

   void emit(std::ostream & os) const;
};

struct TernaryOpExpression : public ArithmeticOpExpression {
   std::vector<Statement> statements;

   void emit(std::ostream & os) const;
};

struct ScalarDeclarationExprExpression : public VariableDeclarationExpression {
   std::vector<Statement> statements;

   void emit(std::ostream & os) const;
};

struct ArrayDeclarationExprExpression : public VariableDeclarationExpression {
   std::vector<Statement> statements;

   void emit(std::ostream & os) const;
};

struct TupleDeclarationExprExpression : public VariableDeclarationExpression {
   std::vector<Statement> statements;

   void emit(std::ostream & os) const;
};

struct ReturnExpression {
   std::vector<Statement> statement;
   std::string chplLine;
   void emit(std::ostream & os) const;
};

struct ScopeExpression : public ExpressionBase {
   uast::AstNode const* node;
   SymbolTable symbolTable;

   void emit(std::ostream & os) const;
};

struct FunctionDeclarationExpression : public ScopeExpression {
   Symbol symbol;
   std::vector<Statement> statements;
   std::string chplLine;

   void emit(std::ostream & os) const;
};

struct FunctionCallExpression : public ExpressionBase {
   Symbol symbol;
   std::vector<Statement> arguments;
   std::string chplLine;
   SymbolTable & symbolTable;

   void emit(std::ostream & os) const;
};

struct ConditionedExpression : public ScopeExpression {
   std::vector<Statement> conditions; 
   std::vector<Statement> statements; 
   void emit(std::ostream & os) const;
};

struct ConditionalExpression : public ScopeExpression {
   Symbol symbol;
   std::vector<ConditionedExpression> exprs;
   void emit(std::ostream & os) const;
};

struct ForLoopExpression : public ScopeExpression {
   Symbol symbol;
   std::optional<Symbol> iterator;
   // this needs to store Statements
   //Symbol indexSet;
   std::vector<Statement> indexSet;
   std::vector<Statement> statements;
   std::string chplLine;

   bool isArrayInitForLoop = false;

   void emit(std::ostream & os) const;
};

struct ForallLoopExpression : public ScopeExpression {
   Symbol symbol;
   std::vector<std::optional<Symbol>> iterator;
   //Symbol indexSet;
   std::vector<Statement> indexSet;
   std::vector<Statement> statements;
   std::string chplLine;
   
   bool isZippedIter = false;

   void emit(std::ostream & os) const;
};

struct CoforallLoopExpression : public ScopeExpression {
   Symbol symbol;
   std::optional<Symbol> iterator;
   //Symbol indexSet;
   std::vector<Statement> indexSet;
   std::vector<Statement> statements;
   std::string chplLine;

   void emit(std::ostream & os) const;
};

struct OnExpression : public ScopeExpression {
   Symbol symbol;
   std::optional<Symbol> OnLocale;
   std::vector<Statement> OnLocaleVarExpr;
   std::vector<Statement> OnLocaleVarsUsedInExpr;
   std::vector<Statement> statements;
   std::string chplLine;

   void emit(std::ostream & os) const;
};

struct RecordDeclarationExpression : public ScopeExpression {
   Symbol symbol;
   std::vector<Statement> statements;
   std::string chplLine;

   void emit(std::ostream & os) const;
};

struct ClassDeclarationExpression : public RecordDeclarationExpression {
   void emit(std::ostream & os) const;
};

struct ModuleDeclarationExpression : public RecordDeclarationExpression {
   void emit(std::ostream & os) const;
};

struct ProgramTree {

   std::vector<Statement> statements;

   ProgramTree() : statements() {}
};

/*
   std::shared_ptr<BeginExpression>,
   std::shared_ptr<CobeginExpression>,
   std::shared_ptr<MethodCallExpression>,
*/

} /* namespace hpx */ } /* namespace ast */ } /* namespace chplx */

#endif
