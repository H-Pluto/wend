#pragma once

#include <QLabel>
#include <QPoint>

class PetWidget : public QLabel {
    // Q_OBJECT 是 Qt 的宏，告诉 MOC（元对象编译器）这个类需要 Qt 的元对象功能。
    Q_OBJECT
    
public:
    explicit PetWidget(QWidget *parent = nullptr);  // explicit 必须显示定义

protected:
    void mousePressEvent(QMouseEvent *event);

    void mouseMoveEvent(QMouseEvent *event);

    void contextMenuEvent(QContextMenuEvent *event);

private:
    QPoint m_dragPosition;
};