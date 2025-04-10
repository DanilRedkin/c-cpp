#include "cell.h"
#include <QString>

Cell::Cell(QWidget *parent)
    : QPushButton(parent), is_mine(false), mines_around(0), revealed(false), currentView(ViewState::Hidden) {
    updateAppearance();
}

void Cell::setRevealed() {
    revealed = true;
    setCurrentView(ViewState::Revealed);
}

bool Cell::isRevealed() const {
    return revealed;
}

bool Cell::isMine() const {
    return is_mine;
}

void Cell::setIsMine(bool mine) {
    is_mine = mine;
}

int Cell::minesAround() const {
    return mines_around;
}

void Cell::setMinesAround(int count) {
    mines_around = count;
}

void Cell::setCurrentView(ViewState view) {
    currentView = view;
    updateAppearance();
}

Cell::ViewState Cell::getCurrentView() const {
    return currentView;
}

void Cell::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        emit leftClicked();
    } else if (event->button() == Qt::RightButton) {
        emit rightClicked();
    } else if (event->button() == Qt::MiddleButton) {
        emit centerClicked();
    }
    QPushButton::mousePressEvent(event);
}

void Cell::updateAppearance() {
    QString style;
    switch (currentView) {
    case ViewState::Questioned:
        setText("❓");
        style = "QPushButton { width: 40px; height: 40px; background-color: white; border: 1px solid black; border-radius: 5px; text-align: center; font-size: 18px; }";
        break;
    case ViewState::Hidden:
        setText("");
        style = "QPushButton { width: 40px; height: 40px; background-color: white; border: 1px solid black; border-radius: 5px; text-align: center; font-size: 18px; }";
        break;
    case ViewState::Revealed:
        if (is_mine) {
            setText("💥");
            style = "QPushButton { width: 40px; height: 40px; background-color: #F7C6C7; border: 1px solid black; border-radius: 5px; text-align: center; font-size: 18px; }";
        } else if (minesAround() > 0) {
            setText(QString::number(minesAround()));
            style = "QPushButton { width: 40px; height: 40px; background-color: white; border: 1px solid black; border-radius: 5px; text-align: center; font-size: 18px; }";
        } else {
            setText("");
            style = "QPushButton { width: 40px; height: 40px; background-color: #B3CDE0; border: 1px solid black; border-radius: 5px; text-align: center; font-size: 18px; }";
        }
        break;
    case ViewState::Flagged:
        setText("🚩");
        style = "QPushButton { width: 40px; height: 40px; background-color: white; border: 1px solid black; border-radius: 5px; text-align: center; font-size: 18px; }";
        break;
    case ViewState::ExplodedMine:
        setText("💥");
        style = "QPushButton { width: 40px; height: 40px; background-color: red; border: 1px solid black; border-radius: 5px; text-align: center; font-size: 18px; }";
        break;
    }
    setStyleSheet(style);
}
