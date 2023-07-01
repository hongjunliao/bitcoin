/*!
 * @author hongjun.liao <docici@126.com>, @date 2023/6/29
 *
 * rebuild the old Bitcoin code(Bitcoin v0.1.5) on a modern Linux/Ubuntu distribution
 * */

// Copyright (c) 2009 Satoshi Nakamoto
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

/////////////////////////////////////////////////////////////////////////////////////////////
#include "headers.h"
#include "db.h"
#include "main.h"

extern bool fClient;




//
// CDB
//

//static CCriticalSection cs_db;
static bool fDbEnvInit = false;
DbEnv dbenv(0);
static map<string, int> mapFileUseCount;

class CDBInit
{
public:
    CDBInit()
    {
    }
    ~CDBInit()
    {
        if (fDbEnvInit)
        {
            dbenv.close(0);
            fDbEnvInit = false;
        }
    }
}
instance_of_cdbinit;

/////////////////////////////////////////////////////////////////////////////////////////////

bool CDB::TxnBegin()
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

CDB::CDB(const char* pszFile, const char* pszMode, bool fTxn) : pdb(NULL)
{
    int ret;
    if (pszFile == NULL)
        return;

    bool fCreate = strchr(pszMode, 'c');
    bool fReadOnly = (!strchr(pszMode, '+') && !strchr(pszMode, 'w'));
    unsigned int nFlags = DB_THREAD;
    if (fCreate)
        nFlags |= DB_CREATE;
    else if (fReadOnly)
        nFlags |= DB_RDONLY;
    if (!fReadOnly || fTxn)
        nFlags |= DB_AUTO_COMMIT;

    //CRITICAL_BLOCK(cs_db)
    {
        if (!fDbEnvInit)
        {
//            string strAppDir = GetAppDir();
//            string strLogDir = strAppDir + "\\database";
//            _mkdir(strLogDir.c_str());
//            printf("dbenv.open strAppDir=%s\n", strAppDir.c_str());

        	string strAppDir("."), strLogDir(".");
            dbenv.set_lg_dir(strLogDir.c_str());
            dbenv.set_lg_max(10000000);
            dbenv.set_lk_max_locks(10000);
            dbenv.set_lk_max_objects(10000);
            dbenv.set_errfile(fopen("db.log", "a")); /// debug
            ///dbenv.log_set_config(DB_LOG_AUTO_REMOVE, 1); /// causes corruption
            try{
				ret = dbenv.open(strAppDir.c_str(),
	                             DB_CREATE     |
								 DB_INIT_LOCK  |
								 DB_INIT_LOG   |
								 DB_INIT_MPOOL |
								 DB_INIT_TXN   |
								 DB_THREAD     |
								 DB_PRIVATE    |
								 DB_RECOVER,
								 0);

				fDbEnvInit = true;
			} catch (DbException &e) {
				std::cerr << "Database error: " << e.what() << std::endl;
			} catch (std::exception &e) {
				std::cerr << "Error: " << e.what() << std::endl;
			} catch(...){}
            if (ret > 0)
                throw std::runtime_error(strprintf("CDB() : error %d opening database environment\n", ret));
            fDbEnvInit = true;
        }

        strFile = pszFile;
        ++mapFileUseCount[strFile];
    }

    pdb = new Db(&dbenv, 0);

    try{
        ret = pdb->open(NULL,      // Txn pointer
                        pszFile,   // Filename
                        "main",    // Logical db name
                        DB_BTREE,  // Database type
                        nFlags,    // Flags
                        0);
	} catch (DbException &e) {
		std::cerr << "Database error: " << e.what() << std::endl;
	} catch (std::exception &e) {
		std::cerr << "Error: " << e.what() << std::endl;
	} catch(...){}

    if (ret > 0)
    {
        delete pdb; pdb = NULL;
        //CRITICAL_BLOCK(cs_db)
            --mapFileUseCount[strFile];
        strFile = "";
        throw std::runtime_error(strprintf("CDB() : can't open database file %s, error %d\n", pszFile, ret));
    }

    if (fCreate && !Exists(string("version")))
        WriteVersion(VERSION);

    RandAddSeed();
}

void CDB::Close()
{
    if (!pdb)
        return;
    if (!vTxn.empty())
        vTxn.front()->abort();
    vTxn.clear();
    pdb->close(0);
    delete pdb;
    pdb = NULL;
    dbenv.txn_checkpoint(0, 0, 0);

    //CRITICAL_BLOCK(cs_db)
        --mapFileUseCount[strFile];

    RandAddSeed();
}

void DBFlush(bool fShutdown)
{
    // Flush log data to the actual data file
    //  on all files that are not in use
    printf("DBFlush(%s)\n", fShutdown ? "true" : "false");
    //CRITICAL_BLOCK(cs_db)
    {
        dbenv.txn_checkpoint(0, 0, 0);
        map<string, int>::iterator mi = mapFileUseCount.begin();
        while (mi != mapFileUseCount.end())
        {
            string strFile = (*mi).first;
            int nRefCount = (*mi).second;
            if (nRefCount == 0)
            {
                dbenv.lsn_reset(strFile.c_str(), 0);
                mapFileUseCount.erase(mi++);
            }
            else
                mi++;
        }
        if (fShutdown)
        {
            char** listp;
            if (mapFileUseCount.empty())
                dbenv.log_archive(&listp, DB_ARCH_REMOVE);
            dbenv.close(0);
            fDbEnvInit = false;
        }
    }
}






//
// CTxDB
//
CTxDB::CTxDB(const char* pszMode, bool fTxn)
	: CDB(!fClient ? "blkindex.dat" : NULL, pszMode, fTxn) { }


bool CTxDB::ReadTxIndex(uint256 hash, CTxIndex& txindex)
{
    assert(!fClient);
    txindex.SetNull();
    return Read(make_pair(string("tx"), hash), txindex);
}

bool CTxDB::UpdateTxIndex(uint256 hash, const CTxIndex& txindex)
{
    assert(!fClient);
    return Write(make_pair(string("tx"), hash), txindex);
}

bool CTxDB::AddTxIndex(const CTransaction& tx, const CDiskTxPos& pos, int nHeight)
{
    assert(!fClient);

    // Add to tx index
    uint256 hash = tx.GetHash();
    CTxIndex txindex(pos, tx.vout.size());
    return Write(make_pair(string("tx"), hash), txindex);
}

bool CTxDB::EraseTxIndex(const CTransaction& tx)
{
    assert(!fClient);
    uint256 hash = tx.GetHash();

    return Erase(make_pair(string("tx"), hash));
}

bool CTxDB::ContainsTx(uint256 hash)
{
    assert(!fClient);
    return Exists(make_pair(string("tx"), hash));
}

bool CTxDB::ReadOwnerTxes(uint160 hash160, int nMinHeight, vector<CTransaction>& vtx)
{
    assert(!fClient);
    vtx.clear();

    // Get cursor
    Dbc* pcursor = GetCursor();
    if (!pcursor)
        return false;

    unsigned int fFlags = DB_SET_RANGE;
    for(;;)
    {
        // Read next record
        CDataStream ssKey;
        if (fFlags == DB_SET_RANGE)
            ssKey << string("owner") << hash160 << CDiskTxPos(0, 0, 0);
        CDataStream ssValue;
        int ret = ReadAtCursor(pcursor, ssKey, ssValue, fFlags);
        fFlags = DB_NEXT;
        if (ret == DB_NOTFOUND)
            break;
        else if (ret != 0)
            return false;

        // Unserialize
        string strType;
        uint160 hashItem;
        CDiskTxPos pos;
        ssKey >> strType >> hashItem >> pos;
        int nItemHeight;
        ssValue >> nItemHeight;

        // Read transaction
        if (strType != "owner" || hashItem != hash160)
            break;
        if (nItemHeight >= nMinHeight)
        {
            vtx.resize(vtx.size()+1);
            if (!vtx.back().ReadFromDisk(pos))
                return false;
        }
    }
    return true;
}

bool CTxDB::ReadDiskTx(uint256 hash, CTransaction& tx, CTxIndex& txindex)
{
    assert(!fClient);
    tx.SetNull();
    if (!ReadTxIndex(hash, txindex))
        return false;
    return (tx.ReadFromDisk(txindex.pos));
}

bool CTxDB::ReadDiskTx(uint256 hash, CTransaction& tx)
{
    CTxIndex txindex;
    return ReadDiskTx(hash, tx, txindex);
}

bool CTxDB::ReadDiskTx(COutPoint outpoint, CTransaction& tx, CTxIndex& txindex)
{
    return ReadDiskTx(outpoint.hash, tx, txindex);
}

bool CTxDB::ReadDiskTx(COutPoint outpoint, CTransaction& tx)
{
    CTxIndex txindex;
    return ReadDiskTx(outpoint.hash, tx, txindex);
}

bool CTxDB::WriteBlockIndex(const CDiskBlockIndex& blockindex)
{
    return Write(make_pair(string("blockindex"), blockindex.GetBlockHash()), blockindex);
}

bool CTxDB::EraseBlockIndex(uint256 hash)
{
    return Erase(make_pair(string("blockindex"), hash));
}

bool CTxDB::ReadHashBestChain(uint256& hashBestChain)
{
    return Read(string("hashBestChain"), hashBestChain);
}

bool CTxDB::WriteHashBestChain(uint256 hashBestChain)
{
    return Write(string("hashBestChain"), hashBestChain);
}

/////////////////////////////////////////////////////////////////////////////////////////////
//
// CAddrDB
//

bool CAddrDB::WriteAddress(const CAddress& addr)
{
    return Write(make_pair(string("addr"), addr.GetKey()), addr);
}

bool CAddrDB::LoadAddresses()
{
	return false;
//    //CRITICAL_BLOCK(cs_mapIRCAddresses)
//    //CRITICAL_BLOCK(cs_mapAddresses)
//    {
//        // Load user provided addresses
//        CAutoFile filein = fopen("addr.txt", "rt");
//        if (filein)
//        {
//            try
//            {
//                char psz[1000];
//                while (fgets(psz, sizeof(psz), filein))
//                {
//                    CAddress addr(psz, NODE_NETWORK);
//                    if (addr.ip != 0)
//                    {
//                        AddAddress(*this, addr);
//                        mapIRCAddresses.insert(make_pair(addr.GetKey(), addr));
//                    }
//                }
//            }
//            catch (...) { }
//        }
//
//        // Get cursor
//        Dbc* pcursor = GetCursor();
//        if (!pcursor)
//            return false;
//
//        for(;;)
//        {
//            // Read next record
//            CDataStream ssKey;
//            CDataStream ssValue;
//            int ret = ReadAtCursor(pcursor, ssKey, ssValue);
//            if (ret == DB_NOTFOUND)
//                break;
//            else if (ret != 0)
//                return false;
//
//            // Unserialize
//            string strType;
//            ssKey >> strType;
//            if (strType == "addr")
//            {
//                CAddress addr;
//                ssValue >> addr;
//                mapAddresses.insert(make_pair(addr.GetKey(), addr));
//            }
//        }
//
//        //// debug print
//        printf("mapAddresses:\n");
//        foreach(const PAIRTYPE(vector<unsigned char>, CAddress)& item, mapAddresses)
//            item.second.print();
//        printf("-----\n");
//
//        // Fix for possible bug that manifests in mapAddresses.count in irc.cpp,
//        // just need to call count here and it doesn't happen there.  The bug was the
//        // pack pragma in irc.cpp and has been fixed, but I'm not in a hurry to delete this.
//        mapAddresses.count(vector<unsigned char>(18));
//    }
//
//    return true;
}

bool LoadAddresses()
{
    return CAddrDB("cr+").LoadAddresses();
}


