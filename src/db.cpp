/*!
 * @author hongjun.liao <docici@126.com>, @date 2023/6/29
 *
 * rebuild the old Bitcoin code(Bitcoin v0.1.5) on a modern Linux/Ubuntu distribution
 * */

// Copyright (c) 2009 Satoshi Nakamoto
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

/////////////////////////////////////////////////////////////////////////////////////////////


//
// CDB
//

#include "headers.h"
#include "db.h"

#include <db_cxx.h>
#include <stdexcept>

using std::runtime_error;
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

    ////CRITICAL_BLOCK(cs_db)
    {
        if (!fDbEnvInit)
        {
//            string strAppDir = GetAppDir();
//            string strLogDir = strAppDir + "\\database";
//            _mkdir(strLogDir.c_str());
        	string strAppDir = "./", strLogDir= strAppDir;
            printf("dbenv.open strAppDir=%s\n", strAppDir.c_str());

            dbenv.set_lg_dir(strLogDir.c_str());
            dbenv.set_lg_max(10000000);
            dbenv.set_lk_max_locks(10000);
            dbenv.set_lk_max_objects(10000);
            dbenv.set_errfile(fopen("db.log", "a")); /// debug
            ///dbenv.log_set_config(DB_LOG_AUTO_REMOVE, 1); /// causes corruption
            try{
				ret = dbenv.open(strAppDir.c_str(),
	//                             DB_CREATE     |
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
//            if (ret > 0)
//                throw runtime_error(strprintf("CDB() : error %d opening database environment\n", ret));
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
        ////CRITICAL_BLOCK(cs_db)
            --mapFileUseCount[strFile];
        strFile = "";
        throw runtime_error(strprintf("CDB() : can't open database file %s, error %d\n", pszFile, ret));
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

    ////CRITICAL_BLOCK(cs_db)
        --mapFileUseCount[strFile];

    RandAddSeed();
}

void DBFlush(bool fShutdown)
{
    // Flush log data to the actual data file
    //  on all files that are not in use
    printf("DBFlush(%s)\n", fShutdown ? "true" : "false");
    ////CRITICAL_BLOCK(cs_db)
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
// CAddrDB
//

bool CAddrDB::WriteAddress(const CAddress& addr)
{
    return false; //Write(make_pair(string("addr"), addr.GetKey()), addr);
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




//
// CReviewDB
//

//bool CReviewDB::ReadReviews(uint256 hash, vector<CReview>& vReviews)
//{
//    vReviews.size(); // msvc workaround, just need to do anything with vReviews
//    return Read(make_pair(string("reviews"), hash), vReviews);
//}
//
//bool CReviewDB::WriteReviews(uint256 hash, const vector<CReview>& vReviews)
//{
//    return Write(make_pair(string("reviews"), hash), vReviews);
//}







//
// CWalletDB
//

bool CWalletDB::LoadWallet(vector<unsigned char>& vchDefaultKeyRet)
{
    vchDefaultKeyRet.clear();

    //// todo: shouldn't we catch exceptions and try to recover and continue?
    //CRITICAL_BLOCK(cs_mapKeys)
    //CRITICAL_BLOCK(cs_mapWallet)
    {
        // Get cursor
        Dbc* pcursor = GetCursor();
        if (!pcursor)
            return false;

        for(;;)
        {
            // Read next record
            CDataStream ssKey;
            CDataStream ssValue;
            int ret = ReadAtCursor(pcursor, ssKey, ssValue);
            if (ret == DB_NOTFOUND)
                break;
            else if (ret != 0)
                return false;

            // Unserialize
            // Taking advantage of the fact that pair serialization
            // is just the two items serialized one after the other
            string strType;
            ssKey >> strType;
            if (strType == "name")
            {
                string strAddress;
                ssKey >> strAddress;
                ssValue >> mapAddressBook[strAddress];
            }
            else if (strType == "tx")
            {
               _tx_cb(ssKey, ssValue);
            }
            else if (strType == "key")
            {
                vector<unsigned char> vchPubKey;
                ssKey >> vchPubKey;
                CPrivKey vchPrivKey;
                ssValue >> vchPrivKey;

                _key_cb(vchPrivKey, vchPubKey);
            }
            else if (strType == "defaultkey")
            {
                ssValue >> vchDefaultKeyRet;
            }
            else if (strType == "setting")  /// or settings or option or options or config?
            {
                string strKey;
                ssKey >> strKey;

                _settings_cb(strKey, ssValue);
            }
        }
    }
    return true;
}


