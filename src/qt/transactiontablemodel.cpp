// Copyright (c) 2011-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "transactiontablemodel.h"

#include "addresstablemodel.h"
#include "bitcoinunits.h"
#include "guiconstants.h"
#include "guiutil.h"
#include "optionsmodel.h"
#include "transactiondesc.h"
#include "transactionrecord.h"
#include "walletmodel.h"

#include "main.h"
#include "sync.h"
#include "uint256.h"
#include "util.h"
#include "wallet.h"

#include <boost/filesystem.hpp>

#include "leveldb/db.h"
#include "leveldb/write_batch.h"
#include "mastercore.h"

#include <QColor>
#include <QDateTime>
#include <QDebug>
#include <QIcon>
#include <QList>

extern CWallet* pwalletMain;

// Amount column is right-aligned it contains numbers
static int column_alignments[] = {
        Qt::AlignLeft|Qt::AlignVCenter, /* status */
        Qt::AlignLeft|Qt::AlignVCenter, /* date */
        Qt::AlignLeft|Qt::AlignVCenter, /* type */
        Qt::AlignLeft|Qt::AlignVCenter, /* address */
        Qt::AlignRight|Qt::AlignVCenter /* amount */
    };

// Comparison operator for sort/binary search of model tx list
struct TxLessThan
{
    bool operator()(const TransactionRecord &a, const TransactionRecord &b) const
    {
        return a.hash < b.hash;
    }
    bool operator()(const TransactionRecord &a, const uint256 &b) const
    {
        return a.hash < b;
    }
    bool operator()(const uint256 &a, const TransactionRecord &b) const
    {
        return a < b.hash;
    }
};

// Private implementation
class TransactionTablePriv
{
public:
    TransactionTablePriv(CWallet *wallet, TransactionTableModel *parent) :
        wallet(wallet),
        parent(parent)
    {
    }

    CWallet *wallet;
    TransactionTableModel *parent;

    /* Local cache of wallet.
     * As it is in the same order as the CWallet, by definition
     * this is sorted by sha256.
     */
    QList<TransactionRecord> cachedWallet;

    /* Query entire wallet anew from core.
     */
    void refreshWallet()
    {
        qDebug() << "TransactionTablePriv::refreshWallet";
        cachedWallet.clear();
        {
            LOCK2(cs_main, wallet->cs_wallet);
            for(std::map<uint256, CWalletTx>::iterator it = wallet->mapWallet.begin(); it != wallet->mapWallet.end(); ++it)
            {
                if(TransactionRecord::showTransaction(it->second))
                    cachedWallet.append(TransactionRecord::decomposeTransaction(wallet, it->second));
            }
        }
    }

    /* Update our model of the wallet incrementally, to synchronize our model of the wallet
       with that of the core.

       Call with transaction that was added, removed or changed.
     */
    void updateWallet(const uint256 &hash, int status)
    {
        qDebug() << "TransactionTablePriv::updateWallet : " + QString::fromStdString(hash.ToString()) + " " + QString::number(status);
        {
            LOCK2(cs_main, wallet->cs_wallet);

            // Find transaction in wallet
            std::map<uint256, CWalletTx>::iterator mi = wallet->mapWallet.find(hash);
            bool inWallet = mi != wallet->mapWallet.end();

            // Find bounds of this transaction in model
            QList<TransactionRecord>::iterator lower = qLowerBound(
                cachedWallet.begin(), cachedWallet.end(), hash, TxLessThan());
            QList<TransactionRecord>::iterator upper = qUpperBound(
                cachedWallet.begin(), cachedWallet.end(), hash, TxLessThan());
            int lowerIndex = (lower - cachedWallet.begin());
            int upperIndex = (upper - cachedWallet.begin());
            bool inModel = (lower != upper);

            // Determine whether to show transaction or not
            bool showTransaction = (inWallet && TransactionRecord::showTransaction(mi->second));

            if(status == CT_UPDATED)
            {
                if(showTransaction && !inModel)
                    status = CT_NEW; /* Not in model, but want to show, treat as new */
                if(!showTransaction && inModel)
                    status = CT_DELETED; /* In model, but want to hide, treat as deleted */
            }

            qDebug() << "   inWallet=" + QString::number(inWallet) + " inModel=" + QString::number(inModel) +
                        " Index=" + QString::number(lowerIndex) + "-" + QString::number(upperIndex) +
                        " showTransaction=" + QString::number(showTransaction) + " derivedStatus=" + QString::number(status);

            switch(status)
            {
            case CT_NEW:
                if(inModel)
                {
                    qDebug() << "TransactionTablePriv::updateWallet : Warning: Got CT_NEW, but transaction is already in model";
                    break;
                }
                if(!inWallet)
                {
                    qDebug() << "TransactionTablePriv::updateWallet : Warning: Got CT_NEW, but transaction is not in wallet";
                    break;
                }
                if(showTransaction)
                {
                    // Added -- insert at the right position
                    QList<TransactionRecord> toInsert =
                            TransactionRecord::decomposeTransaction(wallet, mi->second);
                    if(!toInsert.isEmpty()) /* only if something to insert */
                    {
                        parent->beginInsertRows(QModelIndex(), lowerIndex, lowerIndex+toInsert.size()-1);
                        int insert_idx = lowerIndex;
                        foreach(const TransactionRecord &rec, toInsert)
                        {
                            cachedWallet.insert(insert_idx, rec);
                            insert_idx += 1;
                        }
                        parent->endInsertRows();
                    }
                }
                break;
            case CT_DELETED:
                if(!inModel)
                {
                    qDebug() << "TransactionTablePriv::updateWallet : Warning: Got CT_DELETED, but transaction is not in model";
                    break;
                }
                // Removed -- remove entire transaction from table
                parent->beginRemoveRows(QModelIndex(), lowerIndex, upperIndex-1);
                cachedWallet.erase(lower, upper);
                parent->endRemoveRows();
                break;
            case CT_UPDATED:
                // Miscellaneous updates -- nothing to do, status update will take care of this, and is only computed for
                // visible transactions.
                break;
            }
        }
    }

    int size()
    {
        return cachedWallet.size();
    }

    TransactionRecord *index(int idx)
    {
        if(idx >= 0 && idx < cachedWallet.size())
        {
            TransactionRecord *rec = &cachedWallet[idx];

            // Get required locks upfront. This avoids the GUI from getting
            // stuck if the core is holding the locks for a longer time - for
            // example, during a wallet rescan.
            //
            // If a status update is needed (blocks came in since last check),
            //  update the status of this transaction from the wallet. Otherwise,
            // simply re-use the cached status.
            TRY_LOCK(cs_main, lockMain);
            if(lockMain)
            {
                TRY_LOCK(wallet->cs_wallet, lockWallet);
                if(lockWallet && rec->statusUpdateNeeded())
                {
                    std::map<uint256, CWalletTx>::iterator mi = wallet->mapWallet.find(rec->hash);

                    if(mi != wallet->mapWallet.end())
                    {
                        rec->updateStatus(mi->second);
                    }
                }
            }
            return rec;
        }
        else
        {
            return 0;
        }
    }

    QString describe(TransactionRecord *rec, int unit)
    {
        {
            LOCK2(cs_main, wallet->cs_wallet);
            std::map<uint256, CWalletTx>::iterator mi = wallet->mapWallet.find(rec->hash);
            if(mi != wallet->mapWallet.end())
            {
                return TransactionDesc::toHTML(wallet, mi->second, rec->idx, unit);
            }
        }
        return QString("");
    }
};

TransactionTableModel::TransactionTableModel(CWallet* wallet, WalletModel *parent):
        QAbstractTableModel(parent),
        wallet(wallet),
        walletModel(parent),
        priv(new TransactionTablePriv(wallet, this))
{
    columns << QString() << tr("Date") << tr("Type") << tr("Address") << tr("Amount");

    priv->refreshWallet();

    connect(walletModel->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));
}

TransactionTableModel::~TransactionTableModel()
{
    delete priv;
}

void TransactionTableModel::updateTransaction(const QString &hash, int status)
{
    uint256 updated;
    updated.SetHex(hash.toStdString());

    priv->updateWallet(updated, status);
}

void TransactionTableModel::updateConfirmations()
{
    // Blocks came in since last poll.
    // Invalidate status (number of confirmations) and (possibly) description
    //  for all rows. Qt is smart enough to only actually request the data for the
    //  visible rows.
    emit dataChanged(index(0, Status), index(priv->size()-1, Status));
    emit dataChanged(index(0, ToAddress), index(priv->size()-1, ToAddress));
}

int TransactionTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return priv->size();
}

int TransactionTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return columns.length();
}

QString TransactionTableModel::formatTxStatus(const TransactionRecord *wtx) const
{
    QString status;

    switch(wtx->status.status)
    {
    case TransactionStatus::OpenUntilBlock:
        status = tr("Open for %n more block(s)","",wtx->status.open_for);
        break;
    case TransactionStatus::OpenUntilDate:
        status = tr("Open until %1").arg(GUIUtil::dateTimeStr(wtx->status.open_for));
        break;
    case TransactionStatus::Offline:
        status = tr("Offline");
        break;
    case TransactionStatus::Unconfirmed:
        status = tr("Unconfirmed");
        break;
    case TransactionStatus::Confirming:
        status = tr("Confirming (%1 of %2 recommended confirmations)").arg(wtx->status.depth).arg(TransactionRecord::RecommendedNumConfirmations);
        break;
    case TransactionStatus::Confirmed:
        status = tr("Confirmed (%1 confirmations)").arg(wtx->status.depth);
        break;
    case TransactionStatus::Conflicted:
        status = tr("Conflicted");
        break;
    case TransactionStatus::Immature:
        status = tr("Immature (%1 confirmations, will be available after %2)").arg(wtx->status.depth).arg(wtx->status.depth + wtx->status.matures_in);
        break;
    case TransactionStatus::MaturesWarning:
        status = tr("This block was not received by any other nodes and will probably not be accepted!");
        break;
    case TransactionStatus::NotAccepted:
        status = tr("Generated but not accepted");
        break;
    }

    return status;
}

QString TransactionTableModel::formatTxDate(const TransactionRecord *wtx) const
{
    if(wtx->time)
    {
        return GUIUtil::dateTimeStr(wtx->time);
    }
    else
    {
        return QString();
    }
}

/* Look up address in address book, if found return label (address)
   otherwise just return (address)
 */
QString TransactionTableModel::lookupAddress(const std::string &address, bool tooltip) const
{
    QString label = walletModel->getAddressTableModel()->labelForAddress(QString::fromStdString(address));
    QString description;
    if(!label.isEmpty())
    {
        description += label + QString(" ");
    }
    if(label.isEmpty() || walletModel->getOptionsModel()->getDisplayAddresses() || tooltip)
    {
        description += QString("(") + QString::fromStdString(address) + QString(")");
    }
    return description;
}

QString TransactionTableModel::formatTxType(const TransactionRecord *wtx) const
{
    switch(wtx->type)
    {
    case TransactionRecord::RecvWithAddress:
        return tr("Received with");
    case TransactionRecord::RecvFromOther:
        return tr("Received from");
    case TransactionRecord::SendToAddress:
    case TransactionRecord::SendToOther:
        return tr("Sent to");
    case TransactionRecord::SendToSelf:
        return tr("Payment to yourself");
    case TransactionRecord::Generated:
        return tr("Mined");
    default:
        return QString();
    }
}

QVariant TransactionTableModel::txAddressDecoration(const TransactionRecord *wtx) const
{
    switch(wtx->type)
    {
    case TransactionRecord::Generated:
        return QIcon(":/icons/tx_mined");
    case TransactionRecord::RecvWithAddress:
    case TransactionRecord::RecvFromOther:
        return QIcon(":/icons/tx_input");
    case TransactionRecord::SendToAddress:
    case TransactionRecord::SendToOther:
        return QIcon(":/icons/tx_output");
    default:
        return QIcon(":/icons/tx_inout");
    }
    return QVariant();
}

QString TransactionTableModel::formatTxToAddress(const TransactionRecord *wtx, bool tooltip) const
{
    switch(wtx->type)
    {
    case TransactionRecord::RecvFromOther:
        return QString::fromStdString(wtx->address);
    case TransactionRecord::RecvWithAddress:
    case TransactionRecord::SendToAddress:
    case TransactionRecord::Generated:
        return lookupAddress(wtx->address, tooltip);
    case TransactionRecord::SendToOther:
        return QString::fromStdString(wtx->address);
    case TransactionRecord::SendToSelf:
    default:
        return tr("(n/a)");
    }
}

QVariant TransactionTableModel::addressColor(const TransactionRecord *wtx) const
{
    // Show addresses without label in a less visible color
    switch(wtx->type)
    {
    case TransactionRecord::RecvWithAddress:
    case TransactionRecord::SendToAddress:
    case TransactionRecord::Generated:
        {
        QString label = walletModel->getAddressTableModel()->labelForAddress(QString::fromStdString(wtx->address));
        if(label.isEmpty())
            return COLOR_BAREADDRESS;
        } break;
    case TransactionRecord::SendToSelf:
        return COLOR_BAREADDRESS;
    default:
        break;
    }
    return QVariant();
}

QString TransactionTableModel::formatTxAmount(const TransactionRecord *wtx, bool showUnconfirmed) const
{
    QString str = BitcoinUnits::format(walletModel->getOptionsModel()->getDisplayUnit(), wtx->credit + wtx->debit);
    if(showUnconfirmed)
    {
        if(!wtx->status.countsForBalance)
        {
            str = QString("[") + str + QString("]");
        }
    }
    return QString(str);
}

QVariant TransactionTableModel::txStatusDecoration(const TransactionRecord *wtx) const
{
    switch(wtx->status.status)
    {
    case TransactionStatus::OpenUntilBlock:
    case TransactionStatus::OpenUntilDate:
        return QColor(64,64,255);
    case TransactionStatus::Offline:
        return QColor(192,192,192);
    case TransactionStatus::Unconfirmed:
        return QIcon(":/icons/transaction_0");
    case TransactionStatus::Confirming:
        switch(wtx->status.depth)
        {
        case 1: return QIcon(":/icons/transaction_1");
        case 2: return QIcon(":/icons/transaction_2");
        case 3: return QIcon(":/icons/transaction_3");
        case 4: return QIcon(":/icons/transaction_4");
        default: return QIcon(":/icons/transaction_5");
        };
    case TransactionStatus::Confirmed:
        return QIcon(":/icons/transaction_confirmed");
    case TransactionStatus::Conflicted:
        return QIcon(":/icons/transaction_conflicted");
    case TransactionStatus::Immature: {
        int total = wtx->status.depth + wtx->status.matures_in;
        int part = (wtx->status.depth * 4 / total) + 1;
        return QIcon(QString(":/icons/transaction_%1").arg(part));
        }
    case TransactionStatus::MaturesWarning:
    case TransactionStatus::NotAccepted:
        return QIcon(":/icons/transaction_0");
    }
    return QColor(0,0,0);
}

QString TransactionTableModel::formatTooltip(const TransactionRecord *rec) const
{
    QString tooltip = formatTxStatus(rec) + QString("\n") + formatTxType(rec);
    if(rec->type==TransactionRecord::RecvFromOther || rec->type==TransactionRecord::SendToOther ||
       rec->type==TransactionRecord::SendToAddress || rec->type==TransactionRecord::RecvWithAddress)
    {
        tooltip += QString(" ") + formatTxToAddress(rec, true);
    }
    return tooltip;
}

QVariant TransactionTableModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();
    TransactionRecord *rec = static_cast<TransactionRecord*>(index.internalPointer());

    switch(role)
    {
    case Qt::DecorationRole:
        switch(index.column())
        {
        case Status:
            return txStatusDecoration(rec);
        case ToAddress:
            return txAddressDecoration(rec);
        }
        break;
    case Qt::DisplayRole:
        switch(index.column())
        {
        case Date:
            return formatTxDate(rec);
        case Type:
            return formatTxType(rec);
        case ToAddress:
            return formatTxToAddress(rec, false);
        case Amount:
            return formatTxAmount(rec);
        }
        break;
    case Qt::EditRole:
        // Edit role is used for sorting, so return the unformatted values
        switch(index.column())
        {
        case Status:
            return QString::fromStdString(rec->status.sortKey);
        case Date:
            return rec->time;
        case Type:
            return formatTxType(rec);
        case ToAddress:
            return formatTxToAddress(rec, true);
        case Amount:
            return rec->credit + rec->debit;
        }
        break;
    case Qt::ToolTipRole:
        return formatTooltip(rec);
    case Qt::TextAlignmentRole:
        return column_alignments[index.column()];
    case Qt::ForegroundRole:
        // Non-confirmed (but not immature) as transactions are grey
        if(!rec->status.countsForBalance && rec->status.status != TransactionStatus::Immature)
        {
            return COLOR_UNCONFIRMED;
        }
        if(index.column() == Amount && (rec->credit+rec->debit) < 0)
        {
            return COLOR_NEGATIVE;
        }
        if(index.column() == ToAddress)
        {
            return addressColor(rec);
        }
        break;
    case TypeRole:
        return rec->type;
    case DateRole:
        return QDateTime::fromTime_t(static_cast<uint>(rec->time));
    case LongDescriptionRole:
        return priv->describe(rec, walletModel->getOptionsModel()->getDisplayUnit());
    case AddressRole:
        return QString::fromStdString(rec->address);
    case LabelRole:
        return walletModel->getAddressTableModel()->labelForAddress(QString::fromStdString(rec->address));
    case AmountRole:
        return rec->credit + rec->debit;
    case TxIDRole:
        return rec->getTxID();
    case TxHashRole:
        return QString::fromStdString(rec->hash.ToString());
    case ConfirmedRole:
        return rec->status.countsForBalance;
    case FormattedAmountRole:
        return formatTxAmount(rec, false);
    case StatusRole:
        return rec->status.status;
    }
    return QVariant();
}

QVariant TransactionTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation == Qt::Horizontal)
    {
        if(role == Qt::DisplayRole)
        {
            return columns[section];
        }
        else if (role == Qt::TextAlignmentRole)
        {
            return column_alignments[section];
        } else if (role == Qt::ToolTipRole)
        {
            switch(section)
            {
            case Status:
                return tr("Transaction status. Hover over this field to show number of confirmations.");
            case Date:
                return tr("Date and time that the transaction was received.");
            case Type:
                return tr("Type of transaction.");
            case ToAddress:
                return tr("Destination address of transaction.");
            case Amount:
                return tr("Amount removed from or added to balance.");
            }
        }
    }
    return QVariant();
}

QModelIndex TransactionTableModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    TransactionRecord *data = priv->index(row);
    if(data)
    {
        return createIndex(row, column, priv->index(row));
    }
    else
    {
        return QModelIndex();
    }
}

void TransactionTableModel::updateDisplayUnit()
{
    // emit dataChanged to update Amount column with the current unit
    emit dataChanged(index(0, Amount), index(priv->size()-1, Amount));
}


//////
// Private implementation
class msc_AddressTablePriv
{
public:
    CWallet *wallet;
    QList<msc_AddressTableEntry> msc_cachedAddressTable;
    MatrixModel *parent;

    msc_AddressTablePriv(CWallet *wallet, MatrixModel *parent):
        wallet(wallet), parent(parent) {}

    void refreshAddressTable()
    {
    int my_count = 0;

      qDebug() << "msc_AddressTablePriv::refreshAddressTable()";
      qDebug() << __FUNCTION__ << __LINE__ << __FILE__;
        msc_cachedAddressTable.clear();
        {
            LOCK(wallet->cs_wallet);
            BOOST_FOREACH(const PAIRTYPE(CTxDestination, CAddressBookData)& item, wallet->mapAddressBook)
            {
                const CBitcoinAddress& address = item.first;
//                bool fMine = IsMine(*wallet, address.Get());
                ++my_count;
                const std::string& strName = item.second.name;
                msc_cachedAddressTable.append(msc_AddressTableEntry(
                                  QString::fromStdString(strName),
                                  QString::fromStdString(address.ToString())));
            }

            qDebug() << __FUNCTION__ << " found " << my_count << " entries for the cachedAddressTable !!!";
        }
        // qLowerBound() and qUpperBound() require our cachedAddressTable list to be sorted in asc order
        // Even though the map is already sorted this re-sorting step is needed because the originating map
        // is sorted by binary address, not by base58() address.
        qSort(msc_cachedAddressTable.begin(), msc_cachedAddressTable.end(), msc_AddressTableEntryLessThan());
    }

    void updateEntry(const QString &address, const QString &label, bool isMine, const QString &purpose, int status)
    {
          qDebug() << "msc_AddressTablePriv::updateEntry()";

        // Find address / label in model
        QList<msc_AddressTableEntry>::iterator lower = qLowerBound(
            msc_cachedAddressTable.begin(), msc_cachedAddressTable.end(), address, msc_AddressTableEntryLessThan());
        QList<msc_AddressTableEntry>::iterator upper = qUpperBound(
            msc_cachedAddressTable.begin(), msc_cachedAddressTable.end(), address, msc_AddressTableEntryLessThan());
        int lowerIndex = (lower - msc_cachedAddressTable.begin());
        int upperIndex = (upper - msc_cachedAddressTable.begin());
        bool inModel = (lower != upper);

        switch(status)
        {
        case CT_NEW:
            if(inModel)
            {
                qDebug() << "msc_AddressTablePriv::updateEntry : Warning: Got CT_NOW, but entry is already in model";
                break;
            }
            parent->beginInsertRows(QModelIndex(), lowerIndex, lowerIndex);
            msc_cachedAddressTable.insert(lowerIndex, msc_AddressTableEntry(label, address));
            parent->endInsertRows();
            break;
        case CT_UPDATED:
            if(!inModel)
            {
                qDebug() << "msc_AddressTablePriv::updateEntry : Warning: Got CT_UPDATED, but entry is not in model";
                break;
            }
            lower->label = label;
            parent->emitDataChanged(lowerIndex);
            break;
        case CT_DELETED:
            if(!inModel)
            {
                qDebug() << "msc_AddressTablePriv::updateEntry : Warning: Got CT_DELETED, but entry is not in model";
                break;
            }
            parent->beginRemoveRows(QModelIndex(), lowerIndex, upperIndex-1);
            msc_cachedAddressTable.erase(lower, upper);
            parent->endRemoveRows();
            break;
        }
    }

    int size()
    {
        return msc_cachedAddressTable.size();
    }

    msc_AddressTableEntry *index(int idx)
    {
        if(idx >= 0 && idx < msc_cachedAddressTable.size())
        {
            return &msc_cachedAddressTable[idx];
        }
        else
        {
            return 0;
        }
    }
};

// Return the data to which index points.
QVariant MatrixModel::data(const QModelIndex& index, int role) const
{
        if (!index.isValid() || role != Qt::DisplayRole)
            return QVariant();

//        qDebug() << __FUNCTION__ << "row=" << index.row() << "column=" << index.column();

    switch(index.column())
    {
      case 0: return (ql_lab[index.row()]);
      case 1: return (ql_addr[index.row()]);
      case 2: return (ql_msc[index.row()]);
      case 3: return (ql_tmsc[index.row()]);
      default:
//        return m_data[index.row() * m_numColumns + index.column()];
        return QString("*NONE*");
    }
}

void MatrixModel::updateConfirmations(void)
{
}



    MatrixModel::MatrixModel(CWallet* wallet, WalletModel *parent)
        : m_numRows(3),
          m_numColumns(5)
    {
      qDebug() << "CONSTRUCTOR-wallet" << __FILE__ << __FUNCTION__ << __LINE__;
      priv = new msc_AddressTablePriv(wallet, this);
      priv->refreshAddressTable();
    }

    MatrixModel::MatrixModel(int numRows, int numColumns, uint* data, unsigned int propertyId)
        : m_numRows(numRows),
          m_numColumns(numColumns),
          m_data(data)
    {
      qDebug() << "CONSTRUCTOR-Mastercoin" << __FILE__ << __FUNCTION__ << __LINE__;
      columns << tr("Label") << tr("Address") << tr("Reserved") << tr("Available");

      m_numRows=fillin(propertyId);
      m_numColumns=4;

      qDebug() << "numRows=" << m_numRows << "numColumns=" << m_numColumns;
    }

MatrixModel::~MatrixModel()
{
  qDebug() << "DESTRUCTOR" << __FILE__ << __FUNCTION__ << __LINE__;
  // QList is a RAII container and automatically frees all resources upon destruction
}

    int MatrixModel::rowCount(const QModelIndex& parent) const
    {
        return m_numRows;
    }

    int MatrixModel::columnCount(const QModelIndex& parent) const
    {
        return m_numColumns;
    }

void MatrixModel::emitDataChanged(int idx)
{
    emit dataChanged(index(idx, 0, QModelIndex()), index(idx, columns.length()-1, QModelIndex()));
}

QVariant MatrixModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation == Qt::Horizontal)
    {
        if(role == Qt::DisplayRole && section < columns.size())
        {
            return columns[section];
        }
    }

    if(orientation == Qt::Vertical)
    {
        if(role == Qt::DisplayRole)
        {
            return 1+section;
        }
    }

    return QVariant();
}

int MatrixModel::fillin(unsigned int propertyId)
{
int count = 0;

    LOCK(cs_tally);
    for(map<string, CMPTally>::iterator my_it = mp_tally_map.begin(); my_it != mp_tally_map.end(); ++my_it)
       {
          string address = (my_it->first).c_str();
          int64_t available = getMPbalance(address, propertyId, MONEY);
          int64_t reserved = getMPbalance(address, propertyId, SELLOFFER_RESERVE);
          if (propertyId<3) reserved += getMPbalance(address, propertyId, ACCEPT_RESERVE);
          bool divisible = isPropertyDivisible(propertyId);

          ql_lab.append("Test Address Label");
          ql_addr.append((my_it->first).c_str());
          if (divisible)
          {
              ql_msc.append(QString::fromStdString(FormatDivisibleMP(available)));
              ql_tmsc.append(QString::fromStdString(FormatDivisibleMP(reserved)));
          }
          else
          {
              ql_msc.append(QString::fromStdString(FormatIndivisibleMP(available)));
              ql_tmsc.append(QString::fromStdString(FormatIndivisibleMP(reserved)));
          }
       }

	 // ql_lab.append("Test Address Label");
         // ql_addr.append("1EXoDusjGwvnjZUyKkxZ4UHEf77z6A5S4P");
         // ql_msc.append("12345.12345678");
         // ql_tmsc.append("54321.87654321");



//          printf("%34s =>> ", (my_it->first).c_str());
//          (my_it->second).print();

          ++count;
////        }

    qDebug() << "fillin()=" << count;

  return count;
}

