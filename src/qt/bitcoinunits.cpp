// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "bitcoinunits.h"
#include "primitives/transaction.h"
#include "guiconstants.h"
#include <QStringList>
#include <cmath>
#include <QRegularExpression>
BitcoinUnits::BitcoinUnits(QObject *parent):
        QAbstractListModel(parent),
        unitlist(availableUnits())
{
}
QList<BitcoinUnits::Unit> BitcoinUnits::availableUnits()
{
    QList<BitcoinUnits::Unit> unitlist;
    unitlist.append(BTC);
    if(COIN_MODE==DYNAMIC_COIN_MODE){
        unitlist.append(mBTC);
        unitlist.append(uBTC);
    }
    return unitlist;
}
bool BitcoinUnits::valid(int unit)
{
    switch(unit)
    {
    case BTC:
    case mBTC:
    case uBTC:
    case Dollar:
    case BTC_HTML:
        return true;
    default:
        return false;
    }
}
QString BitcoinUnits::name(int unit)
{
    switch(unit)
    {
    case BTC: return QString("OCC");
    case mBTC: return QString("mOCC");
    case uBTC: return QString::fromUtf8("μOCC");
    case Dollar: return QString("<span style='color:#bdeae5;'>$</span>");
    case BTC_HTML: return QString("<span style='color:#bdeae5;'>OCC</span>");
    default: return QString("???");
    }
}
QString BitcoinUnits::description(int unit)
{
    switch(unit)
    {
    case BTC_HTML:case BTC: return QString("OctoinCoins");
    case Dollar: return QString("$");
    case mBTC: return QString("Milli-OctoinCoins (1 / 1" THIN_SP_UTF8 "000)");
    case uBTC: return QString("Micro-OctoinCoins (1 / 1" THIN_SP_UTF8 "000" THIN_SP_UTF8 "000)");
    default: return QString("???");
    }
}
qint64 BitcoinUnits::factor(int unit)
{
    switch (COIN_MODE) {
    case STATIC_COIN_MODE:
        return pow(10,STATIC_DECEMALS);
    case DYNAMIC_COIN_MODE:
        switch(unit)
        {
        case BTC_HTML:case BTC:  return 100000000;
        case mBTC: return 100000;
        case uBTC: return 100;
        default:   return 100000000;
        }
    case CUSTOM_COIN_MODE:{
            return CUSTOM_FACTOR;
        }
    default: return 100000000;
    }
}
int BitcoinUnits::decimals(int unit)
{
    if(COIN_MODE==DYNAMIC_COIN_MODE){
        switch(unit)
        {
        case BTC_HTML:case BTC: return 8;
        case mBTC: return 5;
        case uBTC: return 2;
        default: return 0;
        }
    }else{
        return STATIC_DECEMALS;
    }
}
QString BitcoinUnits::format(int unit, const CAmount& nIn, bool fPlus, SeparatorStyle separators, bool autodecimals)
{
    // Note: not using straight sprintf here because we do NOT want
    // localized number formatting.
    if(!valid(unit))
        return QString(); // Refuse to format invalid unit
    qint64 n = (qint64)nIn;
    qint64 coin = factor(unit);
    int num_decimals = decimals(unit);
    qint64 n_abs = (n > 0 ? n : -n);
    qint64 quotient = n_abs / coin;
    qint64 remainder = n_abs % coin;
    QString quotient_str = QString::number(quotient);
    QString remainder_str = QString::number(remainder).rightJustified(num_decimals, '0');
    // Use SI-style thin space separators as these are locale independent and can't be
    // confused with the decimal marker.
    QChar thin_sp(THIN_SP_CP);
    int q_size = quotient_str.size();
    if (separators == separatorAlways || (separators == separatorStandard && q_size > 4))
        for (int i = 3; i < q_size; i += 3)
            quotient_str.insert(q_size - i, thin_sp);
    if (n < 0)
        quotient_str.insert(0, '-');
    else if (fPlus && n >= 0)
        quotient_str.insert(0, '+');
    QString result = quotient_str + QString(".") + remainder_str;
    if(autodecimals){
         return (remainder > 0)?result.remove(result.lastIndexOf(QRegularExpression("[^0]"))+1 ,10):quotient_str;
    }
    return result;
}
// NOTE: Using formatWithUnit in an HTML context risks wrapping
// quantities at the thousands separator. More subtly, it also results
// in a standard space rather than a thin space, due to a bug in Qt's
// XML whitespace canonicalisation
//
// Please take care to use formatHtmlWithUnit instead, when
// appropriate.
QString BitcoinUnits::formatWithUnit(int unit, const CAmount& amount, bool plussign, SeparatorStyle separators, bool autodecimals)
{
    return format(unit, amount, plussign, separators, autodecimals) + QString(" ") + name(unit);
}
QString BitcoinUnits::formatHtmlWithUnit(int unit, const CAmount& amount, bool plussign, SeparatorStyle separators, bool autodecimals)
{
    QString str(formatWithUnit(unit, amount, plussign, separators, autodecimals));
    str.replace(QChar(THIN_SP_CP), QString(THIN_SP_HTML));
    return QString("<span style='white-space: nowrap;'>%1</span>").arg(str);
}
bool BitcoinUnits::parse(int unit, const QString &value, CAmount *val_out)
{
    if(!valid(unit) || value.isEmpty())
        return false; // Refuse to parse invalid unit or empty string
    int num_decimals = decimals(unit);
    // Ignore spaces and thin spaces when parsing
    QStringList parts = removeSpaces(value).split(".");
    if(parts.size() > 2)
    {
        return false; // More than one dot
    }
    QString whole = parts[0];
    QString decimals;
    if(parts.size() > 1)
    {
        decimals = parts[1];
    }
    if(decimals.size() > num_decimals)
    {
        return false; // Exceeds max precision
    }
    bool ok = false;
    QString str = whole + decimals.leftJustified(num_decimals, '0');
    if(str.size() > 18)
    {
        return false; // Longer numbers will exceed 63 bits
    }
    CAmount retvalue(str.toLongLong(&ok));
    if(val_out)
    {
        *val_out = retvalue;
    }
    return ok;
}
QString BitcoinUnits::getAmountColumnTitle(int unit)
{
    QString amountTitle = QObject::tr("Amount");
    if (BitcoinUnits::valid(unit))
    {
        amountTitle += " ("+BitcoinUnits::name(unit) + ")";
    }
    return amountTitle;
}
int BitcoinUnits::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return unitlist.size();
}
QVariant BitcoinUnits::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    if(row >= 0 && row < unitlist.size())
    {
        Unit unit = unitlist.at(row);
        switch(role)
        {
        case Qt::EditRole:
        case Qt::DisplayRole:
            return QVariant(name(unit));
        case Qt::ToolTipRole:
            return QVariant(description(unit));
        case UnitRole:
            return QVariant(static_cast<int>(unit));
        }
    }
    return QVariant();
}
CAmount BitcoinUnits::maxMoney()
{
    return MAX_MONEY;
}
