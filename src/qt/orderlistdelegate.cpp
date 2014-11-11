#include "orderlistdelegate.h"

	ListDelegate::ListDelegate(QObject *parent)
	{
	
	}
	
	void ListDelegate::paint ( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const{
		QRect r = option.rect;
		
		//Color: #C4C4C4
		QPen linePen(QColor::fromRgb(211,211,211), 1, Qt::SolidLine);
		
		//Color: #005A83
		QPen lineMarkedPen(QColor::fromRgb(0,90,131), 1, Qt::SolidLine);
		
		//Color: #333
		QPen fontPen(QColor::fromRgb(51,51,51), 1, Qt::SolidLine);
		
		//Color: #fff
		QPen fontMarkedPen(Qt::white, 1, Qt::SolidLine);
		
		if(option.state & QStyle::State_Selected){
			QLinearGradient gradientSelected(r.left(),r.top(),r.left(),r.height()+r.top());
			gradientSelected.setColorAt(0.0, QColor::fromRgb(119,213,247));
			gradientSelected.setColorAt(0.9, QColor::fromRgb(27,134,183));
			gradientSelected.setColorAt(1.0, QColor::fromRgb(0,120,174));
			painter->setBrush(gradientSelected);
			painter->drawRect(r);
			
			//BORDER
			painter->setPen(lineMarkedPen);
			painter->drawLine(r.topLeft(),r.topRight());
			painter->drawLine(r.topRight(),r.bottomRight());
			painter->drawLine(r.bottomLeft(),r.bottomRight());
			painter->drawLine(r.topLeft(),r.bottomLeft());
			
			painter->setPen(fontMarkedPen);
			
		} else {
			//BACKGROUND
                        //ALTERNATING COLORS
			painter->setBrush( (index.row() % 2) ? Qt::white : QColor(252,252,252) );
			painter->drawRect(r);
			
			//BORDER
			painter->setPen(linePen);
			painter->drawLine(r.topLeft(),r.topRight());
			painter->drawLine(r.topRight(),r.bottomRight());
			painter->drawLine(r.bottomLeft(),r.bottomRight());
			painter->drawLine(r.topLeft(),r.bottomLeft());
			
			painter->setPen(fontPen);
		}
		
		
    // prepare the data for the entry
    QIcon ic = QIcon(qvariant_cast<QPixmap>(index.data(Qt::DecorationRole)));
    QString txid = index.data(Qt::DisplayRole).toString();
    QString displayText = index.data(Qt::UserRole + 1).toString();
    QString amountBought = index.data(Qt::UserRole + 2).toString();
    QString amountSold = index.data(Qt::UserRole + 3).toString();
    QString status = index.data(Qt::UserRole + 4).toString();

    // add the appropriate status icon
    int imageSpace = 10;
    if (!ic.isNull())
    {
        r = option.rect.adjusted(5, 10, -10, -10);
        ic.paint(painter, r, Qt::AlignVCenter|Qt::AlignLeft);
        imageSpace = 55;
    }

    // add the txid
    r = option.rect.adjusted(imageSpace, 0, -10, -30);
    painter->setFont( QFont( "Lucida Grande", 10, QFont::Bold ) );
    painter->drawText(r.left(), r.top(), r.width(), r.height(), Qt::AlignBottom|Qt::AlignLeft, txid, &r);
    // add the displaytext
    r = option.rect.adjusted(imageSpace, 30, -10, 0);
    painter->setFont( QFont( "Lucida Grande", 8, QFont::Normal ) );
    painter->drawText(r.left(), r.top(), r.width(), r.height(), Qt::AlignLeft, displayText, &r);
    // add the amount bought (green +)
    r = option.rect.adjusted(imageSpace, 30, -60, 0);
    painter->setFont( QFont( "Lucida Grande", 8, QFont::Normal ) );
    painter->drawText(r.left(), r.top(), r.width(), r.height(), Qt::AlignLeft, amountBought, &r);
    // add the amount sold (red -)
    r = option.rect.adjusted(imageSpace, 30, -90, 0);
    painter->setFont( QFont( "Lucida Grande", 8, QFont::Normal ) );
    painter->drawText(r.left(), r.top(), r.width(), r.height(), Qt::AlignLeft, amountSold, &r);
}

QSize ListDelegate::sizeHint ( const QStyleOptionViewItem & option, const QModelIndex & index ) const
{
    return QSize(200, 60); // very dumb value?
}

ListDelegate::~ListDelegate()
{
}
