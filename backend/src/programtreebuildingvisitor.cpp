/*
 * Copyright (c) 2023 Christopher Taylor
 *
 * SPDX-License-Identifier: BSL-1.0
 * Distributed under the Boost Software License, Version 1.0. *(See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#include "chpl/uast/Function.h"
#include "hpx/programtree.hpp"
#include "hpx/programtreebuildingvisitor.hpp"
#include "hpx/utils.hpp"
#include "chpl/uast/all-uast.h"

#include <system_error>
#include <variant>
#include <fstream>
#include <cctype>
#include <numeric>
#include <sstream>
#include <string>
#include <filesystem>
#include <cassert>

using namespace chplx::ast::hpx;
using namespace chpl::uast;
using namespace chpl::ast::visitors::hpx;

// global options
extern bool suppressLineDirectives;
extern bool fullFilePath;

namespace chplx { namespace ast { namespace visitors { namespace hpx {

std::unordered_map<std::string, int> ProgramTreeBuildingVisitor::operatorEncoder = {
    {"=",  0},
    {"+",  1},
    {"-",  2},
    {"*",  3},
    {"/",  4},
    {"%",  5},
    {"[]", 6},
    {"==", 7},
    {"<=>",8},
    {"<<", 9},
    {">>", 10}
};

struct VariableVisitor {

   const std::size_t scopePtr;
   std::string identifier;
   Symbol & sym;
   std::vector<Statement> & curStmts;
   chpl::uast::BuilderResult const& br;
   Context* ctx = nullptr;
   uast::AstNode const* ast;

   std::string emitChapelLine(uast::AstNode const* ast) const {
      auto const fp = br.filePath();
      return chplx::util::emitLineDirective(fp.c_str(), br.idToLocation(ctx, ast->id(), fp).line());
   }

   template<typename T>
   void operator()(T const& t) {
   }
   void operator()(byte_kind const&) {
      curStmts.emplace_back(ScalarDeclarationExpression{{{scopePtr}, identifier, sym.kind, emitChapelLine(ast), sym.kindqualifier, sym.isConfig}});
   }
   void operator()(bool_kind const&) {
      curStmts.emplace_back(ScalarDeclarationExpression{{{scopePtr}, identifier, sym.kind, emitChapelLine(ast), sym.kindqualifier, sym.isConfig}});
   }
   void operator()(int_kind const& i) {
      curStmts.emplace_back(ScalarDeclarationExpression{{{scopePtr}, identifier, sym.kind, emitChapelLine(ast), sym.kindqualifier, sym.isConfig}});
   }
   void operator()(real_kind const&) {
      curStmts.emplace_back(ScalarDeclarationExpression{{{scopePtr}, identifier, sym.kind, emitChapelLine(ast), sym.kindqualifier, sym.isConfig}});
   }
   void operator()(complex_kind const&) {
      curStmts.emplace_back(ScalarDeclarationExpression{{{scopePtr}, identifier, sym.kind, emitChapelLine(ast), sym.kindqualifier, sym.isConfig}});
   }
   void operator()(string_kind const&) {
      curStmts.emplace_back(ScalarDeclarationExpression{{{scopePtr}, identifier, sym.kind, emitChapelLine(ast), sym.kindqualifier, sym.isConfig}});
   }
   void operator()(expr_kind const&) {
      curStmts.emplace_back(
         std::make_shared<ScalarDeclarationExprExpression>(ScalarDeclarationExprExpression{{{scopePtr}, identifier, sym.kind, emitChapelLine(ast), sym.kindqualifier, sym.isConfig},{}})
      );
   }
   void operator()(template_kind const&) {
   }
   void operator()(range_kind const&) {
   }
   void operator()(domain_kind const&) {
   }
   void operator()(std::shared_ptr<func_kind> const&) {
      curStmts.emplace_back(
         std::make_shared<ScalarDeclarationExprExpression>(ScalarDeclarationExprExpression{{{scopePtr}, identifier, sym.kind, emitChapelLine(ast), sym.kindqualifier, sym.isConfig},{}})
      );
   }
   void operator()(std::shared_ptr<record_kind> const&) {
   }
   void operator()(std::shared_ptr<class_kind> const&) {
      curStmts.emplace_back(ScalarDeclarationExpression{{{scopePtr}, identifier, sym.kind, emitChapelLine(ast), sym.kindqualifier, sym.isConfig}});
   }
   void operator()(std::shared_ptr<array_kind> const& t) {
        curStmts.emplace_back(ArrayDeclarationExpression{{{scopePtr}, identifier, sym.kind, emitChapelLine(ast), sym.kindqualifier, sym.isConfig}});
   }
   void operator()(std::shared_ptr<associative_kind> const&) {
   }
};

struct VariableLiteralVisitor {

   const std::size_t scopePtr;
   std::string identifier;
   Symbol & sym;
   std::vector<Statement> & curStmts;
   chpl::uast::BuilderResult const& br;
   Context* ctx = nullptr;
   uast::AstNode const* ast;

   std::string emitChapelLine(uast::AstNode const* ast) const {
      auto const fp = br.filePath();
      return chplx::util::emitLineDirective(fp.c_str(), br.idToLocation(ctx, ast->id(), fp).line());
   }

  template<typename T>
   void operator()(T const& t) {
   }

   void operator()(byte_kind const&) {
      curStmts.push_back(ScalarDeclarationLiteralExpression{{{scopePtr}, identifier, sym.kind, emitChapelLine(ast), sym.kindqualifier, sym.isConfig}, sym.literal});
   }
   void operator()(bool_kind const&) {
      curStmts.push_back(ScalarDeclarationLiteralExpression{{{scopePtr}, identifier, sym.kind, emitChapelLine(ast), sym.kindqualifier, sym.isConfig}, sym.literal});
   }
   void operator()(int_kind const&) {
      curStmts.push_back(ScalarDeclarationLiteralExpression{{{scopePtr}, identifier, sym.kind, emitChapelLine(ast), sym.kindqualifier, sym.isConfig}, sym.literal});
   }
   void operator()(real_kind const&) {
      curStmts.push_back(ScalarDeclarationLiteralExpression{{{scopePtr}, identifier, sym.kind, emitChapelLine(ast), sym.kindqualifier, sym.isConfig}, sym.literal});
   }
   void operator()(complex_kind const&) {
      curStmts.push_back(ScalarDeclarationLiteralExpression{{{scopePtr}, identifier, sym.kind, emitChapelLine(ast), sym.kindqualifier, sym.isConfig}, sym.literal});
   }
   void operator()(string_kind const&) {
      curStmts.push_back(ScalarDeclarationLiteralExpression{{{scopePtr}, identifier, sym.kind, emitChapelLine(ast), sym.kindqualifier, sym.isConfig}, sym.literal});
   }
   void operator()(expr_kind const&) {
      curStmts.emplace_back(
         std::make_shared<ScalarDeclarationExprExpression>(ScalarDeclarationExprExpression{{{scopePtr}, identifier, sym.kind, emitChapelLine(ast), sym.kindqualifier, sym.isConfig},{}})
      );
   }
   void operator()(template_kind const&) {
   }
   void operator()(range_kind const&) {
   }
   void operator()(domain_kind const&) {
   }
   void operator()(std::shared_ptr<func_kind> const&) {
   }
   void operator()(std::shared_ptr<record_kind> const&) {
   }
   void operator()(std::shared_ptr<class_kind> const&) {
   }
   void operator()(std::shared_ptr<array_kind> const& ak) {
      chplx::util::dout << "Array Declaration Literal: " << identifier << std::endl;
      curStmts.push_back(ArrayDeclarationLiteralExpression{{{scopePtr}, identifier, sym.kind, emitChapelLine(ast), sym.kindqualifier, sym.isConfig}, {}});
      // literal arrays always have 1 domain in the symboltable
      //
      assert(ak->args.size() == 1);
      std::get<ArrayDeclarationLiteralExpression>(curStmts.back()).literalValues.push_back(&(ak->args[0]));
   }
   void operator()(std::shared_ptr<associative_kind> const&) {
   }
};

std::string ProgramTreeBuildingVisitor::emitChapelLine(uast::AstNode const* ast) {
   auto const fp = br.filePath();
   return chplx::util::emitLineDirective(fp.c_str(), br.idToLocation(ctx, ast->id(), fp).line());
}

bool ProgramTreeBuildingVisitor::enter(const uast::AstNode * ast) {
   if(chplx::util::compilerDebug) {
      chplx::util::dout << "***Enter AST Node\t" << tagToString(ast->tag()) << std::endl
                << "***\tCurrent Scope\t" << symbolTable.symbolTableRef->id << std::endl
                << "***\tCurrent Scope id \t" << symbolTableRef->id << std::endl
                << "***\tCurrent Statement List Size\t" << curStmts.size() << std::endl
                << "***\t" << emitChapelLine(ast);
   }

   switch(ast->tag()) {
    case asttags::AnonFormal:
    break;
    case asttags::As:
    break;
    case asttags::Array:
    break;
    case asttags::Attribute:
    break;
    case asttags::Break:
    break;
    case asttags::Comment:
    break;
    case asttags::Continue:
    break;
    case asttags::Delete:
    break;
    case asttags::Domain:
    break;
    case asttags::Dot:
    {
    const std::string field_name = dynamic_cast<const Dot*>(ast)->field().str();
    const std::string class_name = dynamic_cast<Identifier const*>(
        dynamic_cast<const Dot*>(ast)->receiver())
                                       ->name()
                                       .c_str();
    const std::string func_name = (class_name == "here" ? ("chplx::") : "") +
        class_name + "." + field_name;
    chplx::util::dout << "Dot Expression: " << func_name << std::endl;
    auto sym = Symbol{{std::make_shared<func_kind>(
                           func_kind{{{}, func_name, {}, int_kind{}}}),
        std::string{func_name}, {}, -1, false, symbolTable.symbolTableRef->id}};
    std::vector<Statement>* cStmts = curStmts.back();
    if (cStmts->size() &&
        std::holds_alternative<ScalarDeclarationExpression>(cStmts->back()))
    {
        ScalarDeclarationExpression stmt =
            std::get<ScalarDeclarationExpression>(cStmts->back());
        cStmts->pop_back();
        cStmts->emplace_back(std::make_shared<BinaryOpExpression>(
            BinaryOpExpression{{{symbolTableRef->id}, "=", ast}, {}}));
        auto& bo =
            std::get<std::shared_ptr<BinaryOpExpression>>(cStmts->back());

        bo->statements.emplace_back(
            std::make_shared<ScalarDeclarationExprExpression>(
                ScalarDeclarationExprExpression{
                    {{stmt.scopeId}, stmt.identifier, stmt.kind, stmt.chplLine,
                        stmt.qualifier, stmt.config},
                    {}}));
        auto fce = std::make_shared<FunctionCallExpression>(
            FunctionCallExpression{{symbolTableRef->id}, std::move(sym), {},
                emitChapelLine(ast), symbolTable});
        bo->statements.emplace_back(fce);
        curStmts.push_back(&(bo->statements));
        pushedDot = true;
        return true;
      }
      else if (cStmts->size() &&
          std::holds_alternative<VariableExpression>(cStmts->back()))
      {
          VariableExpression stmt =
              std::get<VariableExpression>(cStmts->back());
          cStmts->pop_back();
          std::shared_ptr<BinaryOpExpression> bop;
          if (std::holds_alternative<std::shared_ptr<BinaryOpExpression>>(
                  curStmts[curStmts.size() - 2]->back()))
          {
              bop = std::get<std::shared_ptr<BinaryOpExpression>>(
                  curStmts[curStmts.size() - 2]->back());
              auto fce = std::make_shared<FunctionCallExpression>(
                  FunctionCallExpression{{symbolTableRef->id}, std::move(sym),
                      {}, emitChapelLine(ast), symbolTable});
              bop->statements.emplace_back(stmt);
              bop->statements.emplace_back(fce);
              curStmts.push_back(&(bop->statements));
              pushedDot = true;
              return true;
          }
          else
          {
              bop = std::make_shared<BinaryOpExpression>(
                  BinaryOpExpression{{{symbolTableRef->id}, "=", ast}, {}});
          }
          cStmts->emplace_back(bop);

          bop->statements.emplace_back(stmt);
          auto fce = std::make_shared<FunctionCallExpression>(
              FunctionCallExpression{{symbolTableRef->id}, std::move(sym), {},
                  emitChapelLine(ast), symbolTable});
          bop->statements.emplace_back(fce);

          curStmts.push_back(&(bop->statements));
          pushedDot = true;
          return true;
      }
      else if (std::holds_alternative<std::shared_ptr<BinaryOpExpression>>(
                   cStmts->back()))
      {
          auto& bop =
              std::get<std::shared_ptr<BinaryOpExpression>>(cStmts->back());
          auto fce = std::make_shared<FunctionCallExpression>(
              FunctionCallExpression{{symbolTableRef->id}, std::move(sym), {},
                  emitChapelLine(ast), symbolTable});
          std::vector<Statement> bop_stmts_new;
          for (auto& stmt : bop->statements)
          {
              if (std::holds_alternative<VariableExpression>(stmt))
              {
                  auto& varExpr = std::get<VariableExpression>(stmt);
                  bop_stmts_new.emplace_back(varExpr);
                  break;
              }
          }
          bop_stmts_new.emplace_back(fce);
          bop->statements = std::move(bop_stmts_new);
          curStmts.push_back(&(bop->statements));
          pushedDot = true;
          return true;
      }
      else if (curStmts.size() > 1 &&
          std::holds_alternative<std::shared_ptr<BinaryOpExpression>>(
              curStmts[curStmts.size() - 2]->back()))
      {
          auto& bop = std::get<std::shared_ptr<BinaryOpExpression>>(
              curStmts[curStmts.size() - 2]->back());
          auto bop_sym = bop->statements.back();
          auto& bop_stmts = bop->statements;
          if (std::holds_alternative<VariableExpression>(bop->statements[0]))
          {
              auto& varExpr = std::get<VariableExpression>(bop->statements[0]);
          }
          return true;
      }
      else if (cStmts->size() &&
          std::holds_alternative<
              std::shared_ptr<ScalarDeclarationExprExpression>>(cStmts->back()))
      {
          auto& sde =
              std::get<std::shared_ptr<ScalarDeclarationExprExpression>>(
                  cStmts->back());
          cStmts->pop_back();
          cStmts->emplace_back(std::make_shared<BinaryOpExpression>(
              BinaryOpExpression{{{symbolTableRef->id}, "=", ast}, {}}));
          auto& bo =
              std::get<std::shared_ptr<BinaryOpExpression>>(cStmts->back());

          bo->statements.emplace_back(sde);
          auto fce = std::make_shared<FunctionCallExpression>(
              FunctionCallExpression{{symbolTableRef->id}, std::move(sym), {},
                  emitChapelLine(ast), symbolTable});
          bo->statements.emplace_back(fce);
          curStmts.push_back(&(bo->statements));
          pushedDot = true;
          return true;
      }
      else if(cStmts->size() && std::holds_alternative<std::shared_ptr<FunctionCallExpression>>(cStmts->back())) {   
         chplx::util::dout << "345\n";
         std::shared_ptr<FunctionCallExpression> & fce =
            std::get<std::shared_ptr<FunctionCallExpression>>(cStmts->back());
         fce->arguments.push_back(std::make_shared<FunctionCallExpression>(
             FunctionCallExpression{{symbolTableRef->id}, std::move(sym), {},
                 emitChapelLine(ast), symbolTable}));
         pushedDot = true;
         return true;
      }
      else
      {
         chplx::util::dout << "356\n";
          // this is the case where dot expression is used somewhere without assignment
          if(cStmts->size())
          chplx::util::dout << "cStmts back kind: " << cStmts->back().index() << std::endl;
         if(curStmts.size() > 1) chplx::util::dout << "CurStmts back kind: " << curStmts[curStmts.size()-2]->back().index() << std::endl;
         // if(curStmts.size() > 1 && std::holds_alternative<std::shared_ptr<FunctionCallExpression>>(curStmts[curStmts.size()-2]->back())) {
         //    chplx::util::dout << "Found Function Call Expression in Parent List\n";
         //    std::shared_ptr<FunctionCallExpression> & fce =
         //       std::get<std::shared_ptr<FunctionCallExpression>>(curStmts[curStmts.size()-2]->back());
         //    fce->arguments.push_back(std::make_shared<FunctionCallExpression>(
         //        FunctionCallExpression{{symbolTableRef->id}, std::move(sym), {},
         //            emitChapelLine(ast), symbolTable}));
         //    pushedDot = true;
         //    return true;
         // }
          cStmts->emplace_back(std::make_shared<FunctionCallExpression>(
              FunctionCallExpression{{symbolTableRef->id}, std::move(sym), {},
                  emitChapelLine(ast), symbolTable}));
         //  pushedDot = true;
         specialPushedDot = true;
          return true;
      }
    }
    break;
    case asttags::EmptyStmt:
    break;
    case asttags::ErroneousExpression:
    break;
    case asttags::ExternBlock:
    break;
    case asttags::FunctionSignature:
    break;
    case asttags::Identifier:
    {
       std::vector<Statement> * cStmts = curStmts.back();
       const bool cStmtsnz = 0 < cStmts->size();
       std::string identifier{dynamic_cast<Identifier const*>(ast)->name().c_str()};

       chplx::util::dout << "Identifier: " << identifier << std::endl;
       if(curStmts.size()>1){
         chplx::util::dout << "CurStmts Kind: " << curStmts[curStmts.size()-2]->back().index() << std::endl;
       }

       if (curStmts.size() > 1)
       {
         chplx::util::dout << "402\n";
         auto* parentList = curStmts[curStmts.size() - 2];
         if (!parentList->empty() &&
             std::holds_alternative<std::shared_ptr<FunctionCallExpression>>(
                 parentList->back()))
         {
              chplx::util::dout << "411 Found Function Call Expression in Parent List\n";
              std::optional<Symbol> varsym =
                  symbolTable.find(symbolTableRef->id, identifier);
              auto& fce = std::get<std::shared_ptr<FunctionCallExpression>>(
                  parentList->back());

              auto fce_call = fce->symbol.identifier;
              chplx::util::dout << "Function Call Identifier: " << fce_call << std::endl;
              if(cStmtsnz )
              chplx::util::dout << "cStmts back kind: " << cStmts->back().index() << std::endl;
              if (varsym &&
                  !std::holds_alternative<std::shared_ptr<func_kind>>(
                      varsym->kind) && !std::holds_alternative<std::shared_ptr<cxxfunc_kind>>(
                      varsym->kind) && "here" != varsym->identifier && !pushedDot && !specialPushedDot)
              {
                  chplx::util::dout << "Found Symbol: " << varsym->identifier << " kind: " << varsym->kind.index() << std::endl;
                  fce->arguments.emplace_back(
                      VariableExpression{std::make_shared<Symbol>(*varsym)});
                  chplx::util::dout << "Scope ID: " << varsym->scopeId
                            << " symbol ref " << symbolTableRef->id
                            << " SymbolTableRef ID: " << symbolTable.symbolTableRef->id
                            << " inssideOn counter ID: " << isInsideOn
                            <<  (isInsideOn ? (" OnScopeId : " + std::to_string(currentOnExpr->scopeId)): "")
                            << std::endl;
                  if((isInsideOn > 1 || (isInsideOn && varsym->scopeId < currentOnExpr->scopeId)) && (varsym->scopeId <= isInsideOn || symbolTableRef->id >= varsym->scopeId) && identifier != "here") {
                     chplx::util::dout << "Adding OnLocaleVarsUsedInExpr : " << identifier
                               << " to current statements" << std::endl;
                     currentOnExpr->OnLocaleVarsUsedInExpr.emplace_back(VariableExpression{std::make_shared<Symbol>(*varsym)});
                  }
              return true;

              }
         }
       }

       if (1 < curStmts.size() &&
           0 < curStmts[curStmts.size()-2]->size() &&
           std::holds_alternative<std::shared_ptr<BinaryOpExpression>>( curStmts[curStmts.size()-2]->back() ) ) {
            std::shared_ptr<BinaryOpExpression> & boe = std::get<std::shared_ptr<BinaryOpExpression>>( curStmts[curStmts.size()-2]->back() );
            chplx::util::dout << "Binary Op Identifier: " << boe->op << std::endl;
           std::string identifier{dynamic_cast<Identifier const*>(ast)->name().c_str()};

           std::optional<Symbol> varsym =
               symbolTable.find(symbolTableRef->id, identifier);
           if(!varsym) {
              auto rsym =
                 symbolTable.findPrefix(symbolTableRef->id, identifier);

              if(!rsym) {
                 std::cerr << "chplx error: Undefined symbol \"" << identifier << "\" detected; check\t" << emitChapelLine(ast) << std::endl << std::flush;
                 return false;
              }

              auto itr = rsym->first;
              for(; itr != rsym->second; ++itr) {
                 const auto split = itr->first.find('|');
                 const std::string fnident =
                    itr->first.substr(0, split == std::string::npos ? itr->first.size() : split);

                 if(fnident.size() == identifier.size() && fnident.substr(0, identifier.size()) == identifier) {
                    chplx::util::dout << "410 Found Symbol: " << itr->second.identifier << " kind " << itr->second.kind.index() << std::endl;
                    if(itr->second.identifier.find('|') != std::string::npos) continue;
                    cStmts->emplace_back(VariableExpression{std::make_shared<Symbol>(itr->second)});
                    break;
                 }
              }
           }
           else {
              chplx::util::dout << "403 Found Symbol: " << varsym->identifier
                        << " in scope: " << varsym->scopeId << std::endl;
              chplx::util::dout << "Scope ID: " << symbolTable.symbolTableRef->id
                        << "IsInsideOn: " << isInsideOn << std::endl;
              if(isInsideOn && varsym->scopeId <= isInsideOn && varsym->identifier != "writeln" && varsym->identifier != "here") {
                 chplx::util::dout << "407 Adding OnLocaleVarsUsedInExpr : " << identifier
                           << " to current statements Kind: " << varsym->kind.index() << std::endl;

                 chplx::util::dout << "Scope ID: " << varsym->scopeId
                           << " SymbolTableRef ID: " << symbolTable.symbolTableRef->id
                           << " symbolref: " << symbolTableRef->id
                           << " inssideOn counter ID: " << isInsideOn
                           << std::endl;
                 currentOnExpr->OnLocaleVarsUsedInExpr.emplace_back(VariableExpression{std::make_shared<Symbol>(*varsym)});
              }
               if(pushedDot) {
                  return true;
               }
               chplx::util::dout << "Symbol Kind " << varsym->kind.index() << std::endl;
               if(varsym->identifier != "Locales")
              cStmts->emplace_back(VariableExpression{std::make_shared<Symbol>(*varsym)});
           }
       }
       else if(cStmtsnz && std::holds_alternative<std::shared_ptr<ReturnExpression>>(cStmts->back())) {
           std::string identifier{dynamic_cast<Identifier const*>(ast)->name().c_str()};
           std::shared_ptr<ReturnExpression> & ret = std::get<std::shared_ptr<ReturnExpression>>(cStmts->back());
           std::optional<Symbol> varsym =
               symbolTable.find(symbolTableRef->id, identifier);
           if(varsym) { 
              ret->statement.emplace_back(VariableExpression{std::make_shared<Symbol>(*varsym)});
              return true;
           }

           auto rsym =
              symbolTable.findPrefix(symbolTableRef->id, identifier);

           auto itr = rsym->first;
           for(; itr != rsym->second; ++itr) {
              const auto split = itr->first.find('|');
              const std::string fnident =
                 itr->first.substr(0, split == std::string::npos ? itr->first.size() : split);

              if(fnident.size() == identifier.size() && fnident.substr(0, identifier.size()) == identifier) {
                 ret->statement.emplace_back(VariableExpression{std::make_shared<Symbol>(itr->second)});
                 break;
              }
           }
       }
       else if(1 < curStmts.size() && std::holds_alternative<std::shared_ptr<ForLoopExpression>>(curStmts[curStmts.size()-2]->back())) {
          std::shared_ptr<ForLoopExpression> & fle =
             std::get<std::shared_ptr<ForLoopExpression>>(curStmts[curStmts.size()-2]->back());

           std::string identifier{dynamic_cast<Identifier const*>(ast)->name().c_str()};
           std::optional<Symbol> varsym =
               symbolTable.find(symbolTableRef->id, identifier);
            if(fle->indexSet.size() < 2 && varsym->kind.index() == 5) {
               fle->indexSet.emplace_back(VariableExpression{std::make_shared<Symbol>(*varsym)});
               return true;
            }
           
           if (fle->isArrayInitForLoop)
           {
              auto arrayVarsym = pendingArrayForLoopSymbols.front();
              pendingArrayForLoopSymbols.pop_front();
              auto arrayIdentifier = arrayVarsym->identifier;

              auto varsymInsideForLoop = arrayVarsym;
              std::string iteratorName = fle->iterator->identifier;
              auto arrayIdentifierForLoopExpression =
                  arrayIdentifier + "(" + iteratorName + ")";
              std::vector<Statement>* cStmts = curStmts.back();
              varsymInsideForLoop->kind = std::make_shared<func_kind>(func_kind{
                  {symbolTable.symbolTableRef->id, {}, {}, {}}, true, false});

              cStmts->emplace_back(std::make_shared<BinaryOpExpression>(
                  BinaryOpExpression{{{symbolTableRef->id}, "=", ast}, {}}));
              auto& bo =
                  std::get<std::shared_ptr<BinaryOpExpression>>(cStmts->back());

              bo->statements.emplace_back(

                  VariableExpression{std::make_shared<Symbol>(Symbol{
                      varsymInsideForLoop->kind,
                      arrayIdentifierForLoopExpression,    // This is "arr(i)"
                      {}, -1, false, symbolTableRef->id})});

              bo->statements.emplace_back(
                  VariableExpression{std::make_shared<Symbol>(*varsym)});

              return true;
           }

           

           if(varsym) { 
              if(fle->indexSet.size() < 2) {
                 fle->indexSet.emplace_back(VariableExpression{std::make_shared<Symbol>(*varsym)});
              }
              else if(fle->indexSet.size() && std::holds_alternative<std::shared_ptr<BinaryOpExpression>>(fle->indexSet.back())) {
                auto & bo = std::get<std::shared_ptr<BinaryOpExpression>>(fle->indexSet.back());
                bo->statements.emplace_back(VariableExpression{std::make_shared<Symbol>(*varsym)});
              }
              else {
                cStmts->emplace_back(VariableExpression{std::make_shared<Symbol>(*varsym)});
              }
              return true;
           }

           auto rsym =
              symbolTable.findPrefix(symbolTableRef->id, identifier);

           auto itr = rsym->first;
           for(; itr != rsym->second; ++itr) {
              const auto split = itr->first.find('|');
              const std::string fnident =
                 itr->first.substr(0, split == std::string::npos ? itr->first.size() : split);

              if(fnident.size() == identifier.size() && fnident.substr(0, identifier.size()) == identifier) {
                 if(fle->indexSet.size() < 2 && std::holds_alternative<std::shared_ptr<BinaryOpExpression>>(fle->indexSet.back())) {
                    auto & bo = std::get<std::shared_ptr<BinaryOpExpression>>(fle->indexSet.back());
                    bo->statements.emplace_back(VariableExpression{std::make_shared<Symbol>(itr->second)});
                 }
                 else if(fle->indexSet.size() < 2) {
                    fle->indexSet.emplace_back(VariableExpression{std::make_shared<Symbol>(itr->second)});
                 }
                 else {
                    cStmts->emplace_back(VariableExpression{std::make_shared<Symbol>(itr->second)});
                 }
                 break;
              }
           }
       }
       else if(1 < curStmts.size() && std::holds_alternative<std::shared_ptr<ForallLoopExpression>>(curStmts[curStmts.size()-2]->back())) {
          std::shared_ptr<ForallLoopExpression> & fle =
             std::get<std::shared_ptr<ForallLoopExpression>>(curStmts[curStmts.size()-2]->back());

           std::string identifier{dynamic_cast<Identifier const*>(ast)->name().c_str()};
           std::optional<Symbol> varsym =
               symbolTable.find(symbolTableRef->id, identifier);
           if(varsym) { 
              if(fle->indexSet.size() < 2 || (fle->indexSet.size()<3 && fle->isZippedIter)) {
                 fle->indexSet.emplace_back(VariableExpression{std::make_shared<Symbol>(*varsym)});
              }
              else if(fle->indexSet.size() && std::holds_alternative<std::shared_ptr<BinaryOpExpression>>(fle->indexSet.back())) {
                auto & bo = std::get<std::shared_ptr<BinaryOpExpression>>(fle->indexSet.back());
                bo->statements.emplace_back(VariableExpression{std::make_shared<Symbol>(*varsym)});
              }
              else {
                cStmts->emplace_back(VariableExpression{std::make_shared<Symbol>(*varsym)});
              }
              return true;
           }

           auto rsym =
              symbolTable.findPrefix(symbolTableRef->id, identifier);

           auto itr = rsym->first;
           for(; itr != rsym->second; ++itr) {
              const auto split = itr->first.find('|');
              const std::string fnident =
                 itr->first.substr(0, split == std::string::npos ? itr->first.size() : split);

              if(fnident.size() == identifier.size() && fnident.substr(0, identifier.size()) == identifier) {
                 if(fle->indexSet.size() < 2 && std::holds_alternative<std::shared_ptr<BinaryOpExpression>>(fle->indexSet.back())) {
                    auto & bo = std::get<std::shared_ptr<BinaryOpExpression>>(fle->indexSet.back());
                    bo->statements.emplace_back(VariableExpression{std::make_shared<Symbol>(itr->second)});
                 }
                 else if(fle->indexSet.size() < 2) {
                    fle->indexSet.emplace_back(VariableExpression{std::make_shared<Symbol>(itr->second)});
                 }
                 else {
                    cStmts->emplace_back(VariableExpression{std::make_shared<Symbol>(itr->second)});
                 }
                 break;
              }
           }
       }
       else if(1 < curStmts.size() && std::holds_alternative<std::shared_ptr<CoforallLoopExpression>>(curStmts[curStmts.size()-2]->back())) {
          std::shared_ptr<CoforallLoopExpression> & fle =
             std::get<std::shared_ptr<CoforallLoopExpression>>(curStmts[curStmts.size()-2]->back());

           std::string identifier{dynamic_cast<Identifier const*>(ast)->name().c_str()};
           std::optional<Symbol> varsym =
               symbolTable.find(symbolTableRef->id, identifier);
           if(varsym) { 
              chplx::util::dout << "Coforall Loop Expression: " << identifier << " index set size " << fle->indexSet.size() << std::endl;
              chplx::util::dout << "IsInsideOn: " << isInsideOn << std::endl;
              if(fle->indexSet.size() < 2) {
                 if (fle->indexSet.size() > 0)
                 {
                    chplx::util::dout
                        << "Coforall index: "
                        << (std::get<VariableExpression>(fle->indexSet.back()))
                               .sym->identifier
                        << std::endl;
                 }
                 if (fle->indexSet.size() > 0 &&
                     (std::get<VariableExpression>(fle->indexSet.back()))
                             .sym->identifier == "Locales")
                 {
                    //don't do anything for this case
                 }
                 else
                 {
                    fle->indexSet.emplace_back(
                        VariableExpression{std::make_shared<Symbol>(*varsym)});
                     if(isInsideOn){
                        currentOnExpr->OnLocaleVarsUsedInExpr.emplace_back(
                            VariableExpression{std::make_shared<Symbol>(*varsym)});
                     }
                 }
              }
              else if(fle->indexSet.size() && std::holds_alternative<std::shared_ptr<BinaryOpExpression>>(fle->indexSet.back())) {
                auto & bo = std::get<std::shared_ptr<BinaryOpExpression>>(fle->indexSet.back());
                bo->statements.emplace_back(VariableExpression{std::make_shared<Symbol>(*varsym)});
              }
              else {
                cStmts->emplace_back(VariableExpression{std::make_shared<Symbol>(*varsym)});
              }
              return true;
           }

           auto rsym =
              symbolTable.findPrefix(symbolTableRef->id, identifier);

           auto itr = rsym->first;
           for(; itr != rsym->second; ++itr) {
              const auto split = itr->first.find('|');
              const std::string fnident =
                 itr->first.substr(0, split == std::string::npos ? itr->first.size() : split);

              if(fnident.size() == identifier.size() && fnident.substr(0, identifier.size()) == identifier) {
                 if(fle->indexSet.size() < 2 && std::holds_alternative<std::shared_ptr<BinaryOpExpression>>(fle->indexSet.back())) {
                    auto & bo = std::get<std::shared_ptr<BinaryOpExpression>>(fle->indexSet.back());
                    bo->statements.emplace_back(VariableExpression{std::make_shared<Symbol>(itr->second)});
                 }
                 else if(fle->indexSet.size() < 2) {
                    fle->indexSet.emplace_back(VariableExpression{std::make_shared<Symbol>(itr->second)});
                    if(isInsideOn){
                        currentOnExpr->OnLocaleVarsUsedInExpr.emplace_back(
                            VariableExpression{std::make_shared<Symbol>(*varsym)});
                     }
                 }
                 else {
                    cStmts->emplace_back(VariableExpression{std::make_shared<Symbol>(itr->second)});
                 }
                 break;
              }
           }
       }
       else if(1 < curStmts.size() && std::holds_alternative<std::shared_ptr<OnExpression>>(curStmts[curStmts.size()-2]->back())) {
         chplx::util::dout << "723\n";
          std::shared_ptr<OnExpression> & one =
             std::get<std::shared_ptr<OnExpression>>(curStmts[curStmts.size()-2]->back());

           std::string identifier{dynamic_cast<Identifier const*>(ast)->name().c_str()};
           std::optional<Symbol> varsym =
               symbolTable.find(symbolTableRef->id, identifier);
           if (varsym)
           {
              if (one->OnLocaleVarExpr.size() >= 1)
              {
                 chplx::util::dout << "Hello " << identifier << std::endl;
                 chplx::util::dout << "Index " << varsym->kind.index() << std::endl;
                 if (isInsideOn && !varsym->isIntegralKind() &&
                     varsym->identifier != "writeln" &&
                     !std::holds_alternative<std::shared_ptr<func_kind>>(
                         varsym->kind))
                 {
                    chplx::util::dout
                        << "739 Adding OnLocaleVarsUsedInExpr : " << identifier
                        << std::endl;
                    chplx::util::dout << "Scope ID: " << varsym->scopeId
                              << " SymbolTableRef ID: "
                              << symbolTable.symbolTableRef->id
                              << " symbolref: " << symbolTableRef->id
                              << " inssideOn counter ID: " << isInsideOn
                              << std::endl;
                    currentOnExpr->OnLocaleVarsUsedInExpr.emplace_back(
                        VariableExpression{std::make_shared<Symbol>(*varsym)});
                 }
                 if (0 < cStmts->size() &&
                     std::holds_alternative<
                         std::shared_ptr<FunctionCallExpression>>(
                         cStmts->back()) && !pushedDot )
                 {
                    chplx::util::dout << "Adding to function call expression "
                              << identifier << std::endl;
                    varsym->scopeId = symbolTable.symbolTableRef->id;

                    std::shared_ptr<FunctionCallExpression>& fce =
                        std::get<std::shared_ptr<FunctionCallExpression>>(
                            cStmts->back());
                    fce->arguments.push_back(
                        VariableExpression{std::make_shared<Symbol>(*varsym)});
                    //   if (!varsym->isIntegralKind())
                    //       one->OnLocaleVarsUsedInExpr.emplace_back(
                    //           VariableExpression{
                    //               std::make_shared<Symbol>(*varsym)});
                    chplx::util::dout << "Function call: " << fce->symbol.identifier
                              << std::endl;
                    return true;
                 }
                 if (std::holds_alternative<std::shared_ptr<func_kind>>(varsym->kind))
                 {
                    std::shared_ptr<func_kind>& fk =
                        std::get<std::shared_ptr<func_kind>>(varsym->kind);

                    chplx::util::dout << "696\n";
                    cStmts->emplace_back(
                        std::make_shared<FunctionCallExpression>(
                            FunctionCallExpression{{symbolTableRef->id},
                                *varsym, {}, emitChapelLine(ast),
                                symbolTable}));
                    auto& fndecl =
                        std::get<std::shared_ptr<FunctionCallExpression>>(
                            cStmts->back());
                    curStmts.push_back(&fndecl->arguments);
                    return true;
                 }
                 chplx::util::dout << "cStmts back kind: " << cStmts->back().index() << std::endl;
                 cStmts->emplace_back(
                     VariableExpression{std::make_shared<Symbol>(*varsym)});
                 //   if (!varsym->isIntegralKind()){
                 //   one->OnLocaleVarsUsedInExpr.emplace_back(
                 //       VariableExpression{std::make_shared<Symbol>(*varsym)});
                 //    }else{
                 //       chplx::util::dout << "Skipping integral kind variable: "
                 //                 << identifier << std::endl;
                 //    }
                 return true;
              }
              chplx::util::dout << "Byee " << identifier << std::endl;

              if (isInsideOn && isInsideOn >= varsym->scopeId &&
                  !std::holds_alternative<std::shared_ptr<func_kind>>(
                      varsym->kind))
              {
                 chplx::util::dout << "672 Adding OnLocaleVarsUsedInExpr : "
                           << identifier << "Kind: " << varsym->kind.index()
                           << " to current statements" << std::endl;

                 chplx::util::dout << "Scope ID: " << isInsideOn
                           << " SymbolTableRef ID: " << symbolTableRef->id
                           << " SymbolTableRef ID: "
                           << symbolTable.symbolTableRef->id
                           << " varsym ID: " << varsym->scopeId << std::endl;
                 currentOnExpr->OnLocaleVarsUsedInExpr.emplace_back(
                     VariableExpression{std::make_shared<Symbol>(*varsym)});
              }

              varsym->scopeId = symbolTable.symbolTableRef->id;

              one->OnLocale = *varsym;
              one->OnLocaleVarExpr.emplace_back(
                  VariableExpression{std::make_shared<Symbol>(*varsym)});
              return true;
           }

           auto rsym =
              symbolTable.findPrefix(symbolTableRef->id, identifier);
           if (rsym)
           {
              auto itr = rsym->first;
              for (; itr != rsym->second; ++itr)
              {
                 const auto split = itr->first.find('|');
                 const std::string fnident = itr->first.substr(
                     0, split == std::string::npos ? itr->first.size() : split);

                 if (fnident.size() == identifier.size() &&
                     fnident.substr(0, identifier.size()) == identifier)
                 {
                    //   if(one->OnLocaleVarExpr.size() < 1 && std::holds_alternative<std::shared_ptr<BinaryOpExpression>>(one->OnLocaleVarExpr.back())) {
                    //      auto & bo = std::get<std::shared_ptr<BinaryOpExpression>>(one->OnLocaleVarExpr.back());
                    //      bo->statements.emplace_back(VariableExpression{std::make_shared<Symbol>(itr->second)});
                    //   }
                    //   else
                    if (one->OnLocaleVarExpr.size() < 1)
                    {
                        one->OnLocale = itr->second;
                        one->OnLocaleVarExpr.emplace_back(VariableExpression{
                            std::make_shared<Symbol>(itr->second)});
                        assert(one->OnLocaleVarExpr.size() == 1);
                    }
                    else
                    {
                        chplx::util::dout
                            << "Last expr kind : " << cStmts->back().index()
                            << std::endl;
                        cStmts->emplace_back(VariableExpression{
                            std::make_shared<Symbol>(itr->second)});
                    }
                    break;
                 }
              }
           }
       }
       else {
          std::string identifier{dynamic_cast<Identifier const*>(ast)->name().c_str()};
          std::optional<Symbol> varsym =
             symbolTable.find(symbolTableRef->id, identifier);

          if (pushedDot)
          {                      
              return true;
          }

          if ((cStmts->size() &&
                  std::holds_alternative<
                      std::shared_ptr<FunctionCallExpression>>(cStmts->back())))
          {
              auto& fce = std::get<std::shared_ptr<FunctionCallExpression>>(
                  cStmts->back());
              // possible function redefinition
              // for example:
              // if here.id is used in a function then it will generate chplx::here.id(), here
              if (varsym && isInsideOn && isInsideOn >= varsym->scopeId &&
                  !std::holds_alternative<std::shared_ptr<func_kind>>(
                      varsym->kind) && varsym->identifier != "here")
              {
                 chplx::util::dout << "734 Adding OnLocaleVarsUsedInExpr : "
                           << identifier << "Kind: " << varsym->kind.index()
                           << " to current statements" << std::endl;

                 chplx::util::dout << "Scope ID: " << isInsideOn
                           << " SymbolTableRef ID: " << symbolTableRef->id
                           << " SymbolTableRef ID: "
                           << symbolTable.symbolTableRef->id
                           << " varsym ID: " << varsym->scopeId << std::endl;
                 currentOnExpr->OnLocaleVarsUsedInExpr.emplace_back(
                     VariableExpression{std::make_shared<Symbol>(*varsym)});
              }
              else if (varsym &&
                  !std::holds_alternative<std::shared_ptr<func_kind>>(
                      varsym->kind) &&
                  varsym->identifier != "here")
              {
                 chplx::util::dout << "742 Skipping OnLocaleVarsUsedInExpr : "
                           << identifier << " isInsideOn: " << isInsideOn
                           << " varsym scope ID: " << varsym->scopeId
                           << std::endl;
              }
              if (fce->symbol.identifier.find(identifier + ".") !=
                  std::string::npos)
              {
                 return true;
              }
              fce->arguments.push_back(
                  VariableExpression{std::make_shared<Symbol>(*varsym)});
              chplx::util::dout << "Function call: " << fce->symbol.identifier
                        << std::endl;
              return true;
          }

          if (1 < curStmts.size() &&
              std::holds_alternative<std::shared_ptr<FunctionCallExpression>>(
                  curStmts[curStmts.size() - 2]->back()))
          {
              chplx::util::dout << "935 Found FunctionCallExpression in curStmts\n";
              chplx::util::dout << "Identifier: " << identifier << std::endl;
              if(varsym)
              chplx::util::dout << "Varsim kind: " << varsym->kind.index() << std::endl;
              std::optional<std::pair<std::map<std::string, Symbol>::iterator,
                  std::map<std::string, Symbol>::iterator>>
                  fnsym =
                      symbolTable.findPrefix(symbolTableRef->id, identifier);
              auto itr = fnsym->first;
              for (; itr != fnsym->second; ++itr)
              {
                 const auto split = itr->first.find('|');
                 const std::string fnident = itr->first.substr(
                     0, split == std::string::npos ? itr->first.size() : split);

                 if (fnident.size() == identifier.size() &&
                     fnident.substr(0, identifier.size()) == identifier)
                 {
                    if (itr->second.identifier.find('|') != std::string::npos ||
                        std::holds_alternative<std::shared_ptr<func_kind>>(
                            itr->second.kind))
                    {
                        chplx::util::dout
                            << "Skipping symbol with '|' in identifier: "
                            << itr->second.identifier << " func: "
                            << std::holds_alternative<
                                   std::shared_ptr<func_kind>>(itr->second.kind)
                            << std::endl;
                        continue;
                    }
                    chplx::util::dout << "Found Symbol: " << itr->second.identifier
                              << " kind " << itr->second.kind.index()
                              << std::endl;
                    cStmts->emplace_back(VariableExpression{
                        std::make_shared<Symbol>(itr->second)});
                    break;
                 }
                 else
                 {
                    chplx::util::dout << "Skipping symbol: " << itr->second.identifier
                              << " does not match identifier: " << identifier
                              << " kind: " << itr->second.kind.index()
                              << std::endl;
                 }
              }
              return true;
          }

          if(varsym) {
               if (!varsym->isIntegralKind())
               {
                  chplx::util::dout << "Adding variable: " << identifier
                            << " to current statements" << std::endl;
                  if(isInsideOn){
                     chplx::util::dout << "On Scope " << currentOnExpr->scopeId << " varssym scope ID: " << varsym->scopeId
                              << " isInsideOn: " << isInsideOn << " symbol scope ID: " << symbolTable.symbolTableRef->id << std::endl;
                  }
                  if (isInsideOn && !std::holds_alternative<std::shared_ptr<func_kind>>(
                           varsym->kind))
                  {
                     chplx::util::dout
                           << "734 Adding OnLocaleVarsUsedInExpr : " << identifier
                           << "Kind: " << varsym->kind.index()
                           << " to current statements" << std::endl;

                     chplx::util::dout << "Scope ID: " << isInsideOn
                                 << " SymbolTableRef ID: " << symbolTableRef->id
                                 << " SymbolTableRef ID: " << symbolTable.symbolTableRef->id
                                 << " varsym ID: " << varsym->scopeId
                                 << std::endl;
                     currentOnExpr->OnLocaleVarsUsedInExpr.emplace_back(
                           VariableExpression{std::make_shared<Symbol>(*varsym)});
                  }
                  else if (varsym &&
                      !std::holds_alternative<std::shared_ptr<func_kind>>(
                          varsym->kind) &&
                      varsym->identifier != "here")
                  {
                     chplx::util::dout << "784 Skipping OnLocaleVarsUsedInExpr : "
                               << identifier << " isInsideOn: " << isInsideOn
                               << " varsym scope ID: " << varsym->scopeId
                               << std::endl;
                  }
                  else if (varsym &&
                      std::holds_alternative<std::shared_ptr<func_kind>>(
                          varsym->kind))
                  {
                     chplx::util::dout << "1016" << std::endl;
                     std::shared_ptr<func_kind>& fk =
                         std::get<std::shared_ptr<func_kind>>(varsym->kind);
                     // symbolTableRef = symbolTable.lut[fk->lutId];

                     // chplx::util::dout << "847\n";
                     // cStmts->emplace_back(
                     //     std::make_shared<FunctionCallExpression>(
                     //         FunctionCallExpression{{symbolTableRef->id},
                     //             *varsym, {}, emitChapelLine(ast),
                     //             symbolTable}));
                     // auto& fndecl =
                     //     std::get<std::shared_ptr<FunctionCallExpression>>(
                     //         cStmts->back());
                     // curStmts.push_back(&fndecl->arguments);
                     return true;
                  }
                  chplx::util::dout << "1001 Scope ID: " << varsym->scopeId
                            << " SymbolTableRef ID: " << symbolTable.symbolTableRef->id
                            << " inssideOn counter ID: " << isInsideOn
                            << std::endl;
                 varsym->scopeId = symbolTable.symbolTableRef->id;
                 cStmts->emplace_back(
                     VariableExpression{std::make_shared<Symbol>(*varsym)});
               }
               else
               {
                  if(cStmts->size())
                  chplx::util::dout << "935 cStmts kind: " << cStmts->back().index() << std::endl;
                 if (cStmts->size() &&
                     std::holds_alternative<ArrayDeclarationExpression>(
                         cStmts->back()))
                 {
                    cStmts->emplace_back(LiteralExpression{int_kind{}, ast});
                 }
                 else if (cStmts->size() &&
                     std::holds_alternative<LiteralExpression>(cStmts->back()))
                 {
                    cStmts->emplace_back(
                        VariableExpression{std::make_shared<Symbol>(*varsym)});
                 }
               //   else{
               //    cStmts->emplace_back(
               //          VariableExpression{std::make_shared<Symbol>(*varsym)});
               //   }
               }
               
               chplx::util::dout << "920\n";
          }
          else {
             std::optional< std::pair< std::map<std::string, Symbol>::iterator, std::map<std::string, Symbol>::iterator > > fnsym =
                symbolTable.findPrefix(symbolTableRef->id, identifier);
             
             if(!fnsym) {
                 std::cerr << "chplx error: Undefined symbol \"" << identifier
                           << "\" detected; check\t" << emitChapelLine(ast)
                           << std::endl << std::flush;
                 assert(fnsym.has_value());
             } 
             else {
                auto itr = fnsym->first;
                for(; itr != fnsym->second; ++itr) {
                    const auto split = itr->first.find('|');
                    const std::string fnident =
                       itr->first.substr(0, split == std::string::npos ? itr->first.size() : split);

                    if(fnident.size() == identifier.size() && fnident.substr(0, identifier.size()) == identifier) {
                        if(itr->second.identifier.find('|') != std::string::npos) {
                           chplx::util::dout << "Skipping symbol with '|' in identifier: " << itr->second.identifier << std::endl;
                           continue;
                        }
                        cStmts->emplace_back(VariableExpression{std::make_shared<Symbol>(itr->second)});
                        break;
                    }else{
                        chplx::util::dout << "Skipping symbol: " << itr->second.identifier << " does not match identifier: " << identifier << std::endl;
                    }
                }
             }
          }
       }
    }
    break;
    case asttags::Import:
    break;
    case asttags::Include:
    break;
    case asttags::Let:
    break;
    case asttags::New:
    break;
    case asttags::Range:
    {
       //std::vector<Statement> * cStmts = curStmts.back();

       if (1 < curStmts.size() && std::holds_alternative<std::shared_ptr<ForLoopExpression>>( curStmts[curStmts.size()-2]->back() ) ) {
          std::optional<Symbol> varsym =
             symbolTable.find(symbolTableRef->id, std::string{"range_" + emitChapelLine(ast)});
          if(varsym) {
             std::shared_ptr<ForLoopExpression> & fl =
                std::get<std::shared_ptr<ForLoopExpression>>(curStmts[curStmts.size()-2]->back());

             std::shared_ptr<func_kind> & fc = std::get<std::shared_ptr<func_kind>>(fl->symbol.kind);

             //if(!(fl->indexSet)) {
             //   fl->indexSet = fc->args[0];
             //}
          }
       }
       else if (1 < curStmts.size() && std::holds_alternative<std::shared_ptr<ForallLoopExpression>>( curStmts[curStmts.size()-2]->back() ) ) {
          std::optional<Symbol> varsym =
             symbolTable.find(symbolTableRef->id, std::string{"range_" + emitChapelLine(ast)});
          if(varsym) {
             std::shared_ptr<ForallLoopExpression> & fl =
                std::get<std::shared_ptr<ForallLoopExpression>>(curStmts[curStmts.size()-2]->back());

             std::shared_ptr<func_kind> & fc = std::get<std::shared_ptr<func_kind>>(fl->symbol.kind);

             //if(!(fl->indexSet)) {
             //   fl->indexSet = fc->args[0];
             //}
          }
       }
       else if (1 < curStmts.size() && std::holds_alternative<std::shared_ptr<CoforallLoopExpression>>( curStmts[curStmts.size()-2]->back() ) ) {
          std::optional<Symbol> varsym =
             symbolTable.find(symbolTableRef->id, std::string{"range_" + emitChapelLine(ast)});
          if(varsym) {
             std::shared_ptr<CoforallLoopExpression> & fl =
                std::get<std::shared_ptr<CoforallLoopExpression>>(curStmts[curStmts.size()-2]->back());

             std::shared_ptr<func_kind> & fc = std::get<std::shared_ptr<func_kind>>(fl->symbol.kind);

             //if(!(fl->indexSet)) {
             //   fl->indexSet = fc->args[0];
             //}
          }
       }
       else if (1 < curStmts.size() && std::holds_alternative<std::shared_ptr<OnExpression>>( curStmts[curStmts.size()-2]->back() ) ) {
          std::optional<Symbol> varsym =
             symbolTable.find(symbolTableRef->id, std::string{"on_" + emitChapelLine(ast)});
          if(varsym) {
             std::shared_ptr<OnExpression> & fl =
                std::get<std::shared_ptr<OnExpression>>(curStmts[curStmts.size()-2]->back());

             std::shared_ptr<func_kind> & fc = std::get<std::shared_ptr<func_kind>>(fl->symbol.kind);

             //if(!(fl->indexSet)) {
             //   fl->indexSet = fc->args[0];
             //}
          }
       }
    }
    break;
    case asttags::Require:
    break;
    case asttags::Return:
    {
       // check to see if the function symbol has a 'kind' set;
       // if not set, set it to something

       std::vector<Statement> * cStmts = curStmts.back();

       if(std::holds_alternative<std::shared_ptr<FunctionDeclarationExpression>>(curStmts[curStmts.size()-2]->back())) {

           // check to see if the return value is defined...
           // if not defined, set to `auto`
           //
           auto & fnptr = std::get<std::shared_ptr<FunctionDeclarationExpression>>(curStmts[curStmts.size()-2]->back());
           auto & fnk = std::get<std::shared_ptr<func_kind>>(fnptr->symbol.kind)->retKind;

           if(std::holds_alternative<nil_kind>(fnk)) {
              fnk = auto_kind{};
           }

           cStmts->emplace_back(
               std::make_shared<ReturnExpression>(ReturnExpression{{}, {emitChapelLine(ast)}})
           );

           curStmts.push_back(&(std::get<std::shared_ptr<ReturnExpression>>(cStmts->back())->statement));
       }
    }
    break;
    case asttags::Throw:
    break;
    case asttags::TypeQuery:
    break;
    case asttags::Use:
    break;
    case asttags::VisibilityClause:
    break;
    case asttags::WithClause:
    break;
    case asttags::Yield:
    break;
    case asttags::START_Literal:
    break;
    case asttags::BoolLiteral:
    {
       std::vector<Statement> * cStmts = curStmts.back();
       if (1 < curStmts.size() && std::holds_alternative<std::shared_ptr<BinaryOpExpression>>( curStmts[curStmts.size()-2]->back() ) ) {
           cStmts->emplace_back(LiteralExpression{bool_kind{}, ast});
       }
       else if(0 < cStmts->size() && std::holds_alternative<std::shared_ptr<FunctionCallExpression>>(cStmts->back())) {
          std::shared_ptr<FunctionCallExpression> & fce =
             std::get<std::shared_ptr<FunctionCallExpression>>(cStmts->back()); 
          fce->arguments.push_back(LiteralExpression{bool_kind{}, ast});
       }
       else if(0 < cStmts->size() && std::holds_alternative<std::shared_ptr<ReturnExpression>>(cStmts->back())) {
           std::shared_ptr<ReturnExpression> & ret = std::get<std::shared_ptr<ReturnExpression>>(cStmts->back());
           ret->statement.emplace_back(LiteralExpression{bool_kind{}, ast});
       }
/*
       else if(0 < cStmts->size() && std::holds_alternative<TupleDeclarationLiteralExpression>(cStmts->back())) {
          //auto & sle = std::get<TupleDeclarationLiteralExpression>(cStmts->back());
          //sle.literalValues.push_back(LiteralExpression{bool_kind{}, ast});
          //sle.kind = bool_kind{};
       }
*/
       else {
           cStmts->emplace_back(LiteralExpression{bool_kind{}, ast});
       }
    }
    break;
    case asttags::ImagLiteral:
    break;
    case asttags::IntLiteral:
    {
       std::vector<Statement> * cStmts = curStmts.back();

       if(1 < curStmts.size() )
        chplx::util::dout << "Intliteral CurStmts kind: " << curStmts[curStmts.size()-2]->back().index() << std::endl;

       if(cStmts->size())
       chplx::util::dout << "Intliteral cStmts kind: " << cStmts->back().index() << std::endl;

       if (1 < curStmts.size() && std::holds_alternative<std::shared_ptr<BinaryOpExpression>>( curStmts[curStmts.size()-2]->back() ) ) {
           if(0 < cStmts->size() && std::holds_alternative<TupleDeclarationLiteralExpression>( cStmts->back() )) {
              return true;
/*
              TupleDeclarationLiteralExpression & tdl =
                 std::get<TupleDeclarationLiteralExpression>(cStmts->back());
                 tdl.literalValues.push_back(LiteralExpression{int_kind{}, ast});
*/
           }
           else {
              cStmts->emplace_back(LiteralExpression{int_kind{}, ast});
           }
       }
       else if(1 < curStmts.size() && std::holds_alternative<std::shared_ptr<ForLoopExpression>>(curStmts[curStmts.size()-2]->back())) {
          std::shared_ptr<ForLoopExpression> & fle =
             std::get<std::shared_ptr<ForLoopExpression>>(curStmts[curStmts.size()-2]->back());

          if(fle->indexSet.size() && std::holds_alternative<std::shared_ptr<BinaryOpExpression>>(fle->indexSet.back())) {
             auto & bo = std::get<std::shared_ptr<BinaryOpExpression>>(fle->indexSet.back());
             bo->statements.emplace_back(LiteralExpression{int_kind{}, ast});
          }
          else {
             fle->indexSet.emplace_back(LiteralExpression{int_kind{}, ast});
          }
       }
       else if(1 < curStmts.size() && std::holds_alternative<std::shared_ptr<ForallLoopExpression>>(curStmts[curStmts.size()-2]->back())) {
          std::shared_ptr<ForallLoopExpression> & fle =
             std::get<std::shared_ptr<ForallLoopExpression>>(curStmts[curStmts.size()-2]->back());

          if(fle->indexSet.size() && std::holds_alternative<std::shared_ptr<BinaryOpExpression>>(fle->indexSet.back())) {
             auto & bo = std::get<std::shared_ptr<BinaryOpExpression>>(fle->indexSet.back());
             bo->statements.emplace_back(LiteralExpression{int_kind{}, ast});
          }
          else {
             fle->indexSet.emplace_back(LiteralExpression{int_kind{}, ast});
          }
       }
       else if(1 < curStmts.size() && std::holds_alternative<std::shared_ptr<CoforallLoopExpression>>(curStmts[curStmts.size()-2]->back())) {
          std::shared_ptr<CoforallLoopExpression> & fle =
             std::get<std::shared_ptr<CoforallLoopExpression>>(curStmts[curStmts.size()-2]->back());

           chplx::util::dout << "Coforall indexset size: " << fle->indexSet.size() << std::endl;
           chplx::util::dout << "Coforall indexset size: " << (fle->indexSet.size() ?  std::holds_alternative<std::shared_ptr<BinaryOpExpression>>(fle->indexSet.back()) : 0) << std::endl;

          if(fle->indexSet.size() && std::holds_alternative<std::shared_ptr<BinaryOpExpression>>(fle->indexSet.back())) {
             auto & bo = std::get<std::shared_ptr<BinaryOpExpression>>(fle->indexSet.back());
             bo->statements.emplace_back(LiteralExpression{int_kind{}, ast});
          }
          else {
             if(fle->indexSet.size() && std::holds_alternative<VariableExpression>(fle->indexSet.back())){
               auto var = std::get<VariableExpression>(fle->indexSet.back());
               chplx::util::dout << "Coforall index: " << var.sym->identifier << std::endl;
               if(var.sym->identifier == "Locales"){
                  cStmts->emplace_back(LiteralExpression{int_kind{}, ast});
                  return true;
               }
             }else{
               if(fle->indexSet.size())
               chplx::util::dout << "Coforall index set kind: " << fle->indexSet.back().index() << std::endl;
             }
             fle->indexSet.emplace_back(LiteralExpression{int_kind{}, ast});
          }
       }
       else if(1 < curStmts.size() && std::holds_alternative<std::shared_ptr<OnExpression>>(curStmts[curStmts.size()-2]->back())) {
            chplx::util::dout << "Int Literal : " << int_kind::value(ast) << std::endl;
            chplx::util::dout << "Kind : " <<  cStmts->back().index() << std::endl;
            if (0 < cStmts->size() &&
                std::holds_alternative<std::shared_ptr<FunctionCallExpression>>(
                    cStmts->back()))
            {
             std::shared_ptr<FunctionCallExpression>& fce =
                 std::get<std::shared_ptr<FunctionCallExpression>>(
                     cStmts->back());
             fce->arguments.push_back(LiteralExpression{int_kind{}, ast});
            }
            else if (0 < cStmts->size() &&
                std::holds_alternative<ScalarDeclarationExpression>(
                    cStmts->back()))
            {
             auto sde = std::get<ScalarDeclarationExpression>(cStmts->back());
             cStmts->pop_back();
             cStmts->push_back(ScalarDeclarationLiteralExpression{
                 {{sde.scopeId}, sde.identifier, int_kind{}, sde.chplLine,
                     sde.qualifier, sde.config},
                 {ast}});
            }
            else if (0 < cStmts->size() &&
                std::holds_alternative<ScalarDeclarationLiteralExpression>(
                    cStmts->back()))
            {
             auto& sle =
                 std::get<ScalarDeclarationLiteralExpression>(cStmts->back());
             sle.literalValue.push_back(ast);
            }
            else
            {
               chplx::util::dout << "Adding int literal: " <<  std::endl;
             cStmts->emplace_back(LiteralExpression{int_kind{}, ast});
            }
             return true;
       }
       else if(0 < cStmts->size() && std::holds_alternative<std::shared_ptr<FunctionCallExpression>>(cStmts->back())) {
          std::shared_ptr<FunctionCallExpression> & fce =
             std::get<std::shared_ptr<FunctionCallExpression>>(cStmts->back()); 
          fce->arguments.push_back(LiteralExpression{int_kind{}, ast});
       }
       else if(0 < cStmts->size() && std::holds_alternative<std::shared_ptr<ReturnExpression>>(cStmts->back())) {
           std::shared_ptr<ReturnExpression> & ret = std::get<std::shared_ptr<ReturnExpression>>(cStmts->back());
           ret->statement.emplace_back(LiteralExpression{int_kind{}, ast});
       }
       else if(0 < cStmts->size() && std::holds_alternative<ScalarDeclarationExpression>(cStmts->back())) {
          auto sde = std::get<ScalarDeclarationExpression>(cStmts->back());
          cStmts->pop_back();
          cStmts->push_back( ScalarDeclarationLiteralExpression{{{sde.scopeId}, sde.identifier, int_kind{}, sde.chplLine, sde.qualifier, sde.config}, {ast}} );
       }
       else if(0 < cStmts->size() && std::holds_alternative<ScalarDeclarationLiteralExpression>(cStmts->back())) {
          auto & sle = std::get<ScalarDeclarationLiteralExpression>(cStmts->back());
          sle.literalValue.push_back(ast);
//          sle.kind = int_kind{};
       }
      //  else if (0 < cStmts->size() &&
      //      std::holds_alternative<VariableExpression>(cStmts->back()))
      //  {
      //     auto scalarDecl = std::get<VariableExpression>(cStmts->back());

      //     cStmts->pop_back();
      //     cStmts->emplace_back(std::make_shared<BinaryOpExpression>(
      //         BinaryOpExpression{{{symbolTableRef->id}, "=", ast}, {}}));

      //     auto& bo =
      //         std::get<std::shared_ptr<BinaryOpExpression>>(cStmts->back());
      //     bo->statements.emplace_back(
      //         std::make_shared<ScalarDeclarationExprExpression>(
      //             ScalarDeclarationExprExpression{
      //                 {{scalarDecl.sym->scopeId}, scalarDecl.sym->identifier,
      //                     scalarDecl.sym->kind, emitChapelLine(ast),
      //                     scalarDecl.sym->kindqualifier,
      //                     scalarDecl.sym->isConfig},
      //                 {}}));
      //     //  curStmts.push_back(&bo->statements);
      //  }

/*
       else if(0 < cStmts->size() && std::holds_alternative<TupleDeclarationLiteralExpression>(cStmts->back())) {
          //auto & sle = std::get<TupleDeclarationLiteralExpression>(cStmts->back());
          //sle.literalValues.push_back(LiteralExpression{int_kind{}, ast});
          //sle.kind = int_kind{};
       }
*/
       else {
           cStmts->emplace_back(LiteralExpression{int_kind{}, ast});
       }
    }
    break;
    case asttags::RealLiteral:
    {
       std::vector<Statement> * cStmts = curStmts.back();
       if (1 < curStmts.size() && std::holds_alternative<std::shared_ptr<BinaryOpExpression>>( curStmts[curStmts.size()-2]->back() ) ) {
           cStmts->emplace_back(LiteralExpression{real_kind{}, ast});
       }
       else if(0 < cStmts->size() && std::holds_alternative<std::shared_ptr<FunctionCallExpression>>(cStmts->back())) {
          std::shared_ptr<FunctionCallExpression> & fce =
             std::get<std::shared_ptr<FunctionCallExpression>>(cStmts->back()); 
          fce->arguments.push_back(LiteralExpression{real_kind{}, ast});
       }
       else if(0 < cStmts->size() && std::holds_alternative<std::shared_ptr<ReturnExpression>>(cStmts->back())) {
           std::shared_ptr<ReturnExpression> & ret = std::get<std::shared_ptr<ReturnExpression>>(cStmts->back());
           ret->statement.emplace_back(LiteralExpression{real_kind{}, ast});
       }
/*
       else if(0 < cStmts->size() && std::holds_alternative<TupleDeclarationLiteralExpression>(cStmts->back())) {
          //auto & sle = std::get<TupleDeclarationLiteralExpression>(cStmts->back());
          //sle.literalValues.push_back(LiteralExpression{real_kind{},ast});
          //sle.kind = real_kind{};
       }
*/
       else if(0 < cStmts->size() && std::holds_alternative<ScalarDeclarationExpression>(cStmts->back())) {
          auto sde = std::get<ScalarDeclarationExpression>(cStmts->back());
          cStmts->pop_back();
          cStmts->push_back( ScalarDeclarationLiteralExpression{{{sde.scopeId}, sde.identifier, real_kind{}, sde.chplLine, sde.qualifier, sde.config}, {ast}} );
       }
       else if(0 < cStmts->size() && std::holds_alternative<ScalarDeclarationLiteralExpression>(cStmts->back())) {
          auto & sle = std::get<ScalarDeclarationLiteralExpression>(cStmts->back());
          sle.literalValue.push_back(ast);
       }
       else {
           cStmts->emplace_back(LiteralExpression{real_kind{}, ast});
       }
    }
    break;
    case asttags::UintLiteral:
    break;
    case asttags::START_StringLikeLiteral:
    break;
    case asttags::BytesLiteral:
    break;
    case asttags::CStringLiteral:
    break;
    case asttags::StringLiteral:
    {
       std::vector<Statement> * cStmts = curStmts.back();
       chplx::util::dout << "StringLiteral: " << string_kind::value(ast) << std::endl;
       if (1 < curStmts.size())
           chplx::util::dout << "Kind: "
                     << curStmts[curStmts.size() - 2]->back().index()
                     << std::endl;
       if (1 < curStmts.size() && std::holds_alternative<std::shared_ptr<BinaryOpExpression>>( curStmts[curStmts.size()-2]->back() ) ) {
          cStmts->emplace_back(LiteralExpression{string_kind{}, ast});
       }
       // uncommenting below will cause string literals to be in whatever calls happen to be
       // at the end of the statement list, which is not what we want
       // writeln("Hello", betad(1), "World");
       // Enter writeln: you emit a FunctionCallExpression for writeln, then push its args-list onto curStmts.
       // StringLiteral "Hello"->falls to the else-branch and goes into writeln's args.
       // Enter betad(1): emit its call, push betad's args-list.
       // IntLiteral 1->goes into betad's args.
       // Exit betad: pop back to writeln's args-list.
       // StringLiteral "World"->now:
       // curStmts.back() is writeln's args (a vector containing "Hello" and the betad call).
       // BUT cStmts->back() is the very last element of that list, which is the betad call.
       // The special-case sees a FunctionCallExpression at the back, and so it wrongly appends "World" into betad's arguments instead of writeln's.
       else if (1 < curStmts.size() &&
           std::holds_alternative<std::shared_ptr<FunctionCallExpression>>(
               curStmts[curStmts.size() - 2]->back()))
       {
          chplx::util::dout << "1270 StringLiteral: " << string_kind::value(ast)
                    << std::endl;
          std::shared_ptr<FunctionCallExpression>& fce =
              std::get<std::shared_ptr<FunctionCallExpression>>(
                  curStmts[curStmts.size() - 2]->back());
          fce->arguments.push_back(LiteralExpression{string_kind{}, ast});
       }
       else if(0 < cStmts->size() && std::holds_alternative<std::shared_ptr<ReturnExpression>>(cStmts->back())) {
           std::shared_ptr<ReturnExpression> & ret = std::get<std::shared_ptr<ReturnExpression>>(cStmts->back());
           ret->statement.emplace_back(LiteralExpression{string_kind{}, ast});
       }
/*
       else if(0 < cStmts->size() && std::holds_alternative<TupleDeclarationLiteralExpression>(cStmts->back())) {
          //auto & sle = std::get<TupleDeclarationLiteralExpression>(cStmts->back());
          //sle.literalValues.push_back(LiteralExpression{string_kind{},ast});
          //sle.kind = string_kind{};
       }
*/
       else {
         chplx::util::dout << "Cstmts back kind: " << cStmts->back().index() << std::endl;
         chplx::util::dout << "1296 \n";
           cStmts->emplace_back(LiteralExpression{string_kind{}, ast});
       }
    }
    break;
    case asttags::END_StringLikeLiteral:
    break;
    case asttags::END_Literal:
    break;
    case asttags::START_Call:
    break;
    case asttags::FnCall:
    {
       std::vector<Statement> * cStmts = curStmts.back();

       const FnCall* fc = dynamic_cast<const FnCall*>(ast);
       std::string identifier{dynamic_cast<const Identifier*>(fc->calledExpression())->name().c_str()};

       chplx::util::dout << "FnCall: " << identifier << std::endl;
       if(curStmts.size() > 1 ) chplx::util::dout << "CurStmts kind: " << curStmts[curStmts.size()-2]->back().index() << std::endl;

       if (curStmts.size() > 1 &&
           std::holds_alternative<std::shared_ptr<ForLoopExpression>>(
               curStmts[curStmts.size() - 2]->back()))
       {
           std::shared_ptr<ForLoopExpression>& fle =
               std::get<std::shared_ptr<ForLoopExpression>>(
                   curStmts[curStmts.size() - 2]->back());
           std::optional<Symbol> varsym =
               symbolTable.find(symbolTableRef->id, identifier);

           if (fle->isArrayInitForLoop && pendingArrayForLoopSymbols.size() > 0)
           {
             auto arrayVarsym = pendingArrayForLoopSymbols.front();
             pendingArrayForLoopSymbols.pop_front();
             auto arrayIdentifier = arrayVarsym->identifier;

             auto varsymInsideForLoop = arrayVarsym;
             std::string iteratorName = fle->iterator->identifier;
             auto arrayIdentifierForLoopExpression =
                 arrayIdentifier + "(" + iteratorName + ")";

             varsymInsideForLoop->kind = std::make_shared<func_kind>(func_kind{
                 {symbolTable.symbolTableRef->id, {}, {}, {}}, true, false});

             cStmts->emplace_back(std::make_shared<BinaryOpExpression>(
                 BinaryOpExpression{{{symbolTableRef->id}, "=", ast}, {}}));
             auto& bo =
                 std::get<std::shared_ptr<BinaryOpExpression>>(cStmts->back());

             bo->statements.emplace_back(

                 VariableExpression{std::make_shared<Symbol>(Symbol{
                     varsymInsideForLoop->kind,
                     arrayIdentifierForLoopExpression,    // This is "arr(i)"
                     {}, -1, false, symbolTableRef->id})});

             {
                std::optional<std::pair<std::map<std::string, Symbol>::iterator,
                    std::map<std::string, Symbol>::iterator>>
                    fsym =
                        symbolTable.findPrefix(symbolTableRef->id, identifier);

                auto itr = fsym->first;
                for (; itr != fsym->second; ++itr)
                {
                    const auto split = itr->first.find('|');
                    const std::string fnident = itr->first.substr(0,
                        split == std::string::npos ? itr->first.size() : split);

                    if (fnident.size() == identifier.size() &&
                        fnident.substr(0, identifier.size()) == identifier)
                    {
                        bo->statements.emplace_back(
                            std::make_shared<FunctionCallExpression>(
                                FunctionCallExpression{{symbolTableRef->id},
                                    itr->second, {}, emitChapelLine(ast),
                                    symbolTable}));
                        curStmts.push_back(
                            &(std::get<std::shared_ptr<FunctionCallExpression>>(
                                bo->statements.back())
                                    ->arguments));
                        break;
                    }
                }
             }

             return true;
           }
       }

       if (1 < curStmts.size() &&
           std::holds_alternative<std::shared_ptr<BinaryOpExpression>>( curStmts[curStmts.size()-2]->back() ) &&
           fc->calledExpression()->tag() == asttags::Identifier &&
           !fc->callUsedSquareBrackets() ) {
           std::string identifier{dynamic_cast<const Identifier*>(fc->calledExpression())->name().c_str()};

           std::optional<Symbol> varsym =
              symbolTable.find(symbolTableRef->id, identifier);

           if(varsym && std::holds_alternative<std::shared_ptr<array_kind>>(varsym->kind)) {
              auto paren = symbolTable.find(symbolTableRef->id, "[]");

              auto encop = operatorEncoder.find("[]");
              if(std::end(operatorEncoder) == encop) {
                 std::cerr << "programtreebuildingvisitor.cpp, enter, OpCall, identifier not found" << std::endl << std::flush;
                 return false;
              }

              cStmts->emplace_back(
                 std::make_shared<FunctionCallExpression>(
                    FunctionCallExpression{{symbolTableRef->id}, {*paren}, {}, emitChapelLine(ast), symbolTable}
              ));
              curStmts.push_back(&(std::get<std::shared_ptr<FunctionCallExpression>>(cStmts->back())->arguments));
           }
           else { 
              auto rsym =
                symbolTable.findPrefix(symbolTableRef->id, identifier);

              auto itr = rsym->first;
              for(; itr != rsym->second; ++itr) {

                  const auto split = itr->first.find('|');
                  const std::string fnident =
                     itr->first.substr(0, split == std::string::npos ? itr->first.size() : split);

                  if(fnident.size() == identifier.size() && fnident.substr(0, identifier.size()) == identifier) {
                      cStmts->emplace_back(
                         std::make_shared<FunctionCallExpression>(
                            FunctionCallExpression{{symbolTableRef->id}, {itr->second}, {}, emitChapelLine(ast), symbolTable}
                      ));
                      curStmts.push_back(&(std::get<std::shared_ptr<FunctionCallExpression>>(cStmts->back())->arguments));
                      break;
                  }
              }
           }
           return true;
       }
       // function called
       //
       else if(fc->calledExpression()->tag() == asttags::Identifier && !fc->callUsedSquareBrackets()) {
           std::string identifier{dynamic_cast<const Identifier*>(fc->calledExpression())->name().c_str()};
           std::optional<Symbol> fnsym =
              symbolTable.find(symbolTableRef->id, identifier);

           if(fnsym) { 
               chplx::util::dout << "Function Call: " << identifier << " fnsym kind: " << fnsym->kind.index() << std::endl;
              if(std::holds_alternative<std::shared_ptr<tuple_kind>>(fnsym->kind) && 
                 0 == cStmts->size() &&
                 std::holds_alternative<std::shared_ptr<TupleDeclarationExprExpression>>(curStmts[curStmts.size()-2]->back())) {

                 curStmts.pop_back();
                 cStmts = curStmts.back();
                 auto & se = std::get<std::shared_ptr<TupleDeclarationExprExpression>>(cStmts->back());
                 curStmts.emplace_back(&(se->statements));
                 cStmts = curStmts.back();
                 cStmts->emplace_back(
                    std::make_shared<FunctionCallExpression>(
                       FunctionCallExpression{{symbolTableRef->id}, *fnsym, {}, emitChapelLine(ast), symbolTable}
                 ));
                 curStmts.push_back(&(std::get<std::shared_ptr<FunctionCallExpression>>(cStmts->back())->arguments));
                 return true;
              }
              else if(cStmts->size() && std::holds_alternative<std::shared_ptr<ScalarDeclarationExprExpression>>(cStmts->back())) {
                 std::shared_ptr<ScalarDeclarationExprExpression> stmt =
                    std::get<std::shared_ptr<ScalarDeclarationExprExpression>>(cStmts->back());
                 cStmts->pop_back();
                 cStmts->emplace_back(
                    std::make_shared<BinaryOpExpression>(BinaryOpExpression{
                       {{symbolTableRef->id}, "=", ast}, {}
                    })
                 );
                 auto & bo = std::get<std::shared_ptr<BinaryOpExpression>>(cStmts->back());
                 bo->statements.emplace_back(
                    std::make_shared<TupleDeclarationExprExpression>(TupleDeclarationExprExpression
                       {{{stmt->scopeId}, stmt->identifier, stmt->kind, stmt->chplLine, stmt->qualifier, stmt->config}, {}}
                 ));
                 bo->statements.emplace_back(
                    std::make_shared<FunctionCallExpression>(
                       FunctionCallExpression{{symbolTableRef->id}, *fnsym, {}, emitChapelLine(ast), symbolTable}
                 ));
                 curStmts.push_back(&(std::get<std::shared_ptr<FunctionCallExpression>>(bo->statements.back())->arguments));
                 return true;
              }
              else if(cStmts->size() && 
                 !std::holds_alternative<std::shared_ptr<cxxfunc_kind>>(fnsym->kind) &&
                 identifier != "writeln" &&
                 std::holds_alternative<ScalarDeclarationLiteralExpression>(cStmts->back()) ) {
                 chplx::util::dout << "Adding to scalar decl 1\n";
                 chplx::util::dout << "Fn holds or not : "
                           << std::holds_alternative<
                                  std::shared_ptr<FunctionCallExpression>>(
                                  cStmts->back())
                           << std::endl;
                 ScalarDeclarationLiteralExpression stmt =
                     std::get<ScalarDeclarationLiteralExpression>(
                         cStmts->back());
                 if (stmt.literalValue.size())
                 {
                      std::optional<
                          std::pair<std::map<std::string, Symbol>::iterator,
                              std::map<std::string, Symbol>::iterator>>
                          fsym = symbolTable.findPrefix(
                              symbolTableRef->id, identifier);
                      assert(fsym);
                      auto itr = fsym->first;
                      for (; itr != fsym->second; ++itr)
                      {
                        const auto split = itr->first.find('|');
                        const std::string fnident = itr->first.substr(0,
                            split == std::string::npos ? itr->first.size() :
                                                         split);

                        if (fnident.size() == identifier.size() &&
                            fnident.substr(0, identifier.size()) ==
                                identifier &&
                            !std::holds_alternative<std::shared_ptr<func_kind>>(
                                itr->second.kind))
                        {
                           chplx::util::dout << "Found function: " << itr->first
                                     << std::endl;
                           chplx::util::dout << "Adding to scalar decl " << identifier
                                     << std::endl;
                           chplx::util::dout << "Adding to scalar decl kind "
                                     << itr->second.kind.index() << " val "
                                     << std::holds_alternative<
                                            std::shared_ptr<func_kind>>(
                                            itr->second.kind)
                                     << std::endl;
                           cStmts->emplace_back(
                               std::make_shared<FunctionCallExpression>(
                                   FunctionCallExpression{{symbolTableRef->id},
                                       itr->second, {}, emitChapelLine(ast),
                                       symbolTable}));
                           curStmts.push_back(
                               &(std::get<
                                   std::shared_ptr<FunctionCallExpression>>(
                                   cStmts->back())
                                       ->arguments));
                           break;
                           return true;
                        }
                      }
                 }

                 chplx::util::dout << "Adding to scalar decl 2\n";

                 cStmts->pop_back();
                 cStmts->emplace_back(
                    std::make_shared<BinaryOpExpression>(BinaryOpExpression{
                       {{symbolTableRef->id}, "=", ast}, {}
                    })
                 );
                 auto & bo = std::get<std::shared_ptr<BinaryOpExpression>>(cStmts->back());
                 bo->statements.emplace_back(stmt);
                 bo->statements.emplace_back(
                    std::make_shared<FunctionCallExpression>(
                       FunctionCallExpression{{symbolTableRef->id}, *fnsym, {}, emitChapelLine(ast), symbolTable}
                    ));
                 curStmts.push_back(&(std::get<std::shared_ptr<FunctionCallExpression>>(bo->statements.back())->arguments));
                 return true;
              }
           }
           std::optional< std::pair< std::map<std::string, Symbol>::iterator, std::map<std::string, Symbol>::iterator > > fsym =
              symbolTable.findPrefix(symbolTableRef->id, identifier);

           if(!fsym) {
              std::cerr << "chplx error: Undefined symbol \"" << identifier << "\" detected; check\t" << emitChapelLine(ast) << std::endl << std::flush;
              return false;
           }
           else {
            chplx::util::dout << "1523 Function Call: " << identifier << std::endl;
             if(cStmts->size() )
             chplx::util::dout << "cStmts kind: " << cStmts->back().index() << std::endl;
              if(0 < cStmts->size() && std::holds_alternative<ScalarDeclarationExpression>(cStmts->back())) {
                 auto scalarDecl = std::get<ScalarDeclarationExpression>(cStmts->back());

                 cStmts->pop_back();
                 cStmts->emplace_back(
                     std::make_shared<BinaryOpExpression>(BinaryOpExpression{
                         {{symbolTableRef->id}, "=", ast}, {}
                     })
                 );
                 auto & bo = std::get<std::shared_ptr<BinaryOpExpression>>(cStmts->back());
                 bo->statements.emplace_back(
                    std::make_shared<ScalarDeclarationExprExpression>(ScalarDeclarationExprExpression
                       {{{scalarDecl.scopeId}, scalarDecl.identifier, scalarDecl.kind, scalarDecl.chplLine, scalarDecl.qualifier, scalarDecl.config}, {}}
                 ));

                 auto itr = fsym->first;

                 for(; itr != fsym->second; ++itr) {
                    const auto split = itr->first.find('|');
                    const std::string fnident =
                       itr->first.substr(0, split == std::string::npos ? itr->first.size() : split);

                    if(fnident.size() == identifier.size() && fnident.substr(0, identifier.size()) == identifier) {
                       bo->statements.emplace_back(
                          std::make_shared<FunctionCallExpression>(
                             FunctionCallExpression{{symbolTableRef->id}, itr->second, {}, emitChapelLine(ast), symbolTable}
                       ));
                       curStmts.push_back(&(std::get<std::shared_ptr<FunctionCallExpression>>(bo->statements.back())->arguments));
                       break;
                    }
                 }
              }
              else if(0 < cStmts->size() && std::holds_alternative<ScalarDeclarationLiteralExpression>(cStmts->back())) {
                 auto scalarDecl = std::get<ScalarDeclarationLiteralExpression>(cStmts->back());
                 if(scalarDecl.literalValue.size() < 1) {
                    cStmts->pop_back();
                    cStmts->emplace_back(
                       std::make_shared<BinaryOpExpression>(BinaryOpExpression{
                          {{symbolTableRef->id}, "=", ast}, {}
                       })
                    );
                    auto & bo = std::get<std::shared_ptr<BinaryOpExpression>>(cStmts->back());
                    bo->statements.emplace_back(
                       std::make_shared<ScalarDeclarationExprExpression>(ScalarDeclarationExprExpression
                          {{{scalarDecl.scopeId}, scalarDecl.identifier, scalarDecl.kind, scalarDecl.chplLine, scalarDecl.qualifier, scalarDecl.config}, {}}
                    ));

                    auto itr = fsym->first;
                    for(; itr != fsym->second; ++itr) {
                       const auto split = itr->first.find('|');
                       const std::string fnident =
                          itr->first.substr(0, split == std::string::npos ? itr->first.size() : split);

                       if(fnident.size() == identifier.size() && fnident.substr(0, identifier.size()) == identifier) {
                          bo->statements.emplace_back(
                             std::make_shared<FunctionCallExpression>(
                                FunctionCallExpression{{symbolTableRef->id}, itr->second, {}, emitChapelLine(ast), symbolTable}
                          ));
                          curStmts.push_back(&(std::get<std::shared_ptr<FunctionCallExpression>>(bo->statements.back())->arguments));
                          break;
                       }
                    }
                 }
                 else {
                    auto itr = fsym->first;
                    for(; itr != fsym->second; ++itr) {
                       if(itr->first.size() >= identifier.size() && itr->first.substr(0, identifier.size()) == identifier) {
                          cStmts->emplace_back(
                             std::make_shared<FunctionCallExpression>(
                                FunctionCallExpression{{symbolTableRef->id}, itr->second, {}, emitChapelLine(ast), symbolTable}
                          ));

                          curStmts.push_back(&(std::get<std::shared_ptr<FunctionCallExpression>>(cStmts->back())->arguments));
                          break;
                       }
                    }
                 }
              }
              else if(0 < cStmts->size() && std::holds_alternative<std::shared_ptr<ScalarDeclarationExprExpression>>(cStmts->back())) {
                 chplx::util::dout << "1848" << std::endl;  
                 std::shared_ptr<ScalarDeclarationExprExpression> stmt =
                    std::get<std::shared_ptr<ScalarDeclarationExprExpression>>(cStmts->back());

                 cStmts->pop_back();
                 cStmts->emplace_back(
                     std::make_shared<BinaryOpExpression>(BinaryOpExpression{
                         {{symbolTableRef->id}, "=", ast}, {}
                     })
                 );
                 auto & bo = std::get<std::shared_ptr<BinaryOpExpression>>(cStmts->back());
                 bo->statements.emplace_back(stmt);

                 auto itr = fsym->first;
                 for(; itr != fsym->second; ++itr) {
                    const auto split = itr->first.find('|');
                    const std::string fnident =
                       itr->first.substr(0, split == std::string::npos ? itr->first.size() : split);

                    if(fnident.size() == identifier.size() && fnident.substr(0, identifier.size()) == identifier) {
                       bo->statements.emplace_back(
                          std::make_shared<FunctionCallExpression>(
                             FunctionCallExpression{{symbolTableRef->id}, itr->second, {}, emitChapelLine(ast), symbolTable}
                       ));
                       curStmts.push_back(&(std::get<std::shared_ptr<FunctionCallExpression>>(bo->statements.back())->arguments));
                       break;
                    }
                 }
              }
              else if(0 < cStmts->size() && std::holds_alternative<ArrayDeclarationLiteralExpression>(cStmts->back())) {
                 break;
              }   
              else if(0 < cStmts->size() && std::holds_alternative<VariableExpression>(cStmts->back())) {
                 //cStmts->pop_back();
                 chplx::util::dout << "1636\n";

                 if (std::holds_alternative<VariableExpression>(cStmts->back()))
                 {
                    chplx::util::dout << "1639\n";
                    auto scalarDecl =
                        std::get<VariableExpression>(cStmts->back());
                    chplx::util::dout << "kind: " << scalarDecl.sym->kind.index()
                              << std::endl;
                    chplx::util::dout << "isTypeKind: " << scalarDecl.sym->isTypeKind()
                              << std::endl;

                    auto itr = fsym->first;
                    for (; itr != fsym->second; ++itr)
                    {
                       const auto split = itr->first.find('|');
                       const std::string fnident = itr->first.substr(0,
                           split == std::string::npos ? itr->first.size() :
                                                        split);

                       if (fnident.size() == identifier.size() &&
                           fnident.substr(0, identifier.size()) == identifier)
                       {
                          chplx::util::dout << "1905 Adding Function Call: " << identifier
                                    << std::endl;
                          cStmts->emplace_back(
                              std::make_shared<FunctionCallExpression>(
                                  FunctionCallExpression{{symbolTableRef->id},
                                      itr->second, {}, emitChapelLine(ast),
                                      symbolTable}));
                          curStmts.push_back(&(
                              std::get<std::shared_ptr<FunctionCallExpression>>(
                                  cStmts->back())
                                  ->arguments));
                          break;
                       }
                    }

                  //   if (!scalarDecl.sym->isTypeKind())
                  //   {
                  //      cStmts->pop_back();
                  //      cStmts->emplace_back(
                  //          std::make_shared<BinaryOpExpression>(
                  //              BinaryOpExpression{
                  //                  {{symbolTableRef->id}, "=", ast}, {}}));

                  //      auto& bo = std::get<std::shared_ptr<BinaryOpExpression>>(
                  //          cStmts->back());
                  //      bo->statements.emplace_back(
                  //          std::make_shared<ScalarDeclarationExprExpression>(
                  //              ScalarDeclarationExprExpression{
                  //                  {{scalarDecl.sym->scopeId},
                  //                      scalarDecl.sym->identifier,
                  //                      scalarDecl.sym->kind,
                  //                      emitChapelLine(ast),
                  //                      scalarDecl.sym->kindqualifier,
                  //                      scalarDecl.sym->isConfig},
                  //                  {}}));
                  //   }
                  //   else
                  //   {
                  //    //   cStmts->pop_back();
                  //     return true;
                  //   }
                 }
                 else if(std::holds_alternative<ScalarDeclarationLiteralExpression>(cStmts->back())){
                    chplx::util::dout << "1916\n";
                    auto scalarDecl = std::get<ScalarDeclarationLiteralExpression>(cStmts->back());

                    cStmts->pop_back();
                    cStmts->emplace_back(
                       std::make_shared<BinaryOpExpression>(BinaryOpExpression{
                          {{symbolTableRef->id}, "=", ast}, {}
                       })
                    );
                    auto & bo = std::get<std::shared_ptr<BinaryOpExpression>>(cStmts->back());
                    bo->statements.emplace_back(
                       std::make_shared<ScalarDeclarationExprExpression>(ScalarDeclarationExprExpression
                          {{{scalarDecl.scopeId}, scalarDecl.identifier, scalarDecl.kind, scalarDecl.chplLine, scalarDecl.qualifier, scalarDecl.config}, {}}
                    ));
                 }
                 else if (std::holds_alternative<
                         std::shared_ptr<BinaryOpExpression>>(cStmts->back()))
                 {
                  chplx::util::dout << "1934" << std::endl;
                    auto& bo = std::get<std::shared_ptr<BinaryOpExpression>>(
                        cStmts->back());
                    auto itr = fsym->first;
                    for (; itr != fsym->second; ++itr)
                    {
                    const auto split = itr->first.find('|');
                    const std::string fnident = itr->first.substr(0,
                        split == std::string::npos ? itr->first.size() : split);

                    if (fnident.size() == identifier.size() &&
                        fnident.substr(0, identifier.size()) == identifier)
                    {
                          bo->statements.emplace_back(
                              std::make_shared<FunctionCallExpression>(
                                  FunctionCallExpression{{symbolTableRef->id},
                                      itr->second, {}, emitChapelLine(ast),
                                      symbolTable}));
                          curStmts.push_back(&(
                              std::get<std::shared_ptr<FunctionCallExpression>>(
                                  bo->statements.back())
                                  ->arguments));
                          break;
                    }
                    }
                 }
              }
              else {
                 chplx::util::dout  << "1687\n";
                 auto itr = fsym->first;
                 for(; itr != fsym->second; ++itr) {
                    const auto split = itr->first.find('|');
                    const std::string fnident =
                       itr->first.substr(0, split == std::string::npos ? itr->first.size() : split);

                    if(fnident.size() == identifier.size() && fnident.substr(0, identifier.size()) == identifier) {
                       chplx::util::dout << "Adding Function Call: " << identifier << std::endl;
                       cStmts->emplace_back(
                          std::make_shared<FunctionCallExpression>(
                             FunctionCallExpression{{symbolTableRef->id}, itr->second, {}, emitChapelLine(ast), symbolTable}
                       ));
                       curStmts.push_back(&(std::get<std::shared_ptr<FunctionCallExpression>>(cStmts->back())->arguments));
                       break;
                    }
                 }
              }
           }
       }
       // array index invoked
       //
       else if(fc->calledExpression()->tag() == asttags::Identifier && fc->callUsedSquareBrackets()) {
           std::optional<Symbol> varsym =
               symbolTable.find(symbolTableRef->id, "[]");

           auto encop = operatorEncoder.find("[]");
           if(std::end(operatorEncoder) == encop) {
               std::cerr << "programtreebuildingvisitor.cpp, enter, OpCall, identifier not found" << std::endl << std::flush;
               return false;
           }

           chplx::util::dout << "Identifier Array index invoked: " << identifier << std::endl;

           {
               std::string identifier{
                   dynamic_cast<const Identifier*>(fc->calledExpression())
                       ->name()
                       .c_str()};
               std::optional<std::pair<std::map<std::string, Symbol>::iterator,
                   std::map<std::string, Symbol>::iterator>>
                   fsym =
                       symbolTable.findPrefix(symbolTableRef->id, identifier);
               if (0 < cStmts->size() &&
                   std::holds_alternative<ScalarDeclarationExpression>(
                       cStmts->back()))
               {
                 auto scalarDecl =
                     std::get<ScalarDeclarationExpression>(cStmts->back());

                 cStmts->pop_back();
                 cStmts->emplace_back(std::make_shared<BinaryOpExpression>(
                     BinaryOpExpression{{{symbolTableRef->id}, "=", ast}, {}}));
                 auto& bo = std::get<std::shared_ptr<BinaryOpExpression>>(
                     cStmts->back());
                 bo->statements.emplace_back(
                     std::make_shared<ScalarDeclarationExprExpression>(
                         ScalarDeclarationExprExpression{
                             {{scalarDecl.scopeId}, scalarDecl.identifier,
                                 scalarDecl.kind, scalarDecl.chplLine,
                                 scalarDecl.qualifier, scalarDecl.config},
                             {}}));

                 auto itr = fsym->first;

                 for (; itr != fsym->second; ++itr)
                 {
                    const auto split = itr->first.find('|');
                    const std::string fnident = itr->first.substr(0,
                        split == std::string::npos ? itr->first.size() : split);

                    if (fnident.size() == identifier.size() &&
                        fnident.substr(0, identifier.size()) == identifier)
                    {
                       bo->statements.emplace_back(
                           std::make_shared<FunctionCallExpression>(
                               FunctionCallExpression{{symbolTableRef->id},
                                   itr->second, {}, emitChapelLine(ast),
                                   symbolTable}));
                       curStmts.push_back(
                           &(std::get<std::shared_ptr<FunctionCallExpression>>(
                               bo->statements.back())
                                   ->arguments));
                       break;
                    }
                 }
                 return true;
               }
           }
           chplx::util::dout << "here\n";
           if (curStmts.size() > 2 && curStmts[curStmts.size() - 2]->size() &&
               std::holds_alternative<std::shared_ptr<OnExpression>>(
                   curStmts[curStmts.size() - 2]->back()))
           {
               chplx::util::dout << "2095" << std::endl;
               auto OnExpr = std::get<std::shared_ptr<OnExpression>>(
                   curStmts[curStmts.size() - 2]->back());
               if (!OnExpr->OnLocale)
               {
                 chplx::util::dout << "Identifier On Locale: " << identifier
                           << std::endl;
                  
                 assert(varsym.has_value() && "This symbol should exist");
                 OnExpr->OnLocale = *varsym;
                 OnExpr->OnLocaleVarExpr.emplace_back(
                     VariableExpression{std::make_shared<Symbol>(
                         Symbol{varsym->kind, varsym->identifier, {}, -1,
                                false, symbolTableRef->id})});

                  assert(false && "Currently we do not support direct array indexing on OnLocale");
                 return false;
               }
           }
         //   chplx::util::dout << "IsInsideOn " << isInsideOn << std::endl;
         //   chplx::util::dout << "ScopeId: " << symbolTableRef->id << std::endl;
         //   chplx::util::dout << "varsym->scopeId: " << varsym->scopeId << std::endl;
         //   if (isInsideOn &&
         //       (varsym->scopeId <= isInsideOn ||
         //           symbolTableRef->id >= varsym->scopeId) &&
         //       identifier != "here")
         //   {
         //       chplx::util::dout << "2116 Adding OnLocaleVarsUsedInExpr : " << varsym->identifier
         //                 << " to current statements kind: " << varsym->kind.index() << std::endl;
         //       currentOnExpr->OnLocaleVarsUsedInExpr.emplace_back(
         //           VariableExpression{std::make_shared<Symbol>(*varsym)});
         //   }
           cStmts->emplace_back(
              std::make_shared<FunctionCallExpression>(
                 FunctionCallExpression{{symbolTableRef->id}, {*varsym}, {}, emitChapelLine(ast), symbolTable}
           ));
           curStmts.push_back(&(std::get<std::shared_ptr<FunctionCallExpression>>(cStmts->back())->arguments));
       }
       else {
           std::cerr << "programtreebuildingvisitor.cpp, enter, FnCall, identifier not found" << std::endl << std::flush;
           return false;
       }
    }
    break;
    case asttags::OpCall:
    {
       OpCall const* ptr =
          dynamic_cast<OpCall const*>(ast);

       std::string identifier{ptr->op().c_str()};

       std::optional<Symbol> varsym =
           symbolTable.find(symbolTableRef->id, identifier);
       if(!varsym) {
           std::cerr << "programtreebuildingvisitor.cpp, enter, OpCall, identifier not found" << std::endl << std::flush;
           return false;
       }

       auto encop = operatorEncoder.find(identifier);
       if(std::end(operatorEncoder) == encop) {
           std::cerr << "programtreebuildingvisitor.cpp, enter, OpCall, identifier not found" << std::endl << std::flush;
           return false;
       }

       std::vector<Statement> * cStmts = curStmts.back();

       chplx::util::dout << "Binary encop second: " << encop->second << std::endl;

       switch(encop->second) {
           case 0: // =
           case 1: // +
           case 2: // -
           case 3: // *
           case 4: // /
           case 5: // %
           case 7: // ==
           case 9: // <<
           case 10: // >>
           case 8: // <=>
           {
               if(0 < cStmts->size() && std::holds_alternative<std::shared_ptr<ScalarDeclarationExprExpression>>(cStmts->back())) {
                  std::shared_ptr<ScalarDeclarationExprExpression> stmt =
                    std::get<std::shared_ptr<ScalarDeclarationExprExpression>>(cStmts->back());

                  cStmts->pop_back();
                  cStmts->emplace_back(
                     std::make_shared<BinaryOpExpression>(BinaryOpExpression{
                        {{symbolTableRef->id}, "=", ast}, {}
                     })
                  );
                  auto & bo = std::get<std::shared_ptr<BinaryOpExpression>>(cStmts->back());
                  bo->statements.emplace_back(stmt);
                  bo->statements.emplace_back(
                     std::make_shared<BinaryOpExpression>(BinaryOpExpression{
                        {{symbolTableRef->id}, identifier, ast}, {}
                     })
                  );
                  auto & nbo = std::get<std::shared_ptr<BinaryOpExpression>>(bo->statements.back());
                  curStmts.push_back(&(nbo->statements));
               }
               else if(0 < cStmts->size() &&
                       std::holds_alternative<ScalarDeclarationExpression>(cStmts->back()) &&
                       encop->second != 0) {
                  // note without that final conditional on '=' operator
                  // the following code:
                  //
                  // var c : int;
                  // c = c + 1;
                  //
                  // would generate:
                  //
                  // auto c = c + 1;
                  //
                  ScalarDeclarationExpression stmt =
                    std::get<ScalarDeclarationExpression>(cStmts->back());

                  cStmts->pop_back();
                  cStmts->emplace_back(
                     std::make_shared<BinaryOpExpression>(BinaryOpExpression{
                        {{symbolTableRef->id}, "=", ast}, {}
                     })
                  );
                  auto & bo = std::get<std::shared_ptr<BinaryOpExpression>>(cStmts->back());
                  bo->statements.emplace_back(
                     std::make_shared<ScalarDeclarationExprExpression>(
                        ScalarDeclarationExprExpression{{{stmt.scopeId}, stmt.identifier, stmt.kind, stmt.chplLine, stmt.qualifier, stmt.config},{}}
                  ));
                  bo->statements.emplace_back(
                     std::make_shared<BinaryOpExpression>(BinaryOpExpression{
                        {{symbolTableRef->id}, identifier, ast}, {}
                     })
                  );
                  auto & nbo = std::get<std::shared_ptr<BinaryOpExpression>>(bo->statements.back());
                  curStmts.push_back(&(nbo->statements));
               }
/*
               else if(0 < cStmts->size() && std::holds_alternative<ScalarDeclarationLiteralExpression>(cStmts->back())) {
                  ScalarDeclarationLiteralExpression sle =
                     std::get<ScalarDeclarationLiteralExpression>(cStmts->back());

                  cStmts->pop_back();
                  cStmts->emplace_back(
                     std::make_shared<ScalarDeclarationExprExpression>(ScalarDeclarationExprExpression{
                        {{sle.scopeId}, sle.identifier, sle.kind, sle.chplLine, sle.qualifier, sle.config},
                     {
                        std::make_shared<BinaryOpExpression>(BinaryOpExpression{
                           {{symbolTableRef->id}, identifier, ast}, {}
                        })
                     }
                  }));

                  auto & bo = std::get<std::shared_ptr<ScalarDeclarationExprExpression>>(cStmts->back());
                  auto & nbo = std::get<std::shared_ptr<BinaryOpExpression>>(bo->statements.back());
                  curStmts.push_back(&(nbo->statements));
               }
*/
               else if(1 < curStmts.size() && curStmts[curStmts.size()-2]->size() && std::holds_alternative<std::shared_ptr<ForLoopExpression>>(curStmts[curStmts.size()-2]->back())) {
                  std::shared_ptr<ForLoopExpression> & fle =
                     std::get<std::shared_ptr<ForLoopExpression>>(curStmts[curStmts.size()-2]->back());

                  std::string arrayIdentifier = "";
                  std::optional<Symbol> arrayVarsym{};

                  // Search in parent scope for array variables
                  if(!pendingArrayForLoopSymbols.empty() && fle->isArrayInitForLoop) {
                     arrayVarsym = pendingArrayForLoopSymbols.front();
                     pendingArrayForLoopSymbols.pop_front();
                     arrayIdentifier = arrayVarsym->identifier;

                     auto varsymInsideForLoop = arrayVarsym;
                     std::string iteratorName = fle->iterator->identifier;
                     auto arrayIdentifierForLoopExpression = arrayIdentifier + "(" + iteratorName + ")";

                     std::vector<Statement> * cStmts = curStmts.back();
                     varsymInsideForLoop->kind = std::make_shared<func_kind>(func_kind{{
                           symbolTable.symbolTableRef->id, {}, {}, {}}, true, false});
                     cStmts->emplace_back(
                              std::make_shared<BinaryOpExpression>(BinaryOpExpression{
                                 {{symbolTableRef->id}, "=", ast}, {}
                              })
                           );
                     auto & bo = std::get<std::shared_ptr<BinaryOpExpression>>(cStmts->back());

                     
                     bo->statements.emplace_back(
                        
                        VariableExpression{std::make_shared<Symbol>(Symbol{
                           varsymInsideForLoop->kind,
                           arrayIdentifierForLoopExpression, // This is "arr(i)"
                           {}, -1, false, symbolTableRef->id
                        })}
                     );

                     bo->statements.emplace_back(
                        std::make_shared<BinaryOpExpression>(
                           BinaryOpExpression{ {{symbolTableRef->id}, identifier, ast}, {} }
                        )
                     );

                     auto & rhsOp = std::get<std::shared_ptr<BinaryOpExpression>>(bo->statements.back());
                     curStmts.push_back(&(rhsOp->statements));
                  }


                  if(fle->indexSet.size() < 2) {
                     fle->indexSet.emplace_back(
                        std::make_shared<BinaryOpExpression>(BinaryOpExpression{
                           {{symbolTableRef->id}, identifier, ast}, {}
                        })
                     );
                  }else{
                     if(!arrayVarsym) {
                        cStmts->emplace_back(
                           std::make_shared<BinaryOpExpression>(BinaryOpExpression{
                              {{symbolTableRef->id}, identifier, ast}, {}
                           })
                        );
                        auto & nbo = std::get<std::shared_ptr<BinaryOpExpression>>(cStmts->back());
                        curStmts.push_back(&(nbo->statements));
                     }
                  }
               }
               else if(1 < curStmts.size() && curStmts[curStmts.size()-2]->size() && std::holds_alternative<std::shared_ptr<ForallLoopExpression>>(curStmts[curStmts.size()-2]->back())) {
                  std::shared_ptr<ForallLoopExpression> & fle =
                     std::get<std::shared_ptr<ForallLoopExpression>>(curStmts[curStmts.size()-2]->back());

                  if(fle->indexSet.size() < 2) {
                     fle->indexSet.emplace_back(
                        std::make_shared<BinaryOpExpression>(BinaryOpExpression{
                           {{symbolTableRef->id}, identifier, ast}, {}
                        })
                     );
                  }
                  else {
                     cStmts->emplace_back(
                        std::make_shared<BinaryOpExpression>(BinaryOpExpression{
                           {{symbolTableRef->id}, identifier, ast}, {}
                        })
                     );
                     auto & nbo = std::get<std::shared_ptr<BinaryOpExpression>>(cStmts->back());
                     curStmts.push_back(&(nbo->statements));
                  }
               }
               else if(1 < curStmts.size() && curStmts[curStmts.size()-2]->size() && std::holds_alternative<std::shared_ptr<CoforallLoopExpression>>(curStmts[curStmts.size()-2]->back())) {
                  std::shared_ptr<CoforallLoopExpression> & fle =
                     std::get<std::shared_ptr<CoforallLoopExpression>>(curStmts[curStmts.size()-2]->back());

                  if(fle->indexSet.size() < 2) {
                     fle->indexSet.emplace_back(
                        std::make_shared<BinaryOpExpression>(BinaryOpExpression{
                           {{symbolTableRef->id}, identifier, ast}, {}
                        })
                     );
                  }
                  else {
                     cStmts->emplace_back(
                        std::make_shared<BinaryOpExpression>(BinaryOpExpression{
                           {{symbolTableRef->id}, identifier, ast}, {}
                        })
                     );
                     auto & nbo = std::get<std::shared_ptr<BinaryOpExpression>>(cStmts->back());
                     curStmts.push_back(&(nbo->statements));
                  }
               }
               else if(1 < curStmts.size() && curStmts[curStmts.size()-2]->size() && std::holds_alternative<std::shared_ptr<OnExpression>>(curStmts[curStmts.size()-2]->back())) {
                  std::shared_ptr<OnExpression> & one =
                     std::get<std::shared_ptr<OnExpression>>(curStmts[curStmts.size()-2]->back());

                  // if(one->OnLocaleVarExpr.size() < 1) {
                  //    assert(false);
                  //    one->OnLocaleVarExpr.emplace_back(std::make_shared<BinaryOpExpression>(BinaryOpExpression{
                  //       {{symbolTableRef->id}, identifier, ast}, {}
                  //    }));
                  // }
                  // else {
                     cStmts->emplace_back(
                        std::make_shared<BinaryOpExpression>(BinaryOpExpression{
                           {{symbolTableRef->id}, identifier, ast}, {}
                        })
                     );
                     auto & nbo = std::get<std::shared_ptr<BinaryOpExpression>>(cStmts->back());
                     curStmts.push_back(&(nbo->statements));
                  // }
               }
               else {
                  cStmts->emplace_back(
                     std::make_shared<BinaryOpExpression>(BinaryOpExpression{
                        {{symbolTableRef->id}, identifier, ast}, {}
                     })
                  );
                  auto & bo = std::get<std::shared_ptr<BinaryOpExpression>>(cStmts->back());
                  curStmts.emplace_back(&(bo->statements));
               }
           }
           break;
       }
    }
    break;
    case asttags::PrimCall:
    break;
    case asttags::Reduce:
    break;
    case asttags::ReduceIntent:
    break;
    case asttags::Scan:
    break;
    case asttags::Tuple:
    {
       if (1 < curStmts.size() && std::holds_alternative<std::shared_ptr<BinaryOpExpression>>( curStmts[curStmts.size()-2]->back() ) ) {
          std::string identifier{"tuplelit_" + emitChapelLine(ast)};
          std::optional<Symbol> varsym =
             symbolTable.find(symbolTableRef->id, identifier);

          std::vector<Statement> * cStmts = curStmts.back();
          if(varsym) {
             cStmts->emplace_back(
                TupleDeclarationLiteralExpression{
                   {{symbolTableRef->id},
                   std::string{},
                      std::make_shared<tuple_kind>(
                         tuple_kind{{
                            0,
                            identifier,
                            {},
                            std::make_shared<kind_node_type>(kind_node_type{{std::make_shared<tuple_kind>(tuple_kind{{}})}})
                      }}),
                   emitChapelLine(ast), 0, false},{*varsym}
             });
          }
          else {
             cStmts->emplace_back(
                TupleDeclarationLiteralExpression{
                   {{symbolTableRef->id},
                   std::string{},
                      std::make_shared<tuple_kind>(
                         tuple_kind{{
                            0,
                            identifier,
                            {},
                            std::make_shared<kind_node_type>(kind_node_type{{std::make_shared<tuple_kind>(tuple_kind{{}})}})
                      }}),
                   emitChapelLine(ast), 0, false},{}
             });
          }

       }
    }
    break;
    case asttags::Zip:
    isInsideZip = true;
    break;
    case asttags::END_Call:
    break;
    case asttags::MultiDecl:
    break;
    case asttags::TupleDecl:
    {
    ++isInsideForallTuple;
    }
    break;
    case asttags::ForwardingDecl:
    break;
    case asttags::EnumElement:
    break;
    case asttags::START_VarLikeDecl:
    break;
    case asttags::Formal:
    break;
    case asttags::TaskVar:
    break;
    case asttags::VarArgFormal:
    break;
    case asttags::Variable:
    {
       std::string identifier =
          std::string{dynamic_cast<NamedDecl const*>(ast)->name().c_str()};
       std::optional<Symbol> varsym{};
       std::optional<Symbol> varsymInsideForLoop{};
       symbolTable.find(symbolTableRef->id, identifier, varsym);
       bool stmt = true;

       chplx::util::dout << "Variable identifier: " << identifier << std::endl;
       if(curStmts.size() > 1 && curStmts[curStmts.size()-2]->size())
       chplx::util::dout << "curStmts kind: " << curStmts[curStmts.size()-2]->back().index() << std::endl;
       for(auto& vec: curStmts) {
         for(auto& stmt: *vec) {
           chplx::util::dout << "stmt kind: " << stmt.index() << std::endl;
         }
       }
       std::vector<Statement> * cStmts = curStmts.back();
       if(cStmts && cStmts->size()) chplx::util::dout << "cStmts kind: " << cStmts->back().index() << std::endl;

       if (1 < curStmts.size() && std::holds_alternative<std::shared_ptr<ForLoopExpression>>( curStmts[curStmts.size()-2]->back() ) ) {
           std::shared_ptr<ForLoopExpression> & fl =
               std::get<std::shared_ptr<ForLoopExpression>>(curStmts[curStmts.size()-2]->back());

           if (!fl->iterator)
           {
             fl->iterator = *varsym;
             stmt = false;
           }

           if (fl->isArrayInitForLoop)
           {
             // This is the iterator variable in an array init for-loop
             // We need to find the target array variable from the parent scope
             // Look for array variable in the parent scope that's being initialized
             // Search in parent scope for array variables
             const auto parentScopeId = symbolTable.parentSymbolTableId;
             const auto curScopeId = symbolTable.symbolTableRef->id;
             const auto maxScope = std::max(parentScopeId, curScopeId);
             const auto minScope = std::min(parentScopeId, curScopeId);
             for (int scope = minScope; scope <= maxScope; ++scope)
             {
                     std::optional<
                         std::pair<std::map<std::string, Symbol>::iterator,
                             std::map<std::string, Symbol>::iterator>>
                         allSymbolsParScope = symbolTable.findPrefix(scope, "");
                     if (allSymbolsParScope.has_value())
                     {
                        for (auto it = allSymbolsParScope->first;
                             it != allSymbolsParScope->second; ++it)
                        {
                       if (std::holds_alternative<std::shared_ptr<func_kind>>(
                               it->second.kind))
                       {
                          auto func_sym = std::get<std::shared_ptr<func_kind>>(
                              it->second.kind);
                          if (func_sym->isArrayInitForLoop)
                          {
                              const auto& candidate = func_sym->arraySym;
                              const auto& candidateArrIdentifier =
                                  func_sym->arrayIdentifier;

                              if (pendingArrayForLoopSymbolsMap.find(
                                      candidateArrIdentifier) !=
                                  pendingArrayForLoopSymbolsMap.end())
                              {
                                  // Already added this symbol
                                  continue;
                              }
                              pendingArrayForLoopSymbolsMap
                                  [candidateArrIdentifier] = true;

                              pendingArrayForLoopSymbols.push_back(candidate);
                          }
                       }
                        }
                     }
             }

             assert(pendingArrayForLoopSymbols.size()); // Should have atleast one element
           }
       }
       else if (1 < curStmts.size() && std::holds_alternative<std::shared_ptr<ForallLoopExpression>>( curStmts[curStmts.size()-2]->back() ) ) {
           std::shared_ptr<ForallLoopExpression> & fl =
               std::get<std::shared_ptr<ForallLoopExpression>>(curStmts[curStmts.size()-2]->back());
            if(isInsideForallTuple == 2){
               fl->isZippedIter = true;
            }
            if (fl->iterator.size() && !fl->iterator[0] &&
                isInsideForallTuple != 2)
            {
               fl->iterator[0] = *varsym;
               stmt = false;
            }
            else
            {
               fl->iterator.emplace_back(*varsym);
               stmt = false;
            }
           chplx::util::dout << "Inside Forall : " << isInsideForallTuple << " stmt : " << stmt << std::endl;
       }
       else if (1 < curStmts.size() && std::holds_alternative<std::shared_ptr<CoforallLoopExpression>>( curStmts[curStmts.size()-2]->back() ) ) {
           std::shared_ptr<CoforallLoopExpression> & fl =
               std::get<std::shared_ptr<CoforallLoopExpression>>(curStmts[curStmts.size()-2]->back());
            chplx::util::dout << "Does iterator have value? " << fl->iterator.has_value() << "\n";
            if (!fl->iterator)
            {
             fl->iterator = *varsym;
             stmt = false;
            }
       }
       else if (1 < curStmts.size() && std::holds_alternative<std::shared_ptr<OnExpression>>( curStmts[curStmts.size()-2]->back() ) ) {
           std::shared_ptr<OnExpression> & one =
               std::get<std::shared_ptr<OnExpression>>(curStmts[curStmts.size()-2]->back());

            chplx::util::dout << "Variabl " << identifier <<   " \n";
           if (one->OnLocale)
           {
             stmt = true;
           }
           else
           {
             one->OnLocale = *varsym;
             stmt = false;
           }
           chplx::util::dout << "stmt : " <<stmt << "\n";
      }
      chplx::util::dout << "varsym kind : " << varsym->kind.index() << " does varsym hold val: " << varsym.has_value() << "\n";
      chplx::util::dout << "stmt : " << stmt << "\n";
      chplx::util::dout << "IsInsideOn: " << isInsideOn << std::endl;
       if(stmt) {
         if(isInsideForallTuple == 2){
            std::vector<Statement> * cStmts = curStmts.back();

             // this situation is not likely to happen
             //
             chplx::util::dout << "2225\n";
             if(varsym->literal.size() ) {
                   std::visit(
                      VariableLiteralVisitor{symbolTableRef->id, identifier, *varsym, *cStmts, br, ctx, ast},
                      varsym->kind
                   );
                }
                else {
                   std::visit(
                   VariableVisitor{symbolTableRef->id, identifier, *varsym, *cStmts, br, ctx, ast},
                   varsym->kind
                   );
             }
             return true;
         }
          if(varsym &&
             std::holds_alternative<std::shared_ptr<array_kind>>(varsym->kind) &&
             std::get<std::shared_ptr<array_kind>>(varsym->kind)->args.size() &&
             std::holds_alternative<std::shared_ptr<domain_kind>>(std::get<std::shared_ptr<array_kind>>(varsym->kind)->args.back().kind) &&
             std::holds_alternative<std::shared_ptr<range_kind>>(
                std::get<std::shared_ptr<domain_kind>>(std::get<std::shared_ptr<array_kind>>(varsym->kind)->args.back().kind)->args.back().kind
          )) {
             std::vector<Statement> * cStmts = curStmts.back();

             // this situation is not likely to happen
             //
             chplx::util::dout << "2173\n";
             if(varsym->literal.size() ||
                std::holds_alternative<std::shared_ptr<kind_node_type>>(std::get<std::shared_ptr<array_kind>>(varsym->kind)->retKind) ) {
                   std::visit(
                      VariableLiteralVisitor{symbolTableRef->id, identifier, *varsym, *cStmts, br, ctx, ast},
                      varsym->kind
                   );
                }
                else {
                   std::visit(
                   VariableVisitor{symbolTableRef->id, identifier, *varsym, *cStmts, br, ctx, ast},
                   varsym->kind
                   );
             }

             if(std::holds_alternative<std::shared_ptr<ScalarDeclarationExprExpression>>(cStmts->back())) {
               auto & se = std::get<std::shared_ptr<ScalarDeclarationExprExpression>>(cStmts->back());
               curStmts.emplace_back(&(se->statements));
             }
          }
          else if(varsym &&
             std::holds_alternative<std::shared_ptr<array_kind>>(varsym->kind) &&
             std::get<std::shared_ptr<array_kind>>(varsym->kind)->args.size() &&
             std::holds_alternative<std::shared_ptr<domain_kind>>(std::get<std::shared_ptr<array_kind>>(varsym->kind)->args.back().kind)) {
               chplx::util::dout << "2197\n";
             std::vector<Statement> * cStmts = curStmts.back();
             auto & ak = std::get<std::shared_ptr<array_kind>>(varsym->kind);
             if(ak->args.size()) {
               cStmts->emplace_back(ArrayDeclarationLiteralExpression{{{symbolTableRef->id}, identifier, varsym->kind, emitChapelLine(ast), varsym->kindqualifier, varsym->isConfig},{}});
                std::get<ArrayDeclarationLiteralExpression>(cStmts->back()).literalValues.push_back(&(ak->args[0]));
             }
          }
          else if(varsym && std::holds_alternative<std::shared_ptr<tuple_kind>>(varsym->kind)) {
             std::shared_ptr<tuple_kind> & tk =
                std::get<std::shared_ptr<tuple_kind>>(varsym->kind);
               chplx::util::dout << "Tuple proc\n";

             if(0 < tk->args.size()) {
               std::vector<Statement> * cStmts = curStmts.back();
               cStmts->emplace_back(TupleDeclarationLiteralExpression{{{symbolTableRef->id}, identifier, varsym->kind, emitChapelLine(ast), varsym->kindqualifier, varsym->isConfig},{}});
/*
               auto & tdle = std::get<TupleDeclarationLiteralExpression>(cStmts->back());
               for(auto arg : tk->args) {
                  tdle.literalValues.push_back(LiteralExpression{arg.kind,arg.literal[0]});
               }
*/
             }
             else {
               std::vector<Statement> * cStmts = curStmts.back();
               cStmts->emplace_back(TupleDeclarationExpression{{{symbolTableRef->id}, identifier, varsym->kind, emitChapelLine(ast), varsym->kindqualifier, varsym->isConfig}});
               //auto & te = std::get<TupleDeclarationExpression>(cStmts->back());
               //curStmts.emplace_back(&(te->statements));
             }
          }
          else if(varsym) {
             std::vector<Statement> * cStmts = curStmts.back();
             chplx::util::dout << "2228\n";
             if(cStmts->size())
             chplx::util::dout << "Last expr kind: " << cStmts->back().index() << std::endl;
             if((std::holds_alternative<std::monostate>(varsym->kind) ||
                std::holds_alternative<nil_kind>(varsym->kind)) && !isInsideZip ) {
                  chplx::util::dout << "2231\n";
               cStmts->emplace_back(
                  std::make_shared<ScalarDeclarationExprExpression>(ScalarDeclarationExprExpression{
                     {{symbolTableRef->id}, identifier, varsym->kind, emitChapelLine(ast), varsym->kindqualifier, varsym->isConfig},{}}
                  ));
               auto & se = std::get<std::shared_ptr<ScalarDeclarationExprExpression>>(cStmts->back());
               curStmts.emplace_back(&(se->statements));
             }
             else if(std::holds_alternative<std::shared_ptr<tuple_kind>>(varsym->kind)) {
               cStmts->emplace_back(
                  std::make_shared<TupleDeclarationExprExpression>(TupleDeclarationExprExpression{
                     {{symbolTableRef->id}, identifier, varsym->kind, emitChapelLine(ast), varsym->kindqualifier, varsym->isConfig},{}}
                  ));
               auto & te = std::get<std::shared_ptr<TupleDeclarationExprExpression>>(cStmts->back());
               curStmts.emplace_back(&(te->statements));
             }
             else {
                chplx::util::dout << "Varsym literal size: " << varsym->literal.size() << std::endl;
                if(varsym->literal.size() ||
                   ( std::holds_alternative<std::shared_ptr<array_kind>>(varsym->kind) &&
                     std::holds_alternative<std::shared_ptr<kind_node_type>>(std::get<std::shared_ptr<array_kind>>(varsym->kind)->retKind)) ) {
                   chplx::util::dout << "Variable 2246\n";
                   chplx::util::dout << "Kind: " << varsym->kind.index() << std::endl;
                   chplx::util::dout << "isInsideOn: " << isInsideOn << std::endl;
                   chplx::util::dout << "Scope : " << symbolTableRef->id << std::endl;
                   chplx::util::dout << "Var scope: " << varsym->scopeId << std::endl;
                   if(isInsideOn) varsym->scopeId = symbolTable.symbolTableRef->id;
                   std::visit(
                      VariableLiteralVisitor{symbolTableRef->id, identifier, *varsym, *cStmts, br, ctx, ast},
                      varsym->kind
                   );
                }
                else {
                   chplx::util::dout  << "2229 Identifier: " << identifier << std::endl;
                   chplx::util::dout << "Scope : " << symbolTableRef->id << std::endl;
                   chplx::util::dout << "Var scope: " << varsym->scopeId << std::endl;
                   chplx::util::dout << "OnIndex scope: " << isInsideOn << std::endl;
                   chplx::util::dout << "Kind: " << varsym->kind.index() << std::endl;
                   if (isInsideOn && !varsym->isIntegralKind() &&
                       varsym->scopeId < symbolTable.symbolTableRef->id && varsym->scopeId < isInsideOn)
                   {
                       chplx::util::dout << "2184 Adding OnLocaleVarsUsedInExpr for: "
                                 << identifier << std::endl;
                       chplx::util::dout << "Scope : " << symbolTableRef->id
                                 << std::endl;
                       chplx::util::dout << "Var scope: " << varsym->scopeId
                                 << std::endl;
                       currentOnExpr->OnLocaleVarsUsedInExpr.emplace_back(
                           VariableExpression{std::make_shared<Symbol>(
                               Symbol{varsym->kind, identifier, {}, -1, false,
                                   symbolTableRef->id})});
                   }
                   std::visit(
                      VariableVisitor{symbolTableRef->id, identifier, *varsym, *cStmts, br, ctx, ast},
                      varsym->kind
                   );
                }
             }
          }
       }
       chplx::util::dout << "Variable proc end\n";
    }
    break;
    case asttags::END_VarLikeDecl:
    break;
    case asttags::Enum:
    break;
    case asttags::Catch:
    break;
    case asttags::Cobegin:
    break;
    case asttags::Conditional:
    {
       std::string identifier{"if" + emitChapelLine(ast)};

       std::optional<Symbol> varsym =
          symbolTable.find(symbolTableRef->id, identifier);

       if(!varsym) {
          identifier = "else" + identifier;
          varsym =
             symbolTable.find(symbolTableRef->id, identifier);
       }

       if(varsym.has_value() && std::holds_alternative<std::shared_ptr<func_kind>>(varsym->kind)) {
          std::vector<Statement> * cStmts = curStmts.back();

          std::shared_ptr<func_kind> & fk = std::get<std::shared_ptr<func_kind>>(varsym->kind);
          symbolTableRef = symbolTable.lut[fk->lutId];

          if (1 < curStmts.size() && std::holds_alternative<std::shared_ptr<ConditionalExpression>>( curStmts[curStmts.size()-2]->back() )) {
             curStmts.pop_back();
             cStmts = curStmts.back();

             auto ce = std::get<std::shared_ptr<ConditionalExpression>>(cStmts->back());
             ce->exprs.back().scopeId = fk->lutId;
             ce->exprs.back().node = const_cast<uast::AstNode *>(ast);
             curStmts.emplace_back(&(ce->exprs.back().conditions));
          }
          else {
             cStmts->emplace_back(
                std::make_shared<ConditionalExpression>(
                   ConditionalExpression{{{fk->lutId}, ast}, *varsym, {ConditionedExpression{{{fk->lutId}, ast},{},{}}}}
             ));
             auto & fndecl = std::get<std::shared_ptr<ConditionalExpression>>(cStmts->back());
             curStmts.emplace_back(&(fndecl->exprs.back().conditions));
          }
       }
       else {
          std::optional<Symbol> condsym =
             symbolTable.find(identifier);
          std::cerr << std::string{"Error attempting to find: " + identifier} << std::endl;
          assert(condsym.has_value()); 
       }
    }
    break;
    case asttags::Implements:
    break;
    case asttags::Label: // contains a loop
    break;
    case asttags::Select:
    break;
    case asttags::Sync:
    break;
    case asttags::Try:
    break;
    case asttags::START_SimpleBlockLike:
    break;
    case asttags::Begin:
    break;
    case asttags::Block:
    {
       if (1 < curStmts.size() && 0 < curStmts[curStmts.size()-2]->size() && std::holds_alternative<std::shared_ptr<ConditionalExpression>>( curStmts[curStmts.size()-2]->back() )) {
          curStmts.pop_back();
          std::vector<Statement> * cStmts = curStmts.back();
          auto & fndecl = std::get<std::shared_ptr<ConditionalExpression>>(cStmts->back());
          curStmts.emplace_back(&(fndecl->exprs.back().statements));
       }
       else if(0 < curStmts.size() && 0 < curStmts[curStmts.size()-1]->size() && std::holds_alternative<std::shared_ptr<ConditionalExpression>>( curStmts[curStmts.size()-1]->back() )) {
          std::vector<Statement> * cStmts = curStmts.back();
          auto & ce = std::get<std::shared_ptr<ConditionalExpression>>(cStmts->back());
          ce->exprs.emplace_back(
             ConditionedExpression{{{symbolTableRef->id}, ast},{},{}}
          );
          curStmts.emplace_back(&(ce->exprs.back().statements));
       }
    }
    break;
    case asttags::Defer:
    break;
    case asttags::Local:
    break;
    case asttags::Manage:
    break;
    case asttags::On:
    {
      std::string identifier{"on_" + emitChapelLine(ast)};
       std::optional<Symbol> varsym =
          symbolTable.find(symbolTableRef->id, identifier);

       if(varsym.has_value() && std::holds_alternative<std::shared_ptr<func_kind>>(varsym->kind)) {
          ++isInsideOn;
          std::vector<Statement> * cStmts = curStmts.back();

          std::shared_ptr<func_kind> & fk = std::get<std::shared_ptr<func_kind>>(varsym->kind);
          symbolTableRef = symbolTable.lut[fk->lutId];

          cStmts->emplace_back(
             std::make_shared<OnExpression>(
                OnExpression{{{fk->lutId}, ast, {}}, *varsym, {}, {}, {}, {}, emitChapelLine(ast)}
          ));

          auto fndecl = std::get<std::shared_ptr<OnExpression>>(cStmts->back());
          onExprStack.push_back(fndecl);
          curStmts.emplace_back(&(fndecl->statements));
          currentOnExpr = onExprStack.back();
       }
    }
    break;
    case asttags::Serial:
    break;
    case asttags::When:
    break;
    case asttags::END_SimpleBlockLike:
    break;
    case asttags::START_Loop:
    break;
    case asttags::DoWhile:
    break;
    case asttags::While:
    break;
    case asttags::START_IndexableLoop:
    break;
    case asttags::BracketLoop:
    break;
    case asttags::Coforall:
    {
       std::string identifier{"coforall" + emitChapelLine(ast)};
       std::optional<Symbol> varsym =
          symbolTable.find(symbolTableRef->id, identifier);

       if(varsym.has_value() && std::holds_alternative<std::shared_ptr<func_kind>>(varsym->kind)) {
          std::vector<Statement> * cStmts = curStmts.back();

          std::shared_ptr<func_kind> & fk = std::get<std::shared_ptr<func_kind>>(varsym->kind);
          symbolTableRef = symbolTable.lut[fk->lutId];

          cStmts->emplace_back(
             std::make_shared<CoforallLoopExpression>(
                CoforallLoopExpression{{{fk->lutId}, ast, {}}, *varsym, {}, {}, {}, emitChapelLine(ast)}
          ));

          auto & fndecl = std::get<std::shared_ptr<CoforallLoopExpression>>(cStmts->back());
          curStmts.emplace_back(&(fndecl->statements));
       }
    }
    break;
    case asttags::For:
    {
       std::string identifier{"for" + emitChapelLine(ast)};
       std::optional<Symbol> varsym =
          symbolTable.find(symbolTableRef->id, identifier);
       bool isArrayInitForLoop = false;
      
      if(!varsym) {
         identifier = "array_init_for" + emitChapelLine(ast);
         varsym = symbolTable.find(symbolTableRef->id, identifier);
         if(varsym.has_value()) isArrayInitForLoop = true;
      }

       if(varsym.has_value() && std::holds_alternative<std::shared_ptr<func_kind>>(varsym->kind)) {
          chplx::util::dout << "For loop identifier: " << identifier << std::endl;
          std::vector<Statement> * cStmts = curStmts.back();

          std::shared_ptr<func_kind> & fk = std::get<std::shared_ptr<func_kind>>(varsym->kind);
          symbolTableRef = symbolTable.lut[fk->lutId];

          cStmts->emplace_back(
             std::make_shared<ForLoopExpression>(
                ForLoopExpression{{{fk->lutId}, ast, {}}, *varsym, {}, {}, {}, emitChapelLine(ast),isArrayInitForLoop}
          ));

          auto & fndecl = std::get<std::shared_ptr<ForLoopExpression>>(cStmts->back());
          curStmts.push_back(  &fndecl->statements );
       }else{
         chplx::util::dout << "for error: " << identifier << std::endl;
         chplx::util::dout << "scope: " << symbolTableRef->id << std::endl;
       }
    }
    break;
    case asttags::Forall:
    {
       std::string identifier{"forall" + emitChapelLine(ast)};
       std::optional<Symbol> varsym =
          symbolTable.find(symbolTableRef->id, identifier);

       if(varsym.has_value() && std::holds_alternative<std::shared_ptr<func_kind>>(varsym->kind)) {
          std::vector<Statement> * cStmts = curStmts.back();

          std::shared_ptr<func_kind> & fk = std::get<std::shared_ptr<func_kind>>(varsym->kind);
          symbolTableRef = symbolTable.lut[fk->lutId];

          cStmts->emplace_back(
             std::make_shared<ForallLoopExpression>(
                ForallLoopExpression{{{fk->lutId}, ast, {}}, *varsym, {}, {}, {}, emitChapelLine(ast)}
          ));

          auto & fndecl = std::get<std::shared_ptr<ForallLoopExpression>>(cStmts->back());
          curStmts.emplace_back(&(fndecl->statements));

          isInsideForallTuple = 1;
       }
    }
    break;
    case asttags::Foreach:
    break;
    case asttags::END_IndexableLoop:
    break;
    case asttags::END_Loop:
    break;
    case asttags::START_Decl:
    break;
    case asttags::START_NamedDecl:
    break;
    case asttags::START_TypeDecl:
    break;
    case asttags::Function:
    {
       // pattern to repeat in the symbol table builder
       //
       struct ProgramTreeFunctionVisitor {
          bool enter(uast::AstNode const* ast) {
             const auto tag = ast->tag();
             if(tag == asttags::Function && !complete) {
                lookup += std::string{dynamic_cast<Function const*>(ast)->name().c_str()};
             }
             else if(tag == asttags::Identifier && !complete) {
                lookup += "|" + std::string{dynamic_cast<Identifier const*>(ast)->name().c_str()};
             }
             else if(tag == asttags::Block && !complete) {
                complete = true;
             }
             return true;
          }

          void exit(uast::AstNode const* ast) {
          }

          bool complete;
          std::string lookup;
       };

       // check to see if the function symbol has a 'kind' set;
       // if not set, set it to something (unknown is OK)
       //
       if(node.has_value() && ast != (*node)) {
          ProgramTreeFunctionVisitor v{false, {}};
          ast->traverse(v);

          std::optional<Symbol> sym =
             symbolTable.find(symbolTableRef->id, v.lookup);

          if(sym) {
             std::shared_ptr<func_kind> & fk = std::get<std::shared_ptr<func_kind>>(sym->kind);
             symbolTableRef = symbolTable.lut[fk->lutId];

             std::vector<Statement> * cStmts = curStmts.back();

             cStmts->emplace_back(
                std::make_shared<FunctionDeclarationExpression>(
                   FunctionDeclarationExpression{{{fk->lutId}, ast, {}}, *sym, {}, emitChapelLine(ast)}
             ));

             auto & fndecl = std::get<std::shared_ptr<FunctionDeclarationExpression>>(cStmts->back());
             curStmts.emplace_back(&(fndecl->statements));
          }
          else {
             std::optional< std::pair< std::map<std::string, Symbol>::iterator, std::map<std::string, Symbol>::iterator > > fnsym
                = symbolTable.findPrefix(symbolTableRef->id, v.lookup);
             assert(fnsym.has_value());

             std::map<std::string, Symbol>::iterator val = fnsym->second;
             for(std::map<std::string, Symbol>::iterator itr = fnsym->first; itr != fnsym->second; ++itr) {
                 if(v.lookup == itr->first) {
                    val = itr;
                 } 
             }

             if(val == fnsym->second) { return false; }

             //Symbol & fsym = val->second;

             std::shared_ptr<func_kind> & fk = std::get<std::shared_ptr<func_kind>>(sym->kind);
             symbolTableRef = symbolTable.lut[fk->lutId];
          }
       }
    }
    break;
    case asttags::Interface:
    break;
    case asttags::Module:
    {
       std::string lookup = static_cast<Module const*>(ast)->name().str();

       std::optional<Symbol> sym =
          symbolTable.find(symbolTableRef->id, lookup);

       if(sym) {
          std::shared_ptr<module_kind> & fk = std::get<std::shared_ptr<module_kind>>(sym->kind);
          symbolTableRef = symbolTable.lut[fk->lutId];

          std::vector<Statement> * cStmts = curStmts.back();

          cStmts->emplace_back(
             std::make_shared<ModuleDeclarationExpression>(
                ModuleDeclarationExpression{{{{fk->lutId}, ast, {}}, *sym, {}, emitChapelLine(ast)}}
          ));

          auto & fndecl = std::get<std::shared_ptr<ModuleDeclarationExpression>>(cStmts->back());
          curStmts.emplace_back(&(fndecl->statements));
       }
       else {
          std::optional< std::pair< std::map<std::string, Symbol>::iterator, std::map<std::string, Symbol>::iterator > > fnsym
             = symbolTable.findPrefix(symbolTableRef->id, lookup);
          assert(fnsym.has_value());

          std::map<std::string, Symbol>::iterator val = fnsym->second;
          for(std::map<std::string, Symbol>::iterator itr = fnsym->first; itr != fnsym->second; ++itr) {
             if(lookup == itr->first) {
                val = itr;
             } 
          }

          if(val == fnsym->second) { return false; }

          //Symbol & fsym = val->second;

          std::shared_ptr<module_kind> & fk = std::get<std::shared_ptr<module_kind>>(sym->kind);
          symbolTableRef = symbolTable.lut[fk->lutId];
       }
    }
    break;
    case asttags::START_AggregateDecl:
    break;
    case asttags::Record:
    {
       std::string lookup = static_cast<Class const*>(ast)->name().str();

       std::optional<Symbol> sym =
          symbolTable.find(symbolTableRef->id, lookup);

       if(sym) {
          std::shared_ptr<record_kind> & fk = std::get<std::shared_ptr<record_kind>>(sym->kind);
          symbolTableRef = symbolTable.lut[fk->lutId];

          std::vector<Statement> * cStmts = curStmts.back();

          cStmts->emplace_back(
             std::make_shared<RecordDeclarationExpression>(
                RecordDeclarationExpression{{{fk->lutId}, ast, {}}, *sym, {}, emitChapelLine(ast)}
          ));

          auto & fndecl = std::get<std::shared_ptr<RecordDeclarationExpression>>(cStmts->back());
          curStmts.emplace_back(&(fndecl->statements));
       }
       else {
          std::optional< std::pair< std::map<std::string, Symbol>::iterator, std::map<std::string, Symbol>::iterator > > fnsym
             = symbolTable.findPrefix(symbolTableRef->id, lookup);

          assert(fnsym.has_value());

          std::map<std::string, Symbol>::iterator val = fnsym->second;
          for(std::map<std::string, Symbol>::iterator itr = fnsym->first; itr != fnsym->second; ++itr) {
             if(lookup == itr->first) {
                val = itr;
             } 
          }

          if(val == fnsym->second) { return false; }

          //Symbol & fsym = val->second;

          std::shared_ptr<record_kind> & fk = std::get<std::shared_ptr<record_kind>>(sym->kind);
          symbolTableRef = symbolTable.lut[fk->lutId];
       }
    }
    break;
    case asttags::Class:
    {
       std::string lookup = static_cast<Class const*>(ast)->name().str();

       std::optional<Symbol> sym =
          symbolTable.find(symbolTableRef->id, lookup);

       if(sym) {
          std::shared_ptr<class_kind> & fk = std::get<std::shared_ptr<class_kind>>(sym->kind);
          symbolTableRef = symbolTable.lut[fk->lutId];

          std::vector<Statement> * cStmts = curStmts.back();

          cStmts->emplace_back(
             std::make_shared<ClassDeclarationExpression>(
                ClassDeclarationExpression{{{{fk->lutId}, ast, {}}, *sym, {}, emitChapelLine(ast)}}
          ));

          auto & fndecl = std::get<std::shared_ptr<ClassDeclarationExpression>>(cStmts->back());
          curStmts.emplace_back(&(fndecl->statements));
       }
       else {
          std::optional< std::pair< std::map<std::string, Symbol>::iterator, std::map<std::string, Symbol>::iterator > > fnsym
             = symbolTable.findPrefix(symbolTableRef->id, lookup);

          assert(fnsym.has_value());

          std::map<std::string, Symbol>::iterator val = fnsym->second;
          for(std::map<std::string, Symbol>::iterator itr = fnsym->first; itr != fnsym->second; ++itr) {
             if(lookup == itr->first) {
                val = itr;
             } 
          }

          if(val == fnsym->second) { return false; }

          //Symbol & fsym = val->second;

          std::shared_ptr<class_kind> & fk = std::get<std::shared_ptr<class_kind>>(sym->kind);
          symbolTableRef = symbolTable.lut[fk->lutId];
       }
    }
    break;
    case asttags::Union:
    break;
    case asttags::END_AggregateDecl:
    break;
    case asttags::END_Decl:
    break;
    case asttags::END_NamedDecl:
    break;
    case asttags::END_TypeDecl:
    break;
    case asttags::NUM_AST_TAGS:
    break;
    case asttags::AST_TAG_UNKNOWN:
    break;
    default:
    break;
   }

   return true;
}

void ProgramTreeBuildingVisitor::exit(const uast::AstNode * ast) {
   if(chplx::util::compilerDebug) {
      chplx::util::dout << "---Exit AST Node\t" << tagToString(ast->tag()) << std::endl
                << "---\tCurrent Scope\t" << symbolTable.symbolTableRef->id << std::endl
                << "---\tCurrent Scope id \t" << symbolTableRef->id << std::endl
                << "---\tCurrent Statement List Size\t" << curStmts.size() << std::endl
                << "---\t" << emitChapelLine(ast);
   }

   switch(ast->tag()) {
    case asttags::AnonFormal:
    break;
    case asttags::As:
    break;
    case asttags::Array:
    break;
    case asttags::Attribute:
    break;
    case asttags::Break:
    break;
    case asttags::Comment:
    break;
    case asttags::Continue:
    break;
    case asttags::Delete:
    break;
    case asttags::Domain:
    break;
    case asttags::Dot:
    {
    if (pushedDot)
    {
          curStmts.pop_back();
          pushedDot = false;
    }
    specialPushedDot = false;
    }
    break;
    case asttags::EmptyStmt:
    break;
    case asttags::ErroneousExpression:
    break;
    case asttags::ExternBlock:
    break;
    case asttags::FunctionSignature:
    break;
    case asttags::Identifier:
    break;
    case asttags::Import:
    break;
    case asttags::Include:
    break;
    case asttags::Let:
    break;
    case asttags::New:
    break;
    case asttags::Range:
    break;
    case asttags::Require:
    break;
    case asttags::Throw:
    break;
    case asttags::TypeQuery:
    break;
    case asttags::Use:
    break;
    case asttags::VisibilityClause:
    break;
    case asttags::WithClause:
    break;
    case asttags::Yield:
    break;
    case asttags::START_Literal:
    break;
    case asttags::BoolLiteral:
    break;
    case asttags::ImagLiteral:
    break;
    case asttags::IntLiteral:
    break;
    case asttags::RealLiteral:
    break;
    case asttags::UintLiteral:
    break;
    case asttags::START_StringLikeLiteral:
    break;
    case asttags::BytesLiteral:
    break;
    case asttags::CStringLiteral:
    break;
    case asttags::StringLiteral:
    break;
    case asttags::END_StringLikeLiteral:
    break;
    case asttags::END_Literal:
    break;
    case asttags::START_Call:
    break;
    case asttags::For:
    case asttags::Forall:
    isInsideForallTuple = 0;
    break;
    case asttags::Return:
    case asttags::FnCall:
    {
      if(curStmts.back()->size()) {
         chplx::util::dout << "Fn Call Pop back: " << curStmts.back()->back().index() << std::endl;
      }

      // if (curStmts.size() > 1 && std::holds_alternative<std::shared_ptr<OnExpression>>(
      //          curStmts[curStmts.size() - 2]->back()))
      // {
      //       return;
      // }
      chplx::util::dout << "Popping \n";
      for(auto& stmt: *curStmts.back())
      chplx::util::dout << "before Cur stmts kind : " << stmt.index() << std::endl;
    if(std::holds_alternative<std::shared_ptr<ForLoopExpression>>(curStmts[curStmts.size()-2]->back())) {
          return;
       }
       else if(std::holds_alternative<std::shared_ptr<ForallLoopExpression>>(curStmts[curStmts.size()-2]->back())) {
          return;
       }
       else if(std::holds_alternative<std::shared_ptr<CoforallLoopExpression>>(curStmts[curStmts.size()-2]->back())) {
          return;
       }
       else if(std::holds_alternative<std::shared_ptr<OnExpression>>(curStmts[curStmts.size()-2]->back())) {
          return;
       }
      curStmts.pop_back();
      for(auto& stmt: *curStmts.back())
      chplx::util::dout << "after Cur stmts kind : " << stmt.index() << std::endl;
    }
    break;
    case asttags::OpCall:
    {
       // these conditionals are a side effect of the addition of expression support
       // to the range of loops
       //
       if(std::holds_alternative<std::shared_ptr<ForLoopExpression>>(curStmts[curStmts.size()-2]->back())) {
          return;
       }
       else if(std::holds_alternative<std::shared_ptr<ForallLoopExpression>>(curStmts[curStmts.size()-2]->back())) {
          return;
       }
       else if(std::holds_alternative<std::shared_ptr<CoforallLoopExpression>>(curStmts[curStmts.size()-2]->back())) {
          return;
       }
       else if(std::holds_alternative<std::shared_ptr<OnExpression>>(curStmts[curStmts.size()-2]->back())) {
          return;
       }
       curStmts.pop_back();
    }
    break;
    case asttags::PrimCall:
    break;
    case asttags::Reduce:
    break;
    case asttags::ReduceIntent:
    break;
    case asttags::Scan:
    break;
    case asttags::Tuple:
    break;
    case asttags::Zip:
    isInsideZip = false;
    break;
    case asttags::END_Call:
    break;
    case asttags::MultiDecl:
    break;
    case asttags::TupleDecl:
    --isInsideForallTuple;
    break;
    case asttags::ForwardingDecl:
    break;
    case asttags::EnumElement:
    break;
    case asttags::START_VarLikeDecl:
    break;
    case asttags::Formal:
    break;
    case asttags::TaskVar:
    break;
    case asttags::VarArgFormal:
    break;
    case asttags::Variable:
    {
       if(curStmts.size() > 1 && std::holds_alternative<std::shared_ptr<ScalarDeclarationExprExpression>>(curStmts[curStmts.size()-2]->back())) {
          curStmts.pop_back();
       }
    }
    break;
    case asttags::END_VarLikeDecl:
    break;
    case asttags::Enum:
    break;
    case asttags::Catch:
    break;
    case asttags::Cobegin:
    break;
    case asttags::Conditional:
    break;
    case asttags::Implements:
    break;
    case asttags::Label: // contains a loop
    break;
    case asttags::Select:
    break;
    case asttags::Sync:
    break;
    case asttags::Try:
    break;
    case asttags::START_SimpleBlockLike:
    break;
    case asttags::Begin:
    break;
    case asttags::Block:
    {
       if(curStmts.back()->size())
       chplx::util::dout << "Block is inside on: " << isInsideOn << " cur scope kind: " << curStmts.back()->back().index() << std::endl;
       if (1 < curStmts.size() && std::holds_alternative<std::shared_ptr<ConditionalExpression>>( curStmts[curStmts.size()-2]->back() )) {
          curStmts.pop_back();

          std::vector<Statement> * cStmts = curStmts.back();
          std::shared_ptr<ConditionalExpression> & fde =
             std::get<std::shared_ptr<ConditionalExpression>>(cStmts->back());
       
          if(0 < symbolTable.lut[fde->scopeId]->parent.index() && std::holds_alternative<std::shared_ptr<SymbolTable::SymbolTableNode>>(symbolTable.lut[fde->scopeId]->parent)) {
             //symbolTableRef = std::get<std::shared_ptr<SymbolTable::SymbolTableNode>>(symbolTable.lut[fde->scopeId]->parent);
          }
          else {
             symbolTableRef = symbolTable.lut[0];
          }
       }
       else if (1 < curStmts.size() && std::holds_alternative<std::shared_ptr<ForLoopExpression>>( curStmts[curStmts.size()-2]->back() )) {
          curStmts.pop_back();

          std::vector<Statement> * cStmts = curStmts.back();
          std::shared_ptr<ForLoopExpression> & fde =
             std::get<std::shared_ptr<ForLoopExpression>>(cStmts->back());
       
          if(0 < symbolTable.lut[fde->scopeId]->parent.index() && std::holds_alternative<std::shared_ptr<SymbolTable::SymbolTableNode>>(symbolTable.lut[fde->scopeId]->parent)) {
             symbolTableRef = std::get<std::shared_ptr<SymbolTable::SymbolTableNode>>(symbolTable.lut[fde->scopeId]->parent);
          }
          else {
             symbolTableRef = symbolTable.lut[0];
          }

       }
       else if (1 < curStmts.size() && std::holds_alternative<std::shared_ptr<ForallLoopExpression>>( curStmts[curStmts.size()-2]->back() )) {
          curStmts.pop_back();

          std::vector<Statement> * cStmts = curStmts.back();
          std::shared_ptr<ForallLoopExpression> & fde =
             std::get<std::shared_ptr<ForallLoopExpression>>(cStmts->back());
       
          if(0 < symbolTable.lut[fde->scopeId]->parent.index() && std::holds_alternative<std::shared_ptr<SymbolTable::SymbolTableNode>>(symbolTable.lut[fde->scopeId]->parent)) {
             symbolTableRef = std::get<std::shared_ptr<SymbolTable::SymbolTableNode>>(symbolTable.lut[fde->scopeId]->parent);
          }
          else {
             symbolTableRef = symbolTable.lut[0];
          }
       }
       else if (1 < curStmts.size() && std::holds_alternative<std::shared_ptr<CoforallLoopExpression>>( curStmts[curStmts.size()-2]->back() )) {
          curStmts.pop_back();

          std::vector<Statement> * cStmts = curStmts.back();
          std::shared_ptr<CoforallLoopExpression> & fde =
             std::get<std::shared_ptr<CoforallLoopExpression>>(cStmts->back());
       
          if(0 < symbolTable.lut[fde->scopeId]->parent.index() && std::holds_alternative<std::shared_ptr<SymbolTable::SymbolTableNode>>(symbolTable.lut[fde->scopeId]->parent)) {
             symbolTableRef = std::get<std::shared_ptr<SymbolTable::SymbolTableNode>>(symbolTable.lut[fde->scopeId]->parent);
          }
          else {
             symbolTableRef = symbolTable.lut[0];
          }
       }
       else if (1 < curStmts.size() && std::holds_alternative<std::shared_ptr<OnExpression>>( curStmts[curStmts.size()-2]->back() )) {
         if(isInsideOn) break; 
         curStmts.pop_back();

          std::vector<Statement> * cStmts = curStmts.back();
          std::shared_ptr<OnExpression> & fde =
             std::get<std::shared_ptr<OnExpression>>(cStmts->back());
       
          if(0 < symbolTable.lut[fde->scopeId]->parent.index() && std::holds_alternative<std::shared_ptr<SymbolTable::SymbolTableNode>>(symbolTable.lut[fde->scopeId]->parent)) {
             symbolTableRef = std::get<std::shared_ptr<SymbolTable::SymbolTableNode>>(symbolTable.lut[fde->scopeId]->parent);
          }
          else {
             symbolTableRef = symbolTable.lut[0];
          }
       }
    }
    break;
    case asttags::Defer:
    break;
    case asttags::Local:
    break;
    case asttags::Manage:
    break;
    case asttags::On:
    {
      // leave the OnExpression’s statements
      if(curStmts.back() && curStmts.size())
      for(auto& stmt: *curStmts.back())
      chplx::util::dout << "before Cur stmts kind : " << stmt.index() << std::endl;
      if(curStmts.back() && curStmts.size()>1)
      curStmts.pop_back();
      if(curStmts.back() && curStmts.size())
      for(auto& stmt: *curStmts.back())
      chplx::util::dout << "after Cur stmts kind : " << stmt.index() << std::endl;
      assert(!onExprStack.empty());
      // restore symbolTableRef to the On’s parent scope
      auto onExpr =  onExprStack.back();
      onExprStack.pop_back();
      auto & parentVar = symbolTable.lut[onExpr->scopeId]->parent;
      if (auto pNode =
              std::get_if<std::shared_ptr<SymbolTable::SymbolTableNode>>(
                  &parentVar))
      {
         symbolTableRef = *pNode;
      }
      else
      {
         symbolTableRef = symbolTable.lut[0];
      }
      --isInsideOn;

      for (auto& var_ : onExpr->OnLocaleVarsUsedInExpr)
      {
         auto var = std::get<VariableExpression>(var_);
         chplx::util::dout << "Scope of var: " << var.sym->identifier << " is "
                   << var.sym->scopeId
                   << "  Current scope: " << currentOnExpr->scopeId
                   << "  symboltableref scope: "
                   << symbolTable.symbolTableRef->id
                   << " OnIndex scope: " << isInsideOn << std::endl;
      }

      if (!onExprStack.size())
      {
         currentOnExpr = nullptr;
         assert(isInsideOn == 0);
      }
      else
      {
         auto prev_on_locale_vars = currentOnExpr->OnLocaleVarsUsedInExpr;
         
         currentOnExpr = onExprStack.back();
         std::copy(prev_on_locale_vars.begin(), prev_on_locale_vars.end(),
             std::back_inserter(currentOnExpr->OnLocaleVarsUsedInExpr));
         chplx::util::dout
             << "*********************************************************\n";
         currentOnExpr->OnLocaleVarsUsedInExpr.erase(
             std::remove_if(currentOnExpr->OnLocaleVarsUsedInExpr.begin(),
                 currentOnExpr->OnLocaleVarsUsedInExpr.end(),
                 [&](auto& var_) {
                     auto var = std::get<VariableExpression>(var_);
                     return var.sym->scopeId >= currentOnExpr->scopeId;
                 }),
             currentOnExpr->OnLocaleVarsUsedInExpr.end());
         for (auto& var_ : currentOnExpr->OnLocaleVarsUsedInExpr)
         {
             auto var = std::get<VariableExpression>(var_);
             chplx::util::dout << "Scope of var: " << var.sym->identifier << " is "
                       << var.sym->scopeId
                       << "  Current scope: " << currentOnExpr->scopeId
                       << "  symboltableref scope: "
                       << symbolTable.symbolTableRef->id
                       << " OnIndex scope: " << isInsideOn << std::endl;
         }
         chplx::util::dout << "****************************************************"
                      "*****\n";
      }
    }
    break;
    case asttags::Serial:
    break;
    case asttags::When:
    break;
    case asttags::END_SimpleBlockLike:
    break;
    case asttags::START_Loop:
    break;
    case asttags::DoWhile:
    break;
    case asttags::While:
    break;
    case asttags::START_IndexableLoop:
    break;
    case asttags::BracketLoop:
    break;
    case asttags::Coforall:
    break;
    case asttags::Foreach:
    break;
    case asttags::END_IndexableLoop:
    break;
    case asttags::END_Loop:
    break;
    case asttags::START_Decl:
    break;
    case asttags::START_NamedDecl:
    break;
    case asttags::START_TypeDecl:
    break;
    case asttags::Function:
    {
       curStmts.pop_back();
       std::vector<Statement> * cStmts = curStmts.back();

       if(cStmts->size() == 0 || !std::holds_alternative<std::shared_ptr<FunctionDeclarationExpression>>(cStmts->back()) ) {
         if(cStmts && cStmts->size() > 0) 
         chplx::util::dout << "3555 Stmt kind: " << cStmts->back().index() << std::endl;
         break;
       }

       std::shared_ptr<FunctionDeclarationExpression> & fde =
           std::get<std::shared_ptr<FunctionDeclarationExpression>>(cStmts->back());
       
       if(0 < symbolTable.lut[fde->scopeId]->parent.index() && std::holds_alternative<std::shared_ptr<SymbolTable::SymbolTableNode>>(symbolTable.lut[fde->scopeId]->parent)) {
           symbolTableRef = std::get<std::shared_ptr<SymbolTable::SymbolTableNode>>(symbolTable.lut[fde->scopeId]->parent);
       }
       else {
           symbolTableRef = symbolTable.lut[0];
       }
    }
    break;
    case asttags::Interface:
    break;
    case asttags::Module:
    break;
    case asttags::START_AggregateDecl:
    break;
    case asttags::Class:
    break;
    case asttags::Record:
    break;
    case asttags::Union:
    break;
    case asttags::END_AggregateDecl:
    break;
    case asttags::END_Decl:
    break;
    case asttags::END_NamedDecl:
    break;
    case asttags::END_TypeDecl:
    break;
    case asttags::NUM_AST_TAGS:
    break;
    case asttags::AST_TAG_UNKNOWN:
    break;
    default:
    break;
   }
}

} /* namespace hpx */ } /* namespace visitors */ } /* namespace ast */ } /* namespace chpl */
