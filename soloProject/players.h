#ifndef PLAYERS_H
#define PLAYERS_H
#include <QPixmap>
#include <ctime>
#include <cstdlib>
#include <iostream>
#include <QObject>
//#include "GameWindow.h"

using namespace std;

class Players : public QObject
{

public:
    Players();
    void myHand(QPixmap);
    std::vector<string> cardCombo1;
    std::vector<string> cardCombo2;
    std::vector<string> cardCombo3;
    std::vector<string> cardCombo4;
    std::vector<string> cardCombo5;
    std::vector<string> cardCombo6;
    std::vector<string> cardCombo7;
    std::vector<string> cardCombo8;
    std::vector<string> cardCombo9;
    std::vector<string> cardCombo10;
    std::vector<string> cardsAnalyzed;
    bool analyzed;

    int myCardIndex;
    void setName(QString);
    QPixmap *myCard;
    int cash;
    int tempCash;
    int aiAutomatedBet(bool,int);
    int chance;
    int chanceLow;
    int chanceHigh;
    bool fold;
    char cardSuite[5];
    bool straightFlush;
    bool straight;
    bool flush;
    bool fullHouse;
    bool four;
    bool three;
    bool two;
    bool one;
    vector<int> fourKindCombo;
    vector<int> threeKindCombo;
    vector<int> twoPairCombo;
    vector<int> singlePairCombo;
    vector<int> bestHand;
    int cardNumbericValue[5];
    int cardScore;
    int handScore;
    QString handName;
    QString userName;

private:




signals:

public slots:
    void dealerCashResetThread();
    void playerCashResetThread();

};

#endif // PLAYERS_H
