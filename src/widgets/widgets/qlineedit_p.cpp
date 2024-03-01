/****************************************************************************
**
** Copyright (C) 2015 The XD Company Ltd.
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtWidgets module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qlineedit.h"
#include "qlineedit_p.h"

#ifndef QT_NO_LINEEDIT

#include "qvariant.h"
#include "qabstractitemview.h"
#include "qdrag.h"
#include "qwidgetaction.h"
#include "qclipboard.h"
#ifndef QT_NO_ACCESSIBILITY
#include "qaccessible.h"
#endif
#ifndef QT_NO_IM
#include "qinputmethod.h"
#include "qlist.h"
#endif
#include <qpropertyanimation.h>

QT_BEGIN_NAMESPACE

const int QLineEditPrivate::verticalMargin(1);
const int QLineEditPrivate::horizontalMargin(2);

QRect QLineEditPrivate::adjustedControlRect(const QRect &rect) const
{
    QRect widgetRect = !rect.isEmpty() ? rect : q_func()->rect();
    QRect cr = adjustedContentsRect();
    int cix = cr.x() - hscroll + horizontalMargin;
    return widgetRect.translated(QPoint(cix, vscroll));
}

int QLineEditPrivate::xToPos(int x, QTextLine::CursorPosition betweenOrOn) const
{
    QRect cr = adjustedContentsRect();
    x-= cr.x() - hscroll + horizontalMargin;
    return control->xToPos(x, betweenOrOn);
}

bool QLineEditPrivate::inSelection(int x) const
{
    x -= adjustedContentsRect().x() - hscroll + horizontalMargin;
    return control->inSelection(x);
}

QRect QLineEditPrivate::cursorRect() const
{
    return adjustedControlRect(control->cursorRect());
}

#ifndef QT_NO_COMPLETER

void QLineEditPrivate::_q_completionHighlighted(const QString &newText)
{
    Q_Q(QLineEdit);
    if (control->completer()->completionMode() != QCompleter::InlineCompletion) {
        q->setText(newText);
    } else {
        int c = control->cursor();
        QString text = control->text();
        q->setText(text.left(c) + newText.mid(c));
        control->moveCursor(control->end(), false);
#ifndef Q_OS_ANDROID
        const bool mark = true;
#else
        const bool mark = (imHints & Qt::ImhNoPredictiveText);
#endif
        control->moveCursor(c, mark);
    }
}

#endif // QT_NO_COMPLETER

void QLineEditPrivate::_q_handleWindowActivate()
{
    Q_Q(QLineEdit);
    if (!q->hasFocus() && control->hasSelectedText())
        control->deselect();
}

void QLineEditPrivate::_q_textEdited(const QString &text)
{
    Q_Q(QLineEdit);
    emit q->textEdited(text);
#ifndef QT_NO_COMPLETER
    if (control->completer()
        && control->completer()->completionMode() != QCompleter::InlineCompletion)
        control->complete(-1); // update the popup on cut/paste/del
#endif
}

void QLineEditPrivate::_q_cursorPositionChanged(int from, int to)
{
    Q_Q(QLineEdit);
    q->update();
    emit q->cursorPositionChanged(from, to);
}

#ifdef QT_KEYPAD_NAVIGATION
void QLineEditPrivate::_q_editFocusChange(bool e)
{
    Q_Q(QLineEdit);
    q->setEditFocus(e);
}
#endif

void QLineEditPrivate::_q_selectionChanged()
{
    Q_Q(QLineEdit);
    if (control->preeditAreaText().isEmpty()) {
        QStyleOptionFrame opt;
        q->initStyleOption(&opt);
        bool showCursor = control->hasSelectedText() ?
                          q->style()->styleHint(QStyle::SH_BlinkCursorWhenTextSelected, &opt, q):
                          q->hasFocus();
        setCursorVisible(showCursor);
    }

    emit q->selectionChanged();
#ifndef QT_NO_ACCESSIBILITY
    QAccessibleTextSelectionEvent ev(q, control->selectionStart(), control->selectionEnd());
    ev.setCursorPosition(control->cursorPosition());
    QAccessible::updateAccessibility(&ev);
#endif
}

void QLineEditPrivate::_q_updateNeeded(const QRect &rect)
{
    q_func()->update(adjustedControlRect(rect));
}

void QLineEditPrivate::init(const QString& txt)
{
    Q_Q(QLineEdit);
    control = new QWidgetLineControl(txt);
    control->setParent(q);
    control->setFont(q->font());
    QObject::connect(control, SIGNAL(textChanged(QString)),
            q, SIGNAL(textChanged(QString)));
    QObject::connect(control, SIGNAL(textEdited(QString)),
            q, SLOT(_q_textEdited(QString)));
    QObject::connect(control, SIGNAL(cursorPositionChanged(int,int)),
            q, SLOT(_q_cursorPositionChanged(int,int)));
    QObject::connect(control, SIGNAL(selectionChanged()),
            q, SLOT(_q_selectionChanged()));
    QObject::connect(control, SIGNAL(accepted()),
            q, SIGNAL(returnPressed()));
    QObject::connect(control, SIGNAL(editingFinished()),
            q, SIGNAL(editingFinished()));
#ifdef QT_KEYPAD_NAVIGATION
    QObject::connect(control, SIGNAL(editFocusChange(bool)),
            q, SLOT(_q_editFocusChange(bool)));
#endif
    QObject::connect(control, SIGNAL(cursorPositionChanged(int,int)),
            q, SLOT(updateMicroFocus()));

    QObject::connect(control, SIGNAL(textChanged(QString)),
            q, SLOT(updateMicroFocus()));

    // for now, going completely overboard with updates.
    QObject::connect(control, SIGNAL(selectionChanged()),
            q, SLOT(update()));

    QObject::connect(control, SIGNAL(selectionChanged()),
            q, SLOT(updateMicroFocus()));

    QObject::connect(control, SIGNAL(displayTextChanged(QString)),
            q, SLOT(update()));

    QObject::connect(control, SIGNAL(updateNeeded(QRect)),
            q, SLOT(_q_updateNeeded(QRect)));

    QStyleOptionFrame opt;
    q->initStyleOption(&opt);
    control->setPasswordCharacter(q->style()->styleHint(QStyle::SH_LineEdit_PasswordCharacter, &opt, q));
    control->setPasswordMaskDelay(q->style()->styleHint(QStyle::SH_LineEdit_PasswordMaskDelay, &opt, q));
#ifndef QT_NO_CURSOR
    q->setCursor(Qt::IBeamCursor);
#endif
    q->setFocusPolicy(Qt::StrongFocus);
    q->setAttribute(Qt::WA_InputMethodEnabled);
    //   Specifies that this widget can use more, but is able to survive on
    //   less, horizontal space; and is fixed vertically.
    q->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed, QSizePolicy::LineEdit));
    q->setBackgroundRole(QPalette::Base);
    q->setAttribute(Qt::WA_KeyCompression);
    q->setMouseTracking(true);
    q->setAcceptDrops(true);

    q->setAttribute(Qt::WA_MacShowFocusRect);
}

QRect QLineEditPrivate::adjustedContentsRect() const
{
    Q_Q(const QLineEdit);
    QStyleOptionFrame opt;
    q->initStyleOption(&opt);
    QRect r = q->style()->subElementRect(QStyle::SE_LineEditContents, &opt, q);
    r.setX(r.x() + effectiveLeftTextMargin());
    r.setY(r.y() + topTextMargin);
    r.setRight(r.right() - effectiveRightTextMargin());
    r.setBottom(r.bottom() - bottomTextMargin);
    return r;
}

void QLineEditPrivate::setCursorVisible(bool visible)
{
    Q_Q(QLineEdit);
    if ((bool)cursorVisible == visible)
        return;
    cursorVisible = visible;
    if (control->inputMask().isEmpty())
        q->update(cursorRect());
    else
        q->update();
}

void QLineEditPrivate::updatePasswordEchoEditing(bool editing)
{
    Q_Q(QLineEdit);
    control->updatePasswordEchoEditing(editing);
    q->setAttribute(Qt::WA_InputMethodEnabled, shouldEnableInputMethod());
}

void QLineEditPrivate::resetInputMethod()
{
    Q_Q(QLineEdit);
    if (q->hasFocus() && qApp) {
        QGuiApplication::inputMethod()->reset();
    }
}

/*!
  This function is not intended as polymorphic usage. Just a shared code
  fragment that calls QInputMethod::invokeAction for this
  class.
*/
bool QLineEditPrivate::sendMouseEventToInputContext( QMouseEvent *e )
{
#if !defined QT_NO_IM
    if ( control->composeMode() ) {
        int tmp_cursor = xToPos(e->pos().x());
        int mousePos = tmp_cursor - control->cursor();
        if ( mousePos < 0 || mousePos > control->preeditAreaText().length() )
            mousePos = -1;

        if (mousePos >= 0) {
            if (e->type() == QEvent::MouseButtonRelease)
                QGuiApplication::inputMethod()->invokeAction(QInputMethod::Click, mousePos);

            return true;
        }
    }
#else
    Q_UNUSED(e);
#endif

    return false;
}

#ifndef QT_NO_DRAGANDDROP
void QLineEditPrivate::drag()
{
    Q_Q(QLineEdit);
    dndTimer.stop();
    QMimeData *data = new QMimeData;
    data->setText(control->selectedText());
    QDrag *drag = new QDrag(q);
    drag->setMimeData(data);
    Qt::DropAction action = drag->start();
    if (action == Qt::MoveAction && !control->isReadOnly() && drag->target() != q)
        control->removeSelection();
}

#endif // QT_NO_DRAGANDDROP

void QLineEditPrivate::drawText(QPainter *p, QStyleOptionFrame *panel)
{
    QLineEditPrivate *d = this;
    Q_Q(QLineEdit);

    // Intentionally NOT storing `QStyle` (may somehow change).
    QRect r = q->style()->subElementRect(QStyle::SE_LineEditContents, panel, q);
    r.setX(r.x() + d->effectiveLeftTextMargin());
    r.setY(r.y() + d->topTextMargin);
    r.setRight(r.right() - d->effectiveRightTextMargin());
    r.setBottom(r.bottom() - d->bottomTextMargin);
    p->setClipRect(r);

    QFontMetrics fm = q->fontMetrics();
    Qt::Alignment va = QStyle::visualAlignment(d->control->layoutDirection(), QFlag(d->alignment));
    switch (va & Qt::AlignVertical_Mask) {
     case Qt::AlignBottom:
         d->vscroll = r.y() + r.height() - fm.height() - d->verticalMargin;
         break;
     case Qt::AlignTop:
         d->vscroll = r.y() + d->verticalMargin;
         break;
     default:
         //center
         d->vscroll = r.y() + (r.height() - fm.height() + 1) / 2;
         break;
    }
    QRect lineRect(r.x() + d->horizontalMargin, d->vscroll, r.width() - 2*d->horizontalMargin, fm.height());

    int minLB = qMax(0, -fm.minLeftBearing());
    int minRB = qMax(0, -fm.minRightBearing());

    if (d->shouldShowPlaceholderText()) {
        if (!d->placeholderText.isEmpty()) {
            const bool focus = panel->state & QStyle::State_HasFocus;
            QColor col = panel->palette.text().color();

            // Only these values were visible in Windows's sharp view mode
            // (i.e. maybe activated by CTRL+SHIFT+PRINTSCREEN shortcut).
            col.setAlpha(focus ? 128 : 200);

            // No need to backup pen (we reset it below).
            p->setPen(col);

            QString elidedText = fm.elidedText(d->placeholderText, Qt::ElideRight, lineRect.width());
            p->drawText(lineRect.adjusted(minLB, 0, 0, 0), va, elidedText);
            if( ! focus) // We would always return if we didn't want text cursor visible.
                return;
        }
    }
    // XD does not depend on QPainter's state
    // (hence set below even if that's default).
    p->setPen(panel->palette.text().color());

    int cix = qRound(d->control->cursorToX());

    // horizontal scrolling. d->hscroll is the left indent from the beginning
    // of the text line to the left edge of lineRect. we update this value
    // depending on the delta from the last paint event; in effect this means
    // the below code handles all scrolling based on the textline (widthUsed,
    // minLB, minRB), the line edit rect (lineRect) and the cursor position (cix).
    int widthUsed = qRound(d->control->naturalTextWidth()) + 1 + minRB;
    if ((minLB + widthUsed) <=  lineRect.width()) {
        // text fits in lineRect; use hscroll for alignment
        switch (va & ~(Qt::AlignAbsolute|Qt::AlignVertical_Mask)) {
        case Qt::AlignRight:
            d->hscroll = widthUsed - lineRect.width() + 1;
            break;
        case Qt::AlignHCenter:
            d->hscroll = (widthUsed - lineRect.width()) / 2;
            break;
        default:
            // Left
            d->hscroll = 0;
            break;
        }
        d->hscroll -= minLB;
    } else if (cix - d->hscroll >= lineRect.width()) {
        // text doesn't fit, cursor is to the right of lineRect (scroll right)
        d->hscroll = cix - lineRect.width() + 1;
    } else if (cix - d->hscroll < 0 && d->hscroll < widthUsed) {
        // text doesn't fit, cursor is to the left of lineRect (scroll left)
        d->hscroll = cix;
    } else if (widthUsed - d->hscroll < lineRect.width()) {
        // text doesn't fit, text document is to the left of lineRect; align
        // right
        d->hscroll = widthUsed - lineRect.width() + 1;
    } else {
        //in case the text is bigger than the lineedit, the hscroll can never be negative
        d->hscroll = qMax(0, d->hscroll);
    }

    // the y offset is there to keep the baseline constant in case we have script changes in the text.
    QPoint topLeft = lineRect.topLeft() - QPoint(d->hscroll, d->control->ascent() - fm.ascent());

    // draw text, selections and cursors
    int flags = QWidgetLineControl::DrawText;

#ifdef QT_KEYPAD_NAVIGATION
    if (!QApplication::keypadNavigationEnabled() || hasEditFocus())
#endif
    if (d->control->hasSelectedText() || (d->cursorVisible && !d->control->inputMask().isEmpty() && !d->control->isReadOnly())){
        flags |= QWidgetLineControl::DrawSelections;
        // Palette only used for selections/mask and may not be in sync
        if (d->control->palette() != panel->palette
           || d->control->palette().currentColorGroup() != panel->palette.currentColorGroup())
            d->control->setPalette(panel->palette);
    }

    // Asian users see an IM selection text as cursor on candidate
    // selection phase of input method, so the ordinary cursor should be
    // invisible if we have a preedit string.
    if (d->cursorVisible /*&& ! d->control->isReadOnly()*/)
        flags |= QWidgetLineControl::DrawCursor;

    d->control->setCursorWidth(q->style()->pixelMetric(QStyle::PM_TextCursorWidth));
    d->control->draw(p, topLeft, r, flags);
}

QLineEditIconButton::QLineEditIconButton(QWidget *parent)
    : QToolButton(parent)
    , m_opacity(0)
{
    setFocusPolicy(Qt::NoFocus);
}

QLineEditPrivate *QLineEditIconButton::lineEditPrivate() const
{
    QLineEdit *le = qobject_cast<QLineEdit *>(parentWidget());
    return le ? static_cast<QLineEditPrivate *>(qt_widget_private(le)) : Q_NULLPTR;
}

void QLineEditIconButton::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    QWindow *window = Q_NULLPTR;
    if (const QWidget *nativeParent = nativeParentWidget())
        window = nativeParent->windowHandle();
    // Note isDown should really use the active state but in most styles
    // this has no proper feedback
    QIcon::Mode state = QIcon::Disabled;
    if (isEnabled())
        state = isDown() ? QIcon::Selected : QIcon::Normal;
    const QLineEditPrivate *lep = lineEditPrivate();
    const int iconWidth = lep ? lep->sideWidgetParameters().iconSize : 16;
    const QSize iconSize(iconWidth, iconWidth);
    const QPixmap iconPixmap = icon().pixmap(window, iconSize, state, QIcon::Off);
    QRect pixmapRect = QRect(QPoint(0, 0), iconSize);
    pixmapRect.moveCenter(rect().center());
    painter.setOpacity(m_opacity);
    painter.drawPixmap(pixmapRect, iconPixmap);
}

void QLineEditIconButton::actionEvent(QActionEvent *e)
{
    switch (e->type()) {
    case QEvent::ActionChanged: {
        const QAction *action = e->action();
        if (isVisibleTo(parentWidget()) != action->isVisible()) {
            setVisible(action->isVisible());
            if (QLineEditPrivate *lep = lineEditPrivate())
                lep->positionSideWidgets();
        }
    }
        break;
    default:
        break;
    }
    QToolButton::actionEvent(e);
}

void QLineEditIconButton::setOpacity(qreal value)
{
    if (!qFuzzyCompare(m_opacity, value)) {
        m_opacity = value;
        updateCursor();
        update();
    }
}

#ifndef QT_NO_ANIMATION
void QLineEditIconButton::startOpacityAnimation(qreal endValue)
{
    QPropertyAnimation *animation = new QPropertyAnimation(this, QByteArrayLiteral("opacity"));
    animation->setDuration(160);
    animation->setEndValue(endValue);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}
#endif

void QLineEditIconButton::updateCursor()
{
#ifndef QT_NO_CURSOR
    setCursor(qFuzzyCompare(m_opacity, qreal(1.0)) || !parentWidget() ? QCursor(Qt::ArrowCursor) : parentWidget()->cursor());
#endif
}

void QLineEditPrivate::_q_textChanged(const QString &text)
{
    if (hasSideWidgets()) {
        const int newTextSize = text.size();
        if (!newTextSize || !lastTextSize) {
            lastTextSize = newTextSize;
#ifndef QT_NO_ANIMATION
            const bool fadeIn = newTextSize > 0;
            foreach (const SideWidgetEntry &e, leadingSideWidgets) {
                if (e.flags & SideWidgetFadeInWithText)
                   static_cast<QLineEditIconButton *>(e.widget)->animateShow(fadeIn);
            }
            foreach (const SideWidgetEntry &e, trailingSideWidgets) {
                if (e.flags & SideWidgetFadeInWithText)
                   static_cast<QLineEditIconButton *>(e.widget)->animateShow(fadeIn);
            }
#endif
        }
    }
}

void QLineEditPrivate::_q_clearButtonClicked()
{
    Q_Q(QLineEdit);
    if (!q->text().isEmpty()) {
        q->clear();
        emit q->textEdited(QString());
    }
}

QLineEditPrivate::SideWidgetParameters QLineEditPrivate::sideWidgetParameters() const
{
    Q_Q(const QLineEdit);
    SideWidgetParameters result;
    result.iconSize = q->height() < 34 ? 16 : 32;
    result.margin = result.iconSize / 4;
    result.widgetWidth = result.iconSize + 6;
    result.widgetHeight = result.iconSize + 2;
    return result;
}

QIcon QLineEditPrivate::clearButtonIcon() const
{
    Q_Q(const QLineEdit);
    QStyleOptionFrame styleOption;
    q->initStyleOption(&styleOption);
    return q->style()->standardIcon(QStyle::SP_LineEditClearButton, &styleOption, q);
}

void QLineEditPrivate::setClearButtonEnabled(bool enabled)
{
    foreach (const SideWidgetEntry &e, trailingSideWidgets) {
        if (e.flags & SideWidgetClearButton) {
            e.action->setEnabled(enabled);
            break;
        }
    }
}

void QLineEditPrivate::positionSideWidgets()
{
    Q_Q(QLineEdit);
    if (hasSideWidgets()) {
        const QRect contentRect = q->rect();
        const SideWidgetParameters p = sideWidgetParameters();
        const int delta = p.margin + p.widgetWidth;
        QRect widgetGeometry(QPoint(p.margin, (contentRect.height() - p.widgetHeight) / 2),
                             QSize(p.widgetWidth, p.widgetHeight));
        foreach (const SideWidgetEntry &e, leftSideWidgetList()) {
            e.widget->setGeometry(widgetGeometry);
            if (e.action->isVisible())
                widgetGeometry.moveLeft(widgetGeometry.left() + delta);
        }
        widgetGeometry.moveLeft(contentRect.width() - p.widgetWidth - p.margin);
        foreach (const SideWidgetEntry &e, rightSideWidgetList()) {
            e.widget->setGeometry(widgetGeometry);
            if (e.action->isVisible())
                widgetGeometry.moveLeft(widgetGeometry.left() - delta);
        }
    }
}

QLineEditPrivate::PositionIndexPair QLineEditPrivate::findSideWidget(const QAction *a) const
{
    for (int i = 0; i < leadingSideWidgets.size(); ++i) {
        if (a == leadingSideWidgets.at(i).action)
            return PositionIndexPair(QLineEdit::LeadingPosition, i);
    }
    for (int i = 0; i < trailingSideWidgets.size(); ++i) {
        if (a == trailingSideWidgets.at(i).action)
            return PositionIndexPair(QLineEdit::TrailingPosition, i);
    }
    return PositionIndexPair(QLineEdit::LeadingPosition, -1);
}

QWidget *QLineEditPrivate::addAction(QAction *newAction, QAction *before, QLineEdit::ActionPosition position, int flags)
{
    Q_Q(QLineEdit);
    if (!newAction)
        return 0;
    if (!hasSideWidgets()) { // initial setup.
        QObject::connect(q, SIGNAL(textChanged(QString)), q, SLOT(_q_textChanged(QString)));
        lastTextSize = q->text().size();
    }
    QWidget *w = 0;
    // Store flags about QWidgetAction here since removeAction() may be called from ~QAction,
    // in which a qobject_cast<> no longer works.
    if (QWidgetAction *widgetAction = qobject_cast<QWidgetAction *>(newAction)) {
        if ((w = widgetAction->requestWidget(q)))
            flags |= SideWidgetCreatedByWidgetAction;
    }
    if (!w) {
        QLineEditIconButton *toolButton = new QLineEditIconButton(q);
        toolButton->setIcon(newAction->icon());
        toolButton->setOpacity(lastTextSize > 0 || !(flags & SideWidgetFadeInWithText) ? 1 : 0);
        if (flags & SideWidgetClearButton)
            QObject::connect(toolButton, SIGNAL(clicked()), q, SLOT(_q_clearButtonClicked()));
        toolButton->setDefaultAction(newAction);
        w = toolButton;
    }
    // If there is a 'before' action, it takes preference
    PositionIndexPair positionIndex = before ? findSideWidget(before) : PositionIndexPair(position, -1);
    SideWidgetEntryList &list = positionIndex.first == QLineEdit::TrailingPosition ? trailingSideWidgets : leadingSideWidgets;
    if (positionIndex.second < 0)
        positionIndex.second = list.size();
    list.insert(positionIndex.second, SideWidgetEntry(w, newAction, flags));
    positionSideWidgets();
    w->show();
    return w;
}

void QLineEditPrivate::removeAction(QAction *action)
{
    Q_Q(QLineEdit);
    const PositionIndexPair positionIndex = findSideWidget(action);
    if (positionIndex.second == -1)
        return;
     SideWidgetEntryList &list = positionIndex.first == QLineEdit::TrailingPosition ? trailingSideWidgets : leadingSideWidgets;
     SideWidgetEntry entry = list.takeAt(positionIndex.second);
     if (entry.flags & SideWidgetCreatedByWidgetAction)
         static_cast<QWidgetAction *>(entry.action)->releaseWidget(entry.widget);
     else
         delete entry.widget;
     positionSideWidgets();
     if (!hasSideWidgets()) // Last widget, remove connection
         QObject::disconnect(q, SIGNAL(textChanged(QString)), q, SLOT(_q_textChanged(QString)));
     q->update();
}

QT_END_NAMESPACE

#include "moc_qlineedit_p.cpp"

#endif
