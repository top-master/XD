#ifndef LIB_FLAG_ITERATOR_H
#define LIB_FLAG_ITERATOR_H

#include <proitems.h>
#include <makefiledeps.h>
#include <project.h>
#include <option.h>

QT_BEGIN_NAMESPACE

/**
 * TODO: maybe replace `parseLibFlag(...)` usages with this class,
 * like we did in `winmakefile.cpp` file.
 */
class LibFlagIterator
{
public:
    /// WARNING: does not take ownership of given `folders`
    /// (caller should delete it).
    inline LibFlagIterator(
            QMakeProject &project, ProStringList &projectVariable,
            QList<QMakeLocalFileName> *folders = Q_NULLPTR)
        : list(projectVariable)
        , folderPrefix(project.isActiveConfig("llvm_linker") ? "-L" : "/LIBPATH:")
        , it(list.begin())
        , excludedFolders(folders != Q_NULLPTR ? folders : &tmp)
        , m_prefixLength(0)
        , m_isFolder(false)
        , m_isLibrary(false)
        , m_isFile(false)
    {
    }

    /// Skips and translates folder-paths.
    inline bool next() {
        while (it != list.end()) {
            m_current = (*it).trimmed().toQString();

            QString rawPath;
            if (m_current.startsWith("/LIBPATH:")) {
                m_prefixLength = 9;
                rawPath = m_current.mid(9);
            } else if(m_current.startsWith("-L") || m_current.startsWith("/L")) {
                m_prefixLength = 2;
                rawPath = Option::fixPathToTargetOS(m_current.mid(2), false, false);
            } else {
                m_isFolder = m_isLibrary = m_isFile = false;
                QChar firstChar = m_current.length() >= 1
                        ? m_current.at(0) : QChar();
                if (m_current.length() >= 2
                        && m_current.at(1) == QLatin1Char('l')
                        && (firstChar == QLatin1Char('/') || firstChar == QLatin1Char('-'))
                ) {
                    // Library-name flag.
                    m_prefixLength = 2;
                    m_isLibrary = true;
                } else if (firstChar == QLatin1Char('-')) {
                    // Unknwon flag.
                    m_prefixLength = 1;
                } else {
                    // Anything that is not a flag is a file.
                    m_prefixLength = 0;
                    m_isFile = true;
                }
                ++it;
                return true;
            }

            QMakeLocalFileName localPath(rawPath);
            if (excludedFolders->contains(localPath)) {
                // Same as `++it`, but removes current.
                it = list.erase(it);
            } else {
                m_isFolder = true;
                m_isLibrary = m_isFile = false;
                excludedFolders->append(localPath);
                m_current = folderPrefix + localPath.real();
                (*it) = m_current;
                ++it;
                return true;
            }
        }

        return false;
    }

    inline bool hasNext() const { return it != list.end(); }

    /// Wether `current()` refers to a folder and/or directory.
    inline bool isFolder() const { return m_isFolder; }

    /// Wether `current()` refers to a `/l` or `-l` prefixed library-name.
    inline bool isLibrary() const { return m_isLibrary; }

    /// Wether `current()` refers to a raw file-path
    /// (not `/l` prefixed library-name, nor a `-` prefixed flag).
    inline bool isFile() const { return m_isFile; }

    inline bool isUnknownFlag() const { return m_prefixLength == 1; }

    /// Current flag/option (maybe use currentValue() instead).
    inline const QString &current() const { return m_current; }
    inline bool setCurrent(const QString &value, int prefixLength = 0) {
        Q_IF (it != list.begin()) {
            m_current = value;
            m_prefixLength = prefixLength;
            (*(it - 1)) = value;
            return true;
        }
        return false;
    }

    /// NOTE: if successful hasNext() returns \c true.
    inline void setNext(const ProString &flag) {
        it = list.insert(it, flag);
    }

    /// Current flag's value if knwon, else just removes flag-prefix.
    inline QString currentValue() const {
        QString path = m_current.right(m_current.length() - m_prefixLength);
        if (m_prefixLength == 2) {
            path = Option::fixPathToTargetOS(path, false, false);
        }
        return path;
    }

    inline void setCurrentValue(const QString &value) {
        QString flag = currentPrefix() + value;
        setCurrent(flag);
    }

    inline QString currentPrefix() const {
        return m_current.left(m_prefixLength);
    }

    inline const QList<QMakeLocalFileName> &toDirs() const {
        return *excludedFolders;
    }

private:
    // Config.
    ProStringList &list;
    const char *folderPrefix;

    // State.
    ProStringList::Iterator it;
    QList<QMakeLocalFileName> tmp;
    QList<QMakeLocalFileName> *excludedFolders;
    int m_prefixLength;
    bool m_isFolder;
    bool m_isLibrary;
    bool m_isFile;
    QString m_current;
};

QT_END_NAMESPACE

#endif // LIB_FLAG_ITERATOR_H
