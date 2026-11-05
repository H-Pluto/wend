#pragma once

#include <QLabel>
#include <QPoint>
#include <QPixmap>
#include <QSize>
#include <QMovie>

class QAbstractAnimation;
class QMouseEvent;
class QContextMenuEvent;

class PetWidget : public QLabel {
    Q_OBJECT

public:
    explicit PetWidget(QWidget *parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    QMovie *m_movie = nullptr;
    QSize m_targetSize{200, 200};

    QPixmap makeRoundedPixmap(const QSize &size);
    void applySize(const QSize &size);
    void startBreathing();

    QPoint m_dragPosition;
    QAbstractAnimation *m_sizeAnim = nullptr;
    bool m_animating = false;
};