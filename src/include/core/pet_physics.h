#pragma once

#include <QPointF>
#include <QRectF>
#include <QSizeF>

#include <box2d/box2d.h>

// 把桌宠包成 Box2D 刚体：重力下落、屏边碰撞、拖拽抛出、落地缓冲。
// 对外一律用屏幕像素坐标（逻辑像素），内部换算成米，避免像素量级的数值精度问题。
class PetPhysics {
public:
    PetPhysics();
    ~PetPhysics();

    PetPhysics(const PetPhysics &) = delete;
    PetPhysics &operator=(const PetPhysics &) = delete;

    void setSize(const QSizeF &sizePx);
    void setBounds(const QRectF &boundsPx);
    void reset(const QPointF &centerPx);

    void beginDrag(const QPointF &centerPx);
    void dragTo(const QPointF &centerPx);
    void endDrag(const QPointF &velocityPx);

    void jump(float speedPx);
    void step(float dt);

    QPointF center() const;
    float angle() const;
    float impactSpeed() const { return m_impactPx; }
    bool nearGround() const;
    bool resting() const;
    bool dragging() const { return m_dragging; }

private:
    void createBody();
    void destroyBody();
    void rebuildWalls();
    void applySelfRighting(float dt);

    QPointF clampToBounds(const QPointF &p) const;
    b2Vec2 toWorld(const QPointF &p) const;
    QPointF toScreen(b2Vec2 p) const;

    b2WorldId m_world = b2_nullWorldId;
    b2BodyId m_body = b2_nullBodyId;

    b2BodyId m_wallGround = b2_nullBodyId;
    b2BodyId m_wallLeft = b2_nullBodyId;
    b2BodyId m_wallRight = b2_nullBodyId;
    b2BodyId m_wallTop = b2_nullBodyId;

    QRectF m_bounds;
    QSizeF m_size{130.0, 170.0};
    float m_groundY = 0.0f; // 地面线，世界坐标（米）

    bool m_dragging = false;
    float m_impactPx = 0.0f;
};
