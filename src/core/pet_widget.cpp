#include "../include/core/pet_widget.h"
#include <QMouseEvent>
#include <QMenu>
#include <QAction>
#include <QApplication>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>


/* 定义构造函数 */
PetWidget::PetWidget(QWidget *parent) : QLabel(parent) {
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground); // 窗口透明背景
    resize(200, 200); // 设置大小

    QPixmap src(":/images/aa.jpg");
    QPixmap scaled = src.scaled(200, 200, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    
    // 创建透明画布
    QPixmap rounded(scaled.size());
    rounded.fill(Qt::transparent);

    QPainter painter(&rounded);
    painter.setRenderHint(QPainter::Antialiasing);
    QPainterPath path;
    path.addRoundedRect(rounded.rect(), 20, 20);   // 20 是圆角半径
    painter.setClipPath(path);
    painter.drawPixmap(0, 0, scaled);
    painter.end();

    setPixmap(rounded);
    setAlignment(Qt::AlignCenter);
    setStyleSheet("font-size: 20px; color: white; background: transparent;");
}

/* 记录鼠标点击位置 */
void PetWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_dragPosition = event->globalPos() - frameGeometry().topLeft();
        event->accept();
    }
}

/* 计算鼠标移动距离同步移动 */
void PetWidget::mouseMoveEvent(QMouseEvent *event) {
    if (event->buttons() & Qt::LeftButton) {
        move(event->globalPos() - m_dragPosition);
        event->accept();
    }
}

/* 右键菜单 */
void PetWidget::contextMenuEvent(QContextMenuEvent *event) {
    QMenu menu(this);
    QAction *quitAct = menu.addAction("退出");
    connect(quitAct, &QAction::triggered, qApp, &QApplication::quit);
    menu.exec(event->globalPos());
}