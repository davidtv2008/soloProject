#ifndef DECK_H
#define DECK_H

#include <QWidget>
#include <QPixmap>
#include <QString>
#include <QLabel>
#include <QVBoxLayout>
#include <iostream>
#include <String>
#include <sstream>
#include <vector>
#include <QVector>
#include <algorithm>
#include <ctime>
#include <cstdlib>

using namespace std;

class Deck : public QWidget
{
    Q_OBJECT
public:
    explicit Deck(QWidget *parent = 0);

    int cardIndex;
    int lastChar;
    int lengthChar;
    std::vector<string> cardTextRepresentation;
    std::vector<string> cardText;
    //string cardText;
    QPixmap getCardImage();
    void shuffle();
    QPixmap *card;

private:
    QPixmap *cardImage;
    QString *imageSuit;
    string cardString;
    int cardNumber;
    std::vector<QPixmap> cardVector;

signals:

public slots:

};

#endif // DECK_H
