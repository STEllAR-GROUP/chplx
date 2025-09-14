# Contributing - ChplX Compiler **Backend** (Minimal Guide)

This document is a focused guide for extending the **backend** in two common cases:
1) adding a **new data type**, and
2) adding a **new construct** (example: `Locales` / `on` support).

## Backend passes (where your changes will land)

- **Symbol pass** - `symbolbuildingvisitor.(hpp|cpp)`  
  Builds the hierarchical `SymbolTable`, seeds built‑ins (e.g., `bool`, `int`, `Locales`, `here`, `numLocales`), and creates scopes.

- **IR build pass** - `programtreebuildingvisitor.(hpp|cpp)`  
  Converts Chapel uAST to backend IR nodes declared in `programtree.hpp`, binding identifiers to `Symbol`s in the current scope.

- **Code generation** - `codegenvisitor.(hpp|cpp)`  
  Emits C++ that calls ChplX runtime helpers like `chplx::forLoop`, `chplx::forall`, `chplx::coforall`, and `chplx::on`.  
  It also injects `#line` mappings via `chplx::util::emitLineDirective(...)` from `utils.(hpp|cpp)`.

Support code used by all passes:
- **Types & symbols** - `symboltypes.(hpp|cpp)` (e.g., `array_kind`, `tuple_kind`, `range_kind`, `domain_kind`, `locale_kind`, `func_kind`, etc.; `Symbol`, `SymbolTable`).
- **IR helpers/printing** - `programtree.(hpp|cpp)` (identifier rewriting like `Locales` --> `chplx::Locales`, expression printers used by codegen).
- **Build/config** - `driver.cpp`, `cmakegen.(hpp|cpp)`, `utils.(hpp|cpp)` (flags like `suppressLineDirectives`, `output_path`, and sets such as `incdirs`, `libdirs`, `libs`, `packages`).

---

## A. Add a **new data type**

> You will touch: `symboltypes.hpp`, `symbolbuildingvisitor.cpp`, `programtreebuildingvisitor.cpp`, `codegenvisitor.cpp` (and possibly `programtree.cpp` for pretty‑printing).

1. **Introduce a kind**
   - In `symboltypes.hpp`, add a new `*_kind` that fits the existing taxonomy (scalars like `int_kind`, containers like `array_kind`, etc.).  
     See the current list (e.g., `array_kind`, `tuple_kind`, `range_kind`, `domain_kind`, `locale_kind`, …) and mirror the closest pattern.

2. **Seed a built‑in (only if it has a Chapel keyword)**
   - In `symbolbuildingvisitor.cpp`, extend the block where other primitives are added using `addSymbolEntry("name", Symbol{...});`.  
     Use the **same field ordering/initialization** as neighboring entries (don’t invent a new layout).

3. **Bind the type when building IR**
   - In `programtreebuildingvisitor.cpp`, find the code paths that build declarations and literals for similar types and replicate that pattern for your new kind.  
     Common nodes you may need to instantiate are visible in `programtree.hpp` (e.g., `ScalarDeclarationExpression`, `ArrayDeclarationExpression`, `TupleDeclarationExpression`, `LiteralExpression`, `VariableExpression`).  
     Ensure you store the resolved **symbol id** on the IR node so codegen can look it up later.

4. **(Optional) Pretty‑print expressions/declarations**
   - If your type has a literal or declaration form that needs special text, add/adjust the corresponding `emit(std::ostream&)` in `programtree.cpp`.  
     Note: not every IR node’s `emit()` does the full final printing - **codegenvisitor** is authoritative for producing C++.

5. **Map the type in codegen**
   - In `codegenvisitor.cpp`, handle your new kind where other kinds are handled (there are overloads/visitors for `array_kind`, `range_kind`, etc.).  
     Emit the appropriate C++ representation (and any required headers via `generateApplicationHeader(...)`).

6. **Smoke test**
   - Small Chapel snippet that declares/initializes the new type. Run through ChplX and confirm the generated C++ uses the expected C++ type/form.

---

## B. Add a **new construct** (example: `Locales` / `on`)

The backend already supports `Locales`, `here`, and `numLocales`:
- They are **seeded** in `symbolbuildingvisitor.cpp`.
- Identifiers are **rewritten** in `programtree.cpp` (`Locales` --> `chplx::Locales`, `here` --> `chplx::here`, `numLocales` --> `chplx::numLocales`).
- Codegen emits:
  - Loops: `chplx::forLoop(...)`, `chplx::forall(...)`, `chplx::coforall(...)`.
  - Placement: `chplx::on(target, [](auto loc){ ... }, loc);`

To introduce or extend a construct, follow the existing shapes for `{For, Forall, Coforall}LoopExpression` and `OnExpression`:

1. **Symbols (only if you add a new surface identifier)**
   - Add a new `addSymbolEntry("Name", Symbol{...});` in `symbolbuildingvisitor.cpp` with an appropriate `*_kind`.  
     If you are only extending behavior of existing constructs, you won’t need new symbols.

2. **IR nodes and binding**
   - If your construct is already represented in `programtree.hpp` (e.g., `OnExpression` or loop expressions), reuse it.  
     Otherwise, add a new IR node there and teach the builder to construct it in `programtreebuildingvisitor.cpp`.  
     For loops, the **index set** lives in `node->indexSet` and the induction variable in `node->iterator`.  
     For `on`, the codegen expects `OnExpression` with `OnLocaleVarExpr` (the target expression) and `OnLocale` (the bound symbol for the body parameter).

3. **Identifier rewriting (if applicable)**
   - In `programtree.cpp`, mirror the existing rewriting used for `Locales`/`here`/`numLocales` if your new construct needs to map a Chapel name to a namespaced C++ identifier.

4. **Code generation**
   - Extend the corresponding `operator()(std::shared_ptr<...> const&)` in `codegenvisitor.cpp`.  
     Loops currently special‑case the `Locales` index set by **omitting** an explicit `chplx::Range{...}` and directly emitting the expression.  
     `OnExpression` is emitted as:
     ```cpp
     chplx::on(<targetExpr>, [](auto <locVar>) {
       // body
     }, <locVar>);
     ```

5. **Round‑trip test**
   ```chapel
   writeln(numLocales);
   forall loc in Locales do on loc {
     writeln("hello from ", here.id);
   }
   ```
   Expect `forall` --> `chplx::forall(...)` and the `on` clause --> `chplx::on(...)` with a lambda in the generated C++.

---

## C. **Scope** handling (Symbol Table ↔ IR ↔ Codegen)

ChplX tracks **lexical scopes** in a tree: Module --> Function/Proc --> Block/Loop/`on` body, etc.

1. **Creating scopes in the Symbol pass** (`symbolbuildingvisitor.cpp`)  
   Use the existing `pushScope()` / `popScope()` calls around constructs that introduce a new scope (modules, functions/procs, blocks, loops, `on` bodies).  
   Insert new declarations with `addSymbolEntry(...)` while the current scope is active.

2. **Binding in the IR builder** (`programtreebuildingvisitor.cpp`)  
   Mirror the same scope nesting so identifier uses can be resolved with `symbolTable.find(...)`.  
   Attach the **resolved symbol id** to the IR nodes (e.g., for `VariableExpression` and declaration nodes).  
   For loops, create a loop scope and place the **induction variable** symbol inside it. For `on`, the body has its own scope.

3. **Printing & codegen**  
   - `programtree.cpp` emits nested blocks and rewrites certain identifiers to namespaced forms.  
   - `codegenvisitor.cpp` emits C++ blocks/lambdas that mirror IR scopes; keep declarations in the right block to preserve lifetime and shadowing.

4. **Common pitfalls**  
   - Unbalanced `pushScope`/`popScope` --> symbols appear in sibling scopes.  
   - Forgetting to attach symbol ids to IR nodes --> name resolution fails during codegen.  
   - Missing loop scope --> induction variable leaks outside the loop.  
   - `on` bodies must have their **own** scope distinct from the target expression’s evaluation.

---

## D. Quick file index (what to open for examples)

- `symboltypes.hpp` - all `*_kind` definitions; `Symbol`, `SymbolTable`, and `func*` metadata.  
- `symbolbuildingvisitor.cpp` - where primitives and built‑ins are **seeded**, and where scope is pushed/popped.  
- `programtree.hpp` - IR node types: `ForLoopExpression`, `ForallLoopExpression`, `CoforallLoopExpression`, `OnExpression`, declarations, operators, etc.  
- `programtreebuildingvisitor.cpp` - uAST --> IR construction and **symbol binding**.  
- `programtree.cpp` - identifier rewriting and expression/declaration emit helpers.  
- `codegenvisitor.cpp` - final C++ emission (`chplx::forLoop/forall/coforall/on`, `generateApplicationHeader(...)`, line directives).  
- `utils.(hpp|cpp)` - `emitLineDirective`, flags like `suppressLineDirectives`, and output/build knobs (`incdirs`, `libdirs`, `libs`, `packages`).

---

## E. Minimal checklists

**New type**
- [ ] Add a `*_kind` in `symboltypes.hpp` (mirroring an existing kind).
- [ ] Seed in `symbolbuildingvisitor.cpp` *only if* the type is a Chapel keyword/built‑in.
- [ ] Extend `programtreebuildingvisitor.cpp` to build/bind decls/literals using your kind.
- [ ] (If needed) add `emit()` helpers in `programtree.cpp`.
- [ ] Handle your kind in `codegenvisitor.cpp` and includes via `generateApplicationHeader(...)`.
- [ ] Add a tiny Chapel test, compile, and inspect the generated C++.

**New construct**
- [ ] Seed any new surface identifiers in `symbolbuildingvisitor.cpp` (if applicable).
- [ ] Reuse or add an IR node in `programtree.hpp` and construct it in `programtreebuildingvisitor.cpp`.
- [ ] Add identifier rewriting in `programtree.cpp` (only if needed).
- [ ] Implement emission in `codegenvisitor.cpp` (`forLoop/forall/coforall/on` patterns already exist).
- [ ] Test with a minimal Chapel snippet (including a `Locales` loop if relevant).
