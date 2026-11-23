// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef CONFIG_EVALUATOR_CONTEXT_H
#define CONFIG_EVALUATOR_CONTEXT_H

#include <QtCore/QVariant>
#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QFlags>

class ExpressionCompiler;

namespace Config {

enum NameMismatch {
    NameMismatchIgnore,
    NameMismatchWarn,
    NameMismatchError,
    NameMismatchCritical
};

enum EvalFlag {
    FailedToCompile = 0x0005,
    Fallback = 0x0028,
    FailedToResolve = 0x0140,
    FailedToFallback = 0x0a00,
    Resolved = 0x5000
};
Q_DECLARE_FLAGS(EvalRes, EvalFlag)
Q_DECLARE_OPERATORS_FOR_FLAGS(EvalRes)

enum CompileFlag {
    Compiled = 0x05,
    Mismatched = 0x28
};
Q_DECLARE_FLAGS(CompileRes, CompileFlag)
Q_DECLARE_OPERATORS_FOR_FLAGS(CompileRes)

class Condition;
class Choice;
class Schema;
class Settings;

namespace Evaluator {

class CompileContext
{
public:
    inline explicit CompileContext(const QStringList & names = QStringList(),
                                   const QStringList *enumKeys = Q_NULLPTR,
                                   int mismatch = NameMismatchIgnore)
        : m_names(names), m_isOnNamesCalled(false)
        , m_enumKeys(enumKeys && enumKeys->isEmpty()
            ? anyEnumKeys() : enumKeys ? *enumKeys : QStringList())
        , m_isOnEnumKeysCalled(false)
        , m_mismatchMode(mismatch) {}
    virtual ~CompileContext() {}
    QStringList names() const;
    QStringList enumKeys() const;
    inline static QStringList anyEnumKeys() { return QStringList() << QString(); }
    inline QString lastErrorForCompile() const { return m_lastCE; }
    Q_ALWAYS_INLINE bool lastCompiled() const { return m_lastCompile.includes(Compiled); }
    Q_ALWAYS_INLINE bool mismatched() const { return m_lastCompile.includes(Mismatched); }
    Q_ALWAYS_INLINE int mismatchMode() const { return m_mismatchMode; }
    Q_ALWAYS_INLINE void setMismatchMode(int mode) const { m_mismatchMode = mode; }

protected:
    virtual QStringList onNames() const;
    virtual QStringList onEnumKeys() const;
    mutable QStringList m_names;
    mutable bool m_isOnNamesCalled;
    mutable QStringList m_enumKeys;
    mutable bool m_isOnEnumKeysCalled;

private:
    friend class ::Config::Condition;
    friend class ::Config::Schema;
    friend class ::ExpressionCompiler;
    mutable int m_mismatchMode;
    mutable CompileRes m_lastCompile;
    mutable QString m_lastCE;
};

class MismatchScope
{
    Q_DISABLE_COPY(MismatchScope)
public:
    Q_ALWAYS_INLINE MismatchScope(const CompileContext *context, int mode)
        : m_context(context), m_mode(mode), m_previousMode(0), m_open(false)
    { this->open(); }
    Q_ALWAYS_INLINE ~MismatchScope() { this->close(); }
    Q_ALWAYS_INLINE void open()
    {
        if ( ! m_open) {
            m_previousMode = m_context->mismatchMode();
            m_context->setMismatchMode(m_mode);
            m_open = true;
        }
    }
    Q_ALWAYS_INLINE void close()
    {
        if (m_open) {
            m_context->setMismatchMode(m_previousMode);
            m_open = false;
        }
    }
private:
    const CompileContext *m_context;
    int m_mode;
    int m_previousMode;
    bool m_open;
};

class Context : public CompileContext
{
    typedef CompileContext super;
public:
    inline explicit Context(bool hasPlace = true)
        : m_lastResolvePassed(false), m_hasPlace(hasPlace) {}
    virtual ~Context() {}
    inline EvalRes lastEvalResult() const { return m_lastEvalResult; }
    inline bool lastResolvePassed() const { return m_lastResolvePassed; }
    inline QString lastError() const { return m_lastError; }
    inline bool hasPlace() const { return m_hasPlace; }
    virtual bool predicate(const QString &name, bool *value) const;
    virtual const Settings *settings() const;
    virtual QVariant resolveAtPercent(const QStringList & program, int percent) const;

private:
    friend class ::Config::Condition;
    friend class ::Config::Choice;
    friend class ::Config::Schema;
    mutable EvalRes m_lastEvalResult;
    mutable bool m_lastResolvePassed;
    mutable QString m_lastError;
    bool m_hasPlace;
};

} // Namespace `Evaluator`.

} // Namespace `Config`.

#endif // CONFIG_EVALUATOR_CONTEXT_H
