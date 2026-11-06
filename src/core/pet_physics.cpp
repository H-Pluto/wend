#include "../include/core/pet_physics.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr float kPixelsPerMeter = 100.0f;
constexpr float kGravity = 9.8f;          // m/s^2，屏幕坐标 y 向下
constexpr float kMaxThrowSpeed = 26.0f;   // m/s
constexpr float kWallThickness = 2.0f;    // m，足够厚防止穿透

b2BodyId createWall(b2WorldId world, b2Vec2 center, b2Vec2 halfExtents) {
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_staticBody;
    bodyDef.position = center;
    b2BodyId body = b2CreateBody(world, &bodyDef);

    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.material.friction = 0.8f;
    shapeDef.material.restitution = 0.2f;
    b2Polygon box = b2MakeBox(halfExtents.x, halfExtents.y);
    b2CreatePolygonShape(body, &shapeDef, &box);
    return body;
}
} // namespace

PetPhysics::PetPhysics() {
    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = {0.0f, kGravity};
    m_world = b2CreateWorld(&worldDef);
    createBody();
}

PetPhysics::~PetPhysics() {
    if (b2World_IsValid(m_world)) {
        b2DestroyWorld(m_world);
    }
}

b2Vec2 PetPhysics::toWorld(const QPointF &p) const {
    return {float(p.x() / kPixelsPerMeter), float(p.y() / kPixelsPerMeter)};
}

QPointF PetPhysics::toScreen(b2Vec2 p) const {
    return QPointF(p.x * kPixelsPerMeter, p.y * kPixelsPerMeter);
}

void PetPhysics::createBody() {
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_dynamicBody;
    bodyDef.position = toWorld(clampToBounds(m_bounds.center()));
    bodyDef.linearDamping = 0.15f;
    bodyDef.angularDamping = 3.0f;
    bodyDef.enableSleep = true;
    m_body = b2CreateBody(m_world, &bodyDef);

    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = 1.0f;
    shapeDef.material.friction = 0.6f;
    shapeDef.material.restitution = 0.4f;

    const float hw = float(m_size.width() / 2.0 / kPixelsPerMeter);
    const float hh = float(m_size.height() / 2.0 / kPixelsPerMeter);
    const b2Polygon box = b2MakeBox(hw, hh);
    b2CreatePolygonShape(m_body, &shapeDef, &box);
}

void PetPhysics::destroyBody() {
    if (b2Body_IsValid(m_body)) {
        b2DestroyBody(m_body);
    }
    m_body = b2_nullBodyId;
}

void PetPhysics::setSize(const QSizeF &sizePx) {
    if (sizePx == m_size) {
        return;
    }
    const QPointF c = center();
    const float a = angle();
    m_size = sizePx;
    destroyBody();
    createBody();
    if (!c.isNull()) {
        b2Body_SetTransform(m_body, toWorld(c), b2MakeRot(a));
    }
}

void PetPhysics::setBounds(const QRectF &boundsPx) {
    if (boundsPx == m_bounds) {
        return;
    }
    m_bounds = boundsPx;
    m_groundY = float(boundsPx.bottom() / kPixelsPerMeter);
    rebuildWalls();

    if (b2Body_IsValid(m_body)) {
        const QPointF c = center();
        const QPointF clamped = clampToBounds(c);
        if (clamped != c) {
            b2Body_SetTransform(m_body, toWorld(clamped), b2Body_GetRotation(m_body));
        }
    }
}

void PetPhysics::rebuildWalls() {
    if (b2Body_IsValid(m_wallGround)) b2DestroyBody(m_wallGround);
    if (b2Body_IsValid(m_wallLeft)) b2DestroyBody(m_wallLeft);
    if (b2Body_IsValid(m_wallRight)) b2DestroyBody(m_wallRight);
    if (b2Body_IsValid(m_wallTop)) b2DestroyBody(m_wallTop);

    if (m_bounds.isEmpty() || !b2World_IsValid(m_world)) {
        m_wallGround = m_wallLeft = m_wallRight = m_wallTop = b2_nullBodyId;
        return;
    }

    const float left = float(m_bounds.left() / kPixelsPerMeter);
    const float right = float(m_bounds.right() / kPixelsPerMeter);
    const float top = float(m_bounds.top() / kPixelsPerMeter);
    const float bottom = float(m_bounds.bottom() / kPixelsPerMeter);
    const float cx = (left + right) * 0.5f;
    const float halfW = (right - left) * 0.5f;

    m_wallGround = createWall(m_world,
        {cx, bottom + kWallThickness * 0.5f},
        {halfW + kWallThickness, kWallThickness * 0.5f}
    );
    m_wallLeft = createWall(m_world,
        {left - kWallThickness * 0.5f, (top + bottom) * 0.5f},
        {kWallThickness * 0.5f, (bottom - top) * 0.5f + kWallThickness}
    );
    m_wallRight = createWall(m_world,
        {right + kWallThickness * 0.5f, (top + bottom) * 0.5f},
        {kWallThickness * 0.5f, (bottom - top) * 0.5f + kWallThickness}
    );
    m_wallTop = createWall(m_world,
        {cx, top - kWallThickness * 0.5f},
        {halfW + kWallThickness, kWallThickness * 0.5f}
    );
}

void PetPhysics::reset(const QPointF &centerPx) {
    if (!b2Body_IsValid(m_body)) {
        return;
    }
    b2Body_SetType(m_body, b2_dynamicBody);
    b2Body_SetTransform(m_body, toWorld(clampToBounds(centerPx)), b2MakeRot(0.0f));
    b2Body_SetLinearVelocity(m_body, {0.0f, 0.0f});
    b2Body_SetAngularVelocity(m_body, 0.0f);
    b2Body_SetAwake(m_body, true);
    m_dragging = false;
    m_impactPx = 0.0f;
}

QPointF PetPhysics::clampToBounds(const QPointF &p) const {
    if (m_bounds.isEmpty() || m_size.isEmpty()) {
        return p;
    }
    const qreal hw = m_size.width() / 2.0;
    const qreal hh = m_size.height() / 2.0;
    return QPointF(std::clamp(p.x(), m_bounds.left() + hw, m_bounds.right() - hw),
        std::clamp(p.y(), m_bounds.top() + hh, m_bounds.bottom() - hh)
    );
}

QPointF PetPhysics::center() const {
    if (!b2Body_IsValid(m_body)) {
        return {};
    }
    return toScreen(b2Body_GetPosition(m_body));
}

float PetPhysics::angle() const {
    if (!b2Body_IsValid(m_body)) {
        return 0.0f;
    }
    return b2Rot_GetAngle(b2Body_GetRotation(m_body));
}

bool PetPhysics::nearGround() const {
    if (!b2Body_IsValid(m_body)) {
        return false;
    }
    const float halfDiag = float(std::hypot(m_size.width(), m_size.height()) * 0.5 / kPixelsPerMeter);
    const float y = float(b2Body_GetPosition(m_body).y);
    return y >= m_groundY - halfDiag - 0.02f;
}

bool PetPhysics::resting() const {
    if (!b2Body_IsValid(m_body) || m_dragging) {
        return false;
    }
    const b2Vec2 v = b2Body_GetLinearVelocity(m_body);
    return std::hypot(v.x, v.y) < 0.05f && std::abs(b2Body_GetAngularVelocity(m_body)) < 0.2f;
}

void PetPhysics::beginDrag(const QPointF &centerPx) {
    if (!b2Body_IsValid(m_body)) {
        return;
    }
    m_dragging = true;
    m_impactPx = 0.0f;
    b2Body_SetType(m_body, b2_kinematicBody);
    b2Body_SetLinearVelocity(m_body, {0.0f, 0.0f});
    b2Body_SetAngularVelocity(m_body, 0.0f);
    b2Body_SetTransform(m_body, toWorld(clampToBounds(centerPx)), b2MakeRot(0.0f));
}

void PetPhysics::dragTo(const QPointF &centerPx) {
    if (!m_dragging || !b2Body_IsValid(m_body)) {
        return;
    }
    b2Body_SetTransform(m_body, toWorld(clampToBounds(centerPx)), b2MakeRot(0.0f));
}

void PetPhysics::endDrag(const QPointF &velocityPx) {
    if (!m_dragging || !b2Body_IsValid(m_body)) {
        return;
    }
    m_dragging = false;

    QPointF v = velocityPx;
    const double speed = std::hypot(v.x(), v.y());
    const double maxSpeedPx = kMaxThrowSpeed * kPixelsPerMeter;
    if (speed > maxSpeedPx) {
        v *= maxSpeedPx / speed;
    }

    b2Body_SetType(m_body, b2_dynamicBody);
    b2Body_SetLinearVelocity(m_body, {float(v.x() / kPixelsPerMeter), float(v.y() / kPixelsPerMeter)});
    const float spin = std::clamp(float(v.x() / kPixelsPerMeter * 0.12), -6.0f, 6.0f);
    b2Body_SetAngularVelocity(m_body, spin);
    b2Body_SetAwake(m_body, true);
}

void PetPhysics::jump(float speedPx) {
    if (!b2Body_IsValid(m_body) || m_dragging || !nearGround()) {
        return;
    }
    const float mass = b2Body_GetMass(m_body);
    b2Body_ApplyLinearImpulseToCenter(m_body, {0.0f, -speedPx / kPixelsPerMeter * mass}, true);
}

void PetPhysics::applySelfRighting(float dt) {
    if (!nearGround()) {
        return;
    }
    const b2Vec2 v = b2Body_GetLinearVelocity(m_body);
    if (std::hypot(v.x, v.y) > 0.8f) {
        return; // 还在翻滚/滑行，不干预
    }

    const float a = b2UnwindAngle(angle());
    if (std::abs(a) < 0.005f && std::abs(b2Body_GetAngularVelocity(m_body)) < 0.05f) {
        return;
    }

    // 直接平滑回正角度，避免力矩注入能量导致弹起来
    const float t = 1.0f - std::exp(-8.0f * dt);
    b2Body_SetTransform(m_body, b2Body_GetPosition(m_body), b2MakeRot(a * (1.0f - t)));
    b2Body_SetAngularVelocity(m_body, 0.0f);
}

void PetPhysics::step(float dt) {
    if (m_dragging || !b2Body_IsValid(m_body) || !b2World_IsValid(m_world)) {
        return;
    }

    m_impactPx = 0.0f;
    const bool wasNearGround = nearGround();
    const float vyBefore = b2Body_GetLinearVelocity(m_body).y;

    applySelfRighting(dt);
    b2World_Step(m_world, dt, 4);

    if (!wasNearGround && nearGround() && vyBefore > 1.0f) {
        m_impactPx = vyBefore * kPixelsPerMeter;
    }
}
