#include "cell.h"
#include "minesweeper.h"

#include <QApplication>
#include <QComboBox>
#include <QDebug>
#include <QFile>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QTimer>
#include <QToolBar>
#include <QTranslator>
#include <QVBoxLayout>

Minesweeper::Minesweeper(bool arg, QWidget *parent) :
    QMainWindow(parent), rows(10), cols(10), totalMines(10), flaggedMines(0), firstClick(true), lefthandedMode(false),
    dbgMode(arg), loadedData(false), resumingGameAvalability(true), currentLanguage("fr"), finishedGame(false)
{
    loadSettings();
    createMenu();
    createToolBar();
    askForGameProperties();
}

void Minesweeper::createMenu()
{
    menuBar = new QMenuBar(this);
    setMenuBar(menuBar);

    QMenu *gameMenu = menuBar->addMenu(tr("Game"));

    QAction *newGameAction = new QAction(tr("New Game with Same Parameters"), this);
    connect(newGameAction, &QAction::triggered, this, &Minesweeper::restartGameWithSameParams);
    gameMenu->addAction(newGameAction);

    QAction *newGameWithParamsAction = new QAction(tr("New Game with New Parameters"), this);
    connect(newGameWithParamsAction, &QAction::triggered, this, &Minesweeper::startNewGameWithParams);
    gameMenu->addAction(newGameWithParamsAction);

    languageMenu = menuBar->addMenu(tr("Language"));
    languageActionGroup = new QActionGroup(this);

    englishAction = new QAction("English", this);
    englishAction->setCheckable(true);

    languageMenu->addAction(englishAction);
    languageActionGroup->addAction(englishAction);
    connect(englishAction, &QAction::triggered, this, &Minesweeper::ENtranslate);

    frenchAction = new QAction("Française", this);
    frenchAction->setCheckable(true);

    languageMenu->addAction(frenchAction);
    languageActionGroup->addAction(frenchAction);
    connect(frenchAction, &QAction::triggered, this, &Minesweeper::FRtranslate);

    if (currentLanguage == "en")
    {
        englishAction->setChecked(true);
    }
    else if (currentLanguage == "fr")
    {
        frenchAction->setChecked(true);
    }
}
void Minesweeper::createToolBar()
{
    toolBar = addToolBar(tr("Game Stats"));

    flaggedMinesLabel = new QLCDNumber;
    flaggedMinesLabel->setSegmentStyle(QLCDNumber::Flat);
    flaggedMinesLabel->setDigitCount(3);
    toolBar->addWidget(new QLabel(tr("Flagged:")));
    toolBar->addWidget(flaggedMinesLabel);

    totalMinesLabel = new QLCDNumber;
    totalMinesLabel->setSegmentStyle(QLCDNumber::Flat);
    totalMinesLabel->setDigitCount(3);
    toolBar->addWidget(new QLabel(tr("Total:")));
    toolBar->addWidget(totalMinesLabel);

    remainingMinesLabel = new QLCDNumber;
    remainingMinesLabel->setSegmentStyle(QLCDNumber::Flat);
    remainingMinesLabel->setDigitCount(3);
    toolBar->addWidget(new QLabel(tr("Remaining:")));
    toolBar->addWidget(remainingMinesLabel);

    restartButton = new QPushButton(tr("Restart"));
    toolBar->addWidget(restartButton);
    connect(restartButton, &QPushButton::clicked, this, &Minesweeper::restartGameWithSameParams);

    newGameButton = new QPushButton(tr("New Game"));
    toolBar->addWidget(newGameButton);
    connect(newGameButton, &QPushButton::clicked, this, &Minesweeper::startNewGameWithParams);

    lefthandedButton = new QPushButton(tr("LeftHanded"));
    toolBar->addWidget(lefthandedButton);
    connect(lefthandedButton, &QPushButton::clicked, this, &Minesweeper::toggleLefthandedMode);

    if (dbgMode)
    {
        spy = new QPushButton(tr("Spy"));
        toolBar->addWidget(spy);
        connect(spy, &QPushButton::pressed, this, &Minesweeper::spying);
        connect(spy, &QPushButton::released, this, &Minesweeper::hideSpying);
    }
}

void Minesweeper::updateMinesCounter()
{
    flaggedMinesLabel->display(flaggedMines);
    totalMinesLabel->display(totalMines);
    remainingMinesLabel->display(totalMines - flaggedMines);
}

void Minesweeper::askForGameProperties()
{
    dialog = new QDialog(this);
    dialog->setWindowTitle(tr("Game Properties"));
    QVBoxLayout *layout = new QVBoxLayout(dialog);

    labelRows = new QLabel(tr("Enter rows:"));
    QLineEdit *lineEditRows = new QLineEdit;
    lineEditRows->setText(QString::number(rows));

    labelCols = new QLabel(tr("Enter columns:"));
    QLineEdit *lineEditCols = new QLineEdit;
    lineEditCols->setText(QString::number(cols));

    labelMines = new QLabel(tr("Enter number of mines:"));
    QLineEdit *lineEditMines = new QLineEdit;
    lineEditMines->setText(QString::number(totalMines));

    buttonOk = new QPushButton(tr("Start New Game"));
    buttonResume = new QPushButton(tr("Resume Game"));
    layout->addWidget(labelRows);
    layout->addWidget(lineEditRows);
    layout->addWidget(labelCols);
    layout->addWidget(lineEditCols);
    layout->addWidget(labelMines);
    layout->addWidget(lineEditMines);
    layout->addWidget(buttonOk);
    if (resumingGameAvalability)
    {
        layout->addWidget(buttonResume);
    }
    dialog->setLayout(layout);

    connect(
        buttonOk,
        &QPushButton::clicked,
        [this, lineEditRows, lineEditCols, lineEditMines]()
        {
            int rowsEntered = lineEditRows->text().toInt();
            int colsEntered = lineEditCols->text().toInt();
            int minesEntered = lineEditMines->text().toInt();

            if (rowsEntered > 0 && rowsEntered <= 30 && colsEntered > 0 && colsEntered <= 30 && minesEntered > 0 && minesEntered < 100)
            {
                this->rows = rowsEntered;
                this->cols = colsEntered;
                this->totalMines = minesEntered;
                loadedData = false;

                dialog->accept();
                setupGame();
            }
            else
            {
                QMessageBox::warning(dialog, tr("Invalid Input"), tr("Please enter valid values."));
            }
        });

    connect(buttonResume,
            &QPushButton::clicked,
            [this]()
            {
                if (QFile::exists("game_state.ini") && !finishedGame)
                {
                    loadedData = true;
                    loadGameState();
                    dialog->accept();
                    setupGame();
                }
                else
                {
                    QMessageBox::warning(dialog, tr("No Saved Game"), tr("No saved game found to resume."));
                }
            });

    dialog->exec();
}

void Minesweeper::setupGame()
{
    finishedGame = false;
    if (dbgMode)
    {
        spy->setEnabled(true);
    }
    lefthandedButton->setEnabled(true);

    centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    int buttonSize = 40;
    int totalWidth = buttonSize * cols;
    int totalHeight = buttonSize * rows;

    QWidget *gridContainer = new QWidget(this);
    gridContainer->setFixedSize(totalWidth, totalHeight);
    gridLayout = new QGridLayout(gridContainer);
    gridLayout->setSpacing(0);
    gridLayout->setContentsMargins(0, 0, 0, 0);

    cells.resize(rows);
    mines.resize(rows);
    for (int row = 0; row < rows; ++row)
    {
        mines[row].resize(cols);
    }
    firstClick = true;
    flaggedMines = 0;
    updateMinesCounter();
    if (loadedData)
    {
        for (int row = 0; row < rows; ++row)
        {
            for (int col = 0; col < cols; ++col)
            {
                Cell *cell = cells[row][col];
                gridLayout->addWidget(cell, row, col);
                connectCellSignals(row, col);
            }
        }
    }
    else
    {
        for (int row = 0; row < rows; ++row)
        {
            cells[row].resize(cols);
            for (int col = 0; col < cols; ++col)
            {
                cells[row][col] = new Cell(this);
                gridLayout->addWidget(cells[row][col], row, col);
                connectCellSignals(row, col);
            }
        }
    }
    for (int row = 0; row < rows; ++row)
    {
        gridLayout->setRowStretch(row, 0);
    }
    for (int col = 0; col < cols; ++col)
    {
        gridLayout->setColumnStretch(col, 0);
    }
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->addStretch();
    mainLayout->addWidget(gridContainer, 0, Qt::AlignCenter);
    mainLayout->addStretch();
    // setLayout(mainLayout);
}

void Minesweeper::loadGameState()
{
    QSettings settings("game_state.ini", QSettings::IniFormat);
    settings.beginGroup("Game");
    rows = settings.value("rows", 10).toInt();
    cols = settings.value("cols", 10).toInt();
    totalMines = settings.value("totalMines", 10).toInt();
    firstClick = settings.value("firstClick", true).toBool();
    settings.endGroup();

    cells.resize(rows);
    for (int row = 0; row < rows; ++row)
    {
        cells[row].resize(cols);
    }
    settings.beginGroup("Field");
    for (int row = 0; row < rows; ++row)
    {
        for (int col = 0; col < cols; ++col)
        {
            QString key = QString("cell_%1_%2").arg(row).arg(col);
            Cell *cell = new Cell(this);
            cells[row][col] = cell;

            cell->setIsMine(settings.value(key + "_isMine", false).toBool());
            if (settings.value(key + "_isRevealed", false).toBool())
            {
                cell->setRevealed();
            }
            cell->setMinesAround(settings.value(key + "_minesAround", 0).toInt());
            int viewState = settings.value(key + "_viewState", static_cast< int >(Cell::ViewState::Hidden)).toInt();
            cell->setCurrentView(static_cast< Cell::ViewState >(viewState));
        }
    }
    // currentLanguage = settings.value("language", "en").toString();
    if (currentLanguage == "fr")
    {
        translator.load(":/minesweeper_fr.qm");
        qApp->installTranslator(&translator);
    }
    else if (currentLanguage == "en")
    {
        translator.load(":/minesweeper_en.qm");
        qApp->installTranslator(&translator);
    }
    settings.endGroup();
    loadedData = true;
}

void Minesweeper::connectCellSignals(int row, int col)
{
    connect(cells[row][col],
            &Cell::leftClicked,
            [=]()
            {
                if (lefthandedMode)
                {
                    onRightClick(row, col);
                }
                else
                {
                    onLeftClick(row, col);
                }
            });

    connect(cells[row][col],
            &Cell::rightClicked,
            [=]()
            {
                if (lefthandedMode)
                {
                    onLeftClick(row, col);
                }
                else
                {
                    onRightClick(row, col);
                }
            });

    connect(cells[row][col], &Cell::centerClicked, [=]() { onCenterClick(row, col); });
}

void Minesweeper::generateMines(int firstClickedRow, int firstClickedCol)
{
    srand(static_cast< unsigned int >(time(0)));

    auto isSafeCell = [=](int r, int c)
    {
        return !(r >= firstClickedRow - 1 && r <= firstClickedRow + 1 && c >= firstClickedCol - 1 && c <= firstClickedCol + 1);
    };

    int minesPlaced = 0;
    while (minesPlaced < totalMines)
    {
        int r = rand() % rows;
        int c = rand() % cols;

        if (isSafeCell(r, c) && !cells[r][c]->isMine())
        {
            cells[r][c]->setIsMine(true);
            minesPlaced++;
        }
    }

    for (int row = 0; row < rows; ++row)
    {
        for (int col = 0; col < cols; ++col)
        {
            if (cells[row][col]->isMine())
            {
                cells[row][col]->setMinesAround(-1);
            }
            else
            {
                int mineCount = countAdjacentMines(row, col);
                cells[row][col]->setMinesAround(mineCount);
            }
        }
    }
    updateMinesCounter();
}

int Minesweeper::countAdjacentMines(int row, int col)
{
    int count = 0;
    for (int r = row - 1; r <= row + 1; ++r)
    {
        for (int c = col - 1; c <= col + 1; ++c)
        {
            if (isValidCoord(r, c) && cells[r][c]->isMine())
            {
                count++;
            }
        }
    }
    return count;
}

bool Minesweeper::isValidCoord(int row, int col) const
{
    return row >= 0 && row < rows && col >= 0 && col < cols;
}

void Minesweeper::revealEverything(int clickedRow, int clickedCol, bool spyMode)
{
    QString style;
    for (int row = 0; row < rows; ++row)
    {
        for (int col = 0; col < cols; ++col)
        {
            if (cells[row][col]->isMine() && !(row == clickedRow && col == clickedCol))
            {
                cells[row][col]->setText("💥");
                style = "QPushButton { width: 40px; height: 40px; background-color:  #F7C6C7; border: 1px solid black; "
                        "border-radius: 5px; text-align: center; font-size: 18px; }";
            }
            else if (cells[row][col]->isMine())
            {
                cells[row][col]->setText("🤯");
                style = "QPushButton { width: 40px; height: 40px; background-color: #F7C6C7; border: 1px solid black; "
                        "border-radius: 5px; text-align: center; font-size: 18px; }";
            }
            else if (countAdjacentMines(row, col) != 0)
            {
                cells[row][col]->setText(QString::number(countAdjacentMines(row, col)));
                style = "QPushButton { width: 40px; height: 40px; background-color: white; border: 1px solid black; "
                        "border-radius: 5px; text-align: center; font-size: 18px; }";
            }
            else
            {
                style = "QPushButton { width: 40px; height: 40px; background-color: #B3CDE0; border: 1px solid black; "
                        "border-radius: 5px; text-align: center; font-size: 18px; }";
            }
            cells[row][col]->setStyleSheet(style);
            if (!spyMode)
            {
                cells[row][col]->setDisabled(true);
            }
        }
    }
}

void Minesweeper::revealEmptyCells(int row, int col)
{
    if (!isValidCoord(row, col) || cells[row][col]->isRevealed())
    {
        return;
    }
    if (cells[row][col]->minesAround() != 0)
    {
        cells[row][col]->setRevealed();
        return;
    }
    cells[row][col]->setRevealed();
    cells[row][col]->setDisabled(true);
    for (int r = row - 1; r <= row + 1; ++r)
    {
        for (int c = col - 1; c <= col + 1; ++c)
        {
            if (isValidCoord(r, c) && !(r == row && c == col))
            {
                revealEmptyCells(r, c);
            }
        }
    }
}

bool Minesweeper::checkWinCondition()
{
    for (int row = 0; row < rows; ++row)
    {
        for (int col = 0; col < cols; ++col)
        {
            if (!cells[row][col]->isMine() && !cells[row][col]->isRevealed() && cells[row][col]->text().isEmpty())
            {
                return false;
            }
        }
    }
    return true;
}

void Minesweeper::gameOver()
{
    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Critical);
    updateMinesCounter();
    msgBox.setWindowTitle(tr("Game is over!"));
    msgBox.setText(tr("You pressed on the mine"));
    // lefthandedButton->setEnabled(false);
    //  spy->setEnabled(false);
    finishedGame = true;
    msgBox.exec();
}
void Minesweeper::onLeftClick(int row, int col)
{
    if (firstClick && !loadedData)
    {
        generateMines(row, col);
        qDebug() << "mines are set";
        firstClick = false;
    }
    Cell::ViewState currentView = cells[row][col]->getCurrentView();
    if (currentView == Cell::ViewState::Flagged || currentView == Cell::ViewState::Questioned)
    {
        return;
    }
    else if (cells[row][col]->isMine())
    {
        cells[row][col]->setCurrentView(Cell::ViewState::ExplodedMine);
        revealEverything(row, col, 0);
        gameOver();
        return;
    }
    else if (cells[row][col]->minesAround() == 0)
    {
        revealEmptyCells(row, col);
    }
    else
    {
        cells[row][col]->setRevealed();
    }

    if (checkWinCondition())
    {
        revealEverything(row, col, 0);
        // lefthandedButton->setEnabled(false);
        // spy->setEnabled(false);
        finishedGame = true;
        QMessageBox::information(this, tr("Win!"), tr("Congratulations, you won."));
    }
}

void Minesweeper::onRightClick(int row, int col)
{
    Cell::ViewState currentView = cells[row][col]->getCurrentView();

    switch (currentView)
    {
    case Cell::ViewState::Hidden:
        cells[row][col]->setCurrentView(Cell::ViewState::Flagged);
        ++flaggedMines;
        break;

    case Cell::ViewState::Flagged:
        cells[row][col]->setCurrentView(Cell::ViewState::Questioned);
        --flaggedMines;
        break;

    case Cell::ViewState::Questioned:
        cells[row][col]->setCurrentView(Cell::ViewState::Hidden);
        break;

    default:
        break;
    }
    updateMinesCounter();
}

void Minesweeper::onCenterClick(int row, int col)
{
    Cell::ViewState currentView = cells[row][col]->getCurrentView();

    if (cells[row][col]->text().isEmpty() || currentView == Cell::ViewState::Flagged || currentView == Cell::ViewState::Questioned)
    {
        return;
    }

    int flaggedCount = 0;
    QVector< QPair< int, int > > hidden_cells;
    for (int r = row - 1; r <= row + 1; ++r)
    {
        for (int c = col - 1; c <= col + 1; ++c)
        {
            if (isValidCoord(r, c) && (r != row || c != col))
            {
                if (cells[r][c]->text() == "🚩")
                {
                    ++flaggedCount;
                }
                else if (!cells[r][c]->isRevealed())
                {
                    hidden_cells.append({ r, c });
                }
            }
        }
    }
    QString style;
    if (flaggedCount == cells[row][col]->minesAround())
    {
        for (const auto &cell : hidden_cells)
        {
            int r = cell.first;
            int c = cell.second;
            if (cells[r][c]->isMine())
            {
                // revealEverything(row, col, 0);
                // cells[row][col]->setCurrentView(Cell::ViewState::ExplodedMine);
                // gameOver();
                // return;

                cells[r][c]->setText("💥");
                style = "QPushButton { width: 40px; height: 40px; background-color: gray; border: 1px solid black; "
                        "border-radius: 5px; text-align: center; font-size: 18px; }";
                cells[row][col]->setStyleSheet(style);
                revealEverything(r, c, 0);
                gameOver();
                return;
            }
            else
            {
                if (cells[r][c]->minesAround() == 0)
                {
                    revealEmptyCells(r, c);
                }
                else
                {
                    cells[r][c]->setRevealed();
                    if (checkWinCondition())
                    {
                        revealEverything(row, col, 0);
                        // lefthandedButton->setEnabled(false);
                        // spy->setEnabled(false);
                        finishedGame = true;
                        QMessageBox::information(this, tr("Win!"), tr("Congratulations, you won."));
                    }
                }
            }
        }
    }
    else
    {
        for (const auto &cell : hidden_cells)
        {
            int r = cell.first;
            int c = cell.second;
            if (!cells[r][c]->isRevealed())
            {
                style = "QPushButton { width: 40px; height: 40px; background-color: #F0E68C; border: 1px solid black; "
                        "border-radius: 5px; text-align: center; font-size: 18px; }";
                cells[r][c]->setStyleSheet(style);
            }
        }

        QTimer::singleShot(
            1000,
            [this, hidden_cells]()
            {
                for (const auto &cell : hidden_cells)
                {
                    int r = cell.first;
                    int c = cell.second;
                    QString style;
                    if (!cells[r][c]->isRevealed())
                    {
                        style = "QPushButton { width: 40px; height: 40px; background-color: white; border: 1px solid "
                                "black; border-radius: 5px; text-align: center; font-size: 18px; }";
                        cells[r][c]->setStyleSheet(style);
                    }
                }
            });
    }
}

void Minesweeper::clearField()
{
    if (gridLayout)
    {
        QLayoutItem *item;
        while ((item = gridLayout->takeAt(0)) != nullptr)
        {
            if (item->widget())
            {
                delete item->widget();
            }
            delete item;
        }
        delete gridLayout;
        gridLayout = nullptr;
    }

    cells.clear();
    mines.clear();
    firstClick = true;
    flaggedMines = 0;
}

void Minesweeper::saveGameState()
{
    QSettings settings("game_state.ini", QSettings::IniFormat);

    settings.beginGroup("Game");
    settings.setValue("rows", rows);
    settings.setValue("cols", cols);
    settings.setValue("totalMines", totalMines);
    settings.setValue("firstClick", firstClick);
    settings.setValue("language", currentLanguage);
    settings.endGroup();

    settings.beginGroup("Field");
    for (int row = 0; row < rows; ++row)
    {
        for (int col = 0; col < cols; ++col)
        {
            QString key = QString("cell_%1_%2").arg(row).arg(col);
            Cell *cell = cells[row][col];
            settings.setValue(key + "_isMine", cell->isMine());
            settings.setValue(key + "_isRevealed", cell->isRevealed());
            settings.setValue(key + "_minesAround", cell->minesAround());
            settings.setValue(key + "_viewState", static_cast< int >(cell->getCurrentView()));
        }
    }
    settings.endGroup();
}

void Minesweeper::spying()
{
    for (int row = 0; row < rows; ++row)
    {
        for (int col = 0; col < cols; ++col)
        {
            cells[row][col]->setStyleSheet("background-color: lightgray;");
            revealEverything(row, col, 1);
        }
    }
}

void Minesweeper::hideSpying()
{
    for (int row = 0; row < rows; ++row)
    {
        for (int col = 0; col < cols; ++col)
        {
            Cell::ViewState currentView = cells[row][col]->getCurrentView();
            cells[row][col]->setCurrentView(currentView);
        }
    }
}

void Minesweeper::restartGameWithSameParams()
{
    clearField();
    loadedData = false;
    resumingGameAvalability = false;
    setupGame();
}

void Minesweeper::startNewGameWithParams()
{
    clearField();
    loadedData = false;
    resumingGameAvalability = false;
    askForGameProperties();
}

void Minesweeper::toggleLefthandedMode()
{
    lefthandedMode = !lefthandedMode;
    QString text = lefthandedMode ? "RightHanded" : "LeftHanded";
    lefthandedButton->setText(text);
}

void Minesweeper::saveSettings()
{
    QSettings settings("MyCompany", "minesweeper");
    settings.setValue("finishedGame", finishedGame);
    settings.setValue("language", currentLanguage);
}

void Minesweeper::loadSettings()
{
    QSettings settings("MyCompany", "minesweeper");
    currentLanguage = settings.value("language", "en").toString();
    finishedGame = settings.value("finishedGame", "false").toBool();
    if (currentLanguage == "fr")
    {
        translator.load(":/minesweeper_fr.qm");
        qApp->installTranslator(&translator);
    }
    else if (currentLanguage == "en")
    {
        translator.load(":/minesweeper_en.qm");
        qApp->installTranslator(&translator);
    }
}

void Minesweeper::closeEvent(QCloseEvent *event)
{
    if (!finishedGame)
    {
        saveGameState();
    }
    saveSettings();
    event->accept();
}

void Minesweeper::FRtranslate()
{
    translator.load(":/minesweeper_fr.qm");
    qApp->installTranslator(&translator);
    currentLanguage = "fr";
    toolBar->clear();
    delete toolBar;
    createToolBar();
    delete menuBar;
    createMenu();
    frenchAction->setChecked(true);
}

void Minesweeper::ENtranslate()
{
    translator.load(":/minesweeper_en.qm");
    qApp->installTranslator(&translator);
    currentLanguage = "en";
    toolBar->clear();
    delete toolBar;
    createToolBar();
    delete menuBar;
    createMenu();
    englishAction->setChecked(true);
}
