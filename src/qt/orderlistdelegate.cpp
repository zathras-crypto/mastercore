#include "orderlistdelegate.h"

ListDelegate::ListDelegate(QObject *parent)
{

}

void ListDelegate::paint ( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const
{
    QRect r = option.rect;
    QPen linePen(QColor::fromRgb(211,211,211), 1, Qt::SolidLine);
    QPen fontPen(QColor::fromRgb(51,51,51), 1, Qt::SolidLine);
    painter->setPen(linePen);

    // alt the colors
    painter->setBrush( (index.row() % 2) ? Qt::white : QColor(252,252,252) );
    painter->drawRect(r);
    // draw border
    painter->setPen(linePen);
    painter->drawLine(r.topLeft(),r.topRight());
    painter->drawLine(r.topRight(),r.bottomRight());
    painter->drawLine(r.bottomLeft(),r.bottomRight());
    painter->drawLine(r.topLeft(),r.bottomLeft());
    painter->setPen(fontPen);

    // prepare the data for the entry
// QIcon ic = QIcon(qvariant_cast<QPixmap>(index.data(Qt::DecorationRole)));
//    string shortTXID = QString::fromStdString(index.data(Qt::DisplayRole).toString().toStdString().substr(0,12));
    QString txidsender = "ADD: " + QString::fromStdString(index.data(Qt::UserRole + 5).toString().toStdString().substr(0,18)) + "...";
    txidsender += "   TX: " + QString::fromStdString(index.data(Qt::DisplayRole).toString().toStdString().substr(0,18)) + "...";
//    txidstatus += "....\tSTATUS: " + index.data(Qt::UserRole + 4).toString();
    QString displayText = index.data(Qt::UserRole + 1).toString();
    QString amountBought = index.data(Qt::UserRole + 2).toString();
    QString amountSold = index.data(Qt::UserRole + 3).toString();
    QString status = index.data(Qt::UserRole + 4).toString();
    QString senderText = index.data(Qt::UserRole + 5).toString();
    QString txTimeText = index.data(Qt::UserRole + 6).toString();

    // add the appropriate status icon
    int imageSpace = 10;
    QIcon ic = QIcon(":/icons/meta_cancelled");
    if(status == "CANCELLED") ic =QIcon(":/icons/meta_cancelled");
    if(status == "PART CANCEL") ic = QIcon(":/icons/meta_partialclosed");
    if(status == "FILLED") ic = QIcon(":/icons/meta_filled");
    if(status == "OPEN") ic = QIcon(":/icons/meta_open");
    if(status == "PART FILLED") ic = QIcon(":/icons/meta_partial");
    if (!ic.isNull())
    {
        r = option.rect.adjusted(5, 10, -10, -10);
        ic.paint(painter, r, Qt::AlignVCenter|Qt::AlignLeft);
        imageSpace = 60;
    }

    // setup pens
    QPen penBlack(QColor("#000000"));
    QPen penRed(QColor("#CC0000"));
    QPen penGreen(QColor("#00AA00"));
    QPen penGrey(QColor("#606060"));

    QFont font = painter->font();
    painter->setPen(penBlack);
    // add the status
    font.setItalic(false);
    painter->setFont(font);
    r = option.rect.adjusted(imageSpace-19, 0, -10, -25);
    painter->drawText(r.left(), r.top(), r.width(), r.height(), Qt::AlignBottom|Qt::AlignLeft, status, &r);
    // add the datetime
    painter->setPen(penGrey);
    font.setItalic(true);
    painter->setFont(font);
    r = option.rect.adjusted(imageSpace-19, 25, -10, 0);
    painter->drawText(r.left(), r.top(), r.width(), r.height(), Qt::AlignLeft, txTimeText, &r);
    // add the displaytext
    painter->setPen(penBlack);
    r = option.rect.adjusted(imageSpace+115, 0, -10, -25);
    font.setBold(true);
    font.setItalic(false);
    painter->setFont(font);
    painter->drawText(r.left(), r.top(), r.width(), r.height(), Qt::AlignBottom|Qt::AlignLeft, displayText, &r);
    // add the txid/sender
    painter->setPen(penGrey);
    font.setBold(false);
    painter->setFont(font);
    r = option.rect.adjusted(imageSpace+115, 25, -10, 0);
    painter->drawText(r.left(), r.top(), r.width(), r.height(), Qt::AlignLeft, txidsender, &r);
    font.setBold(true);
    painter->setFont(font);
    if("0 " != amountBought.toStdString().substr(0,2)) painter->setPen(penGreen);
    r = option.rect.adjusted(imageSpace+115, 0, -10, -25);
    painter->drawText(r.left(), r.top(), r.width(), r.height(), Qt::AlignBottom|Qt::AlignRight, amountBought, &r);
    if("0 " != amountSold.toStdString().substr(0,2)) painter->setPen(penRed);
    r = option.rect.adjusted(imageSpace+115, 25, -10, 0);
    painter->drawText(r.left(), r.top(), r.width(), r.height(), Qt::AlignRight, amountSold, &r);
    font.setBold(false);
    painter->setFont(font);

}

QSize ListDelegate::sizeHint ( const QStyleOptionViewItem & option, const QModelIndex & index ) const
{
    return QSize(200, 50); // very dumb value?
}

ListDelegate::~ListDelegate()
{
}
