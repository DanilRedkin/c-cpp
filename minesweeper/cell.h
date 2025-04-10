#ifndef CELL_H
#define CELL_H

#include <QMouseEvent>
#include <QPushButton>
#include <QString>

class Cell : public QPushButton {
    Q_OBJECT

public:
    enum class ViewState {
        Hidden,
        Revealed,
        Questioned,
        Flagged,
        ExplodedMine
    };

    explicit Cell(QWidget *parent = nullptr);

    bool isRevealed() const;
    bool isMine() const;
    void setIsMine(bool mine);

    int minesAround() const;
    void setMinesAround(int count);

    void setRevealed();
    void setCurrentView(ViewState view);
    ViewState getCurrentView() const;

signals:
    void leftClicked();
    void rightClicked();
    void centerClicked();

protected:
    void mousePressEvent(QMouseEvent *event) override;

private:
    bool is_mine;
    int mines_around;
    ViewState currentView;
    bool revealed;

    void updateAppearance();
};

#endif// CELL_H
