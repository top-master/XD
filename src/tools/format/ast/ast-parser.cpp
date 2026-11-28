// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./ast-parser.h"
#include "./comment-attacher.h"

namespace {

int lineOfOffset(const QVector<int > & lineStarts, int offset)
{
    int low = 0;
    int high = lineStarts.size();
    while (low < high) {
        const int mid = low + (high - low) / 2;
        if (lineStarts.at(mid) <= offset) {
            low = mid + 1;
        } else {
            high = mid;
        }
    }
    return qMax(1, low);
}

bool isAccessKeyword(Token tok)
{
    return tok == PUBLIC
        || tok == PROTECTED
        || tok == PRIVATE
        || tok == SIGNALS
        || tok == SLOTS
        || tok == Q_SIGNALS_TOKEN
        || tok == Q_SLOTS_TOKEN;
}

bool isQtMacroToken(Token tok)
{
    return tok >= Q_OBJECT_TOKEN && tok <= Q_REVISION_TOKEN;
}

bool isCastKeyword(Token tok)
{
    return tok == STATIC_CAST
        || tok == DYNAMIC_CAST
        || tok == REINTERPRET_CAST
        || tok == CONST_CAST;
}

int binaryPrecedence(Token tok)
{
    switch (tok) {
    case EQ:
    case PLUS_EQ:
    case MINUS_EQ:
    case STAR_EQ:
    case SLASH_EQ:
    case PERCENT_EQ:
    case HAT_EQ:
    case AND_EQ:
    case OR_EQ:
    case LTLT_EQ:
    case GTGT_EQ:
        return 1;
    case OROR:
        return 2;
    case ANDAND:
        return 3;
    case OR:
        return 4;
    case HAT:
        return 5;
    case AND:
        return 6;
    case EQEQ:
    case NE:
        return 7;
    case LANGLE:
    case RANGLE:
    case LE:
    case GE:
        return 8;
    case LTLT:
    case GTGT:
        return 9;
    case PLUS:
    case MINUS:
        return 10;
    case STAR:
    case SLASH:
    case PERCENT:
        return 11;
    default:
        return 0;
    }
}

class ParserContext
{
public:

    inline ParserContext(
            const QByteArray & text,
            const Symbols & symbols,
            const QVector<int > & lineStarts,
            const QVector<bool > & directiveLines,
            const QVector<int > & matches,
            FormatLanguage::Id language,
            AstArena *arena)
        : m_text(text)
        , m_symbols(symbols)
        , m_lineStarts(lineStarts)
        , m_directiveLines(directiveLines)
        , m_matches(matches)
        , m_language(language)
        , m_arena(arena)
    {
    }

    inline AstNode *parseTranslationUnit(
            int firstTok, int lastTok, AstNode::Kind containerKind)
    {
        AstNode *unit = m_arena->createNode(containerKind);
        if (firstTok <= lastTok
            && firstTok >= 0
            && lastTok < m_symbols.size()
        ) {
            this->setTokenSpan(unit, firstTok, lastTok);
        } else {
            unit->setSpan(0, m_text.size(), -1, -1);
        }

        int i = firstTok;
        while (i <= lastTok) {
            if (this->isDirectiveToken(i)) {
                unit->addChild(this->parseDirective(i, lastTok, &i));
                continue;
            }
            const Token tok = this->tokenAt(i);
            if (tok == SEMIC) {
                ++i;
                continue;
            }
            if (tok == LBRACE) {
                const int close = this->matchToken(i, lastTok);
                if (close > i) {
                    unit->addChild(this->parseCompound(i, close));
                    i = close + 1;
                    continue;
                }
            }
            if (tok == NAMESPACE && this->tokenAt(i - 1) != USING) {
                unit->addChild(this->parseNamespace(i, lastTok, &i));
                continue;
            }
            if (tok == EXTERN && this->tokenAt(i + 1) == STRING_LITERAL) {
                unit->addChild(this->parseLinkageSpec(i, lastTok, &i));
                continue;
            }
            if (tok == TEMPLATE && this->tokenAt(i + 1) == LANGLE) {
                unit->addChild(this->parseTemplateDecl(i, lastTok, &i));
                continue;
            }
            if (tok == CLASS
                || tok == STRUCT
                || tok == UNION
                || tok == ENUM
            ) {
                AstNode *cls = this->tryParseClassOrEnum(i, lastTok, &i);
                if (cls) {
                    unit->addChild(cls);
                    continue;
                }
            }
            if (containerKind == AstNode::ClassDecl
                && isAccessKeyword(tok)
            ) {
                AstNode *acc = this->tryParseAccessSpec(i, lastTok, &i);
                if (acc) {
                    unit->addChild(acc);
                    continue;
                }
            }
            if (isQtMacroToken(tok)
                && tok != Q_SIGNALS_TOKEN
                && tok != Q_SLOTS_TOKEN
                && tok != Q_SIGNAL_TOKEN
                && tok != Q_SLOT_TOKEN
                && tok != Q_INVOKABLE_TOKEN
                && tok != Q_SCRIPTABLE_TOKEN
                && tok != Q_MOC_COMPAT_TOKEN
            ) {
                unit->addChild(this->parseQtMacro(i, lastTok, &i));
                continue;
            }
            if (tok == TYPEDEF || tok == USING) {
                unit->addChild(this->parseTypedefOrUsing(i, lastTok, &i));
                continue;
            }
            AstNode *declOrStmt = this->parseDeclarationOrStatement(
                i,
                lastTok,
                &i,
                containerKind == AstNode::CompoundStmt
            );
            unit->addChild(declOrStmt);
        }
        return unit;
    }

private:

    inline Token tokenAt(int index) const
    {
        return (index >= 0 && index < m_symbols.size())
            ? m_symbols.at(index).token : NOTOKEN;
    }

    inline int tokenLine(int index) const
    {
        return (index >= 0 && index < m_symbols.size())
            ? m_symbols.at(index).lineNum : 0;
    }

    inline int tokenStart(int index) const
    {
        return (index >= 0 && index < m_symbols.size())
            ? m_symbols.at(index).from : 0;
    }

    inline int tokenEnd(int index) const
    {
        return (index >= 0 && index < m_symbols.size())
            ? (m_symbols.at(index).from + m_symbols.at(index).len) : 0;
    }

    inline bool isDirectiveToken(int index) const
    {
        const int line = this->tokenLine(index);
        return line > 0
            && line < m_directiveLines.size()
            && m_directiveLines.at(line);
    }

    inline void setTokenSpan(AstNode *node, int firstTok, int lastTok) const
    {
        if (firstTok < 0
            || lastTok < firstTok
            || lastTok >= m_symbols.size()
        ) {
            return;
        }
        node->setSpan(
            this->tokenStart(firstTok),
            this->tokenEnd(lastTok),
            firstTok,
            lastTok
        );
    }

    inline int matchToken(int openTok, int limitTok) const
    {
        if (openTok >= 0 && openTok < m_matches.size()) {
            const int paired = m_matches.at(openTok);
            if (paired > openTok && paired <= limitTok) {
                return paired;
            }
        }
        const Token open = this->tokenAt(openTok);
        Token close = RPAREN;
        if (open == LBRACE) {
            close = RBRACE;
        } else if (open == LBRACK) {
            close = RBRACK;
        }
        int depth = 1;
        for (int k = openTok + 1; k <= limitTok; ++k) {
            if (this->isDirectiveToken(k)) {
                continue;
            }
            const Token t = this->tokenAt(k);
            if (t == open) {
                ++depth;
            } else if (t == close) {
                --depth;
                if (depth == 0) {
                    return k;
                }
            }
        }
        return -1;
    }

    inline int skipTemplateArgs(int langleTok, int limitTok) const
    {
        if (this->tokenAt(langleTok) != LANGLE) {
            return -1;
        }
        int depth = 0;
        int parenDepth = 0;
        for (int k = langleTok; k <= limitTok; ++k) {
            const Token t = this->tokenAt(k);
            if (t == LPAREN || t == LBRACK) {
                ++parenDepth;
            } else if ((t == RPAREN || t == RBRACK) && parenDepth > 0) {
                --parenDepth;
            } else if (parenDepth == 0) {
                if (t == LANGLE) {
                    ++depth;
                } else if (t == RANGLE) {
                    --depth;
                    if (depth <= 0) {
                        return k;
                    }
                } else if (t == GTGT) {
                    depth -= 2;
                    if (depth <= 0) {
                        return k;
                    }
                } else if (t == SEMIC || t == LBRACE || t == RBRACE) {
                    return -1;
                }
            }
        }
        return -1;
    }

    inline AstNode *parseDirective(int startTok, int limitTok, int *nextTok)
    {
        int endTok = startTok;
        while (endTok + 1 <= limitTok
            && this->isDirectiveToken(endTok + 1)
        ) {
            ++endTok;
        }
        AstNode *dir = m_arena->createNode(AstNode::Directive);
        dir->name = m_symbols.at(startTok).lexem();
        this->setTokenSpan(dir, startTok, endTok);
        *nextTok = endTok + 1;
        return dir;
    }

    inline AstNode *parseNamespace(int startTok, int limitTok, int *nextTok)
    {
        int k = startTok + 1;
        QByteArray nsName;
        while (k <= limitTok
            && this->tokenAt(k) != LBRACE
            && this->tokenAt(k) != EQ
            && this->tokenAt(k) != SEMIC
        ) {
            nsName.append(m_symbols.at(k).lexem());
            ++k;
        }
        if (k <= limitTok && this->tokenAt(k) == LBRACE) {
            const int close = this->matchToken(k, limitTok);
            if (close > k) {
                AstNode *ns = m_arena->createNode(AstNode::NamespaceDecl);
                ns->name = nsName;
                ns->openToken = k;
                ns->closeToken = close;
                if (close - k > 1) {
                    ns->body = this->parseTranslationUnit(
                        k + 1,
                        close - 1,
                        AstNode::NamespaceDecl
                    );
                    ns->addChild(ns->body);
                }
                this->setTokenSpan(ns, startTok, close);
                *nextTok = close + 1;
                return ns;
            }
        }
        return this->parseRawStatement(
            startTok,
            limitTok,
            nextTok,
            AstNode::NamespaceDecl
        );
    }

    inline AstNode *parseLinkageSpec(int startTok, int limitTok, int *nextTok)
    {
        int k = startTok + 2;
        if (k <= limitTok && this->tokenAt(k) == LBRACE) {
            const int close = this->matchToken(k, limitTok);
            if (close > k) {
                AstNode *link = m_arena->createNode(AstNode::LinkageSpec);
                link->name = m_symbols.at(startTok + 1).lexem();
                link->openToken = k;
                link->closeToken = close;
                if (close - k > 1) {
                    link->body = this->parseTranslationUnit(
                        k + 1,
                        close - 1,
                        AstNode::LinkageSpec
                    );
                    link->addChild(link->body);
                }
                this->setTokenSpan(link, startTok, close);
                *nextTok = close + 1;
                return link;
            }
        }
        return this->parseRawStatement(
            startTok,
            limitTok,
            nextTok,
            AstNode::LinkageSpec
        );
    }

    inline AstNode *parseTemplateDecl(int startTok, int limitTok, int *nextTok)
    {
        const int rangle = this->skipTemplateArgs(startTok + 1, limitTok);
        if (rangle < 0 || rangle >= limitTok) {
            return this->parseRawStatement(
                startTok,
                limitTok,
                nextTok,
                AstNode::TemplateDecl
            );
        }
        int innerNext = rangle + 1;
        AstNode *inner = Q_NULLPTR;
        const Token after = this->tokenAt(rangle + 1);
        if (after == CLASS
            || after == STRUCT
            || after == UNION
            || after == ENUM
        ) {
            inner = this->tryParseClassOrEnum(
                rangle + 1,
                limitTok,
                &innerNext
            );
        } else if (after == USING || after == TYPEDEF) {
            inner = this->parseTypedefOrUsing(
                rangle + 1,
                limitTok,
                &innerNext
            );
        }
        if ( ! inner) {
            inner = this->parseDeclarationOrStatement(
                rangle + 1,
                limitTok,
                &innerNext,
                false
            );
        }
        AstNode *tpl = m_arena->createNode(AstNode::TemplateDecl);
        tpl->openToken = startTok + 1;
        tpl->closeToken = rangle;
        tpl->inner = inner;
        tpl->addChild(inner);
        this->setTokenSpan(tpl, startTok, innerNext - 1);
        *nextTok = innerNext;
        return tpl;
    }

    inline AstNode *tryParseClassOrEnum(
            int startTok, int limitTok, int *nextTok)
    {
        const Token key = this->tokenAt(startTok);
        bool isClass = key == CLASS
            || key == STRUCT
            || key == UNION
            || key == ENUM;
        if ( ! isClass) {
            return Q_NULLPTR;
        }
        // Scans header until `;` (forward decl / elaborated type) or `{`
        // unless `(` appears before `{` (e.g. function returning `struct S`).
        int k = startTok + 1;
        QByteArray className;
        int parenDepth = 0;
        while (k <= limitTok) {
            const Token t = this->tokenAt(k);
            if (t == LPAREN) {
                ++parenDepth;
                if (parenDepth == 1 && key != ENUM) {
                    return Q_NULLPTR;
                }
            } else if (t == RPAREN && parenDepth > 0) {
                --parenDepth;
            } else if (parenDepth == 0) {
                if (t == LBRACE) {
                    break;
                }
                if (t == SEMIC) {
                    AstNode *fwd = m_arena->createNode(AstNode::ClassDecl);
                    fwd->name = className;
                    fwd->isEnum = (key == ENUM);
                    fwd->isForward = true;
                    this->setTokenSpan(fwd, startTok, k);
                    *nextTok = k + 1;
                    return fwd;
                }
                if (t == EQ && key != ENUM) {
                    return Q_NULLPTR;
                }
                if (className.isEmpty()
                    && t == IDENTIFIER
                    && (this->tokenAt(k + 1) == COLON
                        || this->tokenAt(k + 1) == LBRACE
                        || this->tokenAt(k + 1) == SEMIC
                        || (this->tokenAt(k + 1) == IDENTIFIER
                            && m_symbols.at(k + 1).lexem() == "final")
                    )
                ) {
                    className = m_symbols.at(k).lexem();
                }
            }
            ++k;
        }
        if (k > limitTok || this->tokenAt(k) != LBRACE) {
            return Q_NULLPTR;
        }
        const int close = this->matchToken(k, limitTok);
        if (close <= k) {
            return Q_NULLPTR;
        }
        int endTok = close;
        if (endTok + 1 <= limitTok && this->tokenAt(endTok + 1) == SEMIC) {
            ++endTok;
        }
        AstNode *cls = m_arena->createNode(AstNode::ClassDecl);
        cls->name = className;
        cls->isEnum = (key == ENUM);
        cls->openToken = k;
        cls->closeToken = close;
        if (key != ENUM && close - k > 1) {
            cls->body = this->parseTranslationUnit(
                k + 1,
                close - 1,
                AstNode::ClassDecl
            );
            cls->addChild(cls->body);
        }
        this->setTokenSpan(cls, startTok, endTok);
        *nextTok = endTok + 1;
        return cls;
    }

    inline AstNode *tryParseAccessSpec(int startTok, int limitTok, int *nextTok)
    {
        int k = startTok + 1;
        if (k <= limitTok && this->tokenAt(k) == SLOTS) {
            ++k;
        }
        if (k <= limitTok && this->tokenAt(k) == Q_SLOTS_TOKEN) {
            ++k;
        }
        if (k <= limitTok && this->tokenAt(k) == COLON) {
            AstNode *spec = m_arena->createNode(AstNode::AccessSpec);
            spec->opToken = startTok;
            this->setTokenSpan(spec, startTok, k);
            *nextTok = k + 1;
            return spec;
        }
        return Q_NULLPTR;
    }

    inline AstNode *parseQtMacro(int startTok, int limitTok, int *nextTok)
    {
        int endTok = startTok;
        if (startTok + 1 <= limitTok
            && this->tokenAt(startTok + 1) == LPAREN
        ) {
            const int close = this->matchToken(startTok + 1, limitTok);
            if (close > startTok + 1) {
                endTok = close;
            }
        }
        AstNode *macro = m_arena->createNode(AstNode::QtMacroDecl);
        macro->name = m_symbols.at(startTok).lexem();
        this->setTokenSpan(macro, startTok, endTok);
        *nextTok = endTok + 1;
        return macro;
    }

    inline AstNode *parseTypedefOrUsing(
            int startTok, int limitTok, int *nextTok)
    {
        int endTok = startTok;
        int depth = 0;
        while (endTok <= limitTok) {
            const Token t = this->tokenAt(endTok);
            if (t == LPAREN || t == LBRACK || t == LBRACE) {
                ++depth;
            } else if ((t == RPAREN
                    || t == RBRACK
                    || t == RBRACE)
                && depth > 0
            ) {
                --depth;
            } else if (depth == 0 && t == SEMIC) {
                break;
            }
            ++endTok;
        }
        if (endTok > limitTok) {
            endTok = limitTok;
        }
        AstNode *decl = m_arena->createNode(AstNode::TypedefUsingDecl);
        this->setTokenSpan(decl, startTok, endTok);
        *nextTok = endTok + 1;
        return decl;
    }

    inline AstNode *parseDeclarationOrStatement(
            int startTok,
            int limitTok,
            int *nextTok,
            bool insideBlock)
    {
        const Token tok = this->tokenAt(startTok);
        if (insideBlock) {
            if (tok == IF) {
                return this->parseIfStmt(startTok, limitTok, nextTok);
            }
            if (tok == FOR) {
                return this->parseForStmt(startTok, limitTok, nextTok);
            }
            if (tok == WHILE) {
                return this->parseWhileStmt(startTok, limitTok, nextTok);
            }
            if (tok == DO) {
                return this->parseDoWhileStmt(startTok, limitTok, nextTok);
            }
            if (tok == SWITCH) {
                return this->parseSwitchStmt(startTok, limitTok, nextTok);
            }
            if (tok == TRY) {
                return this->parseTryStmt(startTok, limitTok, nextTok);
            }
            if (tok == RETURN) {
                return this->parseReturnStmt(startTok, limitTok, nextTok);
            }
            if (tok == CASE || tok == DEFAULT) {
                return this->parseCaseLabel(startTok, limitTok, nextTok);
            }
        }

        // Checks whether this span opens a top-level / member function decl
        // or definition (has a top-level `(params)` before `;` or `=`).
        int parenOpen = -1;
        int eqBeforeParen = -1;
        int depth = 0;
        int scan = startTok;
        for (; scan <= limitTok; ++scan) {
            if (this->isDirectiveToken(scan)) {
                break;
            }
            const Token t = this->tokenAt(scan);
            if (t == SEMIC || t == LBRACE || t == RBRACE) {
                break;
            }
            if (t == OPERATOR
                && scan + 2 <= limitTok
                && this->tokenAt(scan + 1) == LPAREN
                && this->tokenAt(scan + 2) == RPAREN
            ) {
                scan += 2;
                continue;
            }
            if (t == LBRACK) {
                const int rbrack = this->matchToken(scan, limitTok);
                if (rbrack > scan) {
                    scan = rbrack;
                    continue;
                }
            }
            if (t == LPAREN) {
                if (depth == 0) {
                    if (eqBeforeParen < 0
                        && scan > startTok
                        && ! (insideBlock
                            && this->tokenAt(scan - 1) == IF)
                    ) {
                        parenOpen = scan;
                    }
                    break;
                }
                ++depth;
            } else if (t == EQ) {
                eqBeforeParen = scan;
                break;
            }
        }

        if (parenOpen > startTok) {
            const int parenClose = this->matchToken(parenOpen, limitTok);
            if (parenClose > parenOpen) {
                // Checks whether token after qualifiers is `{` or `;`.
                int k = parenClose + 1;
                int colonTok = -1;
                bool hasArrowReturn = false;
                while (k <= limitTok) {
                    if (this->isDirectiveToken(k)) {
                        break;
                    }
                    const Token t = this->tokenAt(k);
                    if (t == SEMIC || t == LBRACE) {
                        break;
                    }
                    if (t == COLON && colonTok < 0) {
                        colonTok = k;
                    } else if (t == ARROW) {
                        hasArrowReturn = true;
                    } else if (t == LPAREN || t == LBRACE) {
                        const int paired = this->matchToken(k, limitTok);
                        if (paired > k) {
                            k = paired;
                        }
                    } else if (t == RBRACE) {
                        break;
                    }
                    ++k;
                }
                if ( ! insideBlock
                    && k <= limitTok
                    && (this->tokenAt(k) == LBRACE
                        || this->tokenAt(k) == SEMIC)
                ) {
                    Q_UNUSED(hasArrowReturn);
                    return this->parseFunctionDecl(
                        startTok,
                        parenOpen,
                        parenClose,
                        colonTok,
                        k,
                        limitTok,
                        nextTok
                    );
                }
            }
        }

        // Otherwise parses statement / expression up to `;`.
        return this->parseExprStatement(startTok, limitTok, nextTok);
    }

    inline AstNode *parseFunctionDecl(
            int startTok,
            int parenOpen,
            int parenClose,
            int colonTok,
            int termTok,
            int limitTok,
            int *nextTok)
    {
        AstNode *fn = m_arena->createNode(AstNode::FunctionDecl);
        fn->openToken = parenOpen;
        fn->closeToken = parenClose;
        fn->opToken = colonTok;

        for (int k = startTok; k < parenOpen; ++k) {
            if (this->tokenAt(k) == INLINE) {
                fn->isInline = true;
            }
        }
        for (int k = parenClose + 1; k < termTok; ++k) {
            if (this->tokenAt(k) == CONST) {
                fn->isConst = true;
            }
        }

        // Parses comma-separated parameters.
        if (parenClose - parenOpen > 1) {
            this->parseCommaItems(
                parenOpen + 1,
                parenClose - 1,
                AstNode::ParamDecl,
                &fn->items,
                fn
            );
        }

        // Parses constructor member initializers if `: init1, init2` exists.
        if (colonTok > parenClose && termTok > colonTok) {
            this->parseCommaItems(
                colonTok + 1,
                termTok - 1,
                AstNode::CtorInitializer,
                &fn->initializers,
                fn
            );
        }

        int endTok = termTok;
        if (this->tokenAt(termTok) == LBRACE) {
            const int bodyClose = this->matchToken(termTok, limitTok);
            if (bodyClose > termTok) {
                fn->body = this->parseCompound(termTok, bodyClose);
                fn->addChild(fn->body);
                endTok = bodyClose;
            }
        }
        this->setTokenSpan(fn, startTok, endTok);
        *nextTok = endTok + 1;
        return fn;
    }

    inline void parseCommaItems(
            int firstTok,
            int lastTok,
            AstNode::Kind itemKind,
            QVector<AstNode * > *out,
            AstNode *owner)
    {
        int itemStart = firstTok;
        int parenDepth = 0;
        int angleDepth = 0;
        int braceDepth = 0;
        for (int k = firstTok; k <= lastTok; ++k) {
            const Token t = this->tokenAt(k);
            if (t == LPAREN || t == LBRACK) {
                ++parenDepth;
            } else if ((t == RPAREN || t == RBRACK) && parenDepth > 0) {
                --parenDepth;
            } else if (t == LBRACE) {
                ++braceDepth;
            } else if (t == RBRACE && braceDepth > 0) {
                --braceDepth;
            } else if (t == LANGLE && parenDepth == 0 && braceDepth == 0) {
                ++angleDepth;
            } else if ((t == RANGLE
                    || t == GTGT)
                && angleDepth > 0
                && parenDepth == 0
                && braceDepth == 0
            ) {
                angleDepth = qMax(0, angleDepth - (t == GTGT ? 2 : 1));
            } else if (t == COMMA
                && parenDepth == 0
                && angleDepth == 0
                && braceDepth == 0
            ) {
                if (k > itemStart) {
                    AstNode *item = (itemKind == AstNode::RawSpan)
                        ? this->parseExpression(itemStart, k - 1)
                        : this->makeSpanNode(itemKind, itemStart, k - 1);
                    out->append(item);
                    owner->addChild(item);
                }
                itemStart = k + 1;
            }
        }
        if (itemStart <= lastTok) {
            AstNode *item = (itemKind == AstNode::RawSpan)
                ? this->parseExpression(itemStart, lastTok)
                : this->makeSpanNode(itemKind, itemStart, lastTok);
            out->append(item);
            owner->addChild(item);
        }
    }

    inline AstNode *parseCompound(int openBrace, int closeBrace)
    {
        AstNode *block = this->parseTranslationUnit(
            openBrace + 1,
            closeBrace - 1,
            AstNode::CompoundStmt
        );
        block->openToken = openBrace;
        block->closeToken = closeBrace;
        block->isBlock = true;
        this->setTokenSpan(block, openBrace, closeBrace);
        return block;
    }

    inline AstNode *parseStatement(int startTok, int limitTok, int *nextTok)
    {
        if (startTok > limitTok) {
            *nextTok = limitTok + 1;
            return Q_NULLPTR;
        }
        if (this->tokenAt(startTok) == LBRACE) {
            const int close = this->matchToken(startTok, limitTok);
            if (close > startTok) {
                *nextTok = close + 1;
                return this->parseCompound(startTok, close);
            }
        }
        return this->parseDeclarationOrStatement(
            startTok,
            limitTok,
            nextTok,
            true
        );
    }

    inline AstNode *parseIfStmt(int startTok, int limitTok, int *nextTok)
    {
        int lparen = startTok + 1;
        while (lparen <= limitTok
            && this->tokenAt(lparen) != LPAREN
            && this->tokenAt(lparen) != LBRACE
            && this->tokenAt(lparen) != SEMIC
        ) {
            ++lparen;
        }
        const int rparen =
            (lparen <= limitTok && this->tokenAt(lparen) == LPAREN)
            ? this->matchToken(lparen, limitTok) : -1;
        if (rparen < 0) {
            return this->parseRawStatement(
                startTok,
                limitTok,
                nextTok,
                AstNode::IfStmt
            );
        }

        AstNode *stmt = m_arena->createNode(AstNode::IfStmt);
        stmt->openToken = lparen;
        stmt->closeToken = rparen;
        if (rparen - lparen > 1) {
            stmt->cond = this->parseExpression(lparen + 1, rparen - 1);
            stmt->addChild(stmt->cond);
        }
        int cursor = rparen + 1;
        while (cursor <= limitTok && this->isDirectiveToken(cursor)) {
            ++cursor;
        }
        if (cursor <= limitTok) {
            stmt->body = this->parseStatement(cursor, limitTok, &cursor);
            stmt->addChild(stmt->body);
        }
        int afterElse = cursor;
        while (afterElse <= limitTok
            && this->isDirectiveToken(afterElse)
        ) {
            ++afterElse;
        }
        if (afterElse <= limitTok && this->tokenAt(afterElse) == ELSE) {
            int elseStart = afterElse + 1;
            while (elseStart <= limitTok
                && this->isDirectiveToken(elseStart)
            ) {
                ++elseStart;
            }
            if (elseStart <= limitTok) {
                stmt->elseBranch = this->parseStatement(
                    elseStart,
                    limitTok,
                    &cursor
                );
                stmt->addChild(stmt->elseBranch);
            }
        }
        const int endTok = qMax(startTok, cursor - 1);
        this->setTokenSpan(stmt, startTok, endTok);
        *nextTok = cursor;
        return stmt;
    }

    inline AstNode *parseForStmt(int startTok, int limitTok, int *nextTok)
    {
        const int lparen = startTok + 1;
        const int rparen =
            (lparen <= limitTok && this->tokenAt(lparen) == LPAREN)
            ? this->matchToken(lparen, limitTok) : -1;
        if (rparen < 0) {
            return this->parseRawStatement(
                startTok,
                limitTok,
                nextTok,
                AstNode::ForStmt
            );
        }
        AstNode *stmt = m_arena->createNode(AstNode::ForStmt);
        stmt->openToken = lparen;
        stmt->closeToken = rparen;
        if (rparen - lparen > 1) {
            stmt->cond = this->makeSpanNode(
                AstNode::RawSpan,
                lparen + 1,
                rparen - 1
            );
            stmt->addChild(stmt->cond);
        }
        int cursor = rparen + 1;
        if (cursor <= limitTok) {
            stmt->body = this->parseStatement(cursor, limitTok, &cursor);
            stmt->addChild(stmt->body);
        }
        this->setTokenSpan(stmt, startTok, qMax(startTok, cursor - 1));
        *nextTok = cursor;
        return stmt;
    }

    inline AstNode *parseWhileStmt(int startTok, int limitTok, int *nextTok)
    {
        const int lparen = startTok + 1;
        const int rparen =
            (lparen <= limitTok && this->tokenAt(lparen) == LPAREN)
            ? this->matchToken(lparen, limitTok) : -1;
        if (rparen < 0) {
            return this->parseRawStatement(
                startTok,
                limitTok,
                nextTok,
                AstNode::WhileStmt
            );
        }
        AstNode *stmt = m_arena->createNode(AstNode::WhileStmt);
        stmt->openToken = lparen;
        stmt->closeToken = rparen;
        if (rparen - lparen > 1) {
            stmt->cond = this->parseExpression(lparen + 1, rparen - 1);
            stmt->addChild(stmt->cond);
        }
        int cursor = rparen + 1;
        if (cursor <= limitTok) {
            stmt->body = this->parseStatement(cursor, limitTok, &cursor);
            stmt->addChild(stmt->body);
        }
        this->setTokenSpan(stmt, startTok, qMax(startTok, cursor - 1));
        *nextTok = cursor;
        return stmt;
    }

    inline AstNode *parseDoWhileStmt(int startTok, int limitTok, int *nextTok)
    {
        AstNode *stmt = m_arena->createNode(AstNode::WhileStmt);
        stmt->isDoWhile = true;
        int cursor = startTok + 1;
        if (cursor <= limitTok) {
            stmt->body = this->parseStatement(cursor, limitTok, &cursor);
            stmt->addChild(stmt->body);
        }
        if (cursor <= limitTok && this->tokenAt(cursor) == WHILE) {
            const int lparen = cursor + 1;
            const int rparen =
                (lparen <= limitTok && this->tokenAt(lparen) == LPAREN)
                ? this->matchToken(lparen, limitTok) : -1;
            if (rparen > lparen) {
                stmt->openToken = lparen;
                stmt->closeToken = rparen;
                if (rparen - lparen > 1) {
                    stmt->cond = this->parseExpression(
                        lparen + 1,
                        rparen - 1
                    );
                    stmt->addChild(stmt->cond);
                }
                cursor = rparen + 1;
                if (cursor <= limitTok && this->tokenAt(cursor) == SEMIC) {
                    ++cursor;
                }
            }
        }
        this->setTokenSpan(stmt, startTok, qMax(startTok, cursor - 1));
        *nextTok = cursor;
        return stmt;
    }

    inline AstNode *parseSwitchStmt(int startTok, int limitTok, int *nextTok)
    {
        const int lparen = startTok + 1;
        const int rparen =
            (lparen <= limitTok && this->tokenAt(lparen) == LPAREN)
            ? this->matchToken(lparen, limitTok) : -1;
        if (rparen < 0) {
            return this->parseRawStatement(
                startTok,
                limitTok,
                nextTok,
                AstNode::SwitchStmt
            );
        }
        AstNode *stmt = m_arena->createNode(AstNode::SwitchStmt);
        stmt->openToken = lparen;
        stmt->closeToken = rparen;
        if (rparen - lparen > 1) {
            stmt->cond = this->parseExpression(lparen + 1, rparen - 1);
            stmt->addChild(stmt->cond);
        }
        int cursor = rparen + 1;
        if (cursor <= limitTok) {
            stmt->body = this->parseStatement(cursor, limitTok, &cursor);
            stmt->addChild(stmt->body);
        }
        this->setTokenSpan(stmt, startTok, qMax(startTok, cursor - 1));
        *nextTok = cursor;
        return stmt;
    }

    inline AstNode *parseTryStmt(int startTok, int limitTok, int *nextTok)
    {
        AstNode *stmt = m_arena->createNode(AstNode::TryStmt);
        int cursor = startTok + 1;
        if (cursor <= limitTok) {
            stmt->body = this->parseStatement(cursor, limitTok, &cursor);
            stmt->addChild(stmt->body);
        }
        while (cursor <= limitTok && this->tokenAt(cursor) == CATCH) {
            int catchEnd = cursor + 1;
            if (catchEnd <= limitTok
                && this->tokenAt(catchEnd) == LPAREN
            ) {
                const int rparen = this->matchToken(catchEnd, limitTok);
                if (rparen > catchEnd) {
                    catchEnd = rparen + 1;
                }
            }
            if (catchEnd <= limitTok
                && this->tokenAt(catchEnd) == LBRACE
            ) {
                AstNode *catchBody = this->parseStatement(
                    catchEnd,
                    limitTok,
                    &catchEnd
                );
                stmt->items.append(catchBody);
                stmt->addChild(catchBody);
            } else {
                break;
            }
            cursor = catchEnd;
        }
        this->setTokenSpan(stmt, startTok, qMax(startTok, cursor - 1));
        *nextTok = cursor;
        return stmt;
    }

    inline AstNode *parseReturnStmt(int startTok, int limitTok, int *nextTok)
    {
        int endTok = startTok + 1;
        int depth = 0;
        while (endTok <= limitTok) {
            const Token t = this->tokenAt(endTok);
            if (t == LPAREN || t == LBRACK || t == LBRACE) {
                ++depth;
            } else if ((t == RPAREN
                    || t == RBRACK
                    || t == RBRACE)
                && depth > 0
            ) {
                --depth;
            } else if (depth == 0 && (t == SEMIC || t == RBRACE)) {
                break;
            }
            ++endTok;
        }
        AstNode *ret = m_arena->createNode(AstNode::ReturnStmt);
        if (endTok > startTok + 1) {
            ret->inner = this->parseExpression(startTok + 1, endTok - 1);
            ret->addChild(ret->inner);
        }
        const int spanEnd =
            (endTok <= limitTok && this->tokenAt(endTok) == SEMIC)
            ? endTok : (endTok - 1);
        this->setTokenSpan(ret, startTok, qMax(startTok, spanEnd));
        *nextTok = spanEnd + 1;
        return ret;
    }

    inline AstNode *parseCaseLabel(int startTok, int limitTok, int *nextTok)
    {
        int k = startTok + 1;
        while (k <= limitTok
            && this->tokenAt(k) != COLON
            && this->tokenAt(k) != SEMIC
            && this->tokenAt(k) != LBRACE
        ) {
            ++k;
        }
        const int endTok = (k <= limitTok && this->tokenAt(k) == COLON)
            ? k : qMax(startTok, k - 1);
        AstNode *c = m_arena->createNode(AstNode::CaseStmt);
        c->isForward = (this->tokenAt(startTok) == DEFAULT);
        if (endTok > startTok + 1 && ! c->isForward) {
            c->cond = this->parseExpression(startTok + 1, endTok - 1);
            c->addChild(c->cond);
        }
        this->setTokenSpan(c, startTok, endTok);
        *nextTok = endTok + 1;
        return c;
    }

    inline AstNode *parseExprStatement(int startTok, int limitTok, int *nextTok)
    {
        int endTok = startTok;
        int depth = 0;
        bool hasAssignmentOrParen = false;
        while (endTok <= limitTok) {
            const Token t = this->tokenAt(endTok);
            if (this->isDirectiveToken(endTok)) {
                if (endTok > startTok
                    && ! hasAssignmentOrParen
                    && depth == 0
                ) {
                    --endTok;
                    break;
                }
                ++endTok;
                continue;
            }
            if (t == EQ) {
                hasAssignmentOrParen = true;
            }
            if (t == LPAREN || t == LBRACK || t == LBRACE) {
                hasAssignmentOrParen = true;
                ++depth;
            } else if (t == RPAREN || t == RBRACK || t == RBRACE) {
                if (depth == 0) {
                    if (endTok > startTok) {
                        --endTok;
                    }
                    break;
                }
                --depth;
            } else if (depth == 0 && t == SEMIC) {
                break;
            }
            ++endTok;
        }
        if (endTok > limitTok) {
            endTok = limitTok;
        }
        const int exprEnd = (this->tokenAt(endTok) == SEMIC) ? endTok - 1
                                                             : endTok;
        AstNode *stmt = m_arena->createNode(AstNode::ExprStmt);
        if (exprEnd >= startTok) {
            stmt->inner = this->parseExpression(startTok, exprEnd);
            stmt->addChild(stmt->inner);
        }
        this->setTokenSpan(stmt, startTok, endTok);
        *nextTok = endTok + 1;
        return stmt;
    }

    inline AstNode *parseRawStatement(
            int startTok,
            int limitTok,
            int *nextTok,
            AstNode::Kind kind)
    {
        int endTok = startTok;
        int depth = 0;
        while (endTok <= limitTok) {
            const Token t = this->tokenAt(endTok);
            if (t == LPAREN || t == LBRACK || t == LBRACE) {
                ++depth;
            } else if (t == RPAREN || t == RBRACK || t == RBRACE) {
                if (depth > 0) {
                    --depth;
                } else {
                    break;
                }
            } else if (depth == 0 && t == SEMIC) {
                break;
            }
            ++endTok;
        }
        if (endTok > limitTok) {
            endTok = limitTok;
        }
        AstNode *node = this->makeSpanNode(kind, startTok, endTok);
        *nextTok = endTok + 1;
        return node;
    }

    inline AstNode *makeSpanNode(AstNode::Kind kind, int firstTok, int lastTok)
    {
        AstNode *node = m_arena->createNode(kind);
        this->setTokenSpan(node, firstTok, lastTok);
        return node;
    }

    inline AstNode *parseExpression(int firstTok, int lastTok)
    {
        if (firstTok > lastTok) {
            return m_arena->createNode(AstNode::RawSpan);
        }
        if (firstTok == lastTok) {
            const Token t = this->tokenAt(firstTok);
            AstNode *leaf = m_arena->createNode(
                (t == INTEGER_LITERAL
                    || t == FLOATING_LITERAL
                    || t == STRING_LITERAL
                    || t == CHARACTER_LITERAL
                    || t == BOOLEAN_LITERAL)
                    ? AstNode::LiteralExpr : AstNode::IdentExpr
            );
            leaf->name = m_symbols.at(firstTok).lexem();
            this->setTokenSpan(leaf, firstTok, firstTok);
            return leaf;
        }
        return this->parseBinaryOrTernary(firstTok, lastTok);
    }

    inline AstNode *parseBinaryOrTernary(int firstTok, int lastTok)
    {
        // Finds lowest-precedence top-level operator
        // (assignment/binary/ternary).
        int lowestOpTok = -1;
        int lowestPrec = 999;
        int questionTok = -1;
        int parenDepth = 0;
        int bracketDepth = 0;
        int braceDepth = 0;
        int ternaryDepth = 0;

        for (int k = firstTok; k <= lastTok; ++k) {
            const Token t = this->tokenAt(k);
            if (t == LPAREN) {
                ++parenDepth;
                continue;
            }
            if (t == RPAREN) {
                if (parenDepth > 0) {
                    --parenDepth;
                }
                continue;
            }
            if (t == LBRACK) {
                ++bracketDepth;
                continue;
            }
            if (t == RBRACK) {
                if (bracketDepth > 0) {
                    --bracketDepth;
                }
                continue;
            }
            if (t == LBRACE) {
                ++braceDepth;
                continue;
            }
            if (t == RBRACE) {
                if (braceDepth > 0) {
                    --braceDepth;
                }
                continue;
            }
            if (parenDepth > 0 || bracketDepth > 0 || braceDepth > 0) {
                continue;
            }
            if (t == QUESTION) {
                if (ternaryDepth == 0 && questionTok < 0) {
                    questionTok = k;
                }
                ++ternaryDepth;
                continue;
            }
            if (t == COLON && ternaryDepth > 0) {
                --ternaryDepth;
                if (ternaryDepth == 0
                    && questionTok >= 0
                    && lowestPrec > 1
                ) {
                    AstNode *tern = m_arena->createNode(AstNode::TernaryExpr);
                    tern->opToken = questionTok;
                    tern->altToken = k;
                    if (questionTok > firstTok) {
                        tern->cond = this->parseExpression(
                            firstTok,
                            questionTok - 1
                        );
                        tern->addChild(tern->cond);
                    }
                    if (k - questionTok > 1) {
                        tern->body = this->parseExpression(
                            questionTok + 1,
                            k - 1
                        );
                        tern->addChild(tern->body);
                    }
                    if (lastTok > k) {
                        tern->elseBranch = this->parseExpression(
                            k + 1,
                            lastTok
                        );
                        tern->addChild(tern->elseBranch);
                    }
                    this->setTokenSpan(tern, firstTok, lastTok);
                    return tern;
                }
                continue;
            }
            if (ternaryDepth > 0) {
                continue;
            }
            const int prec = binaryPrecedence(t);
            if (prec > 0) {
                // Unary leading `+`, `-`, `*`, `&` at start of expression or
                // right after another operator is NOT a binary split.
                if (k == firstTok) {
                    continue;
                }
                const Token prev = this->tokenAt(k - 1);
                if (binaryPrecedence(prev) > 0
                    || prev == QUESTION
                    || prev == COLON
                    || prev == NOT
                    || prev == TILDE
                    || prev == RETURN
                ) {
                    continue;
                }
                // Right-associative assignment picks leftmost `=`; left-assoc
                // picks rightmost lowest-precedence operator.
                const bool take = (prec == 1) ? (prec < lowestPrec)
                                              : (prec <= lowestPrec);
                if (take) {
                    lowestPrec = prec;
                    lowestOpTok = k;
                }
            }
        }

        if (lowestOpTok > firstTok && lowestOpTok < lastTok) {
            AstNode *bin = m_arena->createNode(AstNode::BinaryExpr);
            bin->opToken = lowestOpTok;
            bin->opText = m_symbols.at(lowestOpTok).lexem();
            bin->lhs = this->parseExpression(firstTok, lowestOpTok - 1);
            bin->rhs = this->parseExpression(lowestOpTok + 1, lastTok);
            bin->addChild(bin->lhs);
            bin->addChild(bin->rhs);
            this->setTokenSpan(bin, firstTok, lastTok);
            return bin;
        }

        return this->parsePostfixOrPrimary(firstTok, lastTok);
    }

    inline AstNode *parsePostfixOrPrimary(int firstTok, int lastTok)
    {
        const Token first = this->tokenAt(firstTok);
        if (first == NOT
            || first == TILDE
            || first == PLUS
            || first == MINUS
            || first == INCR
            || first == DECR
        ) {
            AstNode *un = m_arena->createNode(AstNode::UnaryExpr);
            un->opToken = firstTok;
            un->isPrefix = true;
            un->inner = this->parsePostfixOrPrimary(firstTok + 1, lastTok);
            un->addChild(un->inner);
            this->setTokenSpan(un, firstTok, lastTok);
            return un;
        }

        const Token last = this->tokenAt(lastTok);
        if (last == RPAREN) {
            const int lparen = this->findOpeningBefore(
                firstTok,
                lastTok,
                LPAREN,
                RPAREN
            );
            if (lparen == firstTok) {
                AstNode *p = m_arena->createNode(AstNode::ParenExpr);
                p->openToken = lparen;
                p->closeToken = lastTok;
                if (lastTok - lparen > 1) {
                    p->inner = this->parseExpression(
                        lparen + 1,
                        lastTok - 1
                    );
                    p->addChild(p->inner);
                }
                this->setTokenSpan(p, firstTok, lastTok);
                return p;
            }
            if (lparen > firstTok) {
                AstNode *call = m_arena->createNode(AstNode::CallExpr);
                call->openToken = lparen;
                call->closeToken = lastTok;
                call->lhs = this->parsePostfixOrPrimary(
                    firstTok,
                    lparen - 1
                );
                call->addChild(call->lhs);
                if (call->lhs && call->lhs->name == "connect") {
                    call->isConnectCall = true;
                }
                if (lastTok - lparen > 1) {
                    this->parseCommaItems(
                        lparen + 1,
                        lastTok - 1,
                        AstNode::RawSpan,
                        &call->items,
                        call
                    );
                }
                this->setTokenSpan(call, firstTok, lastTok);
                return call;
            }
        }

        if (last == RBRACK) {
            const int lbrack = this->findOpeningBefore(
                firstTok,
                lastTok,
                LBRACK,
                RBRACK
            );
            if (lbrack > firstTok) {
                AstNode *sub = m_arena->createNode(AstNode::SubscriptExpr);
                sub->openToken = lbrack;
                sub->closeToken = lastTok;
                sub->lhs = this->parsePostfixOrPrimary(
                    firstTok,
                    lbrack - 1
                );
                sub->addChild(sub->lhs);
                if (lastTok - lbrack > 1) {
                    sub->inner = this->parseExpression(
                        lbrack + 1,
                        lastTok - 1
                    );
                    sub->addChild(sub->inner);
                }
                this->setTokenSpan(sub, firstTok, lastTok);
                return sub;
            }
        }

        if (last == RBRACE) {
            const int lbrace = this->findOpeningBefore(
                firstTok,
                lastTok,
                LBRACE,
                RBRACE
            );
            if (lbrace == firstTok) {
                AstNode *init = m_arena->createNode(AstNode::InitListExpr);
                init->openToken = lbrace;
                init->closeToken = lastTok;
                if (lastTok - lbrace > 1) {
                    this->parseCommaItems(
                        lbrace + 1,
                        lastTok - 1,
                        AstNode::RawSpan,
                        &init->items,
                        init
                    );
                }
                this->setTokenSpan(init, firstTok, lastTok);
                return init;
            }
            if (lbrace > firstTok && first == LBRACK) {
                AstNode *lam = m_arena->createNode(AstNode::LambdaExpr);
                lam->openToken = lbrace;
                lam->closeToken = lastTok;
                if (lastTok - lbrace > 1) {
                    lam->body = this->parseCompound(lbrace, lastTok);
                    lam->addChild(lam->body);
                }
                this->setTokenSpan(lam, firstTok, lastTok);
                return lam;
            }
        }

        // Splits trailing `.member` / `->member` for chain expressions.
        int depth = 0;
        for (int k = lastTok; k > firstTok; --k) {
            const Token t = this->tokenAt(k);
            if (t == RPAREN || t == RBRACK || t == RBRACE) {
                ++depth;
            } else if ((t == LPAREN
                    || t == LBRACK
                    || t == LBRACE)
                && depth > 0
            ) {
                --depth;
            } else if (depth == 0
                && (t == DOT
                    || t == ARROW)
                && k < lastTok
            ) {
                AstNode *mem = m_arena->createNode(AstNode::MemberExpr);
                mem->opToken = k;
                mem->opText = m_symbols.at(k).lexem();
                mem->lhs = this->parsePostfixOrPrimary(firstTok, k - 1);
                mem->rhs = this->makeSpanNode(
                    AstNode::IdentExpr,
                    k + 1,
                    lastTok
                );
                mem->name = m_symbols.at(k + 1).lexem();
                mem->addChild(mem->lhs);
                mem->addChild(mem->rhs);
                this->setTokenSpan(mem, firstTok, lastTok);
                return mem;
            }
        }

        if (isCastKeyword(first) && lastTok > firstTok + 3) {
            AstNode *cast = this->makeSpanNode(
                AstNode::CastExpr,
                firstTok,
                lastTok
            );
            cast->name = m_symbols.at(firstTok).lexem();
            return cast;
        }

        return this->makeSpanNode(AstNode::RawSpan, firstTok, lastTok);
    }

    inline int findOpeningBefore(
            int firstTok,
            int closeTok,
            Token openTok,
            Token closeToken) const
    {
        int depth = 0;
        for (int k = closeTok; k >= firstTok; --k) {
            const Token t = this->tokenAt(k);
            if (t == closeToken) {
                ++depth;
            } else if (t == openTok) {
                --depth;
                if (depth == 0) {
                    return k;
                }
            }
        }
        return -1;
    }

    const QByteArray & m_text;
    const Symbols & m_symbols;
    const QVector<int > & m_lineStarts;
    const QVector<bool > & m_directiveLines;
    const QVector<int > & m_matches;
    FormatLanguage::Id m_language;
    AstArena *m_arena;
};

} // namespace

/*!
    @class AstParser
    @internal

    Builds a syntax tree of declarations, statements, and expressions from lexed
    symbols and attaches comments in source order.
*/

/*!
    Parses @a symbols extracted from @a text into an AST anchored in @a arena
    and attaches every comment in @a comments onto its enclosing, preceding, or
    following AST node.
*/
AstParseResult AstParser::parse(
        const QByteArray & text,
        const Symbols & symbols,
        const QList<FormatComment > & comments,
        const QVector<int > & lineStarts,
        const QVector<bool > & directiveLines,
        const QVector<int > & matches,
        FormatLanguage::Id language,
        AstArena *arena)
{
    AstParseResult result;
    if ( ! arena) {
        return result;
    }

    result.comments.reserve(comments.size());
    for (int i = 0; i < comments.size(); ++i) {
        const FormatComment & src = comments.at(i);
        AstComment *ac = arena->createComment();
        ac->index = i;
        ac->offset = src.offset;
        ac->length = src.length;
        ac->line = lineOfOffset(lineStarts, src.offset);
        ac->endLine = lineOfOffset(
            lineStarts,
            src.offset + qMax(0, src.length - 1)
        );
        ac->kind = src.kind;
        ac->trailingHint = src.trailing;
        ac->text = text.mid(src.offset, src.length);
        result.comments.append(ac);
    }

    if ( ! FormatLanguageInfo::supportsCodeFormatting(language)
        || symbols.isEmpty()
    ) {
        result.root = arena->createNode(AstNode::TranslationUnit);
        result.root->setSpan(0, text.size(), -1, -1);
    } else {
        ParserContext ctx(
            text,
            symbols,
            lineStarts,
            directiveLines,
            matches,
            language,
            arena
        );
        result.root = ctx.parseTranslationUnit(
            0,
            symbols.size() - 1,
            AstNode::TranslationUnit
        );
    }

    AstCommentAttacher::attach(result.root, &result.comments, text);
    return result;
}
