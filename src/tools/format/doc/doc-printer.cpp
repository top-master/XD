// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./doc-printer.h"

#include <QtCore/QHash>
#include <QtCore/QSet>
#include <QtCore/QSharedPointer>

namespace {

enum PrintMode {
    ModeBreak = 1,
    ModeFlat = 2
};

struct IndentCommand
{
    enum Type {
        TabIndent = 0,
        WidthSpaces = 2,
        LiteralString = 3
    };

    inline IndentCommand(Type t = TabIndent,
                         int w = 0,
                         const QByteArray & s = QByteArray())
        : type(t), width(w), text(s)
    {
    }

    Type type;
    int width;
    QByteArray text;
};

struct IndentRootSnapshot
{
    QByteArray value;
    int length;
    QVector<IndentCommand > queue;
};

struct IndentState
{
    inline IndentState()
        : length(0), hasRoot(false)
    {
        this->root.length = 0;
    }

    QByteArray value;
    int length;
    QVector<IndentCommand > queue;
    bool hasRoot;
    IndentRootSnapshot root;
};

IndentState rebuildIndent(
        const QVector<IndentCommand > & queue,
        bool hasRoot,
        const IndentRootSnapshot & root,
        const DocPrinter::Options & options)
{
    IndentState next;
    next.queue = queue;
    next.hasRoot = hasRoot;
    next.root = root;

    int lastTabs = 0;
    int lastSpaces = 0;

    for (int i = 0; i < queue.size(); ++i) {
        const IndentCommand & cmd = queue.at(i);
        switch (cmd.type) {
        case IndentCommand::TabIndent: {
            if (options.useTabs) {
                if (lastTabs > 0) {
                    next.value.append(QByteArray(lastTabs, '\t'));
                    next.length += options.tabWidth * lastTabs;
                }
                lastTabs = 0;
                lastSpaces = 0;
                next.value.append('\t');
                next.length += options.tabWidth;
            } else {
                if (lastSpaces > 0) {
                    next.value.append(QByteArray(lastSpaces, ' '));
                    next.length += lastSpaces;
                }
                lastTabs = 0;
                lastSpaces = 0;
                next.value.append(QByteArray(options.tabWidth, ' '));
                next.length += options.tabWidth;
            }
            break;
        }
        case IndentCommand::LiteralString: {
            if (options.useTabs && lastTabs > 0) {
                next.value.append(QByteArray(lastTabs, '\t'));
                next.length += options.tabWidth * lastTabs;
            } else if ( ! options.useTabs && lastSpaces > 0) {
                next.value.append(QByteArray(lastSpaces, ' '));
                next.length += lastSpaces;
            }
            lastTabs = 0;
            lastSpaces = 0;
            next.value.append(cmd.text);
            next.length += cmd.text.size();
            break;
        }
        case IndentCommand::WidthSpaces:
            lastTabs += 1;
            lastSpaces += cmd.width;
            break;
        }
    }

    if (lastSpaces > 0) {
        next.value.append(QByteArray(lastSpaces, ' '));
        next.length += lastSpaces;
    }
    return next;
}

IndentState makeIndented(const IndentState & state,
                         const DocPrinter::Options & options)
{
    QVector<IndentCommand > queue = state.queue;
    queue.append(IndentCommand(IndentCommand::TabIndent));
    return rebuildIndent(queue, state.hasRoot, state.root, options);
}

IndentState makeAligned(const IndentState & state,
                        const DocNode *alignNode,
                        const DocPrinter::Options & options)
{
    if (alignNode->alignKind == DocNode::AlignMarkRoot) {
        IndentState marked = state;
        marked.hasRoot = true;
        marked.root.value = state.value;
        marked.root.length = state.length;
        marked.root.queue = state.queue;
        return marked;
    }
    if (alignNode->alignKind == DocNode::AlignResetRoot) {
        if ( ! state.hasRoot) {
            return IndentState();
        }
        IndentState restored;
        restored.value = state.root.value;
        restored.length = state.root.length;
        restored.queue = state.root.queue;
        restored.hasRoot = state.hasRoot;
        restored.root = state.root;
        return restored;
    }
    if (alignNode->alignKind == DocNode::AlignDedent) {
        QVector<IndentCommand > queue = state.queue;
        if ( ! queue.isEmpty()) {
            queue.remove(queue.size() - 1);
        }
        return rebuildIndent(queue, state.hasRoot, state.root, options);
    }
    if (alignNode->alignKind == DocNode::AlignString) {
        QVector<IndentCommand > queue = state.queue;
        queue.append(
            IndentCommand(IndentCommand::LiteralString, 0, alignNode->text)
        );
        return rebuildIndent(queue, state.hasRoot, state.root, options);
    }
    QVector<IndentCommand > queue = state.queue;
    queue.append(
        IndentCommand(IndentCommand::WidthSpaces, alignNode->alignWidth)
    );
    return rebuildIndent(queue, state.hasRoot, state.root, options);
}

struct PrintCommand
{
    inline PrintCommand()
        : mode(ModeFlat), doc(Q_NULLPTR), fillIndex(0)
    {
    }

    inline PrintCommand(
            const IndentState & ind,
            PrintMode m,
            const DocNode *d,
            int fillIdx = 0)
        : indent(ind), mode(m), doc(d), fillIndex(fillIdx)
    {
    }

    IndentState indent;
    PrintMode mode;
    const DocNode *doc;
    int fillIndex;
};

int trimTrailingSpaces(QByteArray *out)
{
    int removed = 0;
    int i = out->size() - 1;
    while (i >= 0 && (out->at(i) == ' ' || out->at(i) == '\t')) {
        ++removed;
        --i;
    }
    if (removed > 0) {
        out->chop(removed);
    }
    return removed;
}

struct FitsEntry
{
    inline FitsEntry(PrintMode m = ModeFlat,
                     const DocNode *d = Q_NULLPTR,
                     int fillIdx = 0)
        : mode(m), doc(d), fillIndex(fillIdx)
    {
    }

    PrintMode mode;
    const DocNode *doc;
    int fillIndex;
};

bool fits(
        const PrintCommand & next,
        const QVector<PrintCommand > & restCommands,
        int width,
        bool hasLineSuffix,
        const QHash<QByteArray, PrintMode > & groupModeMap,
        bool mustBeFlat = false)
{
    if (width == 0 && next.doc && next.doc->kind != DocNode::Line) {
        // Zero columns can still fit empty or whitespace-trimmable groups.
    }

    int restIdx = restCommands.size();
    QVector<FitsEntry > cmds;
    cmds.reserve(32);
    cmds.append(FitsEntry(next.mode, next.doc, next.fillIndex));

    QByteArray out;

    while (width >= 0) {
        if (cmds.isEmpty()) {
            if (restIdx == 0) {
                return true;
            }
            --restIdx;
            const PrintCommand & rc = restCommands.at(restIdx);
            cmds.append(FitsEntry(rc.mode, rc.doc, rc.fillIndex));
            continue;
        }

        const FitsEntry entry = cmds.last();
        cmds.pop_back();
        const DocNode *doc = entry.doc;
        if ( ! doc) {
            continue;
        }
        const PrintMode mode = entry.mode;

        switch (doc->kind) {
        case DocNode::Text: {
            out.append(doc->text);
            width -= doc->text.size();
            break;
        }
        case DocNode::Verbatim: {
            if (doc->text.contains('\n') || doc->text.contains('\r')) {
                return false;
            }
            out.append(doc->text);
            width -= doc->text.size();
            break;
        }
        case DocNode::Concat: {
            for (int i = doc->parts.size() - 1; i >= 0; --i) {
                cmds.append(FitsEntry(mode, doc->parts.at(i)));
            }
            break;
        }
        case DocNode::Fill: {
            for (int i = doc->parts.size() - 1; i >= entry.fillIndex; --i) {
                cmds.append(FitsEntry(mode, doc->parts.at(i)));
            }
            break;
        }
        case DocNode::Indent:
        case DocNode::Align:
        case DocNode::IndentIfBreak:
        case DocNode::Label: {
            cmds.append(FitsEntry(mode, doc->contents));
            break;
        }
        case DocNode::Trim: {
            const int trimmed = trimTrailingSpaces(&out);
            width += trimmed;
            break;
        }
        case DocNode::Group: {
            if (mustBeFlat && doc->breakState != DocNode::BreakNone) {
                return false;
            }
            const PrintMode groupMode =
                doc->breakState != DocNode::BreakNone
                ? ModeBreak : mode;
            const DocNode *contents =
                ( ! doc->expandedStates.isEmpty() && groupMode == ModeBreak)
                ? doc->expandedStates.last() : doc->contents;
            cmds.append(FitsEntry(groupMode, contents));
            break;
        }
        case DocNode::IfBreak: {
            const PrintMode targetMode = doc->id.isEmpty()
                ? mode : groupModeMap.value(doc->id, ModeFlat);
            const DocNode *branch = targetMode == ModeBreak ? doc->breakContents
                                                            : doc->flatContents;
            cmds.append(FitsEntry(mode, branch));
            break;
        }
        case DocNode::Line: {
            if (mode == ModeFlat || doc->soft) {
                if ( ! doc->hard) {
                    if ( ! doc->soft) {
                        out.append(' ');
                        width -= 1;
                    }
                    break;
                }
                return ! mustBeFlat;
            }
            return ! mustBeFlat;
        }
        case DocNode::LineSuffix: {
            hasLineSuffix = true;
            break;
        }
        case DocNode::LineSuffixBoundary: {
            if (hasLineSuffix) {
                return false;
            }
            break;
        }
        case DocNode::BreakParent:
            break;
        }
    }
    return false;
}

void breakParentGroup(QVector<const DocNode * > & groupStack)
{
    if ( ! groupStack.isEmpty()) {
        const DocNode *parent = groupStack.last();
        if (parent->expandedStates.isEmpty()
            && parent->breakState == DocNode::BreakNone
        ) {
            parent->breakState = DocNode::BreakPropagated;
        }
    }
}

struct TraverseFrame
{
    inline TraverseFrame(const DocNode *n = Q_NULLPTR, bool ex = false)
        : node(n), exiting(ex)
    {
    }

    const DocNode *node;
    bool exiting;
};

void pushChild(QVector<TraverseFrame > & stack, const DocNode *child)
{
    if (child) {
        stack.append(TraverseFrame(child, false));
    }
}

} // namespace

/*!
    @class DocPrinter
    @internal

    Converts an arena-built `DocNode` layout tree into formatted bytes in one
    iterative pass without mutating source text incrementally.
*/

/*!
    Propagates `BreakParent` directives up through enclosing non-conditional
    groups so outer layouts break consistently when an inner construct forces a
    line break.
*/
void DocPrinter::propagateBreaks(const DocNode *root)
{
    if ( ! root) {
        return;
    }
    QSet<const DocNode * > visitedGroups;
    QVector<const DocNode * > groupStack;
    QVector<TraverseFrame > stack;
    stack.reserve(128);
    stack.append(TraverseFrame(root, false));

    while ( ! stack.isEmpty()) {
        const TraverseFrame frame = stack.last();
        stack.pop_back();
        const DocNode *node = frame.node;
        if ( ! node) {
            continue;
        }

        if (frame.exiting) {
            if (node->kind == DocNode::Group && ! groupStack.isEmpty()) {
                const DocNode *group = groupStack.last();
                groupStack.pop_back();
                if (group->breakState != DocNode::BreakNone) {
                    breakParentGroup(groupStack);
                }
            }
            continue;
        }

        if (node->kind == DocNode::BreakParent) {
            breakParentGroup(groupStack);
            continue;
        }

        if (node->kind == DocNode::Group) {
            stack.append(TraverseFrame(node, true));
            groupStack.append(node);
            if (visitedGroups.contains(node)) {
                continue;
            }
            visitedGroups.insert(node);
        }

        switch (node->kind) {
        case DocNode::Concat:
        case DocNode::Fill:
            for (int i = node->parts.size() - 1; i >= 0; --i) {
                pushChild(stack, node->parts.at(i));
            }
            break;
        case DocNode::Indent:
        case DocNode::Align:
        case DocNode::IndentIfBreak:
        case DocNode::LineSuffix:
        case DocNode::Label:
            pushChild(stack, node->contents);
            break;
        case DocNode::Group:
            for (int i = node->expandedStates.size() - 1; i >= 0; --i) {
                pushChild(stack, node->expandedStates.at(i));
            }
            if (node->expandedStates.isEmpty()) {
                pushChild(stack, node->contents);
            }
            break;
        case DocNode::IfBreak:
            pushChild(stack, node->flatContents);
            pushChild(stack, node->breakContents);
            break;
        default:
            break;
        }
    }
}

/*!
    Prints @a root using @a options and returns the formatted UTF-8/byte output.
*/
QByteArray DocPrinter::print(const DocNode *root, const Options & options)
{
    if ( ! root) {
        return QByteArray();
    }

    DocPrinter::propagateBreaks(root);

    DocNode hardLineNoBreak(DocNode::Line);
    hardLineNoBreak.hard = true;

    QVector<PrintCommand > cmds;
    cmds.reserve(256);
    cmds.append(PrintCommand(IndentState(), ModeBreak, root));

    QVector<PrintCommand > lineSuffix;
    QHash<QByteArray, PrintMode > groupModeMap;

    QByteArray out;
    out.reserve(512);
    int pos = 0;
    bool shouldRemeasure = false;

    while ( ! cmds.isEmpty()) {
        const PrintCommand cmd = cmds.last();
        cmds.pop_back();
        const DocNode *doc = cmd.doc;
        if ( ! doc) {
            continue;
        }
        const PrintMode mode = cmd.mode;

        switch (doc->kind) {
        case DocNode::Text: {
            out.append(doc->text);
            pos += doc->text.size();
            break;
        }
        case DocNode::Verbatim: {
            out.append(doc->text);
            const int lastLf = doc->text.lastIndexOf('\n');
            const int lastCr = doc->text.lastIndexOf('\r');
            const int lastNl = qMax(lastLf, lastCr);
            if (lastNl >= 0) {
                pos = doc->text.size() - (lastNl + 1);
            } else {
                pos += doc->text.size();
            }
            break;
        }
        case DocNode::Concat: {
            for (int i = doc->parts.size() - 1; i >= 0; --i) {
                cmds.append(PrintCommand(
                    cmd.indent,
                    mode,
                    doc->parts.at(i)
                ));
            }
            break;
        }
        case DocNode::Indent: {
            cmds.append(
                PrintCommand(
                    makeIndented(cmd.indent, options),
                    mode,
                    doc->contents
                )
            );
            break;
        }
        case DocNode::Align: {
            cmds.append(
                PrintCommand(
                    makeAligned(cmd.indent, doc, options),
                    mode,
                    doc->contents
                )
            );
            break;
        }
        case DocNode::Trim: {
            pos -= trimTrailingSpaces(&out);
            if (pos < 0) {
                pos = 0;
            }
            break;
        }
        case DocNode::Group: {
            PrintMode chosenMode = ModeBreak;
            const DocNode *chosenContents = doc->contents;

            if (mode == ModeFlat && ! shouldRemeasure) {
                chosenMode = doc->breakState != DocNode::BreakNone ? ModeBreak
                                                                   : ModeFlat;
            } else {
                shouldRemeasure = false;
                const PrintCommand flatCandidate(
                    cmd.indent, ModeFlat, doc->contents
                );
                if (doc->breakState == DocNode::BreakNone
                    && fits(
                        flatCandidate,
                        cmds,
                        options.printWidth - pos,
                        ! lineSuffix.isEmpty(),
                        groupModeMap)
                ) {
                    chosenMode = ModeFlat;
                } else if ( ! doc->expandedStates.isEmpty()) {
                    const int lastIdx = doc->expandedStates.size() - 1;
                    chosenContents = doc->expandedStates.at(lastIdx);
                    chosenMode = ModeBreak;
                    for (int i = 0; i <= lastIdx; ++i) {
                        const PrintMode stateMode = (i == lastIdx) ? ModeBreak
                                                                   : ModeFlat;
                        const DocNode *stateDoc = doc->expandedStates.at(i);
                        const PrintCommand candidate(
                            cmd.indent, stateMode, stateDoc
                        );
                        if (i == lastIdx
                            || fits(
                                candidate,
                                cmds,
                                options.printWidth - pos,
                                ! lineSuffix.isEmpty(),
                                groupModeMap)
                        ) {
                            chosenContents = stateDoc;
                            chosenMode = stateMode;
                            break;
                        }
                    }
                } else {
                    chosenMode = ModeBreak;
                }
            }

            if ( ! doc->id.isEmpty()) {
                groupModeMap[doc->id] = chosenMode;
            }
            cmds.append(PrintCommand(
                cmd.indent,
                chosenMode,
                chosenContents
            ));
            break;
        }
        case DocNode::Fill: {
            const int idx = cmd.fillIndex;
            const QVector<const DocNode * > & parts = doc->parts;
            const int remainingCount = parts.size() - idx;
            if (remainingCount <= 0) {
                break;
            }
            const int remainingWidth = options.printWidth - pos;
            const DocNode *content = parts.at(idx);
            const PrintCommand contentFlat(cmd.indent, ModeFlat, content);
            const PrintCommand contentBreak(cmd.indent, ModeBreak, content);

            if (remainingCount == 1) {
                if (fits(
                    contentFlat,
                    cmds,
                    remainingWidth,
                    ! lineSuffix.isEmpty(),
                    groupModeMap
                )) {
                    cmds.append(contentFlat);
                } else {
                    cmds.append(contentBreak);
                }
                break;
            }

            const DocNode *whitespace = parts.at(idx + 1);
            const PrintCommand wsFlat(cmd.indent, ModeFlat, whitespace);
            const PrintCommand wsBreak(cmd.indent, ModeBreak, whitespace);

            if (remainingCount == 2) {
                QVector<PrintCommand > emptyRest;
                const bool contentFits = fits(
                    contentFlat,
                    emptyRest,
                    remainingWidth,
                    ! lineSuffix.isEmpty(),
                    groupModeMap
                );
                if (contentFits) {
                    cmds.append(wsFlat);
                    cmds.append(contentFlat);
                } else {
                    cmds.append(wsBreak);
                    cmds.append(contentBreak);
                }
                break;
            }

            DocNode pairNode(DocNode::Concat);
            pairNode.parts.append(content);
            pairNode.parts.append(whitespace);

            DocNode tripleNode(DocNode::Concat);
            tripleNode.parts.append(content);
            tripleNode.parts.append(whitespace);
            tripleNode.parts.append(parts.at(idx + 2));

            QVector<PrintCommand > emptyRest;
            const bool contentAndWsFit = fits(
                PrintCommand(cmd.indent, ModeFlat, &pairNode),
                emptyRest,
                remainingWidth,
                ! lineSuffix.isEmpty(),
                groupModeMap,
                true
            );
            const bool nextPairFits = fits(
                PrintCommand(cmd.indent, ModeFlat, &tripleNode),
                emptyRest,
                remainingWidth,
                ! lineSuffix.isEmpty(),
                groupModeMap,
                true
            );

            cmds.append(PrintCommand(cmd.indent, mode, doc, idx + 2));
            if (nextPairFits) {
                cmds.append(wsFlat);
                cmds.append(contentFlat);
            } else if (contentAndWsFit) {
                cmds.append(wsBreak);
                cmds.append(contentFlat);
            } else {
                cmds.append(wsBreak);
                cmds.append(contentBreak);
            }
            break;
        }
        case DocNode::IfBreak: {
            const PrintMode targetMode = doc->id.isEmpty()
                ? mode : groupModeMap.value(doc->id, ModeFlat);
            const DocNode *branch = targetMode == ModeBreak ? doc->breakContents
                                                            : doc->flatContents;
            cmds.append(PrintCommand(cmd.indent, mode, branch));
            break;
        }
        case DocNode::IndentIfBreak: {
            const PrintMode groupMode = groupModeMap.value(
                doc->id,
                ModeFlat
            );
            const bool shouldIndent = doc->negate ? (groupMode == ModeFlat)
                                                  : (groupMode == ModeBreak);
            const IndentState nextIndent = shouldIndent
                ? makeIndented(cmd.indent, options) : cmd.indent;
            cmds.append(PrintCommand(nextIndent, mode, doc->contents));
            break;
        }
        case DocNode::LineSuffix: {
            lineSuffix.append(PrintCommand(cmd.indent, mode, doc->contents));
            break;
        }
        case DocNode::LineSuffixBoundary: {
            if ( ! lineSuffix.isEmpty()) {
                cmds.append(PrintCommand(
                    cmd.indent,
                    mode,
                    &hardLineNoBreak
                ));
            }
            break;
        }
        case DocNode::Line: {
            if (mode == ModeFlat) {
                if ( ! doc->hard) {
                    if ( ! doc->soft) {
                        out.append(' ');
                        ++pos;
                    }
                    break;
                }
                shouldRemeasure = true;
            }
            if ( ! lineSuffix.isEmpty()) {
                cmds.append(cmd);
                for (int i = lineSuffix.size() - 1; i >= 0; --i) {
                    cmds.append(lineSuffix.at(i));
                }
                lineSuffix.clear();
                break;
            }
            if (doc->literal) {
                out.append('\n');
                pos = 0;
            } else {
                trimTrailingSpaces(&out);
                out.append(options.endOfLine);
                out.append(cmd.indent.value);
                pos = cmd.indent.length;
            }
            break;
        }
        case DocNode::Label: {
            cmds.append(PrintCommand(cmd.indent, mode, doc->contents));
            break;
        }
        case DocNode::BreakParent:
            break;
        }

        if (cmds.isEmpty() && ! lineSuffix.isEmpty()) {
            for (int i = lineSuffix.size() - 1; i >= 0; --i) {
                cmds.append(lineSuffix.at(i));
            }
            lineSuffix.clear();
        }
    }

    return out;
}
