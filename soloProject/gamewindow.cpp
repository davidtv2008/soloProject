#include "gamewindow.h"

struct Connection{
    QSqlDatabase db;
    bool connected;
};
Connection createConnection();

GameWindow::GameWindow(QWidget *parent) : QWidget(parent)
{ 
    //create a dialog for new username or load profile
    saveDialog = new QFileDialog(this);
    loadProfile =new QPushButton("LOAD PROFILE");
    newProfile = new QPushButton("NEW PROFILE");
    QObject::connect(newProfile,SIGNAL(clicked()),this,SLOT(newProfileDialog()));
    QObject::connect(loadProfile,SIGNAL(clicked()),this,SLOT(loadFileDialog()));
    profileLayout = new QHBoxLayout();
    profileWindow = new QWidget();
    profileLayout->addWidget(newProfile);
    profileLayout->addWidget(loadProfile);

    profileWindow->setLayout(profileLayout);

    profileWindow->show();
    profileWindow->raise();


    //create a shuffled deck
    deck = new Deck();
    // shuffle the deck
    deck->shuffle();

    //initialize whos turn it is, and if ai has called or raised.
    turn = aiPlayer;
    bet = check;
    winner = nothing;
    //playerHand = highCard;
    //dealerHand = highCard;
    dealerAnalyzed = false;
    playerAnalyzed = false;

    //initialize table to 0's to check card occurence when analyzing hands.
    for(int a = 0, b = 0; a < 10; b++)
    {
        //cardOccurence size is 10x13
        cardOccurence[a][b] = 0;
        if(b==12)
        {
            a++;
            b = -1;
        }
    }


    aiRaised = false;
    playerRaised = false;
    tableCardIndex = 4;
    tableLastTurn = 0;
    reveal = new QTimer(this);
    connect(reveal,SIGNAL(timeout()),this,SLOT(allInTimedReveal()));

    dealerCashReset = 1;
    gameOver = new QPushButton();
    gameOver->setText("Your are out of cash, click here to reset cash");

    //initialize pot amount to 0
    potAmount = 0;
    potAmountLabel = new QLabel();

    font = new QFont("Times", 20, QFont::Bold);

    potAmountLabel->setText("     Pot Amount $" + QString::number(potAmount));
    potAmountLabel->setFont(*font);
    potAmountLayout = new QHBoxLayout();
    potAmountLayout->addWidget(potAmountLabel);

    //create a player and the AI to play against
    player = new Players();
    playerCashThread = new QThread();
    player->moveToThread(playerCashThread);
    connect(playerCashThread, &QThread::finished, player, &QObject::deleteLater);
    //connect(playerCashThread, SIGNAL(started()), player, SLOT(playerCashResetThread()));


    playerCashLabel = new QLabel();
    playerCashLabel->setText("             $" + QString::number(player->cash));
    playerCashLabel->setFont(*font);
    playerCashLayout = new QHBoxLayout();
    playerCashLayout->addWidget(playerCashLabel);

    playerNameLabel = new QLabel();
    playerNameLabel->setText(player->userName);
    playerNameLabel->setFont(*font);
    playerNameLayout = new QHBoxLayout();
    playerNameLayout->addWidget(playerNameLabel);

    ai = new Players();
    dealerCashThread = new QThread();
    ai->moveToThread(dealerCashThread);
    connect(dealerCashThread, &QThread::finished, ai, &QObject::deleteLater);

    aiCashLabel = new QLabel();
    ai->cash = 0;
    aiCashLabel->setText("             $" + QString::number(ai->cash));
    aiCashLabel->setFont(*font);
    aiCashLayout = new QHBoxLayout();
    aiCashLayout->addWidget(aiCashLabel);

    handNameLabel = new QLabel();
    handNameLabel->setText("");
    handNameLabel->setFont(*font);
    handNameLayout = new QHBoxLayout();
    handNameLayout->addWidget(handNameLabel);

    //setup menubar with menus and submenus
    menuBar = new QMenuBar();
    fileMenu = new QMenu("File");
    helpMenu = new QMenu("Help");
    submenu[0] = new QAction("Load Profile",this);
    submenu[1] = new QAction("Save Profile",this);
    submenu[2] = new QAction("Close Game",this);
    submenu[3] = new QAction("Hand Ranking",this);
    submenu[4] = new QAction("New Profile",this);
    menuBar->addMenu(fileMenu);
    menuBar->addMenu(helpMenu);
    helpWindow = new QWidget();
    helpWindow->setStyleSheet("background-image: url(:/Images/Background/hands.gif)");
    helpWindow->setGeometry(500,200,500,420);
    helpWindow->setFixedSize(500,420);
    helpWindow->setVisible(false);
    fileMenu->addAction(submenu[4]);
    fileMenu->addAction(submenu[0]);
    fileMenu->addAction(submenu[1]);
    fileMenu->addAction(submenu[2]);
    helpMenu->addAction(submenu[3]);

    //connect submenus to slots for action to perform
    QObject::connect(submenu[1],SIGNAL(triggered()),this,SLOT(saveFileDialog()));
    QObject::connect(submenu[0],SIGNAL(triggered()),this,SLOT(loadFileDialog()));
    QObject::connect(submenu[4],SIGNAL(triggered()),this,SLOT(newProfileDialog()));

    //my push buttons to play,fold,call, and raise
    play = new QPushButton("Play");
    fold = new QPushButton("Fold");
    call = new QPushButton("Check");
    raise = new QPushButton("Raise");
    play->setEnabled(false);
    fold->setEnabled(false);
    call->setEnabled(false);
    raise->setEnabled(false);

    QObject::connect(play,SIGNAL(clicked()),this,SLOT(dealHandCards()));
    QObject::connect(fold,SIGNAL(clicked()),this,SLOT(foldHand()));
    QObject::connect(call,SIGNAL(clicked()),this,SLOT(callCheckHand()));
    QObject::connect(raise,SIGNAL(clicked()),this,SLOT(raiseHand()));
    QObject::connect(submenu[2],SIGNAL(triggered(bool)),this,SLOT(close()));
    connect(submenu[3],SIGNAL(triggered(bool)),this,SLOT(helpMenuWindow()));

    spinBox = new QSpinBox();
    slider = new QSlider(Qt::Vertical);
    raiseLayout = new QVBoxLayout();
    spinBox->setEnabled(false);
    slider->setEnabled(false);

    QObject::connect(spinBox, SIGNAL(valueChanged(int)),
                     slider, SLOT(setValue(int)));
    QObject::connect(slider, SIGNAL(valueChanged(int)),
                     spinBox, SLOT(setValue(int)));

    spinBox->setRange(1,player->cash);
    slider->setRange(1,player->cash);
    spinBox->setValue(1);
    raiseLayout->addWidget(spinBox);
    raiseLayout->addWidget(slider);

    //add the buttons to a horizontal layout
    buttonLayout = new QVBoxLayout();
    buttonLayout->addWidget(play);
    buttonLayout->addWidget(fold);
    buttonLayout->addWidget(call);
    buttonLayout->addWidget(raise);

    //layout to hold AI cards
    aiLayout1 = new QHBoxLayout();
    aiLayout2 = new QHBoxLayout();

    // layout to hold player cards
    playerLayout1 = new QHBoxLayout();
    playerLayout2 = new QHBoxLayout();

    //layout to hold flop, fourth street, river
    openCardLayout = new QHBoxLayout[5]();

    // this gridlayout will be used to manage other layouts locations on table
    gridLayout = new QGridLayout();

    //this will hold my table background
    backGroundLabel = new QLabel();
    backGroundLabel->setPixmap(QPixmap(":/Images/Background/Table.png"));

    //array of size 9 to hold 2 cards AI, 2 cards player, and 5 table
    cardLabel = new QLabel[9]();

    //initialize all cards to backside image
    setDefaultBacksideCard();

    //add the table background
    gridLayout->addWidget(backGroundLabel,0,0,0,0,0);

    //add in the AI, player, and deck layouts to the grid
    gridLayout->addLayout(aiLayout1,0,3,1,1,0);
    gridLayout->addLayout(aiLayout2,0,4,1,1,0);
    gridLayout->addLayout(aiCashLayout,0,4,2,2,0);
    gridLayout->addLayout(handNameLayout,2,1,2,4,0);
    gridLayout->addLayout(&openCardLayout[0],1,1,2,1,0);
    gridLayout->addLayout(&openCardLayout[1],1,2,2,1,0);
    gridLayout->addLayout(&openCardLayout[2],1,3,2,1,0);
    gridLayout->addLayout(&openCardLayout[3],1,4,2,1,0);
    gridLayout->addLayout(&openCardLayout[4],1,5,2,1,0);
    gridLayout->addLayout(playerLayout1,3,3,1,1,0);
    gridLayout->addLayout(playerLayout2,3,4,1,1,0);
    gridLayout->addLayout(playerNameLayout,3,1,1,2,0);
    gridLayout->addLayout(playerCashLayout,3,4,1,2,0);
    gridLayout->addLayout(buttonLayout,3,0,1,1,0);
    gridLayout->addLayout(raiseLayout,2,0,1,1,0);
    gridLayout->addLayout(potAmountLayout,1,1,1,3,0);

    //add the menuBar
    gridLayout->setMenuBar(menuBar);

    setLayout(gridLayout);
    setGeometry(200,50,900,650);
    //prevent resizing
    this->setFixedSize(900,650);
    //show the table all setup with default backside image cards
    show();

    //make user of threads, this will be used to analyze dealers and player hand simultaneously


    profileWindow->raise();
    profileWindow->setFixedSize(300,100);
    profileWindow->setGeometry(500,300,300,100);
    profileWindow->setFocusPolicy(Qt::StrongFocus);
}


void GameWindow::helpMenuWindow()
{
    helpWindow->setVisible(true);
}

void GameWindow::dealHandCards()
{
    //set all settings to its default values
    //*******************************
    //
    //
    //*******************************
    winner = nothing;
    tableCardIndex = 4;
    play->setEnabled(false);
    fold->setEnabled(true);
    call->setEnabled(true);
    raise->setEnabled(true);
    handNameLabel->setVisible(false);


    //shuffle the deck
    setDefaultBacksideCard();
    deck->shuffle();

    //initial play amount is $20
    player->cash -= 20;
    playerCashLabel->setText("             $" + QString::number(player->cash));
    spinBox->setRange(1,player->cash);
    slider->setRange(1,player->cash);
    spinBox->setValue(1);
    ai->cash -= 20;
    aiCashLabel->setText("             $" + QString::number(ai->cash));

    //increase pot amount by initial play amount
    potAmount += 40;
    potAmountLabel->setText("     Pot Amount $" + QString::number(potAmount));

    //deal first 2cards to player and ai
    player->myHand(deck->getCardImage());
    player->myHand(deck->getCardImage());
    ai->myHand(deck->getCardImage());
    ai->myHand(deck->getCardImage());

    //only reveal the players 2 cards
    cardLabel[0].setPixmap(player->myCard[0]);
    cardLabel[1].setPixmap(player->myCard[1]);
}

void GameWindow::foldHand()
{
    winner = dealer;
    //restart game and re-shuffle deck
    deck->shuffle();
    setDefaultBacksideCard();
    call->setText("Check");
    play->setEnabled(true);
    call->setEnabled(false);
    raise->setEnabled(false);
    fold->setEnabled(false);
    ai->cash += potAmount;
    potAmount = 0;
    tableLastTurn = 0;
    aiCashLabel->setText("             $" + QString::number(ai->cash));
    potAmountLabel->setText("     Pot Amount $" + QString::number(potAmount));
    tableCardIndex = 4;
    turn = aiPlayer;
    spinBox->setRange(1,player->cash);
    slider->setRange(1,player->cash);
    spinBox->setValue(1);
    handNameLabel->setVisible(false);
}

void GameWindow::callCheckHand()
{
    raise->setEnabled(false);
    call->setEnabled(false);
    fold->setEnabled(false);
    call->setText("Call");
    if(turn==aiPlayer)
    {
        QTimer::singleShot(2000,this,SLOT(aiAutomate()));
    }

    if(turn==gamer)
    {
        handNameLabel->setVisible(false);
        //potAmount += ai->aiAutomatedBet();
        potAmount += 40;
        player->cash -= 40;
        spinBox->setRange(1,player->cash);
        slider->setRange(1,player->cash);
        spinBox->setValue(1);
        potAmountLabel->setText("     Pot Amount $" + QString::number(potAmount));
        //aiCashLabel->setText("             $" + QString::number(ai->cash));
        playerCashLabel->setText("             $" + QString::number(player->cash));

        if(tableLastTurn==1)
        {
            play->setEnabled(false);
            fold->setEnabled(false);
            call->setEnabled(false);
            raise->setEnabled(false);

            for(int a = 2; a < 4; a++)
            {
                cardLabel[a].setPixmap(ai->myCard[a-2]);
            }

            //check the value of each players hand and determine the winner

            ai->cardCombo1.clear();
            ai->cardCombo2.clear();
            ai->cardCombo3.clear();
            ai->cardCombo4.clear();
            ai->cardCombo5.clear();
            ai->cardCombo6.clear();
            ai->cardCombo7.clear();
            ai->cardCombo8.clear();
            ai->cardCombo9.clear();
            ai->cardCombo10.clear();
            player->cardCombo1.clear();
            player->cardCombo2.clear();
            player->cardCombo3.clear();
            player->cardCombo4.clear();
            player->cardCombo5.clear();
            player->cardCombo6.clear();
            player->cardCombo7.clear();
            player->cardCombo8.clear();
            player->cardCombo9.clear();
            player->cardCombo10.clear();

            player->cardScore = 0;
            ai->cardScore = 0;
            player->analyzed = false;
            ai->analyzed = false;
            determineWinnerHand();
            determineWinnerHand();

            tableLastTurn = 0;
            play->setEnabled(true);
            tableCardIndex = 4;
        }
        else if(tableCardIndex==4 and tableLastTurn == 0)
        {
            while(tableCardIndex < 7)
            {
                //continue
                cardLabel[tableCardIndex].setPixmap(deck->getCardImage());
                tableCardIndex++;
            }
            tableCardIndex--;
            raise->setEnabled(true);
            call->setEnabled(true);
            fold->setEnabled(true);
        }
        else if(tableCardIndex>6)
        {
            cardLabel[tableCardIndex].setPixmap(deck->getCardImage());
            raise->setEnabled(true);
            call->setEnabled(true);
            fold->setEnabled(true);

        }

        tableCardIndex++;

        if(tableCardIndex==9)
        {
            tableCardIndex = 4;
            tableLastTurn = 1;
            call->setEnabled(true);
            raise->setEnabled(true);
            play->setEnabled(false);
            fold->setEnabled(true);
        }

        //raise->setEnabled(true);
        //call->setEnabled(true);

        turn = aiPlayer;
        call->setText("Check");
    }
}


void GameWindow::raiseHand()
{
    if(playerRaised==false)
    {
        call->setEnabled(false);
        fold->setEnabled(false);
        play->setEnabled(false);
        raise->setText("Confirm");
        raise->setEnabled(true);

        if(ai->cash < player->cash)
        {
            spinBox->setRange(1,ai->cash);
            slider->setRange(1,ai->cash);
            //spinBox->setValue(1);
        }
        else
        {
            spinBox->setRange(1,player->cash);
            slider->setRange(1,player->cash);
            //spinBox->setValue(1);
        }
        spinBox->setEnabled(true);
        slider->setEnabled(true);
        playerRaised = true;
    }
    else if (playerRaised==true)
    {
        turn = aiPlayer;
        call->setText("Check");
        spinBox->setEnabled(false);
        slider->setEnabled(false);
        player->cash -= spinBox->value();
        potAmount += spinBox->value();
        potAmountLabel->setText("     Pot Amount $" + QString::number(potAmount));
        playerCashLabel->setText("             $" + QString::number(player->cash));
        raise->setText("Raise");
        potAmount += ai->aiAutomatedBet(playerRaised,spinBox->value());

        playerRaised = false;

        //check if dealer has fold
        if(ai->fold==true)
        {
            QTimer::singleShot(2000,this,SLOT(foldConfirmTimed()));

            //player->cash += potAmount;
            //playerCashLabel->setText("             $" + QString::number(player->cash));
            //potAmount = 0;
            //potAmountLabel->setText("     Pot Amount $" + QString::number(potAmount));

            QTimer::singleShot(4000,this,SLOT(resetCards()));
            raise->setEnabled(false);
            ai->fold = false;
        }
        else if(ai->fold==false and ai->cash >= 0)
        {
            raise->setEnabled(false);
            QTimer::singleShot(2000,this,SLOT(dealerRaiseConfirm()));
        }

        playerRaised = false;
    }
}

void GameWindow::foldConfirmTimed()
{
    raise->setEnabled(false);
    handNameLabel->setText("     Dealer has folded");
    handNameLabel->setVisible(true);
    player->cash += potAmount;
    playerCashLabel->setText("             $" + QString::number(player->cash));
    potAmount = 0;
    potAmountLabel->setText("     Pot Amount $" + QString::number(potAmount));


    //handNameLabel->setText(" Dealer has folded");
}


void GameWindow::dealerRaiseConfirm()
{
    potAmountLabel->setText("     Pot Amount $" + QString::number(potAmount));
    aiCashLabel->setText("             $" + QString::number(ai->cash));
    handNameLabel->setText("     Dealer called raise");
    handNameLabel->setVisible(true);

    if(ai->fold==false and player->cash==0 || ai->fold==false and ai->cash ==0)
    {
        allInReveal();
        raise->setEnabled(false);
        play->setEnabled(false);
    }
    else if(ai->fold==false and ai->cash > 0 and player->cash > 0)
    {
        if(player->cash == 0)
        {
            //tableLastTurn = 1;
        }
        dealTableCards();
    }
    else if(ai->fold==false and ai->cash == 0)
    {
        //tableLastTurn = 1;
        allInReveal();
        raise->setEnabled(false);
        play->setEnabled(false);
    }

    QTimer::singleShot(2000,this,SLOT(handLabelHide()));
}

void GameWindow::handLabelHide()
{
    handNameLabel->setVisible(false);
}

void GameWindow::resetCash()
{
    player->cash = 1000;
    play->setEnabled(true);
}

void GameWindow::dealTableCards()
{
    if(tableLastTurn==1)
    {
        play->setEnabled(false);
        fold->setEnabled(false);
        call->setEnabled(false);
        raise->setEnabled(false);

        for(int a = 2; a < 4; a++)
        {
            cardLabel[a].setPixmap(ai->myCard[a-2]);
            raise->setEnabled(false);
        }
        tableLastTurn = 0;
        play->setEnabled(true);
        raise->setEnabled(false);

        //analyze each players hand, first clear the combos
        ai->cardCombo1.clear();
        ai->cardCombo2.clear();
        ai->cardCombo3.clear();
        ai->cardCombo4.clear();
        ai->cardCombo5.clear();
        ai->cardCombo6.clear();
        ai->cardCombo7.clear();
        ai->cardCombo8.clear();
        ai->cardCombo9.clear();
        ai->cardCombo10.clear();
        player->cardCombo1.clear();
        player->cardCombo2.clear();
        player->cardCombo3.clear();
        player->cardCombo4.clear();
        player->cardCombo5.clear();
        player->cardCombo6.clear();
        player->cardCombo7.clear();
        player->cardCombo8.clear();
        player->cardCombo9.clear();
        player->cardCombo10.clear();

        dealerAnalyzed = false;
        playerAnalyzed = false;
        //analyze hands
        this->determineWinnerHand();
        this->determineWinnerHand();
        //this->determineWinnerHand();
        //this->determineWinnerHand();
        //this->determineWinnerHand();

        //QTimer::singleShot(2000,this,SLOT(resetCards()));
    }
    else if(tableCardIndex==4 and tableLastTurn == 0)
    {
        while(tableCardIndex < 7)
        {
            cardLabel[tableCardIndex].setPixmap(deck->getCardImage());
            tableCardIndex++;
        }
        tableCardIndex--;
        raise->setEnabled(true);
        call->setEnabled(true);
        fold->setEnabled(true);
    }
    else if(tableCardIndex>6)
    {
        cardLabel[tableCardIndex].setPixmap(deck->getCardImage());
        raise->setEnabled(true);
        call->setEnabled(true);
        fold->setEnabled(true);
    }

    tableCardIndex++;

    if(tableCardIndex==9)
    {
        tableCardIndex = 4;
        tableLastTurn = 1;
        call->setEnabled(true);
        raise->setEnabled(true);
        play->setEnabled(false);
        fold->setEnabled(true);
    }


}

void GameWindow::allInReveal()
{    
    for(int a = 2; a < 4; a++)
    {
        cardLabel[a].setPixmap(ai->myCard[a-2]);
    }

    raise->setEnabled(false);
    reveal->start(1000);
}

void GameWindow::allInTimedReveal()
{
    if(tableLastTurn==0)
    {
        cardLabel[tableCardIndex].setPixmap(deck->getCardImage());
    }

    tableCardIndex++;
    if(tableCardIndex==9)
    {
        tableCardIndex = 4;
        reveal->stop();
        raise->setEnabled(false);

        //check the value of each players hand and determine the winner
        ai->cardCombo1.clear();
        ai->cardCombo2.clear();
        ai->cardCombo3.clear();
        ai->cardCombo4.clear();
        ai->cardCombo5.clear();
        ai->cardCombo6.clear();
        ai->cardCombo7.clear();
        ai->cardCombo8.clear();
        ai->cardCombo9.clear();
        ai->cardCombo10.clear();
        player->cardCombo1.clear();
        player->cardCombo2.clear();
        player->cardCombo3.clear();
        player->cardCombo4.clear();
        player->cardCombo5.clear();
        player->cardCombo6.clear();
        player->cardCombo7.clear();
        player->cardCombo8.clear();
        player->cardCombo9.clear();
        player->cardCombo10.clear();


        player->cardScore = 0;
        ai->cardScore = 0;
        player->analyzed = false;
        ai->analyzed = false;
        determineWinnerHand();
        determineWinnerHand();


        play->setEnabled(true);
        tableLastTurn = 0;
    }
}

void GameWindow::aiAutomate()
{
    potAmount += ai->aiAutomatedBet(playerRaised,spinBox->value());
    potAmountLabel->setText("     Pot Amount $" + QString::number(potAmount));
    aiCashLabel->setText("             $" + QString::number(ai->cash));
    handNameLabel->setText("     Dealer Raised $40");
    handNameLabel->setVisible(true);

    turn = gamer;
    fold->setEnabled(true);
    raise->setEnabled(true);
    call->setEnabled(true);

}

void GameWindow::resetCards()
{
    handNameLabel->setVisible(false);
    call->setEnabled(false);
    raise->setEnabled(false);
    play->setEnabled(true);
    tableLastTurn = 0;
    tableCardIndex = 4;
    this->setDefaultBacksideCard();
}

void GameWindow::analyzeDealerHand()
{

}

void GameWindow::determineWinnerHand()
{
    storedHandIndex.clear();
    highestHandIndex = 0;
    pairExistAlready = false;
    threeExistAlready = false;
    straightExists = false;
    flushExists = false;

    for(int a = 0; a < 10; a++)
    {
        handScore[a] = 0;
    }

    if(dealerAnalyzed == false and playerAnalyzed == false)
    {
        for(int a = 0, b = 0; a < 10; b++)
        {
            cardOccurence[a][b] = 0;
            if(b==12)
            {

                a++;
                b = -1;
            }
        }
        ai->handScore = 0;
        //cardOccurence is a 2d array which has 10 rows,13 columns
        //each row stores the possible hand from 5 cards, based on the fact
        //that each player MUST use his 2 given cards, and can only use 3 from the table
        //each column in array represents the 13 cards:
        //a,2,3,4,5,6,7,8,9,10,j,q,k
        //for every card occurence, I will increment which card has appeared
        //this will make it easier to analyze if pairs exist.

        //load the first 5 card combo into vector
        ai->cardCombo1.push_back(deck->cardText[2]);
        ai->cardCombo1.push_back(deck->cardText[3]);
        ai->cardCombo1.push_back(deck->cardText[4]);
        ai->cardCombo1.push_back(deck->cardText[5]);
        ai->cardCombo1.push_back(deck->cardText[6]);

        for(int a = 0, cardNumber = 0; a < 5; a ++)
        {
            if(ai->cardCombo1[a].length() == 3)
            {
                suite[0][a] = ai->cardCombo1[a].substr(2,3);
                cardNumber = QString::fromStdString(ai->cardCombo1[a].substr(0,2)).toInt();
                cardOccurence[0][cardNumber-1]++;
            }
            else if(ai->cardCombo1[a].length() == 2)
            {
                suite[0][a] = ai->cardCombo1[a].substr(1,2);
                cardNumber = QString::fromStdString(ai->cardCombo1[a].substr(0,1)).toInt();
                cardOccurence[0][cardNumber-1]++;
            }
        }

        ai->cardCombo2.push_back(deck->cardText[2]);
        ai->cardCombo2.push_back(deck->cardText[3]);
        ai->cardCombo2.push_back(deck->cardText[4]);
        ai->cardCombo2.push_back(deck->cardText[6]);
        ai->cardCombo2.push_back(deck->cardText[7]);

        for(int a = 0, cardNumber = 0; a < 5; a ++)
        {
            if(ai->cardCombo2[a].length() == 3)
            {
                suite[1][a] = ai->cardCombo2[a].substr(2,3);
                cardNumber = QString::fromStdString(ai->cardCombo2[a].substr(0,2)).toInt();
                cardOccurence[1][cardNumber-1]++;
            }
            else if(ai->cardCombo2[a].length() == 2)
            {
                suite[1][a] = ai->cardCombo2[a].substr(1,2);
                cardNumber = QString::fromStdString(ai->cardCombo2[a].substr(0,1)).toInt();
                cardOccurence[1][cardNumber-1]++;
            }
        }

        ai->cardCombo3.push_back(deck->cardText[2]);
        ai->cardCombo3.push_back(deck->cardText[3]);
        ai->cardCombo3.push_back(deck->cardText[4]);
        ai->cardCombo3.push_back(deck->cardText[7]);
        ai->cardCombo3.push_back(deck->cardText[8]);

        for(int a = 0, cardNumber = 0; a < 5; a ++)
        {
            if(ai->cardCombo3[a].length() == 3)
            {
                suite[2][a] = ai->cardCombo3[a].substr(2,3);
                cardNumber = QString::fromStdString(ai->cardCombo3[a].substr(0,2)).toInt();
                cardOccurence[2][cardNumber-1]++;
            }
            else if(ai->cardCombo3[a].length() == 2)
            {
                suite[2][a] = ai->cardCombo3[a].substr(1,2);
                cardNumber = QString::fromStdString(ai->cardCombo3[a].substr(0,1)).toInt();
                cardOccurence[2][cardNumber-1]++;
            }
        }

        ai->cardCombo4.push_back(deck->cardText[2]);
        ai->cardCombo4.push_back(deck->cardText[3]);
        ai->cardCombo4.push_back(deck->cardText[4]);
        ai->cardCombo4.push_back(deck->cardText[5]);
        ai->cardCombo4.push_back(deck->cardText[7]);

        for(int a = 0, cardNumber = 0; a < 5; a ++)
        {
            if(ai->cardCombo4[a].length() == 3)
            {
                suite[3][a] = ai->cardCombo4[a].substr(2,3);
                cardNumber = QString::fromStdString(ai->cardCombo4[a].substr(0,2)).toInt();
                cardOccurence[3][cardNumber-1]++;
            }
            else if(ai->cardCombo4[a].length() == 2)
            {
                suite[3][a] = ai->cardCombo4[a].substr(1,2);
                cardNumber = QString::fromStdString(ai->cardCombo4[a].substr(0,1)).toInt();
                cardOccurence[3][cardNumber-1]++;
            }
        }

        ai->cardCombo5.push_back(deck->cardText[2]);
        ai->cardCombo5.push_back(deck->cardText[3]);
        ai->cardCombo5.push_back(deck->cardText[4]);
        ai->cardCombo5.push_back(deck->cardText[5]);
        ai->cardCombo5.push_back(deck->cardText[8]);

        for(int a = 0, cardNumber = 0; a < 5; a ++)
        {
            if(ai->cardCombo5[a].length() == 3)
            {
                suite[4][a] = ai->cardCombo5[a].substr(2,3);
                cardNumber = QString::fromStdString(ai->cardCombo5[a].substr(0,2)).toInt();
                cardOccurence[4][cardNumber-1]++;
            }
            else if(ai->cardCombo5[a].length() == 2)
            {
                suite[4][a] = ai->cardCombo5[a].substr(1,2);
                cardNumber = QString::fromStdString(ai->cardCombo5[a].substr(0,1)).toInt();
                cardOccurence[4][cardNumber-1]++;
            }
        }

        ai->cardCombo6.push_back(deck->cardText[2]);
        ai->cardCombo6.push_back(deck->cardText[3]);
        ai->cardCombo6.push_back(deck->cardText[4]);
        ai->cardCombo6.push_back(deck->cardText[6]);
        ai->cardCombo6.push_back(deck->cardText[8]);

        for(int a = 0, cardNumber = 0; a < 5; a ++)
        {
            if(ai->cardCombo6[a].length() == 3)
            {
                suite[5][a] = ai->cardCombo6[a].substr(2,3);
                cardNumber = QString::fromStdString(ai->cardCombo6[a].substr(0,2)).toInt();
                cardOccurence[5][cardNumber-1]++;
            }
            else if(ai->cardCombo6[a].length() == 2)
            {
                suite[5][a] = ai->cardCombo6[a].substr(1,2);
                cardNumber = QString::fromStdString(ai->cardCombo6[a].substr(0,1)).toInt();
                cardOccurence[5][cardNumber-1]++;
            }
        }

        ai->cardCombo7.push_back(deck->cardText[2]);
        ai->cardCombo7.push_back(deck->cardText[3]);
        ai->cardCombo7.push_back(deck->cardText[5]);
        ai->cardCombo7.push_back(deck->cardText[6]);
        ai->cardCombo7.push_back(deck->cardText[7]);

        for(int a = 0, cardNumber = 0; a < 5; a ++)
        {
            if(ai->cardCombo7[a].length() == 3)
            {
                suite[6][a] = ai->cardCombo7[a].substr(2,3);
                cardNumber = QString::fromStdString(ai->cardCombo7[a].substr(0,2)).toInt();
                cardOccurence[6][cardNumber-1]++;
            }
            else if(ai->cardCombo7[a].length() == 2)
            {
                suite[6][a] = ai->cardCombo7[a].substr(1,2);
                cardNumber = QString::fromStdString(ai->cardCombo7[a].substr(0,1)).toInt();
                cardOccurence[6][cardNumber-1]++;
            }
        }

        ai->cardCombo8.push_back(deck->cardText[2]);
        ai->cardCombo8.push_back(deck->cardText[3]);
        ai->cardCombo8.push_back(deck->cardText[5]);
        ai->cardCombo8.push_back(deck->cardText[6]);
        ai->cardCombo8.push_back(deck->cardText[8]);

        for(int a = 0, cardNumber = 0; a < 5; a ++)
        {
            if(ai->cardCombo8[a].length() == 3)
            {
                suite[7][a] = ai->cardCombo8[a].substr(2,3);
                cardNumber = QString::fromStdString(ai->cardCombo8[a].substr(0,2)).toInt();
                cardOccurence[7][cardNumber-1]++;
            }
            else if(ai->cardCombo8[a].length() == 2)
            {
                suite[7][a] = ai->cardCombo8[a].substr(1,2);
                cardNumber = QString::fromStdString(ai->cardCombo8[a].substr(0,1)).toInt();
                cardOccurence[7][cardNumber-1]++;
            }
        }

        ai->cardCombo9.push_back(deck->cardText[2]);
        ai->cardCombo9.push_back(deck->cardText[3]);
        ai->cardCombo9.push_back(deck->cardText[5]);
        ai->cardCombo9.push_back(deck->cardText[7]);
        ai->cardCombo9.push_back(deck->cardText[8]);

        for(int a = 0, cardNumber = 0; a < 5; a ++)
        {
            if(ai->cardCombo9[a].length() == 3)
            {
                suite[8][a] = ai->cardCombo9[a].substr(2,3);
                cardNumber = QString::fromStdString(ai->cardCombo9[a].substr(0,2)).toInt();
                cardOccurence[8][cardNumber-1]++;
            }
            else if(ai->cardCombo9[a].length() == 2)
            {
                suite[8][a] = ai->cardCombo9[a].substr(1,2);
                cardNumber = QString::fromStdString(ai->cardCombo9[a].substr(0,1)).toInt();
                cardOccurence[8][cardNumber-1]++;
            }
        }

        ai->cardCombo10.push_back(deck->cardText[2]);
        ai->cardCombo10.push_back(deck->cardText[3]);
        ai->cardCombo10.push_back(deck->cardText[6]);
        ai->cardCombo10.push_back(deck->cardText[7]);
        ai->cardCombo10.push_back(deck->cardText[8]);

        for(int a = 0, cardNumber = 0; a < 5; a ++)
        {
            if(ai->cardCombo10[a].length() == 3)
            {
                suite[9][a] = ai->cardCombo10[a].substr(2,3);
                cardNumber = QString::fromStdString(ai->cardCombo10[a].substr(0,2)).toInt();
                cardOccurence[9][cardNumber-1]++;
            }
            else if(ai->cardCombo10[a].length() == 2)
            {
                suite[9][a] = ai->cardCombo10[a].substr(1,2);
                cardNumber = QString::fromStdString(ai->cardCombo10[a].substr(0,1)).toInt();
                cardOccurence[9][cardNumber-1]++;
            }
        }

        for(int a = 0, b = 0; a < 10; b++)
        {
            //cardOccurence[a][b] = 0;
            if(b==12)
            {

                a++;
                b = -1;
            }
        }

        for(int a = 0, b = 0; a < 10; b++)
        {
            if(b==4)
            {

                a++;
                b = -1;
            }
        }

        //check hand strength based on points
        //this will analyze each card and assign points based on card
        //this checks for pair, two pair, thee of a king, four of a kind
        //this will NOT check for straights or flush
        //*************test hands**************
        //
        //
        /*
          cardOccurence[0][0] = 3;
          cardOccurence[0][1] = 0;
          cardOccurence[0][2] = 0;
          cardOccurence[0][3] = 0;
          cardOccurence[0][4] = 0;
          cardOccurence[0][5] = 0;
          cardOccurence[0][6] = 0;
          cardOccurence[0][7] = 0;
          cardOccurence[0][8] = 0;
          cardOccurence[0][9] = 0;
          cardOccurence[0][10] = 0;
          cardOccurence[0][11] = 0;
          cardOccurence[0][12] = 2;

          suite[0][0] = 'd';
          suite[0][1] = 'd';
          suite[0][2] = 'd';
          suite[0][3] = 'd';
          suite[0][4] = 'd';
        */
        //
        //
        //*************************************
        for(int a = 0, b = 0; a < 10; b++)
        {
            if(b == 0)
            {
                if(cardOccurence[a][b]==1)
                {
                    handScore[a] += 2600;
                }
                else if(cardOccurence[a][b]==2)
                {
                    if(pairExistAlready)
                    {
                        handScore[a] += (2600 * 2) + 7556;
                    }
                    else if(pairExistAlready ==false)
                    {
                        handScore[a] += (2600 * 2 ) + 5049;
                        pairExistAlready = true;
                    }
                }
                else if(cardOccurence[a][b]==3)
                {
                    if(pairExistAlready)
                    {
                        handScore[a] += (2600 * 3) + 36071;
                    }
                    else if(pairExistAlready == false)
                    {
                        handScore[a] += (2600 * 3) + 21130;
                        threeExistAlready = true;
                    }
                }
                else if(cardOccurence[a][b]==4)
                {
                    handScore[a] += (2600 * 4) + 51565;
                }
            }if(b == 1)
            {
                if(cardOccurence[a][b]==1)
                {
                    handScore[a] += 2;
                }
                else if(cardOccurence[a][b]==2)
                {
                    if(pairExistAlready)
                    {
                        handScore[a] += (2 * 2) + 7556;
                    }
                    else if(threeExistAlready)
                    {
                        handScore[a] += (2 * 2) + 19990;
                    }
                    else if(pairExistAlready ==false)
                    {
                        handScore[a] += (2 * 2 ) + 5049;
                        pairExistAlready = true;
                    }
                    //handScore[a] += (2 * 2 ) + 5049;
                }
                else if(cardOccurence[a][b]==3)
                {
                    if(pairExistAlready)
                    {
                        handScore[a] += (2 * 3) + 36071;
                    }
                    else if(pairExistAlready == false)
                    {
                        handScore[a] += (2 * 3) + 21130;
                        threeExistAlready = true;
                    }
                    //handScore[a] += (2 * 3) + 21130;
                }
                else if(cardOccurence[a][b]==4)
                {
                    handScore[a] += (2 * 4) + 51565;
                }
            }if(b == 2)
            {
                if(cardOccurence[a][b]==1)
                {
                    handScore[a] += 3;
                }
                else if(cardOccurence[a][b]==2)
                {
                    if(pairExistAlready)
                    {
                        handScore[a] += (3 * 2) + 7556;
                    }
                    else if(threeExistAlready)
                    {
                        handScore[a] += (3 * 2) + 19990;
                    }
                    else if(pairExistAlready ==false)
                    {
                        handScore[a] += (3 * 2 ) + 5049;
                        pairExistAlready = true;
                    }
                    //handScore[a] += (3 * 2 ) + 5049;
                }
                else if(cardOccurence[a][b]==3)
                {
                    if(pairExistAlready)
                    {
                        handScore[a] += (3 * 3) + 36071;
                    }
                    else if(pairExistAlready == false)
                    {
                        handScore[a] += (3 * 3) + 21130;
                        threeExistAlready = true;
                    }
                    //handScore[a] += (3 * 3) + 21130;
                }
                else if(cardOccurence[a][b]==4)
                {
                    handScore[a] += (3 * 4) + 51565;
                }
            }if(b == 3)
            {
                if(cardOccurence[a][b]==1)
                {
                    handScore[a] += 4;
                }
                else if(cardOccurence[a][b]==2)
                {
                    if(pairExistAlready)
                    {
                        handScore[a] += (4 * 2) + 7556;
                    }
                    else if(threeExistAlready)
                    {
                        handScore[a] += (4 * 2) + 19990;
                    }
                    else if(pairExistAlready ==false)
                    {
                        handScore[a] += (4 * 2 ) + 5049;
                        pairExistAlready = true;
                    }
                    //handScore[a] += (4 * 2 ) + 5049;
                }
                else if(cardOccurence[a][b]==3)
                {
                    if(pairExistAlready)
                    {
                        handScore[a] += (4 * 3) + 36071;
                    }
                    else if(pairExistAlready == false)
                    {
                        handScore[a] += (4 * 3) + 21130;
                        threeExistAlready = true;
                    }
                    //handScore[a] += (4 * 3) + 21130;
                }
                else if(cardOccurence[a][b]==4)
                {
                    handScore[a] += (4 * 4) + 51565;
                }
            }if(b == 4)
            {
                if(cardOccurence[a][b]==1)
                {
                    handScore[a] += 5;
                }
                else if(cardOccurence[a][b]==2)
                {
                    if(pairExistAlready)
                    {
                        handScore[a] += (5 * 2) + 7556;
                    }
                    else if(threeExistAlready)
                    {
                        handScore[a] += (5 * 2) + 19990;
                    }
                    else if(pairExistAlready ==false)
                    {
                        handScore[a] += (5 * 2 ) + 5049;
                        pairExistAlready = true;
                    }
                    //handScore[a] += (5 * 2 ) + 5049;
                }
                else if(cardOccurence[a][b]==3)
                {
                    if(pairExistAlready)
                    {
                        handScore[a] += (5 * 3) + 36071;
                    }
                    else if(pairExistAlready == false)
                    {
                        handScore[a] += (5 * 3) + 21130;
                        threeExistAlready = true;
                    }
                    //handScore[a] += (5 * 3) + 21130;
                }
                else if(cardOccurence[a][b]==4)
                {
                    handScore[a] += (5 * 4) + 51565;
                }
            }if(b == 5)
            {
                if(cardOccurence[a][b]==1)
                {
                    handScore[a] += 14;
                }
                else if(cardOccurence[a][b]==2)
                {
                    if(pairExistAlready)
                    {
                        handScore[a] += (14 * 2) + 7556;
                    }
                    else if(threeExistAlready)
                    {
                        handScore[a] += (14 * 2) + 19990;
                    }
                    else if(pairExistAlready ==false)
                    {
                        handScore[a] += (14 * 2 ) + 5049;
                        pairExistAlready = true;
                    }
                    //handScore[a] += (14 * 2 ) + 5049;
                }
                else if(cardOccurence[a][b]==3)
                {
                    if(pairExistAlready)
                    {
                        handScore[a] += (14 * 3) + 36071;
                    }
                    else if(pairExistAlready == false)
                    {
                        handScore[a] += (14 * 3) + 21130;
                        threeExistAlready = true;
                    }
                    //handScore[a] += (14 * 3) + 21130;
                }
                else if(cardOccurence[a][b]==4)
                {
                    handScore[a] += (14 * 4) + 51565;
                }
            }if(b == 6)
            {
                if(cardOccurence[a][b]==1)
                {
                    handScore[a] += 26;
                }
                else if(cardOccurence[a][b]==2)
                {
                    if(pairExistAlready)
                    {
                        handScore[a] += (26 * 2) + 7556;
                    }
                    else if(threeExistAlready)
                    {
                        handScore[a] += (26 * 2) + 19990;
                    }
                    else if(pairExistAlready ==false)
                    {
                        handScore[a] += (26 * 2 ) + 5049;
                        pairExistAlready = true;
                    }
                    //handScore[a] += (26 * 2 ) + 5049;
                }
                else if(cardOccurence[a][b]==3)
                {
                    if(pairExistAlready)
                    {
                        handScore[a] += (26 * 3) + 36071;
                    }
                    else if(pairExistAlready == false)
                    {
                        handScore[a] += (26 * 3) + 21130;
                        threeExistAlready = true;
                    }
                    //handScore[a] += (26 * 3) + 21130;
                }
                else if(cardOccurence[a][b]==4)
                {
                    handScore[a] += (26 * 4) + 51565;
                }
            }if(b ==7)
            {
                if(cardOccurence[a][b]==1)
                {
                    handScore[a] += 49;
                }
                else if(cardOccurence[a][b]==2)
                {
                    if(pairExistAlready)
                    {
                        handScore[a] += (49 * 2) + 7556;
                    }
                    else if(threeExistAlready)
                    {
                        handScore[a] += (49 * 2) + 19990;
                    }
                    else if(pairExistAlready ==false)
                    {
                        handScore[a] += (49 * 2 ) + 5049;
                        pairExistAlready = true;
                    }
                    //handScore[a] += (49 * 2 ) + 5049;
                }
                else if(cardOccurence[a][b]==3)
                {
                    if(pairExistAlready)
                    {
                        handScore[a] += (49 * 3) + 36071;
                    }
                    else if(pairExistAlready == false)
                    {
                        handScore[a] += (49 * 3) + 21130;
                        threeExistAlready = true;
                    }
                    //handScore[a] += (49 * 3) + 21130;
                }
                else if(cardOccurence[a][b]==4)
                {
                    handScore[a] += (49 * 4) + 51565;
                }
            }if(b == 8)
            {
                if(cardOccurence[a][b]==1)
                {
                    handScore[a] += 94;
                }
                else if(cardOccurence[a][b]==2)
                {
                    if(pairExistAlready)
                    {
                        handScore[a] += (94 * 2) + 7556;
                    }
                    else if(threeExistAlready)
                    {
                        handScore[a] += (94 * 2) + 19990;
                    }
                    else if(pairExistAlready ==false)
                    {
                        handScore[a] += (94 * 2 ) + 5049;
                        pairExistAlready = true;
                    }
                    //handScore[a] += (94 * 2 ) + 5049;
                }
                else if(cardOccurence[a][b]==3)
                {
                    if(pairExistAlready)
                    {
                        handScore[a] += (94 * 3) + 36071;
                    }
                    else if(pairExistAlready == false)
                    {
                        handScore[a] += (94 * 3) + 21130;
                        threeExistAlready = true;
                    }
                    //handScore[a] += (94 * 3) + 21130;
                }
                else if(cardOccurence[a][b]==4)
                {
                    handScore[a] += (94 * 4) + 51565;
                }
            }if(b == 9)
            {
                if(cardOccurence[a][b]==1)
                {
                    handScore[a] += 183;
                }
                else if(cardOccurence[a][b]==2)
                {
                    if(pairExistAlready)
                    {
                        handScore[a] += (183 * 2) + 7556;
                    }
                    else if(threeExistAlready)
                    {
                        handScore[a] += (183 * 2) + 19990;
                    }
                    else if(pairExistAlready ==false)
                    {
                        handScore[a] += (183 * 2 ) + 5049;
                        pairExistAlready = true;
                    }
                    //handScore[a] += (183 * 2 ) + 5049;
                }
                else if(cardOccurence[a][b]==3)
                {
                    if(pairExistAlready)
                    {
                        handScore[a] += (183 * 3) + 36071;
                    }
                    else if(pairExistAlready == false)
                    {
                        handScore[a] += (183 * 3) + 21130;
                        threeExistAlready = true;
                    }
                    //handScore[a] += (183 * 3) + 21130;
                }
                else if(cardOccurence[a][b]==4)
                {
                    handScore[a] += (183 * 4) + 51565;
                }
            }if(b == 10)
            {
                if(cardOccurence[a][b]==1)
                {
                    handScore[a] += 352;
                }
                else if(cardOccurence[a][b]==2)
                {
                    if(pairExistAlready)
                    {
                        handScore[a] += (352 * 2) + 7556;
                    }
                    else if(threeExistAlready)
                    {
                        handScore[a] += (352 * 2) + 19990;
                    }
                    else if(pairExistAlready ==false)
                    {
                        handScore[a] += (352 * 2 ) + 5049;
                        pairExistAlready = true;
                    }
                    //handScore[a] += (352 * 2 ) + 5049;
                }
                else if(cardOccurence[a][b]==3)
                {
                    if(pairExistAlready)
                    {
                        handScore[a] += (352 * 3) + 36071;
                    }
                    else if(pairExistAlready == false)
                    {
                        handScore[a] += (352 * 3) + 21130;
                        threeExistAlready = true;
                    }
                    //handScore[a] += (352 * 3) + 21130;
                }
                else if(cardOccurence[a][b]==4)
                {
                    handScore[a] += (352 * 4) + 51565;
                }
            }if(b == 11)
            {
                if(cardOccurence[a][b]==1)
                {
                    handScore[a] += 680;
                }
                else if(cardOccurence[a][b]==2)
                {
                    if(pairExistAlready)
                    {
                        handScore[a] += (680 * 2) + 7556;
                    }
                    else if(threeExistAlready)
                    {
                        handScore[a] += (680 * 2) + 19990;
                    }
                    else if(pairExistAlready ==false)
                    {
                        handScore[a] += (680 * 2 ) + 5049;
                        pairExistAlready = true;
                    }
                    //handScore[a] += (680 * 2 ) + 5049;
                }
                else if(cardOccurence[a][b]==3)
                {
                    if(pairExistAlready)
                    {
                        handScore[a] += (680 * 3) + 36071;
                    }
                    else if(pairExistAlready == false)
                    {
                        handScore[a] += (680 * 3) + 21130;
                        threeExistAlready = true;
                    }
                    //handScore[a] += (680 * 3) + 21130;
                }
                else if(cardOccurence[a][b]==4)
                {
                    handScore[a] += (680 * 4) + 51565;
                }
            }if(b == 12)
            {
                if(cardOccurence[a][b]==1)
                {
                    handScore[a] += 1322;
                }
                else if(cardOccurence[a][b]==2)
                {
                    if(pairExistAlready)
                    {
                        handScore[a] += (1322 * 2) + 7556;
                    }
                    else if(threeExistAlready)
                    {
                        handScore[a] += (1322 * 2) + 19990;
                    }
                    else if(pairExistAlready ==false)
                    {
                        handScore[a] += (1322 * 2 ) + 5049;
                        pairExistAlready = true;
                    }
                    //handScore[a] += (1322 * 2 ) + 5049;
                }
                else if(cardOccurence[a][b]==3)
                {
                    if(pairExistAlready)
                    {
                        handScore[a] += (1322 * 3) + 36071;
                    }
                    else if(pairExistAlready == false)
                    {
                        handScore[a] += (1322 * 3) + 21130;
                        threeExistAlready = true;
                    }
                    //handScore[a] += (1322 * 3) + 21130;
                }
                else if(cardOccurence[a][b]==4)
                {
                    handScore[a] += (1322 * 4) + 51565;
                }
                a++;
                pairExistAlready = false;
                threeExistAlready = false;
                b = -1;
            }
        }

        //check if straight exists and add points
        for(int a = 0; a < 10; a++)
        {
            if(cardOccurence[a][0]==1 and
                    cardOccurence[a][12]==1 and
                    cardOccurence[a][11]==1 and
                    cardOccurence[a][10]==1 and
                    cardOccurence[a][9]==1)
            {
                handScore[a] += 30933;
                straightExists = true;
            }
            else if(cardOccurence[a][0]==1 and
                    cardOccurence[a][1]==1 and
                    cardOccurence[a][2]==1 and
                    cardOccurence[a][3]==1 and
                    cardOccurence[a][4]==1)
            {
                handScore[a] -= 2599;
                handScore[a] += 30933;
                straightExists = true;
            }
            else if(cardOccurence[a][1]==1 and
                    cardOccurence[a][2]==1 and
                    cardOccurence[a][3]==1 and
                    cardOccurence[a][4]==1 and
                    cardOccurence[a][5]==1)
            {
                handScore[a] += 30933;
                straightExists = true;
            }
            else if(cardOccurence[a][2]==1 and
                    cardOccurence[a][3]==1 and
                    cardOccurence[a][4]==1 and
                    cardOccurence[a][5]==1 and
                    cardOccurence[a][6]==1)
            {
                handScore[a] += 30933;
                straightExists = true;
            }
            else if(cardOccurence[a][3]==1 and
                    cardOccurence[a][4]==1 and
                    cardOccurence[a][5]==1 and
                    cardOccurence[a][6]==1 and
                    cardOccurence[a][7]==1)
            {
                handScore[a] += 30933;
                straightExists = true;
            }
            else if(cardOccurence[a][4]==1 and
                    cardOccurence[a][5]==1 and
                    cardOccurence[a][6]==1 and
                    cardOccurence[a][7]==1 and
                    cardOccurence[a][8]==1)
            {
                handScore[a] += 30933;
                straightExists = true;
            }
            else if(cardOccurence[a][5]==1 and
                    cardOccurence[a][6]==1 and
                    cardOccurence[a][7]==1 and
                    cardOccurence[a][8]==1 and
                    cardOccurence[a][9]==1)
            {
                handScore[a] += 30933;
                straightExists = true;
            }
            else if(cardOccurence[a][6]==1 and
                    cardOccurence[a][7]==1 and
                    cardOccurence[a][8]==1 and
                    cardOccurence[a][9]==1 and
                    cardOccurence[a][10]==1)
            {
                handScore[a] += 30933;
                straightExists = true;
            }
            else if(cardOccurence[a][7]==1 and
                    cardOccurence[a][8]==1 and
                    cardOccurence[a][9]==1 and
                    cardOccurence[a][10]==1 and
                    cardOccurence[a][11]==1)
            {
                handScore[a] += 30933;
                straightExists = true;
            }
            else if(cardOccurence[a][8]==1 and
                    cardOccurence[a][9]==1 and
                    cardOccurence[a][10]==1 and
                    cardOccurence[a][11]==1 and
                    cardOccurence[a][12]==1)
            {
                handScore[a] += 30933;
                straightExists = true;
            }
        }


        //check for flush and add points if exists
        for(int a = 0; a < 10; a++)
        {
            if(suite[a][0]==suite[a][1] and
                    suite[a][0]==suite[a][2] and
                    suite[a][0]==suite[a][3] and
                    suite[a][0]==suite[a][4])
            {
                if(straightExists)
                {
                    handScore[a] -= 30933;
                    handScore[a] += 63288;
                }
                else if (straightExists == false)
                {
                    handScore[a] += 36071;
                }
            }
        }


        //check which hand has the highest score
        for(int a = 0; a < 10; a++)
        {
            if(ai->handScore <= handScore[a])
            {
                ai->handScore = handScore[a];
            }
        }

        for(int a = 0, b = 0; a < 10; b++)
        {
            //cardOccurence[a][b] = 0;
            cout<<cardOccurence[a][b];
            if(b==12)
            {
                cout<<endl;
                cout<<suite[a][0];
                cout<<suite[a][1];
                cout<<suite[a][2];
                cout<<suite[a][3];
                cout<<suite[a][4];
                cout<<endl;
                a++;
                b = -1;
            }
        }

        /*test print out all scores
        for(int a = 0; a < 10; a++)
        {
            cout<<handScore[a]<<endl;
        }
        */
        //cout<<ai->handScore<<endl;

        dealerAnalyzed = true;
    }
    else if (dealerAnalyzed == true and playerAnalyzed == false)
    {
        for(int a = 0, b = 0; a < 10; b++)
        {
            cardOccurence[a][b] = 0;
            //cout<<cardOccurence[a][b];
            if(b==12)
            {
                a++;
                b = -1;
                // cout<<endl;
            }
        }

        player->handScore = 0;

        //cardOccurence is a 2d array which has 10 rows,13 columns
        //each row stores the possible hand from 5 cards, based on the fact
        //that each player MUST use his 2 given cards, and can only use 3 from the table
        //each column in array represents the 13 cards:
        //a,2,3,4,5,6,7,8,9,10,j,q,k
        //for every card occurence, I will increment which card has appeared
        //this will make it easier to analyze if pairs exist.

        //load the first 5 card combo into vector
        player->cardCombo1.push_back(deck->cardText[0]);
        player->cardCombo1.push_back(deck->cardText[1]);
        player->cardCombo1.push_back(deck->cardText[4]);
        player->cardCombo1.push_back(deck->cardText[5]);
        player->cardCombo1.push_back(deck->cardText[6]);

        for(int a = 0, cardNumber = 0; a < 5; a ++)
        {
            if(player->cardCombo1[a].length() == 3)
            {
                suite[0][a] = player->cardCombo1[a].substr(2,3);
                cardNumber = QString::fromStdString(player->cardCombo1[a].substr(0,2)).toInt();
                cardOccurence[0][cardNumber-1]++;
            }
            else if(player->cardCombo1[a].length() == 2)
            {
                suite[0][a] = player->cardCombo1[a].substr(1,2);
                cardNumber = QString::fromStdString(player->cardCombo1[a].substr(0,1)).toInt();
                cardOccurence[0][cardNumber-1]++;
            }
        }

        player->cardCombo2.push_back(deck->cardText[0]);
        player->cardCombo2.push_back(deck->cardText[1]);
        player->cardCombo2.push_back(deck->cardText[4]);
        player->cardCombo2.push_back(deck->cardText[6]);
        player->cardCombo2.push_back(deck->cardText[7]);

        for(int a = 0, cardNumber = 0; a < 5; a ++)
        {
            if(player->cardCombo2[a].length() == 3)
            {
                suite[1][a] = player->cardCombo2[a].substr(2,3);
                cardNumber = QString::fromStdString(player->cardCombo2[a].substr(0,2)).toInt();
                cardOccurence[1][cardNumber-1]++;
            }
            else if(player->cardCombo2[a].length() == 2)
            {
                suite[1][a] = player->cardCombo2[a].substr(1,2);
                cardNumber = QString::fromStdString(player->cardCombo2[a].substr(0,1)).toInt();
                cardOccurence[1][cardNumber-1]++;
            }
        }

        player->cardCombo3.push_back(deck->cardText[0]);
        player->cardCombo3.push_back(deck->cardText[1]);
        player->cardCombo3.push_back(deck->cardText[4]);
        player->cardCombo3.push_back(deck->cardText[7]);
        player->cardCombo3.push_back(deck->cardText[8]);

        for(int a = 0, cardNumber = 0; a < 5; a ++)
        {
            if(player->cardCombo3[a].length() == 3)
            {
                suite[2][a] = player->cardCombo3[a].substr(2,3);
                cardNumber = QString::fromStdString(player->cardCombo3[a].substr(0,2)).toInt();
                cardOccurence[2][cardNumber-1]++;
            }
            else if(player->cardCombo3[a].length() == 2)
            {
                suite[2][a] = player->cardCombo3[a].substr(1,2);
                cardNumber = QString::fromStdString(player->cardCombo3[a].substr(0,1)).toInt();
                cardOccurence[2][cardNumber-1]++;
            }
        }

        player->cardCombo4.push_back(deck->cardText[0]);
        player->cardCombo4.push_back(deck->cardText[1]);
        player->cardCombo4.push_back(deck->cardText[4]);
        player->cardCombo4.push_back(deck->cardText[5]);
        player->cardCombo4.push_back(deck->cardText[7]);

        for(int a = 0, cardNumber = 0; a < 5; a ++)
        {
            if(player->cardCombo4[a].length() == 3)
            {
                suite[3][a] = player->cardCombo4[a].substr(2,3);
                cardNumber = QString::fromStdString(player->cardCombo4[a].substr(0,2)).toInt();
                cardOccurence[3][cardNumber-1]++;
            }
            else if(player->cardCombo4[a].length() == 2)
            {
                suite[3][a] = player->cardCombo4[a].substr(1,2);
                cardNumber = QString::fromStdString(player->cardCombo4[a].substr(0,1)).toInt();
                cardOccurence[3][cardNumber-1]++;
            }
        }

        player->cardCombo5.push_back(deck->cardText[0]);
        player->cardCombo5.push_back(deck->cardText[1]);
        player->cardCombo5.push_back(deck->cardText[4]);
        player->cardCombo5.push_back(deck->cardText[5]);
        player->cardCombo5.push_back(deck->cardText[8]);

        for(int a = 0, cardNumber = 0; a < 5; a ++)
        {
            if(player->cardCombo5[a].length() == 3)
            {
                suite[4][a] = player->cardCombo5[a].substr(2,3);
                cardNumber = QString::fromStdString(player->cardCombo5[a].substr(0,2)).toInt();
                cardOccurence[4][cardNumber-1]++;
            }
            else if(player->cardCombo5[a].length() == 2)
            {
                suite[4][a] = player->cardCombo5[a].substr(1,2);
                cardNumber = QString::fromStdString(player->cardCombo5[a].substr(0,1)).toInt();
                cardOccurence[4][cardNumber-1]++;
            }
        }

        player->cardCombo6.push_back(deck->cardText[0]);
        player->cardCombo6.push_back(deck->cardText[1]);
        player->cardCombo6.push_back(deck->cardText[4]);
        player->cardCombo6.push_back(deck->cardText[6]);
        player->cardCombo6.push_back(deck->cardText[8]);

        for(int a = 0, cardNumber = 0; a < 5; a ++)
        {
            if(player->cardCombo6[a].length() == 3)
            {
                suite[5][a] = player->cardCombo6[a].substr(2,3);
                cardNumber = QString::fromStdString(player->cardCombo6[a].substr(0,2)).toInt();
                cardOccurence[5][cardNumber-1]++;
            }
            else if(player->cardCombo6[a].length() == 2)
            {
                suite[5][a] = player->cardCombo6[a].substr(1,2);
                cardNumber = QString::fromStdString(player->cardCombo6[a].substr(0,1)).toInt();
                cardOccurence[5][cardNumber-1]++;
            }
        }

        player->cardCombo7.push_back(deck->cardText[0]);
        player->cardCombo7.push_back(deck->cardText[1]);
        player->cardCombo7.push_back(deck->cardText[5]);
        player->cardCombo7.push_back(deck->cardText[6]);
        player->cardCombo7.push_back(deck->cardText[7]);

        for(int a = 0, cardNumber = 0; a < 5; a ++)
        {
            if(player->cardCombo7[a].length() == 3)
            {
                suite[6][a] = player->cardCombo7[a].substr(2,3);
                cardNumber = QString::fromStdString(player->cardCombo7[a].substr(0,2)).toInt();
                cardOccurence[6][cardNumber-1]++;
            }
            else if(player->cardCombo7[a].length() == 2)
            {
                suite[6][a] = player->cardCombo7[a].substr(1,2);
                cardNumber = QString::fromStdString(player->cardCombo7[a].substr(0,1)).toInt();
                cardOccurence[6][cardNumber-1]++;
            }
        }

        player->cardCombo8.push_back(deck->cardText[0]);
        player->cardCombo8.push_back(deck->cardText[1]);
        player->cardCombo8.push_back(deck->cardText[5]);
        player->cardCombo8.push_back(deck->cardText[6]);
        player->cardCombo8.push_back(deck->cardText[8]);

        for(int a = 0, cardNumber = 0; a < 5; a ++)
        {
            if(player->cardCombo8[a].length() == 3)
            {
                suite[7][a] = player->cardCombo8[a].substr(2,3);
                cardNumber = QString::fromStdString(player->cardCombo8[a].substr(0,2)).toInt();
                cardOccurence[7][cardNumber-1]++;
            }
            else if(player->cardCombo8[a].length() == 2)
            {
                suite[7][a] = player->cardCombo8[a].substr(1,2);
                cardNumber = QString::fromStdString(player->cardCombo8[a].substr(0,1)).toInt();
                cardOccurence[7][cardNumber-1]++;
            }
        }

        player->cardCombo9.push_back(deck->cardText[0]);
        player->cardCombo9.push_back(deck->cardText[1]);
        player->cardCombo9.push_back(deck->cardText[5]);
        player->cardCombo9.push_back(deck->cardText[7]);
        player->cardCombo9.push_back(deck->cardText[8]);

        for(int a = 0, cardNumber = 0; a < 5; a ++)
        {
            if(player->cardCombo9[a].length() == 3)
            {
                suite[8][a] = player->cardCombo9[a].substr(2,3);
                cardNumber = QString::fromStdString(player->cardCombo9[a].substr(0,2)).toInt();
                cardOccurence[8][cardNumber-1]++;
            }
            else if(player->cardCombo9[a].length() == 2)
            {
                suite[8][a] = player->cardCombo9[a].substr(1,2);
                cardNumber = QString::fromStdString(player->cardCombo9[a].substr(0,1)).toInt();
                cardOccurence[8][cardNumber-1]++;
            }
        }

        player->cardCombo10.push_back(deck->cardText[0]);
        player->cardCombo10.push_back(deck->cardText[1]);
        player->cardCombo10.push_back(deck->cardText[6]);
        player->cardCombo10.push_back(deck->cardText[7]);
        player->cardCombo10.push_back(deck->cardText[8]);

        for(int a = 0, cardNumber = 0; a < 5; a ++)
        {
            if(player->cardCombo10[a].length() == 3)
            {
                suite[9][a] = player->cardCombo10[a].substr(2,3);
                cardNumber = QString::fromStdString(player->cardCombo10[a].substr(0,2)).toInt();
                cardOccurence[9][cardNumber-1]++;
            }
            else if(player->cardCombo10[a].length() == 2)
            {
                suite[9][a] = player->cardCombo10[a].substr(1,2);
                cardNumber = QString::fromStdString(player->cardCombo10[a].substr(0,1)).toInt();
                cardOccurence[9][cardNumber-1]++;
            }
        }

        //cout<<endl;
        for(int a = 0, b = 0; a < 10; b++)
        {
            //cardOccurence[a][b] = 0;
            //cout<<cardOccurence[a][b];
            if(b==12)
            {

                a++;
                b = -1;
                //cout<<endl;
            }
        }

        for(int a = 0, b = 0; a < 10; b++)
        {
            //cout<<suite[a][b];
            if(b==4)
            {

                a++;
                b = -1;
                //cout<<endl;
            }
        }

        //check hand strength based on points
        //this will analyze each card and assign points based on card
        //this checks for pair, two pair, thee of a king, four of a kind
        //this will NOT check for straights or flush
        //*************test hands**************
        //
        //
        /*
          cardOccurence[0][0] = 3;
          cardOccurence[0][1] = 0;
          cardOccurence[0][2] = 0;
          cardOccurence[0][3] = 0;
          cardOccurence[0][4] = 0;
          cardOccurence[0][5] = 0;
          cardOccurence[0][6] = 0;
          cardOccurence[0][7] = 0;
          cardOccurence[0][8] = 0;
          cardOccurence[0][9] = 0;
          cardOccurence[0][10] = 0;
          cardOccurence[0][11] = 0;
          cardOccurence[0][12] = 2;

          suite[0][0] = 'd';
          suite[0][1] = 'd';
          suite[0][2] = 'd';
          suite[0][3] = 'd';
          suite[0][4] = 'd';
        */
        //
        //
        //*************************************
        for(int a = 0, b = 0; a < 10; b++)
        {
            if(b == 0)
            {
                if(cardOccurence[a][b]==1)
                {
                    handScore[a] += 2600;
                }
                else if(cardOccurence[a][b]==2)
                {
                    if(pairExistAlready)
                    {
                        handScore[a] += (2600 * 2) + 7556;
                    }
                    else if(pairExistAlready ==false)
                    {
                        handScore[a] += (2600 * 2 ) + 5049;
                        pairExistAlready = true;
                    }
                }
                else if(cardOccurence[a][b]==3)
                {
                    if(pairExistAlready)
                    {
                        handScore[a] += (2600 * 3) + 36071;
                    }
                    else if(pairExistAlready == false)
                    {
                        handScore[a] += (2600 * 3) + 21130;
                        threeExistAlready = true;
                    }
                }
                else if(cardOccurence[a][b]==4)
                {
                    handScore[a] += (2600 * 4) + 51565;
                }
            }if(b == 1)
            {
                if(cardOccurence[a][b]==1)
                {
                    handScore[a] += 2;
                }
                else if(cardOccurence[a][b]==2)
                {
                    if(pairExistAlready)
                    {
                        handScore[a] += (2 * 2) + 7556;
                    }
                    else if(threeExistAlready)
                    {
                        handScore[a] += (2 * 2) + 19990;
                    }
                    else if(pairExistAlready ==false)
                    {
                        handScore[a] += (2 * 2 ) + 5049;
                        pairExistAlready = true;
                    }
                    //handScore[a] += (2 * 2 ) + 5049;
                }
                else if(cardOccurence[a][b]==3)
                {
                    if(pairExistAlready)
                    {
                        handScore[a] += (2 * 3) + 36071;
                    }
                    else if(pairExistAlready == false)
                    {
                        handScore[a] += (2 * 3) + 21130;
                        threeExistAlready = true;
                    }
                    //handScore[a] += (2 * 3) + 21130;
                }
                else if(cardOccurence[a][b]==4)
                {
                    handScore[a] += (2 * 4) + 51565;
                }
            }if(b == 2)
            {
                if(cardOccurence[a][b]==1)
                {
                    handScore[a] += 3;
                }
                else if(cardOccurence[a][b]==2)
                {
                    if(pairExistAlready)
                    {
                        handScore[a] += (3 * 2) + 7556;
                    }
                    else if(threeExistAlready)
                    {
                        handScore[a] += (3 * 2) + 19990;
                    }
                    else if(pairExistAlready ==false)
                    {
                        handScore[a] += (3 * 2 ) + 5049;
                        pairExistAlready = true;
                    }
                    //handScore[a] += (3 * 2 ) + 5049;
                }
                else if(cardOccurence[a][b]==3)
                {
                    if(pairExistAlready)
                    {
                        handScore[a] += (3 * 3) + 36071;
                    }
                    else if(pairExistAlready == false)
                    {
                        handScore[a] += (3 * 3) + 21130;
                        threeExistAlready = true;
                    }
                    //handScore[a] += (3 * 3) + 21130;
                }
                else if(cardOccurence[a][b]==4)
                {
                    handScore[a] += (3 * 4) + 51565;
                }
            }if(b == 3)
            {
                if(cardOccurence[a][b]==1)
                {
                    handScore[a] += 4;
                }
                else if(cardOccurence[a][b]==2)
                {
                    if(pairExistAlready)
                    {
                        handScore[a] += (4 * 2) + 7556;
                    }
                    else if(threeExistAlready)
                    {
                        handScore[a] += (4 * 2) + 19990;
                    }
                    else if(pairExistAlready ==false)
                    {
                        handScore[a] += (4 * 2 ) + 5049;
                        pairExistAlready = true;
                    }
                    //handScore[a] += (4 * 2 ) + 5049;
                }
                else if(cardOccurence[a][b]==3)
                {
                    if(pairExistAlready)
                    {
                        handScore[a] += (4 * 3) + 36071;
                    }
                    else if(pairExistAlready == false)
                    {
                        handScore[a] += (4 * 3) + 21130;
                        threeExistAlready = true;
                    }
                    //handScore[a] += (4 * 3) + 21130;
                }
                else if(cardOccurence[a][b]==4)
                {
                    handScore[a] += (4 * 4) + 51565;
                }
            }if(b == 4)
            {
                if(cardOccurence[a][b]==1)
                {
                    handScore[a] += 5;
                }
                else if(cardOccurence[a][b]==2)
                {
                    if(pairExistAlready)
                    {
                        handScore[a] += (5 * 2) + 7556;
                    }
                    else if(threeExistAlready)
                    {
                        handScore[a] += (5 * 2) + 19990;
                    }
                    else if(pairExistAlready ==false)
                    {
                        handScore[a] += (5 * 2 ) + 5049;
                        pairExistAlready = true;
                    }
                    //handScore[a] += (5 * 2 ) + 5049;
                }
                else if(cardOccurence[a][b]==3)
                {
                    if(pairExistAlready)
                    {
                        handScore[a] += (5 * 3) + 36071;
                    }
                    else if(pairExistAlready == false)
                    {
                        handScore[a] += (5 * 3) + 21130;
                        threeExistAlready = true;
                    }
                    //handScore[a] += (5 * 3) + 21130;
                }
                else if(cardOccurence[a][b]==4)
                {
                    handScore[a] += (5 * 4) + 51565;
                }
            }if(b == 5)
            {
                if(cardOccurence[a][b]==1)
                {
                    handScore[a] += 14;
                }
                else if(cardOccurence[a][b]==2)
                {
                    if(pairExistAlready)
                    {
                        handScore[a] += (14 * 2) + 7556;
                    }
                    else if(threeExistAlready)
                    {
                        handScore[a] += (14 * 2) + 19990;
                    }
                    else if(pairExistAlready ==false)
                    {
                        handScore[a] += (14 * 2 ) + 5049;
                        pairExistAlready = true;
                    }
                    //handScore[a] += (14 * 2 ) + 5049;
                }
                else if(cardOccurence[a][b]==3)
                {
                    if(pairExistAlready)
                    {
                        handScore[a] += (14 * 3) + 36071;
                    }
                    else if(pairExistAlready == false)
                    {
                        handScore[a] += (14 * 3) + 21130;
                        threeExistAlready = true;
                    }
                    //handScore[a] += (14 * 3) + 21130;
                }
                else if(cardOccurence[a][b]==4)
                {
                    handScore[a] += (14 * 4) + 51565;
                }
            }if(b == 6)
            {
                if(cardOccurence[a][b]==1)
                {
                    handScore[a] += 26;
                }
                else if(cardOccurence[a][b]==2)
                {
                    if(pairExistAlready)
                    {
                        handScore[a] += (26 * 2) + 7556;
                    }
                    else if(threeExistAlready)
                    {
                        handScore[a] += (26 * 2) + 19990;
                    }
                    else if(pairExistAlready ==false)
                    {
                        handScore[a] += (26 * 2 ) + 5049;
                        pairExistAlready = true;
                    }
                    //handScore[a] += (26 * 2 ) + 5049;
                }
                else if(cardOccurence[a][b]==3)
                {
                    if(pairExistAlready)
                    {
                        handScore[a] += (26 * 3) + 36071;
                    }
                    else if(pairExistAlready == false)
                    {
                        handScore[a] += (26 * 3) + 21130;
                        threeExistAlready = true;
                    }
                    //handScore[a] += (26 * 3) + 21130;
                }
                else if(cardOccurence[a][b]==4)
                {
                    handScore[a] += (26 * 4) + 51565;
                }
            }if(b ==7)
            {
                if(cardOccurence[a][b]==1)
                {
                    handScore[a] += 49;
                }
                else if(cardOccurence[a][b]==2)
                {
                    if(pairExistAlready)
                    {
                        handScore[a] += (49 * 2) + 7556;
                    }
                    else if(threeExistAlready)
                    {
                        handScore[a] += (49 * 2) + 19990;
                    }
                    else if(pairExistAlready ==false)
                    {
                        handScore[a] += (49 * 2 ) + 5049;
                        pairExistAlready = true;
                    }
                    //handScore[a] += (49 * 2 ) + 5049;
                }
                else if(cardOccurence[a][b]==3)
                {
                    if(pairExistAlready)
                    {
                        handScore[a] += (49 * 3) + 36071;
                    }
                    else if(pairExistAlready == false)
                    {
                        handScore[a] += (49 * 3) + 21130;
                        threeExistAlready = true;
                    }
                    //handScore[a] += (49 * 3) + 21130;
                }
                else if(cardOccurence[a][b]==4)
                {
                    handScore[a] += (49 * 4) + 51565;
                }
            }if(b == 8)
            {
                if(cardOccurence[a][b]==1)
                {
                    handScore[a] += 94;
                }
                else if(cardOccurence[a][b]==2)
                {
                    if(pairExistAlready)
                    {
                        handScore[a] += (94 * 2) + 7556;
                    }
                    else if(threeExistAlready)
                    {
                        handScore[a] += (94 * 2) + 19990;
                    }
                    else if(pairExistAlready ==false)
                    {
                        handScore[a] += (94 * 2 ) + 5049;
                        pairExistAlready = true;
                    }
                    //handScore[a] += (94 * 2 ) + 5049;
                }
                else if(cardOccurence[a][b]==3)
                {
                    if(pairExistAlready)
                    {
                        handScore[a] += (94 * 3) + 36071;
                    }
                    else if(pairExistAlready == false)
                    {
                        handScore[a] += (94 * 3) + 21130;
                        threeExistAlready = true;
                    }
                    //handScore[a] += (94 * 3) + 21130;
                }
                else if(cardOccurence[a][b]==4)
                {
                    handScore[a] += (94 * 4) + 51565;
                }
            }if(b == 9)
            {
                if(cardOccurence[a][b]==1)
                {
                    handScore[a] += 183;
                }
                else if(cardOccurence[a][b]==2)
                {
                    if(pairExistAlready)
                    {
                        handScore[a] += (183 * 2) + 7556;
                    }
                    else if(threeExistAlready)
                    {
                        handScore[a] += (183 * 2) + 19990;
                    }
                    else if(pairExistAlready ==false)
                    {
                        handScore[a] += (183 * 2 ) + 5049;
                        pairExistAlready = true;
                    }
                    //handScore[a] += (183 * 2 ) + 5049;
                }
                else if(cardOccurence[a][b]==3)
                {
                    if(pairExistAlready)
                    {
                        handScore[a] += (183 * 3) + 36071;
                    }
                    else if(pairExistAlready == false)
                    {
                        handScore[a] += (183 * 3) + 21130;
                        threeExistAlready = true;
                    }
                    //handScore[a] += (183 * 3) + 21130;
                }
                else if(cardOccurence[a][b]==4)
                {
                    handScore[a] += (183 * 4) + 51565;
                }
            }if(b == 10)
            {
                if(cardOccurence[a][b]==1)
                {
                    handScore[a] += 352;
                }
                else if(cardOccurence[a][b]==2)
                {
                    if(pairExistAlready)
                    {
                        handScore[a] += (352 * 2) + 7556;
                    }
                    else if(threeExistAlready)
                    {
                        handScore[a] += (352 * 2) + 19990;
                    }
                    else if(pairExistAlready ==false)
                    {
                        handScore[a] += (352 * 2 ) + 5049;
                        pairExistAlready = true;
                    }
                    //handScore[a] += (352 * 2 ) + 5049;
                }
                else if(cardOccurence[a][b]==3)
                {
                    if(pairExistAlready)
                    {
                        handScore[a] += (352 * 3) + 36071;
                    }
                    else if(pairExistAlready == false)
                    {
                        handScore[a] += (352 * 3) + 21130;
                        threeExistAlready = true;
                    }
                    //handScore[a] += (352 * 3) + 21130;
                }
                else if(cardOccurence[a][b]==4)
                {
                    handScore[a] += (352 * 4) + 51565;
                }
            }if(b == 11)
            {
                if(cardOccurence[a][b]==1)
                {
                    handScore[a] += 680;
                }
                else if(cardOccurence[a][b]==2)
                {
                    if(pairExistAlready)
                    {
                        handScore[a] += (680 * 2) + 7556;
                    }
                    else if(threeExistAlready)
                    {
                        handScore[a] += (680 * 2) + 19990;
                    }
                    else if(pairExistAlready ==false)
                    {
                        handScore[a] += (680 * 2 ) + 5049;
                        pairExistAlready = true;
                    }
                    //handScore[a] += (680 * 2 ) + 5049;
                }
                else if(cardOccurence[a][b]==3)
                {
                    if(pairExistAlready)
                    {
                        handScore[a] += (680 * 3) + 36071;
                    }
                    else if(pairExistAlready == false)
                    {
                        handScore[a] += (680 * 3) + 21130;
                        threeExistAlready = true;
                    }
                    //handScore[a] += (680 * 3) + 21130;
                }
                else if(cardOccurence[a][b]==4)
                {
                    handScore[a] += (680 * 4) + 51565;
                }
            }if(b == 12)
            {
                if(cardOccurence[a][b]==1)
                {
                    handScore[a] += 1322;
                }
                else if(cardOccurence[a][b]==2)
                {
                    if(pairExistAlready)
                    {
                        handScore[a] += (1322 * 2) + 7556;
                    }
                    else if(threeExistAlready)
                    {
                        handScore[a] += (1322 * 2) + 19990;
                    }
                    else if(pairExistAlready ==false)
                    {
                        handScore[a] += (1322 * 2 ) + 5049;
                        pairExistAlready = true;
                    }
                    //handScore[a] += (1322 * 2 ) + 5049;
                }
                else if(cardOccurence[a][b]==3)
                {
                    if(pairExistAlready)
                    {
                        handScore[a] += (1322 * 3) + 36071;
                    }
                    else if(pairExistAlready == false)
                    {
                        handScore[a] += (1322 * 3) + 21130;
                        threeExistAlready = true;
                    }
                    //handScore[a] += (1322 * 3) + 21130;
                }
                else if(cardOccurence[a][b]==4)
                {
                    handScore[a] += (1322 * 4) + 51565;
                }
                a++;
                pairExistAlready = false;
                threeExistAlready = false;
                b = -1;
            }
        }

        //check if straight exists and add points
        for(int a = 0; a < 10; a++)
        {
            if(cardOccurence[a][0]==1 and
                    cardOccurence[a][12]==1 and
                    cardOccurence[a][11]==1 and
                    cardOccurence[a][10]==1 and
                    cardOccurence[a][9]==1)
            {
                handScore[a] += 30933;
                straightExists = true;
            }
            else if(cardOccurence[a][0]==1 and
                    cardOccurence[a][1]==1 and
                    cardOccurence[a][2]==1 and
                    cardOccurence[a][3]==1 and
                    cardOccurence[a][4]==1)
            {
                handScore[a] -= 2599;
                handScore[a] += 30933;
                straightExists = true;
            }
            else if(cardOccurence[a][1]==1 and
                    cardOccurence[a][2]==1 and
                    cardOccurence[a][3]==1 and
                    cardOccurence[a][4]==1 and
                    cardOccurence[a][5]==1)
            {
                handScore[a] += 30933;
                straightExists = true;
            }
            else if(cardOccurence[a][2]==1 and
                    cardOccurence[a][3]==1 and
                    cardOccurence[a][4]==1 and
                    cardOccurence[a][5]==1 and
                    cardOccurence[a][6]==1)
            {
                handScore[a] += 30933;
                straightExists = true;
            }
            else if(cardOccurence[a][3]==1 and
                    cardOccurence[a][4]==1 and
                    cardOccurence[a][5]==1 and
                    cardOccurence[a][6]==1 and
                    cardOccurence[a][7]==1)
            {
                handScore[a] += 30933;
                straightExists = true;
            }
            else if(cardOccurence[a][4]==1 and
                    cardOccurence[a][5]==1 and
                    cardOccurence[a][6]==1 and
                    cardOccurence[a][7]==1 and
                    cardOccurence[a][8]==1)
            {
                handScore[a] += 30933;
                straightExists = true;
            }
            else if(cardOccurence[a][5]==1 and
                    cardOccurence[a][6]==1 and
                    cardOccurence[a][7]==1 and
                    cardOccurence[a][8]==1 and
                    cardOccurence[a][9]==1)
            {
                handScore[a] += 30933;
                straightExists = true;
            }
            else if(cardOccurence[a][6]==1 and
                    cardOccurence[a][7]==1 and
                    cardOccurence[a][8]==1 and
                    cardOccurence[a][9]==1 and
                    cardOccurence[a][10]==1)
            {
                handScore[a] += 30933;
                straightExists = true;
            }
            else if(cardOccurence[a][7]==1 and
                    cardOccurence[a][8]==1 and
                    cardOccurence[a][9]==1 and
                    cardOccurence[a][10]==1 and
                    cardOccurence[a][11]==1)
            {
                handScore[a] += 30933;
                straightExists = true;
            }
            else if(cardOccurence[a][8]==1 and
                    cardOccurence[a][9]==1 and
                    cardOccurence[a][10]==1 and
                    cardOccurence[a][11]==1 and
                    cardOccurence[a][12]==1)
            {
                handScore[a] += 30933;
                straightExists = true;
            }
        }


        //check for flush and add points if exists
        for(int a = 0; a < 10; a++)
        {
            if(suite[a][0]==suite[a][1] and
                    suite[a][0]==suite[a][2] and
                    suite[a][0]==suite[a][3] and
                    suite[a][0]==suite[a][4])
            {
                if(straightExists)
                {
                    handScore[a] -= 30933;
                    handScore[a] += 63288;
                }
                else if (straightExists == false)
                {
                    handScore[a] += 36071;
                }
            }
        }


        //check which hand has the highest score
        for(int a = 0; a < 10; a++)
        {
            if(player->handScore <= handScore[a])
            {
                player->handScore = handScore[a];
            }
        }

        /*/test print out all scores
        for(int a = 0; a < 10; a++)
        {
            cout<<handScore[a]<<endl;
        }
        */
        //cout<<endl;
        //cout<<player->handScore<<endl;

        //compare player and ai hands to see who wins
        if(ai->handScore==68425)
        {
            ai->handName = "Royal Flush";
        }
        else if(ai->handScore > 63287 and ai->handScore < 65920)
        {
            ai->handName = "Straight Flush";
        }
        else if(ai->handScore > 51564 and ai->handScore < 63288)
        {
            ai->handName = "4 Of A Kind";
        }
        else if(ai->handScore > 41119 and ai->handScore < 51565)
        {
            ai->handName = "Full House";
        }
        else if(ai->handScore > 36070 and ai->handScore < 41120)
        {
            ai->handName = "Flush";
        }
        else if(ai->handScore > 30932 and ai->handScore < 35982)
        {
            ai->handName = "Straight";
        }
        else if(ai->handScore > 21129 and ai->handScore < 30933)
        {
            ai->handName = "3 Of A Kind";
        }
        else if(ai->handScore > 12604 and ai->handScore < 21130)
        {
            ai->handName = "2 Pairs";
        }
        else if(ai->handScore > 5048 and ai->handScore < 12604)
        {
            ai->handName = "1 Pair";
        }
        else if(ai->handScore < 5049)
        {
            ai->handName = "High Card";
        }

        //check players hand name
        if(player->handScore==68425)
        {
            ai->handName = "Royal Flush";
        }
        else if(player->handScore > 63287 and player->handScore < 65920)
        {
            player->handName = "Straight Flush";
        }
        else if(player->handScore > 51564 and player->handScore < 63288)
        {
            player->handName = "4 Of A Kind";
        }
        else if(player->handScore > 41119 and player->handScore < 51565)
        {
            player->handName = "Full House";
        }
        else if(player->handScore > 36070 and player->handScore < 41120)
        {
            player->handName = "Flush";
        }
        else if(player->handScore > 30932 and player->handScore < 35982)
        {
            player->handName = "Straight";
        }
        else if(player->handScore > 21129 and player->handScore < 30933)
        {
            player->handName = "3 Of A Kind";
        }
        else if(player->handScore > 12604 and player->handScore < 21130)
        {
            player->handName = "2 Pairs";
        }
        else if(player->handScore > 5048 and player->handScore < 12604)
        {
            player->handName = "1 Pair";
        }
        else if(player->handScore < 5049)
        {
            player->handName = "High Card";
        }

        if(player->handScore >= ai->handScore)
        {
            handNameLabel->setVisible(true);
            handNameLabel->setText("     Player Wins: " + player->handName);
            handNameLabel->setFont(*font);
            player->cash += potAmount;
            potAmount = 0;
            potAmountLabel->setText("     Pot Amount $" + QString::number(potAmount));
            aiCashLabel->setText("             $" + QString::number(ai->cash));
            playerCashLabel->setText("             $" + QString::number(player->cash));

            if(ai->cash < player->cash)
            {
                spinBox->setRange(1,ai->cash);
                slider->setRange(1,ai->cash);
                spinBox->setValue(1);
            }
            if(ai->cash==0)
            {
                QTimer::singleShot(2000,this,SLOT(dealerGamerCashReset()));
            }
        }
        else if(player->handScore < ai->handScore)
        {
            handNameLabel->setVisible(true);
            handNameLabel->setText("     Dealer Wins: " + ai->handName);
            handNameLabel->setFont(*font);
            ai->cash += potAmount;
            potAmount = 0;
            potAmountLabel->setText("     Pot Amount $" + QString::number(potAmount));
            aiCashLabel->setText("             $" + QString::number(ai->cash));
            playerCashLabel->setText("             $" + QString::number(player->cash));

            if(player->cash==0)
            {
                QTimer::singleShot(2000,this,SLOT(dealerGamerCashReset()));
            }
        }

        playerAnalyzed = false;
        dealerAnalyzed = false;
    }
}

void GameWindow::dealerGamerCashReset()
{
    if(ai->cash==0)
    {
        handNameLabel->setText("     You beat the dealer, his cash will reset");
        ai->cash = 10000;
        aiCashLabel->setText("             $" + QString::number(ai->cash));
    }
    else if (player->cash==0)
    {
        handNameLabel->setText("     You ran out of cash, your cash will reset");
        player->cash = 1000;
        playerCashLabel->setText("             $" + QString::number(player->cash));
    }
}

void GameWindow::setDefaultBacksideCard()
{
    for(int a = 0; a < 9; a++)
    {
        cardLabel[a].setPixmap(QPixmap(":/Images/Cards/backside.jpg"));
        if(a==0)
        {
            playerLayout1->addWidget(&cardLabel[a]);
        }
        else if(a==1)
        {
            playerLayout2->addWidget(&cardLabel[a]);
        }
        else if(a==2)
        {
            aiLayout1->addWidget(&cardLabel[a]);
        }
        else if(a==3)
        {
            aiLayout2->addWidget(&cardLabel[a]);
        }
        else if(a>3)
        {
            openCardLayout[a-4].addWidget(&cardLabel[a]);
        }
    }
}

void GameWindow::newProfileDialog()
{
    player->cash = 1000;
    playerCashLabel->setText("             $" + QString::number(player->cash));
    ai->cash = 10000;
    aiCashLabel->setText("             $" + QString::number(ai->cash));

    bool ok;
    QString text = QInputDialog::getText(this, tr("USERNAME"),
                                         tr("User name:"), QLineEdit::Normal
                                         );
    if (ok && !text.isEmpty())

        player->userName = text;

    playerNameLabel->setText(player->userName);
    profileWindow->hide();
    play->setEnabled(true);

}

void GameWindow::saveFileDialog()
{
    saveDialog->setFileMode(QFileDialog::AnyFile);
    QString testNameDir = saveDialog->getSaveFileName();

    file = new QFile(testNameDir);

    if (!file->open(QIODevice::WriteOnly | QIODevice::Text))
        return;

    QTextStream out(file);
    out << player->userName + "\n";
    out <<QString::number(player->cash) + "\n";
    out <<QString::number(ai->cash);
    file->close();


    //server connection
    Connection connection = createConnection();
        if ( !connection.connected ){
            qDebug() << "Not connected!";
            //return 1;
            return;
        }
        else{
            qDebug() << "Connected!";
            QSqlQuery query;
            query.prepare("SELECT TexasHoldEm FROM Tables");
            query.prepare("INSERT INTO TexasHoldEm (user, cashScore, dealerCash) "
                            "VALUES (:user, :score, :dealerScore)");
            cout<<player->userName.toStdString()<<endl;
            cout<<player->cash<<endl;
            cout<<ai->cash<<endl;
              query.bindValue(":user", player->userName);
              query.bindValue(":score", player->cash);
              query.bindValue(":dealerScore",ai->cash);

            query.exec();
            connection.db.close();
            }
}

void GameWindow::loadFileDialog()
{
    saveDialog->setFileMode(QFileDialog::ExistingFile);
    QString testNameDir = saveDialog->getOpenFileName();
    file2 = new QFile(testNameDir);

    if (!file2->open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QTextStream in(file2);
    QString line = in.readLine();
    player->userName = line;
    playerNameLabel->setText(player->userName);

    line = in.readLine();

    player->cash = line.toInt();
    playerCashLabel->setText("             $" + QString::number(player->cash));

    line = in.readLine();
    ai->cash = line.toInt();
    aiCashLabel->setText("             $" + QString::number(ai->cash));

    file2->close();
    profileWindow->hide();
    play->setEnabled(true);
}

Connection createConnection(){
    QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL");
    db.setHostName("209.129.8.2");
    db.setDatabaseName("48941");
    db.setUserName("48941");
    db.setPassword("48941cis17b");

    Connection connection;
    connection.db = db;
    if (!db.open()) {
        qDebug() << "Database error occurred";
        connection.connected = false;
        return connection;
    }
    connection.connected = true;

    return connection;
}
