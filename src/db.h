/*!
 * @author hongjun.liao <docici@126.com>, @date 2023/6/29
 *
 * rebuild the old Bitcoin code(Bitcoin v0.1.5) on a modern Linux/Ubuntu distribution
 * */

// Copyright (c) 2009 Satoshi Nakamoto
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

/////////////////////////////////////////////////////////////////////////////////////////////

#ifndef BTC_DB_H
#define BTC_DB_H

#include "headers.h"
#include <db_cxx.h>
#include "key.h"
#include "serialize.h"
#include "net.h"
class CTxIndex;
class CDiskTxPos;
class CDataStream;

extern map<string, string> mapAddressBook;
extern bool fClient;


extern DbEnv dbenv;
extern void DBFlush(bool fShutdown);


///////////////////////////////////////////////////////////////////////////////////////////
class CDiskTxPos
{
public:
    unsigned int nFile;
    unsigned int nBlockPos;
    unsigned int nTxPos;

    CDiskTxPos()
    {
        SetNull();
    }

    CDiskTxPos(unsigned int nFileIn, unsigned int nBlockPosIn, unsigned int nTxPosIn)
    {
        nFile = nFileIn;
        nBlockPos = nBlockPosIn;
        nTxPos = nTxPosIn;
    }

    IMPLEMENT_SERIALIZE( READWRITE(FLATDATA(*this)); )
    void SetNull() { nFile = -1; nBlockPos = 0; nTxPos = 0; }
    bool IsNull() const { return (nFile == -1); }

    friend bool operator==(const CDiskTxPos& a, const CDiskTxPos& b)
    {
        return (a.nFile     == b.nFile &&
                a.nBlockPos == b.nBlockPos &&
                a.nTxPos    == b.nTxPos);
    }

    friend bool operator!=(const CDiskTxPos& a, const CDiskTxPos& b)
    {
        return !(a == b);
    }

    string ToString() const
    {
        if (IsNull())
            return strprintf("null");
        else
            return strprintf("(nFile=%d, nBlockPos=%d, nTxPos=%d)", nFile, nBlockPos, nTxPos);
    }

    void print() const
    {
        printf("%s", ToString().c_str());
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////

//
// A txdb record that contains the disk location of a transaction and the
// locations of transactions that spend its outputs.  vSpent is really only
// used as a flag, but having the location is very helpful for debugging.
//
class CTxIndex
{
public:
    CDiskTxPos pos;
    vector<CDiskTxPos> vSpent;

    CTxIndex()
    {
        SetNull();
    }

    CTxIndex(const CDiskTxPos& posIn, unsigned int nOutputs)
    {
        pos = posIn;
        vSpent.resize(nOutputs);
    }

    IMPLEMENT_SERIALIZE
    (
        if (!(nType & SER_GETHASH))
            READWRITE(nVersion);
        READWRITE(pos);
        READWRITE(vSpent);
    )

    void SetNull()
    {
        pos.SetNull();
        vSpent.clear();
    }

    bool IsNull()
    {
        return pos.IsNull();
    }

    friend bool operator==(const CTxIndex& a, const CTxIndex& b)
    {
        if (a.pos != b.pos || a.vSpent.size() != b.vSpent.size())
            return false;
        for (int i = 0; i < a.vSpent.size(); i++)
            if (a.vSpent[i] != b.vSpent[i])
                return false;
        return true;
    }

    friend bool operator!=(const CTxIndex& a, const CTxIndex& b)
    {
        return !(a == b);
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////


class CDB
{
protected:
    Db* pdb;
    string strFile;
    vector<DbTxn*> vTxn;

    explicit CDB(const char* pszFile, const char* pszMode="r+", bool fTxn=false);
    ~CDB() { Close(); }
public:
    void Close();
private:
    CDB(const CDB&);
    void operator=(const CDB&);

protected:
    template<typename K, typename T>
    bool Read(const K& key, T& value)
    {
        if (!pdb)
            return false;

        // Key
        CDataStream ssKey(SER_DISK);
        ssKey.reserve(1000);
        ssKey << key;
        Dbt datKey(&ssKey[0], ssKey.size());

        // Read
        Dbt datValue;
        datValue.set_flags(DB_DBT_MALLOC);
        int ret = pdb->get(GetTxn(), &datKey, &datValue, 0);
        memset(datKey.get_data(), 0, datKey.get_size());
        if (datValue.get_data() == NULL)
            return false;

        // Unserialize value
        CDataStream ssValue((char*)datValue.get_data(), (char*)datValue.get_data() + datValue.get_size(), SER_DISK);
        ssValue >> value;

        // Clear and free memory
        memset(datValue.get_data(), 0, datValue.get_size());
        free(datValue.get_data());
        return (ret == 0);
    }

    template<typename K, typename T>
    bool Write(const K& key, const T& value, bool fOverwrite=true)
    {
        if (!pdb)
            return false;

        // Key
        CDataStream ssKey(SER_DISK);
        ssKey.reserve(1000);
        ssKey << key;
        Dbt datKey(&ssKey[0], ssKey.size());

        // Value
        CDataStream ssValue(SER_DISK);
        ssValue.reserve(10000);
        ssValue << value;
        Dbt datValue(&ssValue[0], ssValue.size());

        // Write
        int ret = pdb->put(GetTxn(), &datKey, &datValue, (fOverwrite ? 0 : DB_NOOVERWRITE));

        // Clear memory in case it was a private key
        memset(datKey.get_data(), 0, datKey.get_size());
        memset(datValue.get_data(), 0, datValue.get_size());
        return (ret == 0);
    }

    template<typename K>
    bool Erase(const K& key)
    {
        if (!pdb)
            return false;

        // Key
        CDataStream ssKey(SER_DISK);
        ssKey.reserve(1000);
        ssKey << key;
        Dbt datKey(&ssKey[0], ssKey.size());

        // Erase
        int ret = pdb->del(GetTxn(), &datKey, 0);

        // Clear memory
        memset(datKey.get_data(), 0, datKey.get_size());
        return (ret == 0 || ret == DB_NOTFOUND);
    }

    template<typename K>
    bool Exists(const K& key)
    {
        if (!pdb)
            return false;

        // Key
        CDataStream ssKey(SER_DISK);
        ssKey.reserve(1000);
        ssKey << key;
        Dbt datKey(&ssKey[0], ssKey.size());

        // Exists
        int ret = pdb->exists(GetTxn(), &datKey, 0);

        // Clear memory
        memset(datKey.get_data(), 0, datKey.get_size());
        return (ret == 0);
    }

    Dbc* GetCursor()
    {
        if (!pdb)
            return NULL;
        Dbc* pcursor = NULL;
        int ret = pdb->cursor(NULL, &pcursor, 0);
        if (ret != 0)
            return NULL;
        return pcursor;
    }

    int ReadAtCursor(Dbc* pcursor, CDataStream& ssKey, CDataStream& ssValue, unsigned int fFlags=DB_NEXT)
    {
        // Read at cursor
        Dbt datKey;
        if (fFlags == DB_SET || fFlags == DB_SET_RANGE || fFlags == DB_GET_BOTH || fFlags == DB_GET_BOTH_RANGE)
        {
            datKey.set_data(&ssKey[0]);
            datKey.set_size(ssKey.size());
        }
        Dbt datValue;
        if (fFlags == DB_GET_BOTH || fFlags == DB_GET_BOTH_RANGE)
        {
            datValue.set_data(&ssValue[0]);
            datValue.set_size(ssValue.size());
        }
        datKey.set_flags(DB_DBT_MALLOC);
        datValue.set_flags(DB_DBT_MALLOC);
        int ret = pcursor->get(&datKey, &datValue, fFlags);
        if (ret != 0)
            return ret;
        else if (datKey.get_data() == NULL || datValue.get_data() == NULL)
            return 99999;

        // Convert to streams
        ssKey.SetType(SER_DISK);
        ssKey.clear();
        ssKey.write((char*)datKey.get_data(), datKey.get_size());
        ssValue.SetType(SER_DISK);
        ssValue.clear();
        ssValue.write((char*)datValue.get_data(), datValue.get_size());

        // Clear and free memory
        memset(datKey.get_data(), 0, datKey.get_size());
        memset(datValue.get_data(), 0, datValue.get_size());
        free(datKey.get_data());
        free(datValue.get_data());
        return 0;
    }

    DbTxn* GetTxn()
    {
        if (!vTxn.empty())
            return vTxn.back();
        else
            return NULL;
    }

public:
    bool TxnBegin()
    {
        if (!pdb)
            return false;
        DbTxn* ptxn = NULL;
        int ret = dbenv.txn_begin(GetTxn(), &ptxn, 0);
        if (!ptxn || ret != 0)
            return false;
        vTxn.push_back(ptxn);
        return true;
    }

    bool TxnCommit()
    {
        if (!pdb)
            return false;
        if (vTxn.empty())
            return false;
        int ret = vTxn.back()->commit(0);
        vTxn.pop_back();
        return (ret == 0);
    }

    bool TxnAbort()
    {
        if (!pdb)
            return false;
        if (vTxn.empty())
            return false;
        int ret = vTxn.back()->abort();
        vTxn.pop_back();
        return (ret == 0);
    }

    bool ReadVersion(int& nVersion)
    {
        nVersion = 0;
        return Read(string("version"), nVersion);
    }

    bool WriteVersion(int nVersion)
    {
        return Write(string("version"), nVersion);
    }
};


//class CReviewDB : public CDB
//{
//public:
//    CReviewDB(const char* pszMode="r+", bool fTxn=false) : CDB("reviews.dat", pszMode, fTxn) { }
//private:
//    CReviewDB(const CReviewDB&);
//    void operator=(const CReviewDB&);
//public:
//    bool ReadUser(uint256 hash, CUser& user)
//    {
//        return Read(make_pair(string("user"), hash), user);
//    }
//
//    bool WriteUser(uint256 hash, const CUser& user)
//    {
//        return Write(make_pair(string("user"), hash), user);
//    }
//
//    bool ReadReviews(uint256 hash, vector<CReview>& vReviews);
//    bool WriteReviews(uint256 hash, const vector<CReview>& vReviews);
//};





class CMarketDB : public CDB
{
public:
    CMarketDB(const char* pszMode="r+", bool fTxn=false) : CDB("market.dat", pszMode, fTxn) { }
private:
    CMarketDB(const CMarketDB&);
    void operator=(const CMarketDB&);
};





class CAddrDB : public CDB
{
public:
    CAddrDB(const char* pszMode="r+", bool fTxn=false) : CDB("addr.dat", pszMode, fTxn) { }
private:
    CAddrDB(const CAddrDB&);
    void operator=(const CAddrDB&);
public:
    bool WriteAddress(const CAddress& addr);
    bool LoadAddresses();
};

bool LoadAddresses();





class CWalletDB : public CDB
{
public:
	std::function<int(CDataStream&, CDataStream &)> _tx_cb;
	std::function<int(CPrivKey const& vchPrivKey, vector<unsigned char> const& vchPubKey)> _key_cb;
	std::function<int(string const& k, CDataStream & v)> _settings_cb;

public:
    CWalletDB(const char* pszMode="r+", bool fTxn=false) : CDB("wallet.dat", pszMode, fTxn) { }
private:
    CWalletDB(const CWalletDB&);
    void operator=(const CWalletDB&);
public:
    bool ReadName(const string& strAddress, string& strName)
    {
        strName = "";
        return Read(make_pair(string("name"), strAddress), strName);
    }

    bool WriteName(const string& strAddress, const string& strName)
    {
        mapAddressBook[strAddress] = strName;
        return Write(make_pair(string("name"), strAddress), strName);
    }

    bool EraseName(const string& strAddress)
    {
        mapAddressBook.erase(strAddress);
        return Erase(make_pair(string("name"), strAddress));
    }
//fixme
//    bool ReadTx(uint256 hash, CWalletTx& wtx)
//    {
//        return Read(make_pair(string("tx"), hash), wtx);
//    }

//    bool WriteTx(uint256 hash, const CWalletTx& wtx)
//    {
//        return Write(make_pair(string("tx"), hash), wtx);
//    }

    bool EraseTx(uint256 hash)
    {
        return Erase(make_pair(string("tx"), hash));
    }

    bool ReadKey(const vector<unsigned char>& vchPubKey, CPrivKey& vchPrivKey)
    {
        vchPrivKey.clear();
        return Read(make_pair(string("key"), vchPubKey), vchPrivKey);
    }

    bool WriteKey(const vector<unsigned char>& vchPubKey, const CPrivKey& vchPrivKey)
    {
        return Write(make_pair(string("key"), vchPubKey), vchPrivKey, false);
    }

    bool ReadDefaultKey(vector<unsigned char>& vchPubKey)
    {
        vchPubKey.clear();
        return Read(string("defaultkey"), vchPubKey);
    }

    bool WriteDefaultKey(const vector<unsigned char>& vchPubKey)
    {
        return Write(string("defaultkey"), vchPubKey);
    }

    template<typename T>
    bool ReadSetting(const string& strKey, T& value)
    {
        return Read(make_pair(string("setting"), strKey), value);
    }

    template<typename T>
    bool WriteSetting(const string& strKey, const T& value)
    {
        return Write(make_pair(string("setting"), strKey), value);
    }

    bool LoadWallet(vector<unsigned char>& vchDefaultKeyRet);
};

bool LoadWallet();

inline bool SetAddressBookName(const string& strAddress, const string& strName)
{
    return CWalletDB().WriteName(strAddress, strName);
}

#endif //#ifndef BTC_DB_H
