#include "txlistdelegate.h"

TXListDelegate::TXListDelegate(QObject *parent)
{

}

void TXListDelegate::paint ( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const
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
    QString txidsender = "ADDR: " + QString::fromStdString(index.data(Qt::UserRole + 5).toString().toStdString().substr(0,18)) + "...";
    txidsender += "   TX: " + QString::fromStdString(index.data(Qt::DisplayRole).toString().toStdString().substr(0,18)) + "...";
//    txidstatus += "....\tSTATUS: " + index.data(Qt::UserRole + 4).toString();
    QString displayType = index.data(Qt::UserRole + 1).toString();
    QString displayAmount = index.data(Qt::UserRole + 2).toString();
    QString displayDirection = index.data(Qt::UserRole + 3).toString();
    QString displayAddress = index.data(Qt::UserRole + 4).toString();
    QString txTimeText = index.data(Qt::UserRole + 5).toString();

    // add the appropriate status icon
    int imageSpace = 10;
    QIcon ic = QIcon(":/icons/meta_cancelled");
    if (!ic.isNull())
    {
        r = option.rect.adjusted(5, 10, -10, -10);
        ic.paint(painter, r, Qt::AlignVCenter|Qt::AlignLeft);
        imageSpace = 30;
    }

    // setup pens
    QPen penBlack(QColor("#000000"));
    QPen penRed(QColor("#CC0000"));
    QPen penGreen(QColor("#00AA00"));
    QPen penGrey(QColor("#606060"));

    QFont font = painter->font();
    // add the datetime
    painter->setPen(penGrey);
//    font.setItalic(true);
    painter->setFont(font);
    r = option.rect.adjusted(imageSpace, 5, -10, 0);
    painter->drawText(r.left(), r.top(), r.width(), r.height(), Qt::AlignLeft, txTimeText, &r);
    // add the displaytype
    painter->setPen(penBlack);
    r = option.rect.adjusted(imageSpace+125, 5, -10, 0);
//    font.setBold(true);
    font.setItalic(false);
    painter->setFont(font);
    painter->drawText(r.left(), r.top(), r.width(), r.height(), Qt::AlignLeft, displayType, &r);
    // add the address
    painter->setPen(penGrey);
    font.setBold(false);
    painter->setFont(font);
    r = option.rect.adjusted(imageSpace+250, 5, -10, 0);
    painter->drawText(r.left(), r.top(), r.width(), r.height(), Qt::AlignLeft, displayAddress, &r);
    // add the amount
    font.setBold(true);
    painter->setFont(font);
    if(displayDirection=="out") { painter->setPen(penRed); } else { painter->setPen(penGreen); }
    r = option.rect.adjusted(imageSpace+450, 5, -10, 0);
    painter->drawText(r.left(), r.top(), r.width(), r.height(), Qt::AlignRight, displayAmount, &r);
    font.setBold(false);
    painter->setFont(font);
}

QSize TXListDelegate::sizeHint ( const QStyleOptionViewItem & option, const QModelIndex & index ) const
{
    return QSize(600, 30); // very dumb value?
}

TXListDelegate::~TXListDelegate()
{
}
