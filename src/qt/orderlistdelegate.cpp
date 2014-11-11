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
    QIcon ic = QIcon(":/icons/balances");
// QIcon ic = QIcon(qvariant_cast<QPixmap>(index.data(Qt::DecorationRole)));
//    string shortTXID = QString::fromStdString(index.data(Qt::DisplayRole).toString().toStdString().substr(0,12));
    QString txidstatus = index.data(Qt::UserRole + 4).toString() + "   (" + QString::fromStdString(index.data(Qt::DisplayRole).toString().toStdString().substr(0,8)) + "...)";
//    txidstatus += "....\tSTATUS: " + index.data(Qt::UserRole + 4).toString();
    QString displayText = index.data(Qt::UserRole + 1).toString();
    QString amountBought = index.data(Qt::UserRole + 2).toString();
    QString amountSold = index.data(Qt::UserRole + 3).toString();
    QString status = index.data(Qt::UserRole + 4).toString();
    QString senderText = index.data(Qt::UserRole + 5).toString();

    // add the appropriate status icon
    int imageSpace = 10;
    if (!ic.isNull())
    {
        r = option.rect.adjusted(5, 10, -10, -10);
        ic.paint(painter, r, Qt::AlignVCenter|Qt::AlignLeft);
        imageSpace = 55;
    }

    // setup pens
    QPen penBlack(QColor("#000000"));
    QPen penRed(QColor("#CC0000"));
    QPen penGreen(QColor("#00AA00"));

    QFont font = painter->font();
    // add the displaytext
    painter->setPen(penBlack);
    r = option.rect.adjusted(imageSpace, 0, -10, -30);
    font.setBold(true);
    painter->setFont(font);
    painter->drawText(r.left(), r.top(), r.width(), r.height(), Qt::AlignBottom|Qt::AlignLeft, displayText, &r);
    // add the txid/status
    font.setBold(false);
    painter->setFont(font);
    r = option.rect.adjusted(imageSpace, 30, -10, 0);
    painter->drawText(r.left(), r.top(), r.width(), r.height(), Qt::AlignLeft, txidstatus, &r);
    font.setBold(true);
    painter->setFont(font);
    if("0 " != amountBought.toStdString().substr(0,2)) painter->setPen(penGreen);
    r = option.rect.adjusted(imageSpace, 0, -10, -30);
    painter->drawText(r.left(), r.top(), r.width(), r.height(), Qt::AlignBottom|Qt::AlignRight, amountBought, &r);
    if("0 " != amountSold.toStdString().substr(0,2)) painter->setPen(penRed);
    r = option.rect.adjusted(imageSpace, 30, -10, 0);
    painter->drawText(r.left(), r.top(), r.width(), r.height(), Qt::AlignRight, amountSold, &r);
    font.setBold(false);
    painter->setFont(font);
}

QSize ListDelegate::sizeHint ( const QStyleOptionViewItem & option, const QModelIndex & index ) const
{
    return QSize(200, 60); // very dumb value?
}

ListDelegate::~ListDelegate()
{
}
