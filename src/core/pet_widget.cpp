#include "../include/core/pet_widget.h"
#include <QMouseEvent>
#include <QMenu>
#include <QAction>
#include <QApplication>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QDebug>
#include <QVariantAnimation>
#include <QSequentialAnimationGroup>
#include <QEasingCurve>

PetWidget::PetWidget(QWidget *parent) : QLabel(parent) {
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setStyleSheet("font-size: 20px; color: white; background: transparent;");

    // 初始化 QMovie，再 applySize（applySize 只 resize + update）
    m_movie = new QMovie(":/images/nai_wa.gif", QByteArray(), this);
    if (m_movie->isValid()) {
        m_movie->setCacheMode(QMovie::CacheAll); // 缓存帧，循环时不易闪烁
        m_movie->start();
        connect(m_movie, &QMovie::frameChanged, this, [this](int) {
            update();
        });
    } else {
        qWarning() << "GIF load failed:" << m_movie->lastErrorString();
    }

    applySize(QSize(200, 200));
}

void PetWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_dragPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
        event->accept();
    }
}

void PetWidget::mouseMoveEvent(QMouseEvent *event) {
    if (event->buttons() & Qt::LeftButton) {
        move(event->globalPosition().toPoint() - m_dragPosition);
        event->accept();
    }
}

void PetWidget::contextMenuEvent(QContextMenuEvent *event) {
    QMenu menu(this);
    QAction *quitAct = menu.addAction("退出");
    connect(quitAct, &QAction::triggered, qApp, &QApplication::quit);
    menu.exec(event->globalPos());
}

void PetWidget::mouseDoubleClickEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        if (!m_animating) {
            startBreathing();
        } else {
            m_animating = false;
            if (m_sizeAnim) {
                m_sizeAnim->stop();
                m_sizeAnim->deleteLater();
                m_sizeAnim = nullptr;
            }
            applySize(QSize(200, 200));
        }
        event->accept();
    }
}

void PetWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);

    if (!m_movie || m_movie->state() != QMovie::Running) {
        return;
    }

    QPixmap frame = m_movie->currentPixmap();
    if (frame.isNull()) {
        return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 圆角裁剪
    QPainterPath path;
    path.addRoundedRect(rect(), 20, 20);
    painter.setClipPath(path);

    // 按 KeepAspectRatioByExpanding 缩放并居中
    QPixmap scaled = frame.scaled(size() * 0.7, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    int x = (width() - scaled.width()) / 2;
    int y = (height() - scaled.height()) / 2;
    painter.drawPixmap(x, y, scaled);
}

void PetWidget::applySize(const QSize &size) {
    QPoint c = geometry().center();
    resize(size);
    move(c - QPoint(size.width() / 2, size.height() / 2));
    update(); // 触发 paintEvent 按新尺寸重绘当前帧
}

void PetWidget::startBreathing() {
    if (m_animating) return;
    m_animating = true;

    auto *grow = new QVariantAnimation(this);
    grow->setStartValue(QSize(200, 200));
    grow->setEndValue(QSize(400, 400));
    grow->setDuration(1500);
    grow->setEasingCurve(QEasingCurve::InOutSine);

    auto *shrink = new QVariantAnimation(this);
    shrink->setStartValue(QSize(400, 400));
    shrink->setEndValue(QSize(200, 200));
    shrink->setDuration(1500);
    shrink->setEasingCurve(QEasingCurve::InOutSine);

    auto *group = new QSequentialAnimationGroup(this);
    group->addAnimation(grow);
    group->addAnimation(shrink);
    group->setLoopCount(-1);

    connect(grow, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) { applySize(v.toSize()); });
    connect(shrink, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) { applySize(v.toSize()); });

    m_sizeAnim = group;
    group->start();
}