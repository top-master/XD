// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef AST_AST_TRANSFORMS_H
#define AST_AST_TRANSFORMS_H

#include "./ast-node.h"
#include "../format-config.h"
#include "../format-file-config.h"

#include <QtCore/QByteArray>
#include <QtCore/QHash>
#include <QtCore/QPair>
#include <QtCore/QSet>
#include <QtCore/QSharedPointer>
#include <QtCore/QString>
#include <QtCore/QVector>

class FormatDocument;

struct AstRuleOptions
{
    inline AstRuleOptions()
        : fixMocMacro(false)
        , spaceLogicalNot(false)
        , checkExcludedFlags(false)
        , checkPrivateAccess(false)
        , checkReferenceLocal(false)
        , checkLambdas(false)
        , splitStatements(false)
        , checkMemberInitializers(false)
        , spaceTemplateClosers(false)
        , spaceReferences(false)
        , closeConditions(false)
        , checkStaticVariables(false)
        , moveOperators(false)
        , joinMembers(false)
        , adjustIncludes(false)
        , wrapConditions(false)
        , wrapArguments(false)
        , wrapParameters(false)
        , wrapTernaries(false)
        , wrapOperators(false)
        , braceControls(false)
        , leadInitializers(false)
        , expandBodies(false)
        , indentContinuations(false)
        , inlineMethods(false)
        , replaceTypes(false)
        , checkConstOverloads(false)
        , checkGuardedGetters(false)
        , layoutConnections(false)
        , checkCodeShapes(false)
        , qualifyMembers(false)
        , manageSuper(false)
        , formatNamespaces(false)
        , labelNamespaces(false)
        , joinExpressions(false)
    {
    }

    inline bool anyCodeRule() const
    {
        return fixMocMacro
            || spaceLogicalNot
            || checkExcludedFlags
            || checkPrivateAccess
            || checkReferenceLocal
            || checkLambdas
            || splitStatements
            || checkMemberInitializers
            || spaceTemplateClosers
            || spaceReferences
            || closeConditions
            || checkStaticVariables
            || moveOperators
            || joinMembers
            || adjustIncludes
            || wrapConditions
            || wrapArguments
            || wrapParameters
            || wrapTernaries
            || wrapOperators
            || braceControls
            || leadInitializers
            || expandBodies
            || indentContinuations
            || inlineMethods
            || replaceTypes
            || checkConstOverloads
            || checkGuardedGetters
            || layoutConnections
            || checkCodeShapes
            || qualifyMembers
            || manageSuper
            || formatNamespaces
            || labelNamespaces
            || joinExpressions;
    }

    bool fixMocMacro;
    bool spaceLogicalNot;
    bool checkExcludedFlags;
    bool checkPrivateAccess;
    bool checkReferenceLocal;
    bool checkLambdas;
    bool splitStatements;
    bool checkMemberInitializers;
    bool spaceTemplateClosers;
    bool spaceReferences;
    bool closeConditions;
    bool checkStaticVariables;
    bool moveOperators;
    bool joinMembers;
    bool adjustIncludes;
    bool wrapConditions;
    bool wrapArguments;
    bool wrapParameters;
    bool wrapTernaries;
    bool wrapOperators;
    bool braceControls;
    bool leadInitializers;
    bool expandBodies;
    bool indentContinuations;
    bool inlineMethods;
    bool replaceTypes;
    bool checkConstOverloads;
    bool checkGuardedGetters;
    bool layoutConnections;
    bool checkCodeShapes;
    bool qualifyMembers;
    bool manageSuper;
    bool formatNamespaces;
    bool labelNamespaces;
    bool joinExpressions;
};

struct AstTransformState
{
    inline AstTransformState()
        : finalDocument(Q_NULLPTR)
        , finalRoot(Q_NULLPTR)
    {
    }

    // Byte span overrides `[from, to)` -> replacement bytes (non-overlapping).
    struct SpanRewrite
    {
        inline SpanRewrite() : from(0), to(0) {}
        inline SpanRewrite(int f, int t, const QByteArray & b)
            : from(f), to(t), bytes(b)
        {
        }
        int from;
        int to;
        QByteArray bytes;
    };

    const FormatDocument *finalDocument;
    AstNode *finalRoot;
    QSharedPointer<FormatDocument > ownedDocument;
    QSharedPointer<AstArena > ownedArena;

    QVector<SpanRewrite > spanRewrites;
    QVector<QPair<int, int > > finalLineRanges;

    // Node-level structural annotations lowered into `DocNode` IR by `AstPrinter`.
    QHash<const AstNode *, QByteArray > namespaceLabels;
    QHash<const AstNode *, int > namespaceLabelReplaceEnd;
};

class AstTransforms
{
public:

    typedef AstRuleOptions Options;

    static void apply(
            AstNode *root,
            QVector<AstComment * > *comments,
            const QByteArray & source,
            const Symbols & symbols,
            const QVector<QPair<int, int > > & disabledRanges,
            const QVector<QPair<int, int > > & lineRanges,
            const QVector<int > & lineStarts,
            const Options & options);

    static AstTransformState runDiagnosticsAndTransforms(
            FormatDocument *document, AstNode *root, const Options & options);
};

#endif // AST_AST_TRANSFORMS_H
