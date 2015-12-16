#include "deck.h"

Deck::Deck(QWidget *parent) : QWidget(parent)
{
    //**********************************************
    //for easier image loading into pixmaps
    cardNumber = 1;
    imageSuit = new QString[4]();
    imageSuit[0] = "c.jpg";
    imageSuit[1] = "d.jpg";
    imageSuit[2] = "h.jpg";
    imageSuit[3] = "s.jpg";
    //**********************************************

    //create a card pixmap to hold the 9 card images that are in play
    //2 player, 2 dealer, 5 table cards.
    card = new QPixmap[9]();

    //load up my 52 card deck into pixmap array of size 52********
    //which represents the games deck
    cardImage = new QPixmap[52]();
    for(int a = 0, b = 0; a < 52 and b < 4; a++, b++)
    {
        string cardNumberString;
        ostringstream convert;
        convert << cardNumber;
        cardNumberString = convert.str();

        cardString = ":/Images/Cards/" + cardNumberString + imageSuit[b].toStdString();
        QString qstr = QString::fromStdString(cardString);

        //load all cards text representation into vector
        //output is ":/Images/Cards/1s.jpg"
        cardTextRepresentation.push_back(cardString);

        if(b==3 and a < 52)
        {
            b = -1;
            cardNumber++;
        }
    }
    //end of loading 52 cards into pixmap array**********************

    shuffle();
}

//shuffle my deck using srand
void Deck::shuffle()
{
    cardText.clear();
    std::srand(unsigned (std::time(0)));
    std::random_shuffle(cardTextRepresentation.begin(),cardTextRepresentation.end());
    cardIndex = -1;
}

//deal card from deck
QPixmap Deck::getCardImage()
{
    cardIndex++;
    if(cardIndex == 52)
    {
        cardIndex = 0;
    }

    card[cardIndex].load(QString::fromStdString(cardTextRepresentation[cardIndex]));
    lastChar = cardTextRepresentation[cardIndex].find_last_of('/');
    lengthChar = cardTextRepresentation[cardIndex].find_last_of('.');
    cardText.push_back(cardTextRepresentation[cardIndex].substr(lastChar+1,(lengthChar-lastChar)-1));
    return card[cardIndex];
}
