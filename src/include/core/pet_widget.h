#pragma once

#include <QElapsedTimer>
#include <QLabel>
#include <QPointF>
#include <QRectF>

#include "pet_physics.h"

class QContextMenuEvent;
class QMouseEvent;
class QMovie;
class QTimer;

class PetWidget : public QLabel {
    Q_OBJECT

public:
    explicit PetWidget(QWidget *parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    void onTick();
    void syncScreenBounds();
    void syncWindowPosition();
    void updateSquash(float dt);
    void finishDrag();

    QMovie *m_movie = nullptr;
    QTimer *m_timer = nullptr;

    PetPhysics m_physics;
    QRectF m_bounds;

    bool m_dragging = false;
    QPointF m_dragOffset;     // 抓取点相对宠物中心的偏移
    QPointF m_dragVelocity;   // px/s，松手时作为抛出速度
    QPointF m_lastDragPos;
    QElapsedTimer m_dragClock;

    QElapsedTimer m_frameClock;
    float m_accumulator = 0.0f;

    float m_squash = 0.0f;    // >0 纵向拉伸，<0 纵向压扁
    float m_squashVelocity = 0.0f;
};
