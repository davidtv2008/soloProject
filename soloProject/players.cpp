#include "players.h"
#include <QObject>

Players::Players()
{
    myCard = new QPixmap[2]();
    myCardIndex = 0;
    cash = 0;
    tempCash = 0;
    chance = 0;
    chanceLow = 0;
    chanceHigh = 10;
    fold = false;
    analyzed = false;
    four = false;
    three = false;
    two = false;
    one = false;
    analyzed = false;
}

void Players::myHand(QPixmap card)
{
    myCard[myCardIndex] = card;
    myCardIndex++;
    if(myCardIndex == 2)
    {
        myCardIndex = 0;
    }
}

void Players::setName(QString name)
{
    userName = name;
}


int Players::aiAutomatedBet(bool raised,int amountRaised)
{
    if(raised == true)
    {
        if(cash >= amountRaised)
        {
            qsrand(qrand());
            chance = (qrand()%((chanceHigh)-chanceLow)+chanceLow);
            if(chance <= 8)
            {
                cash -= amountRaised;
                return amountRaised;
            }
            else if(chance > 8)
            {
                fold = true;
                return 0;
            }
        }
        else if (cash < amountRaised)
        {
            qsrand(qrand());
            chance = (qrand()%((chanceHigh)-chanceLow)+chanceLow);
            if(chance <= 8)
            {
                tempCash = 0;
                tempCash = cash;
                cash -= tempCash;
                return tempCash;
            }
            else if(chance > 8)
            {
                fold = true;
                return 0;
            }
        }
    }
    else if(raised == false)
    {
        cash -= 40;
        return 40;
    }
}

void Players::dealerCashResetThread()
{
    cash = 0;
}

void Players::playerCashResetThread()
{
    cash = 0;
}


