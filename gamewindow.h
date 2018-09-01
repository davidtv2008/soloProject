#ifndef GAMEWINDOW_H
#define GAMEWINDOW_H

#include <QWidget>
#include <QPushButton>
#include <iostream>
#include <QDesktopWidget>
#include <QLabel>
#include <QFont>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QMainWindow>
#include "deck.h"
#include "Players.h"
#include <QSpinBox>
#include <QSlider>
#include <QTimer>
#include <iostream>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QDialog>
#include <QInputDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QSql>
#include <QSqlQuery>
#include <QSqlDatabase>
#include <QThread>


using namespace std;

enum activeTurn{none,gamer,aiPlayer};
enum betting{call,check,raised};
enum win{nothing,dealer,player1};
//enum hand{straightFlush,fourOfAKind,fullHouse,flush,straight,threeOfAKind,twoPair,onePair,highCard};

class GameWindow : public QWidget
{
    Q_OBJECT

public:
    explicit GameWindow(QWidget *parent = 0);

    Players *player;
    Players *ai;

    QThread *dealerCashThread;
    QThread *playerCashThread;

    void setDefaultBacksideCard();
    QTimer *reveal;
    void allInReveal();
    void dealTableCards();
    void analyzeDealerHand();
    void analyzePlayerHand();
    void determineWinnerHand();
    int dealerCashReset;
    int cardOccurence[10][13];
    string suite[10][5];
    bool dealerAnalyzed;
    bool playerAnalyzed;
    //QString winner;
    QString handName;
    QFont *font;

private:

    QInputDialog *userName;
    QDesktopWidget *monitor;
    Deck *deck;
    QLabel *backGroundLabel;
    QLabel *cardLabel;
    QLabel *playerCashLabel;
    QLabel *aiCashLabel;
    QLabel *potAmountLabel;
    QLabel *handNameLabel;
    QLabel *playerNameLabel;

    QPushButton *play;
    QPushButton *fold;
    QPushButton *call;
    QPushButton *raise;
    QPushButton *gameOver;
    QPushButton *loadProfile;
    QPushButton *newProfile;

    QMenuBar *menuBar;
    QMenu *fileMenu;
    QMenu *helpMenu;
    QAction *submenu[5];

    QWidget *profileWindow;

    QHBoxLayout *handNameLayout;
    QHBoxLayout *playerNameLayout;
    QHBoxLayout *openCardLayout;
    QVBoxLayout *buttonLayout;
    QHBoxLayout *playerLayout1;
    QHBoxLayout *playerLayout2;
    QHBoxLayout *playerCashLayout;
    QHBoxLayout *aiLayout1;
    QHBoxLayout *aiLayout2;
    QHBoxLayout *aiCashLayout;
    QHBoxLayout *potAmountLayout;
    QHBoxLayout *profileLayout;
    QVBoxLayout *raiseLayout;
    QGridLayout *gridLayout;

    QFileDialog *saveDialog;
    QFile *file;
    QFile *file2;

    QWidget *helpWindow;

    QSpinBox *spinBox;
    QSlider *slider;

    activeTurn turn;
    betting bet;
    win winner;
    //hand playerHand;
    int playerHandIndex;
    //hand dealerHand;
    int dealerHandIndex;
    bool pairExistAlready;
    bool threeExistAlready;
    bool straightExists;
    bool flushExists;

    vector<int> dealerStoredHandIndex;
    vector<int> playerStoredHandIndex;
    vector<int> storedHandIndex;
    int handScore[10];
    int highestHandIndex;

    bool aiRaised;
    bool playerRaised;
    int potAmount;
    int tableCardIndex;
    int tableLastTurn;


signals:

public slots:
    void dealHandCards();
    void foldHand();
    void callCheckHand();
    void raiseHand();
    void aiAutomate();
    void resetCards();
    void allInTimedReveal();
    void resetCash();
    void handLabelHide();
    void dealerRaiseConfirm();
    void foldConfirmTimed();
    void dealerGamerCashReset();
    void helpMenuWindow();
    void newProfileDialog();
    void saveFileDialog();
    void loadFileDialog();
};

#endif // GAMEWINDOW_H
