#ifndef MINESWEEPER_H
#define MINESWEEPER_H

#include "cell.h"

#include <QActionGroup>
#include <QComboBox>
#include <QDialog>
#include <QGridLayout>
#include <QLCDNumber>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QTranslator>
#include <QVBoxLayout>
#include <QVector>
#include <QWidget>

class Minesweeper : public QMainWindow
{
    Q_OBJECT

private:
    QWidget *centralWidget;
    QGridLayout *gridLayout;
    QVector< QVector< Cell * > > cells;
    QVector< QVector< bool > > mines;
    QLCDNumber *flaggedMinesLabel;
    QLCDNumber *totalMinesLabel;
    QLCDNumber *remainingMinesLabel;
    QPushButton *spy;
    int rows;
    int cols;
    int totalMines;
    int flaggedMines;
    bool firstClick;
    bool lefthandedMode;
    bool dbgMode;
    bool loadedData;
    bool resumingGameAvalability;
    bool finishedGame;
    QToolBar *toolBar;
    QTranslator translator;
    QActionGroup *languageActionGroup;
    QMenu *languageMenu;
    QString currentLanguage;
    QPushButton *newGameButton;
    QPushButton *lefthandedButton;
    QDialog *dialog;
    QPushButton *restartButton;
    QMenuBar *menuBar;

    QLabel *labelRows;
    QLabel *labelCols;
    QLabel *labelMines;
    QComboBox *comboBoxLanguage;
    QPushButton *buttonOk;
    QPushButton *buttonResume;
    QAction *frenchAction;
    QAction *englishAction;

public:
    explicit Minesweeper(bool arg, QWidget *parent = nullptr);
    void createMenu();
    void createToolBar();
    void closeEvent(QCloseEvent *event) override;

private:
    void updateMinesCounter();
    void showHelp();
    void askForGameProperties();
    void setupGame();
    void loadGameState();
    void connectCellSignals(int row, int col);
    void generateMines(int firstClickedRow, int firstClickedCol);
    int countAdjacentMines(int row, int col);
    bool isValidCoord(int row, int col) const;
    void revealEverything(int clickedRow, int clickedCol, bool spyMode);
    void revealEmptyCells(int row, int col);
    bool checkWinCondition();
    void gameOver();
    void onLeftClick(int row, int col);
    void onRightClick(int row, int col);
    void onCenterClick(int row, int col);
    void clearField();
    void saveGameState();
    void changeLanguage(QAction *action);
    void changeeLanguage(QString lang);

private slots:
    void FRtranslate();
    void ENtranslate();
    void spying();
    void hideSpying();
    void restartGameWithSameParams();
    void startNewGameWithParams();
    void toggleLefthandedMode();
    void saveSettings();
    void loadSettings();
};

#endif	  // MINESWEEPER_H
