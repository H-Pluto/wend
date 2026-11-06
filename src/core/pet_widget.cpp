#include "../include/core/pet_widget.h"

#include <QAction>
#include <QApplication>
#include <QContextMenuEvent>
#include <QGuiApplication>
#include <QMenu>
#include <QMouseEvent>
#include <QMovie>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QScreen>
#include <QTimer>
#include <QtMath>

#include <algorithm>
#include <cmath>

namespace {
constexpr int kWindowWidth = 210;   // 窗口给旋转和挤压留出的余量
constexpr int kWindowHeight = 260;
constexpr qreal kBodyWidth = 130.0; // 碰撞盒 = 宠物可见大小
constexpr qreal kBodyHeight = 170.0;

constexpr float kFixedTimeStep = 1.0f / 60.0f;
constexpr float kMaxRenderAngle = 0.5f; // 渲染角度上限，保证不超出窗口
constexpr float kMaxSquash = 0.15f;
constexpr float kPi = 3.14159265358979323846f;
} // namespace

PetWidget::PetWidget(QWidget *parent) : QLabel(parent) {
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedSize(kWindowWidth, kWindowHeight);
    setCursor(Qt::OpenHandCursor);

    m_movie = new QMovie(":/images/nai_wa.gif", QByteArray(), this);
    if (m_movie->isValid()) {
        m_movie->setCacheMode(QMovie::CacheAll); // 缓存帧，循环时不易闪烁
        m_movie->start();
        connect(m_movie, &QMovie::frameChanged, this, [this](int) { update(); });
    } else {
        qWarning() << "GIF load failed:" << m_movie->lastErrorString();
    }

    m_physics.setSize(QSizeF(kBodyWidth, kBodyHeight));
    syncScreenBounds();
    // 从屏幕上方掉下来，开场先展示一下物理
    m_physics.reset(QPointF(m_bounds.center().x(), m_bounds.top() + 120.0));

    m_frameClock.start();
    m_timer = new QTimer(this);
    m_timer->setInterval(16);
    connect(m_timer, &QTimer::timeout, this, &PetWidget::onTick);
    m_timer->start();

    syncWindowPosition();
}

void PetWidget::onTick() {
    const qint64 elapsedNs = m_frameClock.nsecsElapsed();
    m_frameClock.restart();
    const float dt = std::clamp(float(elapsedNs / 1e9), 0.0f, 0.05f);

    syncScreenBounds();

    if (m_dragging) {
        m_accumulator = 0.0f;
    } else {
        m_accumulator = std::min(m_accumulator + dt, 0.2f);
        float impact = 0.0f;
        while (m_accumulator >= kFixedTimeStep) {
            m_physics.step(kFixedTimeStep);
            impact = std::max(impact, m_physics.impactSpeed());
            m_accumulator -= kFixedTimeStep;
        }
        if (impact > 120.0f) {
            m_squash = -std::clamp(impact * 0.00016f, 0.02f, kMaxSquash);
            m_squashVelocity = 0.0f;
        }
    }

    syncWindowPosition();
    updateSquash(dt);
    update();
}

void PetWidget::syncScreenBounds() {
    QScreen *screen = nullptr;
    if (!m_bounds.isEmpty()) {
        screen = QGuiApplication::screenAt(m_physics.center().toPoint());
    }
    if (!screen) {
        screen = QGuiApplication::primaryScreen();
    }
    if (!screen) {
        return;
    }

    const QRectF available(screen->availableGeometry());
    if (available == m_bounds) {
        return;
    }
    m_bounds = available;
    m_physics.setBounds(m_bounds);
}

void PetWidget::syncWindowPosition() {
    const QPointF c = m_physics.center();
    move(qRound(c.x() - width() / 2.0), qRound(c.y() - height() / 2.0));
}

void PetWidget::updateSquash(float dt) {
    constexpr float kStiffness = 260.0f;
    constexpr float kDamping = 14.0f;
    m_squashVelocity += (-kStiffness * m_squash - kDamping * m_squashVelocity) * dt;
    m_squash += m_squashVelocity * dt;
    m_squash = std::clamp(m_squash, -kMaxSquash, kMaxSquash);
}

void PetWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton && !m_dragging) {
        m_dragOffset = event->globalPosition() - m_physics.center();
        m_dragVelocity = QPointF();
        m_lastDragPos = event->globalPosition();
        m_dragClock.restart();
        m_physics.beginDrag(m_physics.center());
        m_dragging = true;
        m_squash = 0.0f;
        m_squashVelocity = 0.0f;
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }
    QLabel::mousePressEvent(event);
}

void PetWidget::mouseMoveEvent(QMouseEvent *event) {
    if (!m_dragging || !(event->buttons() & Qt::LeftButton)) {
        QLabel::mouseMoveEvent(event);
        return;
    }

    const QPointF global = event->globalPosition();
    const qint64 ms = m_dragClock.restart();
    if (ms > 0 && ms < 120) {
        const QPointF v = (global - m_lastDragPos) / (ms / 1000.0);
        m_dragVelocity = m_dragVelocity * 0.55 + v * 0.45;
    } else {
        m_dragVelocity = QPointF();
    }
    m_lastDragPos = global;

    m_physics.dragTo(global - m_dragOffset);
    syncWindowPosition();
    update();
    event->accept();
}

void PetWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton && m_dragging) {
        if (m_dragClock.elapsed() > 120) {
            m_dragVelocity = QPointF(); // 停住后再松手，不抛出
        }
        m_physics.endDrag(m_dragVelocity);
        m_dragging = false;
        m_accumulator = 0.0f;
        setCursor(Qt::OpenHandCursor);
        event->accept();
        return;
    }
    QLabel::mouseReleaseEvent(event);
}

void PetWidget::mouseDoubleClickEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        finishDrag();
        if (m_physics.nearGround()) {
            m_physics.jump(900.0f);
            m_squash = kMaxSquash; // 起跳前先拉长
            m_squashVelocity = 0.0f;
        }
        event->accept();
        return;
    }
    QLabel::mouseDoubleClickEvent(event);
}

void PetWidget::finishDrag() {
    if (!m_dragging) {
        return;
    }
    m_physics.endDrag(QPointF());
    m_dragging = false;
    m_accumulator = 0.0f;
    setCursor(Qt::OpenHandCursor);
}

void PetWidget::contextMenuEvent(QContextMenuEvent *event) {
    QMenu menu(this);
    QAction *resetAct = menu.addAction("回到屏幕中央");
    connect(resetAct, &QAction::triggered, this, [this] {
        finishDrag();
        m_physics.reset(m_bounds.center());
        syncWindowPosition();
    });
    menu.addSeparator();
    QAction *quitAct = menu.addAction("退出");
    connect(quitAct, &QAction::triggered, qApp, &QApplication::quit);
    menu.exec(event->globalPos());
}

void PetWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);

    if (!m_movie || m_movie->state() != QMovie::Running) {
        return;
    }

    const QPixmap frame = m_movie->currentPixmap();
    if (frame.isNull()) {
        return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    // 待机时轻微呼吸，落地/起跳时用压扁拉伸表现弹性
    float breathe = 0.0f;
    if (!m_dragging && m_physics.resting()) {
        breathe = 0.02f * std::sin(float(m_frameClock.elapsed() / 1000.0) * 2.0f * kPi / 3.2f);
    }

    const float squash = m_squash + breathe;
    const float scaleY = 1.0f + squash;
    const float scaleX = 1.0f - squash * 0.5f;
    const float angle = std::clamp(m_physics.angle(), -kMaxRenderAngle, kMaxRenderAngle);

    painter.translate(width() / 2.0, height() / 2.0);
    painter.rotate(qRadiansToDegrees(double(angle)));
    painter.scale(scaleX, scaleY);

    const QRectF box(-kBodyWidth / 2.0, -kBodyHeight / 2.0, kBodyWidth, kBodyHeight);
    QPainterPath clip;
    clip.addRoundedRect(box, 18.0, 18.0);
    painter.setClipPath(clip);

    const QSizeF fitted = QSizeF(frame.size()).scaled(box.size(), Qt::KeepAspectRatio);
    const QRectF target(box.center().x() - fitted.width() / 2.0,
                        box.center().y() - fitted.height() / 2.0,
                        fitted.width(), fitted.height());
    painter.drawPixmap(target, frame, QRectF(frame.rect()));
}
