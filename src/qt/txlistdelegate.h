#include <QPainter>
#include <QAbstractItemDelegate>

class TXListDelegate : public QAbstractItemDelegate
{
	public:
		TXListDelegate(QObject *parent = 0);
	
		void paint ( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const;
		QSize sizeHint ( const QStyleOptionViewItem & option, const QModelIndex & index ) const;
		
		virtual ~TXListDelegate();
	
};
