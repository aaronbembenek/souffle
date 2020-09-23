/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2013, 2015, Oracle and/or its affiliates. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file AstToRamTranslator.h
 *
 * Translator from AST into RAM
 *
 ***********************************************************************/

#pragma once

#include "souffle/utility/ContainerUtil.h"
#include "souffle/utility/FunctionalUtil.h"
#include "souffle/utility/MiscUtil.h"
#include "souffle/utility/StreamUtil.h"
#include "souffle/utility/StringUtil.h"
#include <cassert>
#include <cstddef>
#include <map>
#include <memory>
#include <ostream>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace souffle {
class SymbolTable;
}

namespace souffle::ast {
class Argument;
class Atom;
class Clause;
class Constant;
class Literal;
class NilConstant;
class Node;
class Program;
class QualifiedName;
class Relation;
class SipsMetric;
class TranslationUnit;
}  // namespace souffle::ast

namespace souffle::ast::analysis {
class IOTypeAnalysis;
class AuxiliaryArityAnalysis;
class RecursiveClausesAnalysis;
class TypeEnvironment;
}  // namespace souffle::ast::analysis

namespace souffle::ram {
class Condition;
class Expression;
class Relation;
class RelationReference;
class Statement;
class TranslationUnit;
class TupleElement;
}  // namespace souffle::ram

namespace souffle::ast2ram {

/**
 * Main class for the AST->RAM translator
 */
class AstToRamTranslator {
public:
    AstToRamTranslator();
    ~AstToRamTranslator();

    /** translates AST to translation unit */
    Own<ram::TranslationUnit> translateUnit(ast::TranslationUnit& tu);

    struct Location;
    class ValueIndex;
    class ClauseTranslator;
    class ProvenanceClauseTranslator;

private:
    /** AST program */
    const ast::Program* program = nullptr;

    /** Type environment */
    const ast::analysis::TypeEnvironment* typeEnv = nullptr;

    /** IO Type */
    const ast::analysis::IOTypeAnalysis* ioType = nullptr;

    /** Auxiliary Arity Analysis */
    const ast::analysis::AuxiliaryArityAnalysis* auxArityAnalysis = nullptr;

    /** RAM program */
    Own<ram::Statement> ramMain;

    /** Subroutines */
    std::map<std::string, Own<ram::Statement>> ramSubs;

    /** RAM relations */
    std::map<std::string, Own<ram::Relation>> ramRels;

    /** SIPS metric for reordering */
    Own<ast::SipsMetric> sips;

    /** translate AST to RAM Program */
    void translateProgram(const ast::TranslationUnit& translationUnit);

    /** create a RAM element access node */
    static Own<ram::TupleElement> makeRamTupleElement(const Location& loc);

    /** determine the auxiliary for relations */
    size_t getEvaluationArity(const ast::Atom* atom) const;

    /**
     * assigns names to unnamed variables such that enclosing
     * constructs may be cloned without losing the variable-identity
     */
    void nameUnnamedVariables(ast::Clause* clause);

    /** converts the given relation identifier into a relation name */
    static std::string getRelationName(const ast::QualifiedName& id);

    // TODO (b-scholz): revisit / refactor so that only one directive is translated
    std::vector<std::map<std::string, std::string>> getInputDirectives(const ast::Relation* rel);

    // TODO (b-scholz): revisit / refactor so that only one directive is translated
    std::vector<std::map<std::string, std::string>> getOutputDirectives(const ast::Relation* rel);

    /** create a reference to a RAM relation */
    Own<ram::RelationReference> createRelationReference(const std::string name);

    /** a utility to translate atoms to relations */
    Own<ram::RelationReference> translateRelation(const ast::Atom* atom);

    /** translate an AST relation to a RAM relation */
    Own<ram::RelationReference> translateRelation(
            const ast::Relation* rel, const std::string relationNamePrefix = "");

    /** translate a temporary `delta` relation to a RAM relation for semi-naive evaluation */
    Own<ram::RelationReference> translateDeltaRelation(const ast::Relation* rel);

    /** translate a temporary `new` relation to a RAM relation for semi-naive evaluation */
    Own<ram::RelationReference> translateNewRelation(const ast::Relation* rel);

    /** translate an AST argument to a RAM value */
    Own<ram::Expression> translateValue(const ast::Argument* arg, const ValueIndex& index);

    /** translate an AST constraint to a RAM condition */
    Own<ram::Condition> translateConstraint(const ast::Literal* arg, const ValueIndex& index);

    /** Return a symbol table **/
    SymbolTable& getSymbolTable();

    /** Get ram representation of constant */
    RamDomain getConstantRamRepresentation(const ast::Constant& constant);

    /** translate RAM code for a constant value */
    Own<ram::Expression> translateConstant(ast::Constant const& c);

    /**
     * translate RAM code for the non-recursive clauses of the given relation.
     *
     * @return a corresponding statement or null if there are no non-recursive clauses.
     */
    Own<ram::Statement> translateNonRecursiveRelation(
            const ast::Relation& rel, const ast::analysis::RecursiveClausesAnalysis* recursiveClauses);

    /** translate RAM code for recursive relations in a strongly-connected component */
    Own<ram::Statement> translateRecursiveRelation(const std::set<const ast::Relation*>& scc,
            const ast::analysis::RecursiveClausesAnalysis* recursiveClauses);

    /** translate RAM code for subroutine to get subproofs */
    Own<ram::Statement> makeSubproofSubroutine(const ast::Clause& clause);

    /** translate RAM code for subroutine to get subproofs for non-existence of a tuple */
    Own<ram::Statement> makeNegationSubproofSubroutine(const ast::Clause& clause);
};

/**
 * Concrete attribute
 */
struct AstToRamTranslator::Location {
    int identifier{};
    int element{};
    Own<ram::RelationReference> relation{nullptr};

    Location() = default;

    Location(int ident, int elem, Own<ram::RelationReference> rel = nullptr)
            : identifier(ident), element(elem), relation(std::move(rel)) {}

    Location(const Location& l) : identifier(l.identifier), element(l.element) {
        if (l.relation != nullptr) {
            relation = souffle::clone(l.relation);
        }
    }

    Location& operator=(Location other) {
        identifier = other.identifier;
        element = other.element;
        relation = std::move(other.relation);
        return *this;
    }

    bool operator==(const Location& loc) const {
        return identifier == loc.identifier && element == loc.element;
    }

    bool operator!=(const Location& loc) const {
        return !(*this == loc);
    }

    bool operator<(const Location& loc) const {
        return identifier < loc.identifier || (identifier == loc.identifier && element < loc.element);
    }

    void print(std::ostream& out) const {
        out << "(" << identifier << "," << element << ")";
    }

    friend std::ostream& operator<<(std::ostream& out, const Location& loc) {
        loc.print(out);
        return out;
    }
};

}  // namespace souffle::ast2ram
