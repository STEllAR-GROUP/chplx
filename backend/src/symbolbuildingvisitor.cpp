/*
 * Copyright (c) 2023 Hartmut Kaiser
 * Copyright (c) 2023 Christopher Taylor
 *
 * SPDX-License-Identifier: BSL-1.0
 * Distributed under the Boost Software License, Version 1.0. *(See accompanying
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#include "hpx/symbolbuildingvisitor.hpp"
#include "hpx/util.hpp"
#include "chpl/uast/all-uast.h"

#include <variant>
#include <fstream>
#include <cctype>
#include <filesystem>
#include <numeric>
#include <cassert>

using namespace chpl::uast;

// global options
extern bool suppressLineDirectives;
extern bool fullFilePath;

namespace chpl { namespace ast { namespace visitors { namespace hpx {

void SymbolBuildingVisitor::addSymbolEntry(char const* type, Symbol && symbol) {
   symbolTable.addEntry(symbol.identifier, symbol);
}

SymbolBuildingVisitor::SymbolBuildingVisitor(chpl::uast::BuilderResult const& chapelBr, std::string const& cfps)
   : br(chapelBr), indent(0),
     chplFilePathStr(cfps),
     symstack(), sym(), symnode(), configVars(), symbolTable()
{
   // populate the symbol table with a base set of entries:
   //
   // bool, string, int, real, byte, complex, range, domain, template(?)
   //
   addSymbolEntry("nil", Symbol{{nil_kind{}, std::string{"nil"}, {}, -1, false, symbolTable.symbolTableRef->id}});
   addSymbolEntry("bool", Symbol{{bool_kind{}, std::string{"bool"}, {}, -1, false, symbolTable.symbolTableRef->id}});
   addSymbolEntry("string", Symbol{{string_kind{}, std::string{"string"}, {}, -1, false, symbolTable.symbolTableRef->id}});
   addSymbolEntry("int", Symbol{{int_kind{}, std::string{"int"}, {}, -1, false, symbolTable.symbolTableRef->id}});
   addSymbolEntry("real", Symbol{{real_kind{}, std::string{"real"}, {}, -1, false, symbolTable.symbolTableRef->id}});
   addSymbolEntry("byte", Symbol{{byte_kind{}, std::string{"byte"}, {}, -1, false, symbolTable.symbolTableRef->id}});
   addSymbolEntry("complex", Symbol{{complex_kind{}, std::string{"complex"}, {}, -1, false, symbolTable.symbolTableRef->id}});
   addSymbolEntry("range", Symbol{{range_kind{{}}, std::string{"range"}, {}, -1, false, symbolTable.symbolTableRef->id}});
   addSymbolEntry("domain", Symbol{{domain_kind{}, std::string{"domain"}, {}, -1, false, symbolTable.symbolTableRef->id}});
   addSymbolEntry("?", Symbol{{template_kind{}, std::string{"?"}, {}, -1, false, symbolTable.symbolTableRef->id}});

   // inlinecxx - allows users to inline c++ code into their chapel programs
   //
   addSymbolEntry("inlinecxx",
      Symbol{{std::make_shared<cxxfunc_kind>(cxxfunc_kind{{{}, "inlinecxx", {}, nil_kind{}}}), std::string{"inlinecxx"}, {}, -1, false, symbolTable.symbolTableRef->id}}
   );

   // +,-,*,/,%,=
   //
   addSymbolEntry("+",
      Symbol{{std::make_shared<func_kind>(func_kind{{{}, "+", {}, {}}}), std::string{"+"}, {}, -1, false, symbolTable.symbolTableRef->id}}
   );
   addSymbolEntry("-",
      Symbol{{std::make_shared<func_kind>(func_kind{{{}, "-", {}, {}}}), std::string{"-"}, {}, -1, false, symbolTable.symbolTableRef->id}}
   );
   addSymbolEntry("*",
      Symbol{{std::make_shared<func_kind>(func_kind{{{}, "*", {}, {}}}), std::string{"*"}, {}, -1, false, symbolTable.symbolTableRef->id}}
   );
   addSymbolEntry("/",
      Symbol{{std::make_shared<func_kind>(func_kind{{{}, "/", {}, {}}}), std::string{"/"}, {}, -1, false, symbolTable.symbolTableRef->id}}
   );
   addSymbolEntry("%",
      Symbol{{std::make_shared<func_kind>(func_kind{{{}, "%", {}, {}}}), std::string{"%"}, {}, -1, false, symbolTable.symbolTableRef->id}}
   );
   addSymbolEntry("=",
      Symbol{{std::make_shared<func_kind>(func_kind{{{}, "=", {}, {}}}), std::string{"="}, {}, -1, false, symbolTable.symbolTableRef->id}}
   );
   addSymbolEntry("[]",
      Symbol{{std::make_shared<func_kind>(func_kind{{{}, "[]", {}, {}}}), std::string{"[]"}, {}, -1, false, symbolTable.symbolTableRef->id}}
   );
   addSymbolEntry("==",
      Symbol{{std::make_shared<func_kind>(func_kind{{{}, "==", {}, {}}}), std::string{"=="}, {}, -1, false, symbolTable.symbolTableRef->id}}
   );
   addSymbolEntry("<=>",
      Symbol{{std::make_shared<func_kind>(func_kind{{{}, "<=>", {}, {}}}), std::string{"<=>"}, {}, -1, false, symbolTable.symbolTableRef->id}}
   );
}

std::string SymbolBuildingVisitor::emitChapelLine(uast::AstNode const* ast) {
   auto const fp = br.filePath();
   return chplx::util::emitLineDirective(fp.c_str(), br.idToLocation(ast->id(), fp).line());
}

bool SymbolBuildingVisitor::enter(const uast::AstNode * ast) {
   if(chplx::util::compilerDebug) {
      std::cout << "***Enter AST Node\t" << tagToString(ast->tag()) << std::endl
                << "***\tCurrent Scope\t" << symbolTable.symbolTableRef->id << std::endl
                << "***\t" << emitChapelLine(ast);
   }
   switch(ast->tag()) {
    case asttags::AnonFormal:
    break;
    case asttags::As:
    break;
    case asttags::Array:
    {
        if(sym.has_value()) {
           if(sym->get().kind.index() == 0) {
              sym->get().kind = std::make_shared<array_kind>();
              std::get<std::shared_ptr<array_kind>>(sym->get().kind)->kind =
                 std::make_shared<kind_node_type>(kind_node_type{});
              std::get<std::shared_ptr<array_kind>>(sym->get().kind)->dom =
                 domain_kind{{}};
           }
           else {
              std::get<std::shared_ptr<kind_node_type>>(
                 std::get<std::shared_ptr<array_kind>>(sym->get().kind)->kind
              )->children.emplace_back(
                 std::make_shared<kind_node_type>(kind_node_type{})
              );
           }
        }
    }
    break;
    case asttags::Attributes:
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
    {
        if(sym.has_value()) {
            if(std::holds_alternative<std::monostate>(sym->get().kind)) {
                if(std::holds_alternative<std::shared_ptr<array_kind>>(sym->get().kind)) {
                    std::get<std::shared_ptr<array_kind>>(sym->get().kind)->dom = domain_kind{{}};
                }
            }
        }
    }
    break;
    case asttags::Dot:
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
       std::string identifier_str{dynamic_cast<Identifier const*>(ast)->name().c_str()};

       if(sym && !std::holds_alternative<std::monostate>(sym->get().kind)) {
          const std::size_t lutId =
             std::holds_alternative<std::shared_ptr<func_kind>>(sym->get().kind) ?
                std::get<std::shared_ptr<func_kind>>(sym->get().kind)->lutId :
                0;
          if(std::holds_alternative<std::shared_ptr<array_kind>>(sym->get().kind)) {
             auto fsym = symbolTable.find(lutId, identifier_str);
             if(fsym) {
                std::get<std::shared_ptr<array_kind>>(sym->get().kind)->kind = fsym->kind;
                sym->get().scopeId = symbolTable.symbolTableRef->id;
             }
          }
          else if(std::holds_alternative<std::shared_ptr<func_kind>>(sym->get().kind) && 
                  !std::holds_alternative<std::shared_ptr<cxxfunc_kind>>(sym->get().kind)) {

             std::shared_ptr<func_kind> & fk =
                std::get<std::shared_ptr<func_kind>>(sym->get().kind);

             std::optional<Symbol> fsym;
             symbolTable.find(sym->get().scopeId, identifier_str, fsym);
             //auto fsym = symbolTable.find(sym->get().scopeId, identifier_str);

             if(fsym.has_value()) {
                if(std::holds_alternative<std::shared_ptr<cxxfunc_kind>>(fsym->kind)) {
                      return true;
                }
                else if(0 < fk->args.size()) {
                   if(std::holds_alternative<nil_kind>(fk->args.back().kind)) {
                      return true;
                   }
                   else if(fk->args.back().kind.index() < 1) {
                     fk->args.back().kind = fsym->kind;
                   }
                   else if( !std::holds_alternative<std::shared_ptr<func_kind>>(fsym->kind) && 
                            !std::holds_alternative<std::shared_ptr<cxxfunc_kind>>(fsym->kind) ) {
                      fk->retKind = fsym->kind;
                   }
                }
                else if(0 == fk->args.size()) {
                   fk->retKind = fsym->kind;
                }
             }
          }
          else {
             auto fsym = symbolTable.find(lutId, identifier_str);
             if(fsym.has_value()) {
                sym->get().kind = fsym->kind;
                sym->get().scopeId = symbolTable.symbolTableRef->id;
             }
          }
       }
       else if (sym) {
          std::optional<Symbol> fsym{};
          symbolTable.find(identifier_str, fsym);
          if(fsym.has_value()) {
             sym->get().kind = fsym->kind;
             sym->get().scopeId = symbolTable.symbolTableRef->id;
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
       if(sym && !std::holds_alternative<std::monostate>(sym->get().kind)) {
          if(std::holds_alternative<std::shared_ptr<array_kind>>(sym->get().kind)) {
             std::get<std::shared_ptr<array_kind>>(sym->get().kind)->dom.ranges.push_back(range_kind{});
          }
          else if(std::holds_alternative<std::shared_ptr<func_kind>>(sym->get().kind) && (sym->get().identifier.find("for") != std::string::npos)) {
             std::shared_ptr<func_kind> & fk =
                std::get<std::shared_ptr<func_kind>>(sym->get().kind);

             std::string ident{std::string{"range_" + emitChapelLine(ast)}};
             fk->args.emplace_back(
                Symbol{{
                   range_kind{{}},
                   ident,
                   {}, -1, false, symbolTable.symbolTableRef->id
                }});

             symbolTable.addEntry(fk->lutId, ident, fk->args.back());

             sym.reset();
             sym = symstack.back();
             symnode = const_cast<uast::AstNode*>(ast);
          }
       }
       else {
          symstack.emplace_back(Symbol{{
            range_kind{},
            std::string{"range_" + emitChapelLine(ast)},
            {}, -1, false, symbolTable.symbolTableRef->id
          }});

          sym.reset();
          sym = symstack.back();
          symnode = const_cast<uast::AstNode*>(ast);
       }
    }
    break;
    case asttags::Require:
    break;
    case asttags::Return:
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
        if(!(sym && !std::holds_alternative<std::monostate>(sym->get().kind) && 0 < sym->get().kind.index())) {
           if(std::holds_alternative<std::shared_ptr<func_kind>>(sym->get().kind)) {
              std::shared_ptr<func_kind> & fk =
                 std::get<std::shared_ptr<func_kind>>(sym->get().kind);
              if(0 < fk->args.size() && std::holds_alternative<nil_kind>(fk->args.back().kind)) {
                 return true;
              }
           }

           auto fsym = symbolTable.find("bool");
           if(fsym.has_value()) {
              sym->get().kind = (fsym->kind);
              if(sym->get().literal.size()) {
                 sym->get().literal.push_back(ast);
              }
              else {
                 sym->get().literal = {ast,};
              }
           }
        }
    }
    break;
    case asttags::ImagLiteral:
    {
        if(!(sym && 0 < sym->get().kind.index())) {
           if(std::holds_alternative<std::shared_ptr<func_kind>>(sym->get().kind)) {
              std::shared_ptr<func_kind> & fk =
                 std::get<std::shared_ptr<func_kind>>(sym->get().kind);
              if(0 < fk->args.size() && std::holds_alternative<nil_kind>(fk->args.back().kind)) {
                 return true;
              }
           }

           auto fsym = symbolTable.find("complex");
           if(fsym.has_value()) {
              sym->get().kind = (fsym->kind);
              if(sym->get().literal.size()) {
                 sym->get().literal.push_back(ast);
              }
              else {
                 sym->get().literal = {ast,};
              }
           }
        }
    }
    break;
    case asttags::IntLiteral:
    {
        if(sym && !std::holds_alternative<std::monostate>(sym->get().kind)) {
           if(std::holds_alternative<std::shared_ptr<func_kind>>(sym->get().kind)) {
              std::shared_ptr<func_kind> & fk =
                 std::get<std::shared_ptr<func_kind>>(sym->get().kind);
              if(0 < fk->args.size() && std::holds_alternative<range_kind>(fk->args.back().kind)) {
                 range_kind & symref =
                    std::get<range_kind>(fk->args.back().kind);
                 symref.points.emplace_back( int_kind::value(ast) );
              }
              else if(0 < fk->args.size() && std::holds_alternative<nil_kind>(fk->args.back().kind)) {
                 return true;
              }
           }
           else if(std::holds_alternative<std::shared_ptr<array_kind>>(sym->get().kind)) {
              std::shared_ptr<array_kind> & symref =
                 std::get<std::shared_ptr<array_kind>>(sym->get().kind);

              if(!sym->get().literal.size() && symref->dom.ranges.size()) {
                 symref->dom.ranges.back().points.push_back( int_kind::value(ast) );
              }
              else if(!sym->get().literal.size()) {
                 sym->get().literal.push_back(ast);
                 std::get<std::shared_ptr<kind_node_type>>(symref->kind)->children.emplace_back(
                    int_kind{}
                 );
              }
              else {
                 sym->get().literal.push_back(ast);
                 if(std::holds_alternative<std::monostate>(symref->kind)) {
                     symref->kind = std::make_shared<kind_node_type>(kind_node_type{{}});
                 }
                 std::get<std::shared_ptr<kind_node_type>>(symref->kind)->children.emplace_back(
                    int_kind{}
                 );
              }
           }
           else if(std::holds_alternative<range_kind>(sym->get().kind)) {
              range_kind & symref =
                 std::get<range_kind>(sym->get().kind);
              symref.points.push_back( int_kind::value(ast) );
           }
        }
        else if (sym) {
           auto fsym = symbolTable.find(0, "int");

           if(fsym.has_value()) {
              sym->get().kind = (fsym->kind);
              if(sym->get().literal.size()) {
                 sym->get().literal.push_back(ast);
              }
              else {
                 sym->get().literal = {ast,};
              }
           }
        }
    }
    break;
    case asttags::RealLiteral:
    {
        if(!(sym && 0 < sym->get().kind.index())) {
           if(std::holds_alternative<std::shared_ptr<func_kind>>(sym->get().kind)) {
              std::shared_ptr<func_kind> & fk =
                 std::get<std::shared_ptr<func_kind>>(sym->get().kind);
              if(std::holds_alternative<nil_kind>(fk->args.back().kind)) {
                 return true;
              }
           }

           if(std::holds_alternative<std::shared_ptr<func_kind>>(sym->get().kind)) {
              std::shared_ptr<func_kind> & fk =
                 std::get<std::shared_ptr<func_kind>>(sym->get().kind);
              if(0 < fk->args.size() && std::holds_alternative<nil_kind>(fk->args.back().kind)) {
                 return true;
              }
           }

           auto fsym = symbolTable.find("real");

           if(fsym.has_value()) {
              sym->get().kind = (fsym->kind);
              if(sym->get().literal.size()) {
                 sym->get().literal.push_back(ast);
              }
              else {
                 sym->get().literal = {ast,};
              }
           }
        }
    }
    break;
    case asttags::UintLiteral:
    {
        if(!(sym && 0 < sym->get().kind.index())) {
           if(std::holds_alternative<std::shared_ptr<func_kind>>(sym->get().kind)) {
              std::shared_ptr<func_kind> & fk =
                 std::get<std::shared_ptr<func_kind>>(sym->get().kind);
              if(0 < fk->args.size() && std::holds_alternative<nil_kind>(fk->args.back().kind)) {
                 return true;
              }
           }

           auto fsym = symbolTable.find("int");
           if(fsym.has_value()) {
              sym->get().kind = (fsym->kind);
              if(sym->get().literal.size()) {
                 sym->get().literal.push_back(ast);
              }
              else {
                 sym->get().literal = {ast,};
              }
           }
        }
    }
    break;
    case asttags::START_StringLikeLiteral:
    break;
    case asttags::BytesLiteral:
    {
        if(!(sym && 0 < sym->get().kind.index())) {
           if(std::holds_alternative<std::shared_ptr<func_kind>>(sym->get().kind)) {
              std::shared_ptr<func_kind> & fk =
                 std::get<std::shared_ptr<func_kind>>(sym->get().kind);
              if(0 < fk->args.size() && std::holds_alternative<nil_kind>(fk->args.back().kind)) {
                 return true;
              }
           }

           auto fsym = symbolTable.find("byte");
           if(fsym.has_value()) {
             sym->get().kind = fsym->kind;
             sym->get().literal.push_back(ast);
           }
        }
    }
    break;
    case asttags::CStringLiteral:
    {
        if(!(sym && 0 < sym->get().kind.index())) {
           if(std::holds_alternative<std::shared_ptr<func_kind>>(sym->get().kind)) {
              std::shared_ptr<func_kind> & fk =
                 std::get<std::shared_ptr<func_kind>>(sym->get().kind);
              if(0 < fk->args.size() && std::holds_alternative<nil_kind>(fk->args.back().kind)) {
                 return true;
              }
           }

           auto fsym = symbolTable.find(0,"string");
           if(fsym.has_value()) {
             sym->get().kind = fsym->kind;
             sym->get().literal.push_back(ast);
           }
        }
    }
    break;
    case asttags::StringLiteral:
    {
        if(!(sym && 0 < sym->get().kind.index())) {
           if(sym && std::holds_alternative<std::shared_ptr<func_kind>>(sym->get().kind)) {
              std::shared_ptr<func_kind> & fk =
                 std::get<std::shared_ptr<func_kind>>(sym->get().kind);
              if(0 < fk->args.size() && std::holds_alternative<nil_kind>(fk->args.back().kind)) {
                 return true;
              }
           }

           std::optional<Symbol> fsym{};
           symbolTable.find(0, "string", fsym);

           if(fsym.has_value()) {
             sym->get().kind = fsym->kind;
             sym->get().literal.push_back(ast);
           }
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
        if(sym.has_value() && std::holds_alternative<std::monostate>(sym->get().kind)) {
           const FnCall* fc = dynamic_cast<const FnCall*>(ast);
           std::string identifier{dynamic_cast<const Identifier*>(fc->calledExpression())->name().c_str()};

           auto rsym =
              symbolTable.findPrefix(symbolTable.symbolTableRef->id, identifier);
           auto itr = rsym->first;
           for(; itr != rsym->second; ++itr) {

              auto pipeloc = itr->first.find('|');
              std::string itrstr {(pipeloc == itr->first.npos) ? itr->first : itr->first.substr(0,pipeloc)};

              // assign variable the return value of the function...
              //
              if(itrstr.size() >= identifier.size() &&
                 itrstr == identifier &&
                 0 < itr->second.kind.index() && 
                 std::holds_alternative<std::shared_ptr<func_kind>>(itr->second.kind)
              ) {
                 sym->get().kind = std::get<std::shared_ptr<func_kind>>(itr->second.kind)->retKind;
              }
           }

           if(std::holds_alternative<nil_kind>(sym->get().kind)) {
              sym->get().kind = expr_kind{};
           }
        }
    }
    break;
    case asttags::OpCall:
    {
        if(sym.has_value() && std::holds_alternative<std::monostate>(sym->get().kind)) {
            sym->get().kind = expr_kind{};
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
    break;
    case asttags::Zip:
    break;
    case asttags::END_Call:
    break;
    case asttags::MultiDecl:
    break;
    case asttags::TupleDecl:
    break;
    case asttags::ForwardingDecl:
    break;
    case asttags::EnumElement:
    break;
    case asttags::START_VarLikeDecl:
    break;
    case asttags::Formal:
    {
       if(sym.has_value() && 0 < sym->get().kind.index() &&
          std::holds_alternative<std::shared_ptr<func_kind>>(sym->get().kind)) {

          std::shared_ptr<func_kind> & fk =
             std::get<std::shared_ptr<func_kind>>(sym->get().kind);

          int kindqual = -1;

          Formal::Intent k = dynamic_cast<Formal const*>(ast)->intent();
          if(k == Formal::Intent::DEFAULT_INTENT) {
              kindqual = 0;
          }
          else if(k == Formal::Intent::CONST) {
              kindqual = 1;
          }
          else if(k == Formal::Intent::CONST_REF) {
              kindqual = 2;
          }
          else if(k == Formal::Intent::REF) {
              kindqual = 3;
          }
          else if(k == Formal::Intent::PARAM) {
              kindqual = 4;
          }
          else if(k == Formal::Intent::TYPE) {
              kindqual = 5;
          }
          else if(k == Formal::Intent::IN) {
              kindqual = 6;
          }
          else if(k == Formal::Intent::CONST_IN) {
              kindqual = 7;
          }
          else if(k == Formal::Intent::OUT) {
              kindqual = 8;
          }
          else if(k == Formal::Intent::INOUT) {
              kindqual = 9;
          }

          std::string ident{dynamic_cast<Formal const*>(ast)->name().c_str()};
          fk->args.emplace_back(
             Symbol{{
                kind_types{},
                ident,
                {}, kindqual, false, symbolTable.symbolTableRef->id
             }}
          );

          symbolTable.addEntry(fk->lutId, ident, fk->args.back());
       }
    }
    break;
    case asttags::TaskVar:
    break;
    case asttags::VarArgFormal:
    break;
    case asttags::Variable:
    {
       const Variable::Kind k = dynamic_cast<Variable const*>(ast)->kind();
       int kindqual = -1;

       if(k == Variable::Kind::VAR) {
           kindqual = 0;
       }
       else if(k == Variable::Kind::CONST) {
           kindqual = 1;
       }
       else if(k == Variable::Kind::CONST_REF) {
           kindqual = 2;
       }
       else if(k == Variable::Kind::REF) {
           kindqual = 3;
       }
       else if(k == Variable::Kind::PARAM) {
           kindqual = 4;
       }
       else if(k == Variable::Kind::TYPE) {
           kindqual = 5;
       }
       else if(k == Variable::Kind::INDEX) {
           kindqual = 10;
       }
  
       const bool cfg = dynamic_cast<Variable const*>(ast)->isConfig();

       symstack.emplace_back(Symbol{{
          {},
          std::string{dynamic_cast<NamedDecl const*>(ast)->name().c_str()},
          {}, kindqual, cfg, symbolTable.symbolTableRef->id
       }});

       sym.reset();
       sym = symstack.back();
       symnode = const_cast<uast::AstNode*>(ast);
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
        if(!(sym && !std::holds_alternative<std::shared_ptr<module_kind>>(sym->get().kind))) {
          // symbol.scopePtr = the scope where the function is defined (equivalent to a lutId)
          //
          symstack.emplace_back(
             Symbol{{
                std::make_shared<func_kind>(func_kind{{
                   symbolTable.symbolTableRef->id, {}, {}, {}}}),
                std::string{"if" + emitChapelLine(ast)},
                {}, -1, false, symbolTable.symbolTableRef->id
             }});

          std::shared_ptr<SymbolTable::SymbolTableNode> prevSymbolTableRef = symbolTable.symbolTableRef;
          const std::size_t parScope = symbolTable.symbolTableRef->id;
          symbolTable.pushScope();
          sym.reset();
          sym = symstack.back();

          std::shared_ptr<func_kind> & fk = 
             std::get<std::shared_ptr<func_kind>>(sym->get().kind);

          fk->symbolTableSignature = (sym->get().identifier);
          // func_kind.lutId = the scope where the function's symboltable references
          //
          fk->lutId = symbolTable.symbolTableRef->id;

          symbolTable.parentSymbolTableId = parScope;
          symbolTable.symbolTableRef->parent = prevSymbolTableRef;

          symnode = const_cast<uast::AstNode*>(ast);
       }
       // inside a for loop or a function
       //
       else {
          // nested conditionals have an additional block node before the conditional node in the ast
          // this logic detects the additional block and removes it from the symbol table
          //
          if( sym->get().identifier == symstack.back().identifier &&
              sym->get().identifier.find("else") != std::string::npos ) {
             symstack.pop_back();
             symbolTable.lut.pop_back();
             symbolTable.popScope();
          }

          std::shared_ptr<SymbolTable::SymbolTableNode> prevSymbolTableRef = symbolTable.symbolTableRef;
          const std::size_t parScope = symbolTable.symbolTableRef->id;

          symstack.emplace_back(
             Symbol{{
                std::make_shared<func_kind>(func_kind{{
                   symbolTable.symbolTableRef->id, {}, {}, {}}}),
                std::string{"if" + emitChapelLine(ast)},
                {}, -1, false, symbolTable.symbolTableRef->id
             }});

          symbolTable.pushScope();
          sym.reset();
          sym = symstack.back();

          std::shared_ptr<func_kind> & fk = 
             std::get<std::shared_ptr<func_kind>>(sym->get().kind);

          fk->symbolTableSignature = (sym->get().identifier);
          // func_kind.lutId = the scope where the function's symboltable references
          //
          fk->lutId = symbolTable.symbolTableRef->id;

          symbolTable.parentSymbolTableId = parScope;
          symbolTable.symbolTableRef->parent = prevSymbolTableRef;

          symnode = const_cast<uast::AstNode*>(ast);
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
       if(sym && !std::holds_alternative<std::shared_ptr<module_kind>>(sym->get().kind)) { //0 < sym->get().kind.index()) {
         if(std::holds_alternative<std::shared_ptr<func_kind>>(sym->get().kind)) {
             std::shared_ptr<func_kind> & fk =
                std::get<std::shared_ptr<func_kind>>(sym->get().kind);
             if(0 < fk->args.size() && (fk->args.back().identifier == "nil")) {
                // this 'function' is really a stand-in scope for the following
                // expressions: for, forall, conditional (if/else) clause
                //
                return true;
             }
             else {
                // the block node of the chapel ast indicates the end of a function
                // declaration's argument list; the following empty argument is
                // state information about the ast traversal
                // 
                fk->args.emplace_back(Symbol{{nil_kind{}, std::string{"nil"}, {}, -1, false, symbolTable.symbolTableRef->id}});
             }
          }
       }
       else {
          symstack.emplace_back(
             Symbol{{
                std::make_shared<func_kind>(func_kind{{
                   symbolTable.symbolTableRef->id, {}, {}, {}}}),
                std::string{"else" + emitChapelLine(ast)},
                {}, -1, false, symbolTable.symbolTableRef->id
             }});

          std::shared_ptr<SymbolTable::SymbolTableNode> prevSymbolTableRef = symbolTable.symbolTableRef;
          const std::size_t parScope = symbolTable.symbolTableRef->id;
          symbolTable.pushScope();
          sym.reset();
          sym = symstack.back();

          std::shared_ptr<func_kind> & fk = 
             std::get<std::shared_ptr<func_kind>>(sym->get().kind);

          fk->symbolTableSignature = sym->get().identifier;
          // func_kind.lutId = the scope where the function's symboltable references
          //
          fk->lutId = symbolTable.symbolTableRef->id;

          symbolTable.parentSymbolTableId = parScope;
          symbolTable.symbolTableRef->parent = prevSymbolTableRef;
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
    {
        if(sym.has_value()) {
            sym->get().kind = std::make_shared<array_kind>();
        }
    }
    break;
    case asttags::Coforall:
    {
       if(!(sym && !std::holds_alternative<std::shared_ptr<module_kind>>(sym->get().kind))) { //)0 < sym->get().kind.index())) {
          // symbol.scopePtr = the scope where the function is defined (equivalent to a lutId)
          //
          symstack.emplace_back(
             Symbol{{
                std::make_shared<func_kind>(func_kind{{
                   symbolTable.symbolTableRef->id, {}, {}, {}}}),
                std::string{"coforall" + emitChapelLine(ast)},
                {}, -1, false, symbolTable.symbolTableRef->id
             }});

          std::shared_ptr<SymbolTable::SymbolTableNode> prevSymbolTableRef = symbolTable.symbolTableRef;
          const std::size_t parScope = symbolTable.symbolTableRef->id;
          symbolTable.pushScope();
          sym.reset();
          sym = symstack.back();

          std::shared_ptr<func_kind> & fk = 
             std::get<std::shared_ptr<func_kind>>(sym->get().kind);

          fk->symbolTableSignature = sym->get().identifier;
          // func_kind.lutId = the scope where the function's symboltable references
          //
          fk->lutId = symbolTable.symbolTableRef->id;

          symbolTable.parentSymbolTableId = parScope;
          symbolTable.symbolTableRef->parent = prevSymbolTableRef;

          symnode = const_cast<uast::AstNode*>(ast);
       }
       else {
          symstack.emplace_back(
             Symbol{{
                std::make_shared<func_kind>(func_kind{{
                   symbolTable.symbolTableRef->id, {}, {}, {}}}),
                std::string{"coforall" + emitChapelLine(ast)},
                {}, -1, false, symbolTable.symbolTableRef->id
             }});

          std::shared_ptr<SymbolTable::SymbolTableNode> prevSymbolTableRef = symbolTable.symbolTableRef;
          const std::size_t parScope = symbolTable.symbolTableRef->id;
          symbolTable.pushScope();

          sym.reset();
          sym = symstack.back();

          std::shared_ptr<func_kind> & fk = 
             std::get<std::shared_ptr<func_kind>>(sym->get().kind);

          fk->symbolTableSignature = sym->get().identifier;
          // func_kind.lutId = the scope where the function's symboltable references
          //
          fk->lutId = symbolTable.symbolTableRef->id;

          symbolTable.parentSymbolTableId = parScope;
          symbolTable.symbolTableRef->parent = prevSymbolTableRef;
          symnode = const_cast<uast::AstNode*>(ast);
       }
    }
    break;
    case asttags::For:
    {
        // symbol.scopePtr = the scope where the function is defined (equivalent to a lutId)
        //
        symstack.emplace_back(
           Symbol{{
              std::make_shared<func_kind>(func_kind{{
                 symbolTable.symbolTableRef->id, {}, {}, {}}}),
              std::string{"for" + emitChapelLine(ast)},
              {}, -1, false, symbolTable.symbolTableRef->id
        }});

        std::shared_ptr<SymbolTable::SymbolTableNode> prevSymbolTableRef = symbolTable.symbolTableRef;
        const std::size_t parScope = symbolTable.symbolTableRef->id;

        symbolTable.pushScope();
        sym.reset();
        sym = symstack.back();

        std::shared_ptr<func_kind> & fk = 
           std::get<std::shared_ptr<func_kind>>(sym->get().kind);

        fk->symbolTableSignature = sym->get().identifier;
        // func_kind.lutId = the scope where the function's symboltable references
        //
        fk->lutId = symbolTable.symbolTableRef->id;

        symbolTable.parentSymbolTableId = parScope;
        symbolTable.symbolTableRef->parent = prevSymbolTableRef;

        symnode = const_cast<uast::AstNode*>(ast);
    }
    break;
    case asttags::Forall:
    {
       // symbol.scopePtr = the scope where the function is defined (equivalent to a lutId)
       //
       symstack.emplace_back(
          Symbol{{
             std::make_shared<func_kind>(func_kind{{
                symbolTable.symbolTableRef->id, {}, {}, {}}}),
             std::string{"forall" + emitChapelLine(ast)},
             {}, -1, false, symbolTable.symbolTableRef->id
          }});

       std::shared_ptr<SymbolTable::SymbolTableNode> prevSymbolTableRef = symbolTable.symbolTableRef;
       const std::size_t parScope = symbolTable.symbolTableRef->id;

       symbolTable.pushScope();
       sym.reset();
       sym = symstack.back();

       std::shared_ptr<func_kind> & fk = 
          std::get<std::shared_ptr<func_kind>>(sym->get().kind);

       fk->symbolTableSignature = sym->get().identifier;
       // func_kind.lutId = the scope where the function's symboltable references
       //
       fk->lutId = symbolTable.symbolTableRef->id;

       symbolTable.parentSymbolTableId = parScope;
       symbolTable.symbolTableRef->parent = prevSymbolTableRef;

       symnode = const_cast<uast::AstNode*>(ast);
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

       //if(!(sym && std::holds_alternative<std::shared_ptr<module_kind>>(sym->get().kind))) { // 0 < sym->get().kind.index())) {
       {
          ProgramTreeFunctionVisitor v{false};
          ast->traverse(v);

          // symbol.scopePtr = the scope where the function is defined (equivalent to a lutId)
          //
          symstack.emplace_back(
             Symbol{{
                std::make_shared<func_kind>(func_kind{{
                   symbolTable.symbolTableRef->id, v.lookup, {}, {}}}),
                v.lookup,
                {}, -1, false, symbolTable.symbolTableRef->id
             }});

          std::shared_ptr<SymbolTable::SymbolTableNode> prevSymbolTableRef = symbolTable.symbolTableRef;
          const std::size_t parScope = symbolTable.symbolTableRef->id;
          symbolTable.pushScope();
          sym.reset();
          sym = symstack.back();

          std::shared_ptr<func_kind> & fk = 
             std::get<std::shared_ptr<func_kind>>(sym->get().kind);

          //fk->symbolTableSignature = std::string{v.lookup};
          // func_kind.lutId = the scope where the function's symboltable references
          //

          fk->lutId = symbolTable.symbolTableRef->id;

          symbolTable.parentSymbolTableId = parScope;
          symbolTable.symbolTableRef->parent = prevSymbolTableRef;

          symnode = const_cast<uast::AstNode*>(ast);
       }
    }
    break;
    case asttags::Interface:
    break;
    case asttags::Module:
    {
       std::string lookup = static_cast<Module const*>(ast)->name().str();
       symbolTable.parentModuleId = symbolTable.modules.size();
       // symbol.scopePtr = the scope where the function is defined (equivalent to a lutId)
       //
       symstack.emplace_back(
          Symbol{{
             std::make_shared<module_kind>(module_kind{{
                symbolTable.symbolTableRef->id, lookup, {}, {}}}),
             lookup,
          {}, -1, false, symbolTable.symbolTableRef->id
       }});

       std::shared_ptr<SymbolTable::SymbolTableNode> prevSymbolTableRef = symbolTable.symbolTableRef;
       const std::size_t parScope = symbolTable.symbolTableRef->id;
       symbolTable.pushScope();
       sym.reset();
       sym = symstack.back();

       std::shared_ptr<module_kind> & mk = 
          std::get<std::shared_ptr<module_kind>>(sym->get().kind);

       //fk->symbolTableSignature = std::string{v.lookup};
       // func_kind.lutId = the scope where the function's symboltable references
       //
       mk->lutId = symbolTable.symbolTableRef->id;

       symbolTable.parentSymbolTableId = parScope;
       symbolTable.symbolTableRef->parent = prevSymbolTableRef;

       symnode = const_cast<uast::AstNode*>(ast);
    }
    break;
    case asttags::START_AggregateDecl:
    break;
    case asttags::Record:
    {
       std::string lookup = static_cast<Record const*>(ast)->name().str();

       // symbol.scopePtr = the scope where the function is defined (equivalent to a lutId)
       //
       symstack.emplace_back(
          Symbol{{
             std::make_shared<record_kind>(record_kind{
                symbolTable.symbolTableRef->id, lookup, {}}),
             lookup,
             {}, -1, false, symbolTable.symbolTableRef->id
       }});

       std::shared_ptr<SymbolTable::SymbolTableNode> prevSymbolTableRef = symbolTable.symbolTableRef;
       const std::size_t parScope = symbolTable.symbolTableRef->id;
       symbolTable.pushScope();
       sym.reset();
       sym = symstack.back();

       std::shared_ptr<record_kind> & rk =
          std::get<std::shared_ptr<record_kind>>(sym->get().kind);

       //fk->symbolTableSignature = std::string{lookup};
       // func_kind.lutId = the scope where the function's symboltable references
       //

       rk->lutId = symbolTable.symbolTableRef->id;

       symbolTable.parentSymbolTableId = parScope;
       symbolTable.symbolTableRef->parent = prevSymbolTableRef;

       symnode = const_cast<uast::AstNode*>(ast);
    }
    break;
    case asttags::Class:
    {
       std::string lookup = static_cast<Class const*>(ast)->name().str();

       // symbol.scopePtr = the scope where the function is defined (equivalent to a lutId)
       //
       symstack.emplace_back(
          Symbol{{
             std::make_shared<class_kind>(class_kind{{
                symbolTable.symbolTableRef->id, lookup, {}}, {}}),
             lookup,
             {}, -1, false, symbolTable.symbolTableRef->id
       }});

       std::shared_ptr<SymbolTable::SymbolTableNode> prevSymbolTableRef = symbolTable.symbolTableRef;
       const std::size_t parScope = symbolTable.symbolTableRef->id;
       symbolTable.pushScope();
       sym.reset();
       sym = symstack.back();

       std::shared_ptr<class_kind> & ck =
          std::get<std::shared_ptr<class_kind>>(sym->get().kind);

       //fk->symbolTableSignature = std::string{lookup};
       // func_kind.lutId = the scope where the function's symboltable references
       //

       ck->lutId = symbolTable.symbolTableRef->id;

       symbolTable.parentSymbolTableId = parScope;
       symbolTable.symbolTableRef->parent = prevSymbolTableRef;

       symnode = const_cast<uast::AstNode*>(ast);
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

void SymbolBuildingVisitor::exit(const uast::AstNode * ast) {
   if(chplx::util::compilerDebug) {
      std::cout << "---Exit AST Node\t" << tagToString(ast->tag()) << std::endl
                << "---\tCurrent Scope\t" << symbolTable.symbolTableRef->id << std::endl
                << "---\t" << emitChapelLine(ast);
   }
   switch(ast->tag()) {
    case asttags::AnonFormal:
    break;
    case asttags::As:
    break;
    case asttags::Array:
    {
        if(sym.has_value()) {
           std::get<std::shared_ptr<kind_node_type>>(
              std::get<std::shared_ptr<array_kind>>(sym->get().kind)->kind
           )->children.emplace_back(
              kind_node_term_type{}
           );
        }
    }
    break;
    case asttags::Attributes:
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
    {
        if(sym && 0 < sym->get().kind.index() && std::holds_alternative<range_kind>(sym->get().kind)) {
           symbolTable.addEntry(sym->get().identifier, *sym);
           sym.reset();
           symnode = nullptr;
        }
    }
    break;
    case asttags::Require:
    break;
    case asttags::Return:
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
    case asttags::BytesLiteral:
    break;
    case asttags::CStringLiteral:
    break;
    case asttags::StringLiteral:
    break;
    case asttags::START_StringLikeLiteral:
    break;
    case asttags::END_StringLikeLiteral:
    break;
    case asttags::END_Literal:
    break;
    case asttags::START_Call:
    break;
    case asttags::FnCall:
    break;
    case asttags::OpCall:
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
    break;
    case asttags::END_Call:
    break;
    case asttags::MultiDecl:
    break;
    case asttags::TupleDecl:
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
        if(sym) {
            assert( 0 < sym->get().identifier.size() );
            const std::size_t lutId = sym->get().scopeId;

            if(sym->get().isConfig) {
               configVars.emplace_back(*sym);
            }

            auto lusym = symbolTable.find(lutId, sym->get().identifier);
            if(!lusym) {
                symbolTable.addEntry(lutId, sym->get().identifier, *sym);
                
                sym.reset();
                symnode = nullptr;
                assert(0 < symstack.size());
                symstack.pop_back();
                if (!symstack.empty()) {
                   sym = symstack.back();
                }
            }
            else {
                std::cerr << "chplx : " << sym->get().identifier << " identifier already defined in current scope" << std::endl;
                return;
            }
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
    case asttags::Defer:
    break;
    case asttags::Local:
    break;
    case asttags::Manage:
    break;
    case asttags::On:
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
    case asttags::For:
    break;
    case asttags::Forall:
    break;
    case asttags::Conditional:
    case asttags::Block:
    {
       if(sym) {
          assert( 0 < sym->get().identifier.size() );
          auto lusym = symbolTable.find(sym->get().scopeId, sym->get().identifier);
          const bool is_func = std::holds_alternative<std::shared_ptr<func_kind>>(sym->get().kind);
          if(!lusym && is_func) {
             std::shared_ptr<func_kind> & fk =
                std::get<std::shared_ptr<func_kind>>(sym->get().kind);

             if(fk->retKind.index() < 1) {
                fk->retKind = nil_kind{};
             }

             symbolTable.addEntry(sym->get().scopeId, fk->symbolTableSignature, *sym);

             sym.reset();
             symnode = nullptr;
             if(2 < symstack.size()) {
               symstack.pop_back();
               sym = symstack.back();
             }
             else if(1 < symstack.size()) {
               symstack.pop_back();
             }

             symbolTable.popScope();
          }
          else if(is_func && 0 < sym->get().kind.index()) {
             std::shared_ptr<func_kind> & fk =
                std::get<std::shared_ptr<func_kind>>(sym->get().kind);

             std::cerr << "chplx : " << fk->symbolTableSignature << " identifier already defined in current scope" << std::endl;
             return;
          }
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
       if(sym) {
          assert( sym.has_value() && std::holds_alternative<std::shared_ptr<func_kind>>(sym->get().kind));
          auto lusym = symbolTable.find(sym->get().scopeId, sym->get().identifier);
          if(!lusym) {
             std::shared_ptr<func_kind> & fk =
                std::get<std::shared_ptr<func_kind>>(sym->get().kind);

             if(fk->retKind.index() < 1) {
                fk->retKind = nil_kind{};
             }

             symbolTable.addEntry(sym->get().scopeId, fk->symbolTableSignature, *sym);
             sym.reset();
             symnode = nullptr;

             if(1 < symstack.size()) {
               symstack.pop_back();
             }

             if(2 < symstack.size()) {
               sym = symstack.back();
             }

             symbolTable.popScope();
          }
          else if (0 < sym->get().kind.index() && std::holds_alternative<std::shared_ptr<func_kind>>(sym->get().kind)) {
              std::shared_ptr<func_kind> & fk =
                 std::get<std::shared_ptr<func_kind>>(sym->get().kind);

              std::cerr << "chplx : " << fk->symbolTableSignature << " identifier already defined in current scope" << std::endl;
              return;
           }
          else {
             std::cerr << "chplx : " << sym->get().identifier << " identifier already defined in current scope" << std::endl;
             return;
          }
       }
    }
    break;
    case asttags::Interface:
    break;
    case asttags::Module:
    {
       if(0 < symstack.size()) {
           sym.reset();
           sym = symstack.back();
       }

       if(sym) {
          assert( 0 < sym->get().identifier.size() );
          auto lusym = symbolTable.find(sym->get().scopeId, sym->get().identifier);
          if(!lusym) {
             symbolTable.parentModuleId -= 1;
             std::shared_ptr<module_kind> & fk =
                std::get<std::shared_ptr<module_kind>>(sym->get().kind);

             if(fk->retKind.index() < 1) {
                fk->retKind = nil_kind{};
             }

             symbolTable.modules.push_back(&(sym->get()));
             symbolTable.addEntry(sym->get().scopeId, fk->symbolTableSignature, *sym);
             sym.reset();
             symnode = nullptr;

             if(1 < symstack.size()) {
               symstack.pop_back();
             }
             if(2 < symstack.size()) {
               sym = symstack.back();
             }

             symbolTable.popScope();
          }
          else if (0 < sym->get().kind.index() && std::holds_alternative<std::shared_ptr<module_kind>>(sym->get().kind)) {
              std::shared_ptr<module_kind> & fk =
                 std::get<std::shared_ptr<module_kind>>(sym->get().kind);

              std::cerr << "chplx : " << fk->symbolTableSignature << " identifier already defined in current scope" << std::endl;
              return;
           }
          else {
             std::cerr << "chplx : " << sym->get().identifier << " identifier already defined in current scope" << std::endl;
             return;
          }
       }
    }
    break;
    case asttags::START_AggregateDecl:
    break;
    case asttags::Record:
    {
       if(sym) {
          assert( 0 < sym->get().identifier.size() );
          auto lusym = symbolTable.find(sym->get().scopeId, sym->get().identifier);
          if(!lusym) {
             std::shared_ptr<record_kind> & rk =
                std::get<std::shared_ptr<record_kind>>(sym->get().kind);

             symbolTable.addEntry(sym->get().scopeId, rk->symbolTableSignature, *sym);
             sym.reset();
             symnode = nullptr;

             if(1 < symstack.size()) {
               symstack.pop_back();
             }
             if(2 < symstack.size()) {
               sym = symstack.back();
             }

             symbolTable.popScope();
          }
          else if (0 < sym->get().kind.index() && std::holds_alternative<std::shared_ptr<record_kind>>(sym->get().kind)) {
              std::shared_ptr<record_kind> & rk =
                 std::get<std::shared_ptr<record_kind>>(sym->get().kind);

              std::cerr << "chplx : " << rk->symbolTableSignature << " identifier already defined in current scope" << std::endl;
              return;
           }
          else {
             std::cerr << "chplx : " << sym->get().identifier << " identifier already defined in current scope" << std::endl;
             return;
          }
       }
    }
    break;
    case asttags::Class:
    {
       if(sym) {
          assert( 0 < sym->get().identifier.size() );
          auto lusym = symbolTable.find(sym->get().scopeId, sym->get().identifier);
          if(!lusym) {
             std::shared_ptr<class_kind> & ck =
                std::get<std::shared_ptr<class_kind>>(sym->get().kind);

             symbolTable.addEntry(sym->get().scopeId, ck->symbolTableSignature, *sym);
             sym.reset();
             symnode = nullptr;

             if(1 < symstack.size()) {
               symstack.pop_back();
             }
             if(2 < symstack.size()) {
               sym = symstack.back();
             }

             symbolTable.popScope();
          }
          else if (0 < sym->get().kind.index() && std::holds_alternative<std::shared_ptr<class_kind>>(sym->get().kind)) {
              std::shared_ptr<class_kind> & ck =
                 std::get<std::shared_ptr<class_kind>>(sym->get().kind);

              std::cerr << "chplx : " << ck->symbolTableSignature << " identifier already defined in current scope" << std::endl;
              return;
           }
          else {
             std::cerr << "chplx : " << sym->get().identifier << " identifier already defined in current scope" << std::endl;
             return;
          }
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
}

} /* namespace hpx */ } /* namespace visitors */ } /* namespace ast */ } /* namespace chpl */
