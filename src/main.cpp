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
#include "serialize.h"
#include "uint256.h"
#include "sds/sds.h"	//sds
//#include "script.h"
#include "util.h"
//#include "db.h"

class CDiskTxPos;
class CInPoint;
class CTransaction;
class CScript;
class COutPoint;
class CTxIn;

int nBestHeight = -1;

void PrintBlockTree();
bool LoadBlockIndex(bool fAllowNew);
FILE* AppendBlockFile(unsigned int& nFileRet);
template<typename Stream>
bool ScanMessageStart(Stream& s);
string GetAppDir();
bool CheckDiskSpace(int64 nAdditionalBytes);
FILE* OpenBlockFile(unsigned int nFile, unsigned int nBlockPos, const char* pszMode);
/////////////////////////////////////////////////////////////////////////////////////

//class CDiskTxPos
//{
//public:
//    unsigned int nFile;
//    unsigned int nBlockPos;
//    unsigned int nTxPos;
//
//    CDiskTxPos()
//    {
//        SetNull();
//    }
//
//    CDiskTxPos(unsigned int nFileIn, unsigned int nBlockPosIn, unsigned int nTxPosIn)
//    {
//        nFile = nFileIn;
//        nBlockPos = nBlockPosIn;
//        nTxPos = nTxPosIn;
//    }
////    IMPLEMENT_SERIALIZE( READWRITE(FLATDATA(*this)); )
//    void SetNull() { nFile = -1; nBlockPos = 0; nTxPos = 0; }
//    bool IsNull() const { return (nFile == -1); }
//
//    friend bool operator==(const CDiskTxPos& a, const CDiskTxPos& b)
//    {
//        return (a.nFile     == b.nFile &&
//                a.nBlockPos == b.nBlockPos &&
//                a.nTxPos    == b.nTxPos);
//    }
//
//    friend bool operator!=(const CDiskTxPos& a, const CDiskTxPos& b)
//    {
//        return !(a == b);
//    }
//
//    string ToString() const
//    {
//        if (IsNull())
//            return strprintf("null");
//        else
//            return strprintf("(nFile=%d, nBlockPos=%d, nTxPos=%d)", nFile, nBlockPos, nTxPos);
//    }
//
//    void print() const
//    {
//        printf("%s", ToString().c_str());
//    }
//};

///////////////////////////////////////////////////////////////////////////////////////


class CInPoint
{
public:
    CTransaction* ptx;
    unsigned int n;

    CInPoint() { SetNull(); }
    CInPoint(CTransaction* ptxIn, unsigned int nIn) { ptx = ptxIn; n = nIn; }
    void SetNull() { ptx = NULL; n = -1; }
    bool IsNull() const { return (ptx == NULL && n == -1); }
};
///////////////////////////////////////////////////////////////////////////////////////

class COutPoint
{
public:
    uint256 hash;
    unsigned int n;

    COutPoint() { SetNull(); }
    COutPoint(uint256 hashIn, unsigned int nIn) { hash = hashIn; n = nIn; }
    IMPLEMENT_SERIALIZE( READWRITE(FLATDATA(*this)); )
    void SetNull() { hash = 0; n = -1; }
    bool IsNull() const { return (hash == 0 && n == -1); }

    friend bool operator<(const COutPoint& a, const COutPoint& b)
    {
        return (a.hash < b.hash || (a.hash == b.hash && a.n < b.n));
    }

    friend bool operator==(const COutPoint& a, const COutPoint& b)
    {
        return (a.hash == b.hash && a.n == b.n);
    }

    friend bool operator!=(const COutPoint& a, const COutPoint& b)
    {
        return !(a == b);
    }

    string ToString() const
    {
        sds s = sdscatfmt(sdsempty(), "COutPoint(%s, %d)", hash.ToString().substr(0,6).c_str(), n);
    	string ret(s);
    	sdsfree(s);
    	return ret;
    }

    void print() const
    {
        printf("%s\n", ToString().c_str());
    }
};

///////////////////////////////////////////////////////////////////////////////////////
//
//
// An input of a transaction.  It contains the location of the previous
// transaction's output that it claims and a signature that matches the
// output's public key.
//
//class CTxIn
//{
//public:
//    COutPoint prevout;
//    CScript scriptSig;
//    unsigned int nSequence;
//
//    CTxIn()
//    {
//        nSequence = UINT_MAX;
//    }
//
//    explicit CTxIn(COutPoint prevoutIn, CScript scriptSigIn=CScript(), unsigned int nSequenceIn=UINT_MAX)
//    {
//        prevout = prevoutIn;
//        scriptSig = scriptSigIn;
//        nSequence = nSequenceIn;
//    }
//
//    CTxIn(uint256 hashPrevTx, unsigned int nOut, CScript scriptSigIn=CScript(), unsigned int nSequenceIn=UINT_MAX)
//    {
//        prevout = COutPoint(hashPrevTx, nOut);
//        scriptSig = scriptSigIn;
//        nSequence = nSequenceIn;
//    }
//
//    IMPLEMENT_SERIALIZE
//    (
//        READWRITE(prevout);
//        READWRITE(scriptSig);
//        READWRITE(nSequence);
//    )
//
//    bool IsFinal() const
//    {
//        return (nSequence == UINT_MAX);
//    }
//
//    friend bool operator==(const CTxIn& a, const CTxIn& b)
//    {
//        return (a.prevout   == b.prevout &&
//                a.scriptSig == b.scriptSig &&
//                a.nSequence == b.nSequence);
//    }
//
//    friend bool operator!=(const CTxIn& a, const CTxIn& b)
//    {
//        return !(a == b);
//    }
//
//    string ToString() const
//    {
//        string str;
//        str += strprintf("CTxIn(");
//        str += prevout.ToString();
//        if (prevout.IsNull())
//            str += strprintf(", coinbase %s", HexStr(scriptSig.begin(), scriptSig.end(), false).c_str());
//        else
//            str += strprintf(", scriptSig=%s", scriptSig.ToString().substr(0,24).c_str());
//        if (nSequence != UINT_MAX)
//            str += strprintf(", nSequence=%u", nSequence);
//        str += ")";
//        return str;
//    }
//
//    void print() const
//    {
//        printf("%s\n", ToString().c_str());
//    }
//
//    bool IsMine() const;
//    int64 GetDebit() const;
//};
//
/////////////////////////////////////////////////////////////////////////////////////////
//
//// An output of a transaction.  It contains the public key that the next input
//// must be able to sign with to claim it.
////
//class CTxOut
//{
//public:
//    int64 nValue;
//    CScript scriptPubKey;
//
//public:
//    CTxOut()
//    {
//        SetNull();
//    }
//
//    CTxOut(int64 nValueIn, CScript scriptPubKeyIn)
//    {
//        nValue = nValueIn;
//        scriptPubKey = scriptPubKeyIn;
//    }
//
//    IMPLEMENT_SERIALIZE
//    (
//        READWRITE(nValue);
//        READWRITE(scriptPubKey);
//    )
//
//    void SetNull()
//    {
//        nValue = -1;
//        scriptPubKey.clear();
//    }
//
//    bool IsNull()
//    {
//        return (nValue == -1);
//    }
//
//    uint256 GetHash() const
//    {
//        return SerializeHash(*this);
//    }
//
//    bool IsMine() const
//    {
//        return ::IsMine(scriptPubKey);
//    }
//
//    int64 GetCredit() const
//    {
//        if (IsMine())
//            return nValue;
//        return 0;
//    }
//
//    friend bool operator==(const CTxOut& a, const CTxOut& b)
//    {
//        return (a.nValue       == b.nValue &&
//                a.scriptPubKey == b.scriptPubKey);
//    }
//
//    friend bool operator!=(const CTxOut& a, const CTxOut& b)
//    {
//        return !(a == b);
//    }
//
//    string ToString() const
//    {
//        if (scriptPubKey.size() < 6)
//            return "CTxOut(error)";
//        return strprintf("CTxOut(nValue=%I64d.%08I64d, scriptPubKey=%s)", nValue / COIN, nValue % COIN, scriptPubKey.ToString().substr(0,24).c_str());
//    }
//
//    void print() const
//    {
//        printf("%s\n", ToString().c_str());
//    }
//};
///////////////////////////////////////////////////////////////////////////////////////
//
// The basic transaction that is broadcasted on the network and contained in
// blocks.  A transaction can contain multiple inputs and outputs.
//
//class CTransaction
//{
//public:
//    int nVersion;
//    vector<CTxIn> vin;
//    vector<CTxOut> vout;
//    int nLockTime;
//
//
//    CTransaction()
//    {
//        SetNull();
//    }
//
//    IMPLEMENT_SERIALIZE
//    (
//        READWRITE(this->nVersion);
//        nVersion = this->nVersion;
//        READWRITE(vin);
//        READWRITE(vout);
//        READWRITE(nLockTime);
//    )
//
//    void SetNull()
//    {
//        nVersion = 1;
//        vin.clear();
//        vout.clear();
//        nLockTime = 0;
//    }
//
//    bool IsNull() const
//    {
//        return (vin.empty() && vout.empty());
//    }
//
//    uint256 GetHash() const
//    {
//        return SerializeHash(*this);
//    }
//
//    bool IsFinal() const
//    {
//        if (nLockTime == 0 || nLockTime < nBestHeight)
//            return true;
//        foreach(const CTxIn& txin, vin)
//            if (!txin.IsFinal())
//                return false;
//        return true;
//    }
//
//    bool IsNewerThan(const CTransaction& old) const
//    {
//        if (vin.size() != old.vin.size())
//            return false;
//        for (int i = 0; i < vin.size(); i++)
//            if (vin[i].prevout != old.vin[i].prevout)
//                return false;
//
//        bool fNewer = false;
//        unsigned int nLowest = UINT_MAX;
//        for (int i = 0; i < vin.size(); i++)
//        {
//            if (vin[i].nSequence != old.vin[i].nSequence)
//            {
//                if (vin[i].nSequence <= nLowest)
//                {
//                    fNewer = false;
//                    nLowest = vin[i].nSequence;
//                }
//                if (old.vin[i].nSequence < nLowest)
//                {
//                    fNewer = true;
//                    nLowest = old.vin[i].nSequence;
//                }
//            }
//        }
//        return fNewer;
//    }
//
//    bool IsCoinBase() const
//    {
//        return (vin.size() == 1 && vin[0].prevout.IsNull());
//    }
//
//    bool CheckTransaction() const
//    {
//        // Basic checks that don't depend on any context
//        if (vin.empty() || vout.empty())
//            return error("CTransaction::CheckTransaction() : vin or vout empty");
//
//        // Check for negative values
//        foreach(const CTxOut& txout, vout)
//            if (txout.nValue < 0)
//                return error("CTransaction::CheckTransaction() : txout.nValue negative");
//
//        if (IsCoinBase())
//        {
//            if (vin[0].scriptSig.size() < 2 || vin[0].scriptSig.size() > 100)
//                return error("CTransaction::CheckTransaction() : coinbase script size");
//        }
//        else
//        {
//            foreach(const CTxIn& txin, vin)
//                if (txin.prevout.IsNull())
//                    return error("CTransaction::CheckTransaction() : prevout is null");
//        }
//
//        return true;
//    }
//
//    bool IsMine() const
//    {
//        foreach(const CTxOut& txout, vout)
//            if (txout.IsMine())
//                return true;
//        return false;
//    }
//
//    int64 GetDebit() const
//    {
//        int64 nDebit = 0;
//        foreach(const CTxIn& txin, vin)
//            nDebit += txin.GetDebit();
//        return nDebit;
//    }
//
//    int64 GetCredit() const
//    {
//        int64 nCredit = 0;
//        foreach(const CTxOut& txout, vout)
//            nCredit += txout.GetCredit();
//        return nCredit;
//    }
//
//    int64 GetValueOut() const
//    {
//        int64 nValueOut = 0;
//        foreach(const CTxOut& txout, vout)
//        {
//            if (txout.nValue < 0){
//                printf("CTransaction::GetValueOut() : negative value");
//                return 0;
//            }
//            nValueOut += txout.nValue;
//        }
//        return nValueOut;
//    }
//
//    int64 GetMinFee(bool fDiscount=false) const
//    {
//        // Base fee is 1 cent per kilobyte
//        unsigned int nBytes = ::GetSerializeSize(*this, SER_NETWORK);
//        int64 nMinFee = (1 + (int64)nBytes / 1000) * CENT;
//
//        // First 100 transactions in a block are free
//        if (fDiscount && nBytes < 10000)
//            nMinFee = 0;
//
//        // To limit dust spam, require a 0.01 fee if any output is less than 0.01
//        if (nMinFee < CENT)
//            foreach(const CTxOut& txout, vout)
//                if (txout.nValue < CENT)
//                    nMinFee = CENT;
//
//        return nMinFee;
//    }
//
//
//
//    bool ReadFromDisk(CDiskTxPos pos, FILE** pfileRet=NULL)
//    {
//        CAutoFile filein = OpenBlockFile(pos.nFile, 0, pfileRet ? "rb+" : "rb");
//        if (!filein)
//            return error("CTransaction::ReadFromDisk() : OpenBlockFile failed");
//
//        // Read transaction
//        if (fseek(filein, pos.nTxPos, SEEK_SET) != 0)
//            return error("CTransaction::ReadFromDisk() : fseek failed");
//        filein >> *this;
//
//        // Return file pointer
//        if (pfileRet)
//        {
//            if (fseek(filein, pos.nTxPos, SEEK_SET) != 0)
//                return error("CTransaction::ReadFromDisk() : second fseek failed");
//            *pfileRet = filein.release();
//        }
//        return true;
//    }
//
//
//    friend bool operator==(const CTransaction& a, const CTransaction& b)
//    {
//        return (a.nVersion  == b.nVersion &&
//                a.vin       == b.vin &&
//                a.vout      == b.vout &&
//                a.nLockTime == b.nLockTime);
//    }
//
//    friend bool operator!=(const CTransaction& a, const CTransaction& b)
//    {
//        return !(a == b);
//    }
//
//
//    string ToString() const
//    {
//        string str;
//        str += strprintf("CTransaction(hash=%s, ver=%d, vin.size=%d, vout.size=%d, nLockTime=%d)\n",
//            GetHash().ToString().substr(0,6).c_str(),
//            nVersion,
//            vin.size(),
//            vout.size(),
//            nLockTime);
//        for (int i = 0; i < vin.size(); i++)
//            str += "    " + vin[i].ToString() + "\n";
//        for (int i = 0; i < vout.size(); i++)
//            str += "    " + vout[i].ToString() + "\n";
//        return str;
//    }
//
//    void print() const
//    {
//        printf("%s", ToString().c_str());
//    }
//
//
//
//    bool DisconnectInputs(CTxDB& txdb);
//    bool ConnectInputs(CTxDB& txdb, map<uint256, CTxIndex>& mapTestPool, CDiskTxPos posThisTx, int nHeight, int64& nFees, bool fBlock, bool fMiner, int64 nMinFee=0);
//    bool ClientConnectInputs();
//
//    bool AcceptTransaction(CTxDB& txdb, bool fCheckInputs=true, bool* pfMissingInputs=NULL);
//
//    bool AcceptTransaction(bool fCheckInputs=true, bool* pfMissingInputs=NULL)
//    {
//        CTxDB txdb("r");
//        return AcceptTransaction(txdb, fCheckInputs, pfMissingInputs);
//    }
//
//protected:
//    bool AddToMemoryPool();
//public:
//    bool RemoveFromMemoryPool();
//};

//
//
//
//
////
//// A transaction with a merkle branch linking it to the block chain
////
//class CMerkleTx : public CTransaction
//{
//public:
//    uint256 hashBlock;
//    vector<uint256> vMerkleBranch;
//    int nIndex;
//
//    // memory only
//    mutable bool fMerkleVerified;
//
//
//    CMerkleTx()
//    {
//        Init();
//    }
//
//    CMerkleTx(const CTransaction& txIn) : CTransaction(txIn)
//    {
//        Init();
//    }
//
//    void Init()
//    {
//        hashBlock = 0;
//        nIndex = -1;
//        fMerkleVerified = false;
//    }
//
//    int64 GetCredit() const
//    {
//        // Must wait until coinbase is safely deep enough in the chain before valuing it
//        if (IsCoinBase() && GetBlocksToMaturity() > 0)
//            return 0;
//        return CTransaction::GetCredit();
//    }
//
//    IMPLEMENT_SERIALIZE
//    (
//        nSerSize += SerReadWrite(s, *(CTransaction*)this, nType, nVersion, ser_action);
//        nVersion = this->nVersion;
//        READWRITE(hashBlock);
//        READWRITE(vMerkleBranch);
//        READWRITE(nIndex);
//    )
//
//
//    int SetMerkleBranch(const CBlock* pblock=NULL);
//    int GetDepthInMainChain() const;
//    bool IsInMainChain() const { return GetDepthInMainChain() > 0; }
//    int GetBlocksToMaturity() const;
//    bool AcceptTransaction(CTxDB& txdb, bool fCheckInputs=true);
//    bool AcceptTransaction() { CTxDB txdb("r"); return AcceptTransaction(txdb); }
//};
//
//
//
//
////
//// A transaction with a bunch of additional info that only the owner cares
//// about.  It includes any unrecorded transactions needed to link it back
//// to the block chain.
////
//class CWalletTx : public CMerkleTx
//{
//public:
//    vector<CMerkleTx> vtxPrev;
//    map<string, string> mapValue;
//    vector<pair<string, string> > vOrderForm;
//    unsigned int fTimeReceivedIsTxTime;
//    unsigned int nTimeReceived;  // time received by this node
//    char fFromMe;
//    char fSpent;
//    //// probably need to sign the order info so know it came from payer
//
//    // memory only
//    mutable unsigned int nTimeDisplayed;
//
//
//    CWalletTx()
//    {
//        Init();
//    }
//
//    CWalletTx(const CMerkleTx& txIn) : CMerkleTx(txIn)
//    {
//        Init();
//    }
//
//    CWalletTx(const CTransaction& txIn) : CMerkleTx(txIn)
//    {
//        Init();
//    }
//
//    void Init()
//    {
//        fTimeReceivedIsTxTime = false;
//        nTimeReceived = 0;
//        fFromMe = false;
//        fSpent = false;
//        nTimeDisplayed = 0;
//    }
//
//    IMPLEMENT_SERIALIZE
//    (
//        nSerSize += SerReadWrite(s, *(CMerkleTx*)this, nType, nVersion, ser_action);
//        nVersion = this->nVersion;
//        READWRITE(vtxPrev);
//        READWRITE(mapValue);
//        READWRITE(vOrderForm);
//        READWRITE(fTimeReceivedIsTxTime);
//        READWRITE(nTimeReceived);
//        READWRITE(fFromMe);
//        READWRITE(fSpent);
//    )
//
//    bool WriteToDisk()
//    {
//        return CWalletDB().WriteTx(GetHash(), *this);
//    }
//
//
//    int64 GetTxTime() const;
//
//    void AddSupportingTransactions(CTxDB& txdb);
//
//    bool AcceptWalletTransaction(CTxDB& txdb, bool fCheckInputs=true);
//    bool AcceptWalletTransaction() { CTxDB txdb("r"); return AcceptWalletTransaction(txdb); }
//
//    void RelayWalletTransaction(CTxDB& txdb);
//    void RelayWalletTransaction() { CTxDB txdb("r"); RelayWalletTransaction(txdb); }
//};
//
//
//
//
////
//// A txdb record that contains the disk location of a transaction and the
//// locations of transactions that spend its outputs.  vSpent is really only
//// used as a flag, but having the location is very helpful for debugging.
////
//class CTxIndex
//{
//public:
//    CDiskTxPos pos;
//    vector<CDiskTxPos> vSpent;
//
//    CTxIndex()
//    {
//        SetNull();
//    }
//
//    CTxIndex(const CDiskTxPos& posIn, unsigned int nOutputs)
//    {
//        pos = posIn;
//        vSpent.resize(nOutputs);
//    }
//
//    IMPLEMENT_SERIALIZE
//    (
//        if (!(nType & SER_GETHASH))
//            READWRITE(nVersion);
//        READWRITE(pos);
//        READWRITE(vSpent);
//    )
//
//    void SetNull()
//    {
//        pos.SetNull();
//        vSpent.clear();
//    }
//
//    bool IsNull()
//    {
//        return pos.IsNull();
//    }
//
//    friend bool operator==(const CTxIndex& a, const CTxIndex& b)
//    {
//        if (a.pos != b.pos || a.vSpent.size() != b.vSpent.size())
//            return false;
//        for (int i = 0; i < a.vSpent.size(); i++)
//            if (a.vSpent[i] != b.vSpent[i])
//                return false;
//        return true;
//    }
//
//    friend bool operator!=(const CTxIndex& a, const CTxIndex& b)
//    {
//        return !(a == b);
//    }
//};
//
//
//
//
//
////
//// Nodes collect new transactions into a block, hash them into a hash tree,
//// and scan through nonce values to make the block's hash satisfy proof-of-work
//// requirements.  When they solve the proof-of-work, they broadcast the block
//// to everyone and the block is added to the block chain.  The first transaction
//// in the block is a special one that creates a new coin owned by the creator
//// of the block.
////
//// Blocks are appended to blk0001.dat files on disk.  Their location on disk
//// is indexed by CBlockIndex objects in memory.
////
//class CBlock
//{
//public:
//    // header
//    int nVersion;
//    uint256 hashPrevBlock;
//    uint256 hashMerkleRoot;
//    unsigned int nTime;
//    unsigned int nBits;
//    unsigned int nNonce;
//
//    // network and disk
//    vector<CTransaction> vtx;
//
//    // memory only
//    mutable vector<uint256> vMerkleTree;
//
//
//    CBlock()
//    {
//        SetNull();
//    }
//
//    IMPLEMENT_SERIALIZE
//    (
//        READWRITE(this->nVersion);
//        nVersion = this->nVersion;
//        READWRITE(hashPrevBlock);
//        READWRITE(hashMerkleRoot);
//        READWRITE(nTime);
//        READWRITE(nBits);
//        READWRITE(nNonce);
//
//        // ConnectBlock depends on vtx being last so it can calculate offset
//        if (!(nType & (SER_GETHASH|SER_BLOCKHEADERONLY)))
//            READWRITE(vtx);
//        else if (fRead)
//            const_cast<CBlock*>(this)->vtx.clear();
//    )
//
//    void SetNull()
//    {
//        nVersion = 1;
//        hashPrevBlock = 0;
//        hashMerkleRoot = 0;
//        nTime = 0;
//        nBits = 0;
//        nNonce = 0;
//        vtx.clear();
//        vMerkleTree.clear();
//    }
//
//    bool IsNull() const
//    {
//        return (nBits == 0);
//    }
//
//    uint256 GetHash() const
//    {
//        return Hash(BEGIN(nVersion), END(nNonce));
//    }
//
//
//    uint256 BuildMerkleTree() const
//    {
//        vMerkleTree.clear();
//        foreach(const CTransaction& tx, vtx)
//            vMerkleTree.push_back(tx.GetHash());
//        int j = 0;
//        for (int nSize = vtx.size(); nSize > 1; nSize = (nSize + 1) / 2)
//        {
//            for (int i = 0; i < nSize; i += 2)
//            {
//                int i2 = min(i+1, nSize-1);
//                vMerkleTree.push_back(Hash(BEGIN(vMerkleTree[j+i]),  END(vMerkleTree[j+i]),
//                                           BEGIN(vMerkleTree[j+i2]), END(vMerkleTree[j+i2])));
//            }
//            j += nSize;
//        }
//        return (vMerkleTree.empty() ? 0 : vMerkleTree.back());
//    }
//
//    vector<uint256> GetMerkleBranch(int nIndex) const
//    {
//        if (vMerkleTree.empty())
//            BuildMerkleTree();
//        vector<uint256> vMerkleBranch;
//        int j = 0;
//        for (int nSize = vtx.size(); nSize > 1; nSize = (nSize + 1) / 2)
//        {
//            int i = min(nIndex^1, nSize-1);
//            vMerkleBranch.push_back(vMerkleTree[j+i]);
//            nIndex >>= 1;
//            j += nSize;
//        }
//        return vMerkleBranch;
//    }
//
//    static uint256 CheckMerkleBranch(uint256 hash, const vector<uint256>& vMerkleBranch, int nIndex)
//    {
//        if (nIndex == -1)
//            return 0;
//        foreach(const uint256& otherside, vMerkleBranch)
//        {
//            if (nIndex & 1)
//                hash = Hash(BEGIN(otherside), END(otherside), BEGIN(hash), END(hash));
//            else
//                hash = Hash(BEGIN(hash), END(hash), BEGIN(otherside), END(otherside));
//            nIndex >>= 1;
//        }
//        return hash;
//    }
//
//
//    bool WriteToDisk(bool fWriteTransactions, unsigned int& nFileRet, unsigned int& nBlockPosRet)
//    {
//        // Open history file to append
//        CAutoFile fileout = AppendBlockFile(nFileRet);
//        if (!fileout)
//            return error("CBlock::WriteToDisk() : AppendBlockFile failed");
//        if (!fWriteTransactions)
//            fileout.nType |= SER_BLOCKHEADERONLY;
//
//        // Write index header
//        unsigned int nSize = fileout.GetSerializeSize(*this);
//        fileout << FLATDATA(pchMessageStart) << nSize;
//
//        // Write block
//        nBlockPosRet = ftell(fileout);
//        if (nBlockPosRet == -1)
//            return error("CBlock::WriteToDisk() : ftell failed");
//        fileout << *this;
//
//        return true;
//    }
//
//    bool ReadFromDisk(unsigned int nFile, unsigned int nBlockPos, bool fReadTransactions)
//    {
//        SetNull();
//
//        // Open history file to read
//        CAutoFile filein = OpenBlockFile(nFile, nBlockPos, "rb");
//        if (!filein)
//            return error("CBlock::ReadFromDisk() : OpenBlockFile failed");
//        if (!fReadTransactions)
//            filein.nType |= SER_BLOCKHEADERONLY;
//
//        // Read block
//        filein >> *this;
//
//        // Check the header
//        if (CBigNum().SetCompact(nBits) > bnProofOfWorkLimit)
//            return error("CBlock::ReadFromDisk() : nBits errors in block header");
//        if (GetHash() > CBigNum().SetCompact(nBits).getuint256())
//            return error("CBlock::ReadFromDisk() : GetHash() errors in block header");
//
//        return true;
//    }
//
//
//
//    void print() const
//    {
//        printf("CBlock(hash=%s, ver=%d, hashPrevBlock=%s, hashMerkleRoot=%s, nTime=%u, nBits=%08x, nNonce=%u, vtx=%d)\n",
//            GetHash().ToString().substr(0,14).c_str(),
//            nVersion,
//            hashPrevBlock.ToString().substr(0,14).c_str(),
//            hashMerkleRoot.ToString().substr(0,6).c_str(),
//            nTime, nBits, nNonce,
//            vtx.size());
//        for (int i = 0; i < vtx.size(); i++)
//        {
//            printf("  ");
//            vtx[i].print();
//        }
//        printf("  vMerkleTree: ");
//        for (int i = 0; i < vMerkleTree.size(); i++)
//            printf("%s ", vMerkleTree[i].ToString().substr(0,6).c_str());
//        printf("\n");
//    }
//
//
//    int64 GetBlockValue(int64 nFees) const;
//    bool DisconnectBlock(CTxDB& txdb, CBlockIndex* pindex);
//    bool ConnectBlock(CTxDB& txdb, CBlockIndex* pindex);
//    bool ReadFromDisk(const CBlockIndex* blockindex, bool fReadTransactions);
//    bool AddToBlockIndex(unsigned int nFile, unsigned int nBlockPos);
//    bool CheckBlock() const;
//    bool AcceptBlock();
//};
//
//
//
//
//
//
////
//// The block chain is a tree shaped structure starting with the
//// genesis block at the root, with each block potentially having multiple
//// candidates to be the next block.  pprev and pnext link a path through the
//// main/longest chain.  A blockindex may have multiple pprev pointing back
//// to it, but pnext will only point forward to the longest branch, or will
//// be null if the block is not part of the longest chain.
////
//class CBlockIndex
//{
//public:
//    const uint256* phashBlock;
//    CBlockIndex* pprev;
//    CBlockIndex* pnext;
//    unsigned int nFile;
//    unsigned int nBlockPos;
//    int nHeight;
//
//    // block header
//    int nVersion;
//    uint256 hashMerkleRoot;
//    unsigned int nTime;
//    unsigned int nBits;
//    unsigned int nNonce;
//
//
//    CBlockIndex()
//    {
//        phashBlock = NULL;
//        pprev = NULL;
//        pnext = NULL;
//        nFile = 0;
//        nBlockPos = 0;
//        nHeight = 0;
//
//        nVersion       = 0;
//        hashMerkleRoot = 0;
//        nTime          = 0;
//        nBits          = 0;
//        nNonce         = 0;
//    }
//
//    CBlockIndex(unsigned int nFileIn, unsigned int nBlockPosIn, CBlock& block)
//    {
//        phashBlock = NULL;
//        pprev = NULL;
//        pnext = NULL;
//        nFile = nFileIn;
//        nBlockPos = nBlockPosIn;
//        nHeight = 0;
//
//        nVersion       = block.nVersion;
//        hashMerkleRoot = block.hashMerkleRoot;
//        nTime          = block.nTime;
//        nBits          = block.nBits;
//        nNonce         = block.nNonce;
//    }
//
//    uint256 GetBlockHash() const
//    {
//        return *phashBlock;
//    }
//
//    bool IsInMainChain() const
//    {
//        return (pnext || this == pindexBest);
//    }
//
//    bool EraseBlockFromDisk()
//    {
//        // Open history file
//        CAutoFile fileout = OpenBlockFile(nFile, nBlockPos, "rb+");
//        if (!fileout)
//            return false;
//
//        // Overwrite with empty null block
//        CBlock block;
//        block.SetNull();
//        fileout << block;
//
//        return true;
//    }
//
//    enum { nMedianTimeSpan=11 };
//
//    int64 GetMedianTimePast() const
//    {
//        unsigned int pmedian[nMedianTimeSpan];
//        unsigned int* pbegin = &pmedian[nMedianTimeSpan];
//        unsigned int* pend = &pmedian[nMedianTimeSpan];
//
//        const CBlockIndex* pindex = this;
//        for (int i = 0; i < nMedianTimeSpan && pindex; i++, pindex = pindex->pprev)
//            *(--pbegin) = pindex->nTime;
//
//        sort(pbegin, pend);
//        return pbegin[(pend - pbegin)/2];
//    }
//
//    int64 GetMedianTime() const
//    {
//        const CBlockIndex* pindex = this;
//        for (int i = 0; i < nMedianTimeSpan/2; i++)
//        {
//            if (!pindex->pnext)
//                return nTime;
//            pindex = pindex->pnext;
//        }
//        return pindex->GetMedianTimePast();
//    }
//
//
//
//    string ToString() const
//    {
//        return strprintf("CBlockIndex(nprev=%08x, pnext=%08x, nFile=%d, nBlockPos=%-6d nHeight=%d, merkle=%s, hashBlock=%s)",
//            pprev, pnext, nFile, nBlockPos, nHeight,
//            hashMerkleRoot.ToString().substr(0,6).c_str(),
//            GetBlockHash().ToString().substr(0,14).c_str());
//    }
//
//    void print() const
//    {
//        printf("%s\n", ToString().c_str());
//    }
//};
//
//
//
////
//// Used to marshal pointers into hashes for db storage.
////
//class CDiskBlockIndex : public CBlockIndex
//{
//public:
//    uint256 hashPrev;
//    uint256 hashNext;
//
//    CDiskBlockIndex()
//    {
//        hashPrev = 0;
//        hashNext = 0;
//    }
//
//    explicit CDiskBlockIndex(CBlockIndex* pindex) : CBlockIndex(*pindex)
//    {
//        hashPrev = (pprev ? pprev->GetBlockHash() : 0);
//        hashNext = (pnext ? pnext->GetBlockHash() : 0);
//    }
//
//    IMPLEMENT_SERIALIZE
//    (
//        if (!(nType & SER_GETHASH))
//            READWRITE(nVersion);
//
//        READWRITE(hashNext);
//        READWRITE(nFile);
//        READWRITE(nBlockPos);
//        READWRITE(nHeight);
//
//        // block header
//        READWRITE(this->nVersion);
//        READWRITE(hashPrev);
//        READWRITE(hashMerkleRoot);
//        READWRITE(nTime);
//        READWRITE(nBits);
//        READWRITE(nNonce);
//    )
//
//    uint256 GetBlockHash() const
//    {
//        CBlock block;
//        block.nVersion        = nVersion;
//        block.hashPrevBlock   = hashPrev;
//        block.hashMerkleRoot  = hashMerkleRoot;
//        block.nTime           = nTime;
//        block.nBits           = nBits;
//        block.nNonce          = nNonce;
//        return block.GetHash();
//    }
//
//
//    string ToString() const
//    {
//        string str = "CDiskBlockIndex(";
//        str += CBlockIndex::ToString();
//        str += strprintf("\n                hashBlock=%s, hashPrev=%s, hashNext=%s)",
//            GetBlockHash().ToString().c_str(),
//            hashPrev.ToString().substr(0,14).c_str(),
//            hashNext.ToString().substr(0,14).c_str());
//        return str;
//    }
//
//    void print() const
//    {
//        printf("%s\n", ToString().c_str());
//    }
//};
//
//
//
//
//
//
//
//
////
//// Describes a place in the block chain to another node such that if the
//// other node doesn't have the same branch, it can find a recent common trunk.
//// The further back it is, the further before the fork it may be.
////
//class CBlockLocator
//{
//protected:
//    vector<uint256> vHave;
//public:
//
//    CBlockLocator()
//    {
//    }
//
//    explicit CBlockLocator(const CBlockIndex* pindex)
//    {
//        Set(pindex);
//    }
//
//    explicit CBlockLocator(uint256 hashBlock)
//    {
//        map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.find(hashBlock);
//        if (mi != mapBlockIndex.end())
//            Set((*mi).second);
//    }
//
//    IMPLEMENT_SERIALIZE
//    (
//        if (!(nType & SER_GETHASH))
//            READWRITE(nVersion);
//        READWRITE(vHave);
//    )
//
//    void Set(const CBlockIndex* pindex)
//    {
//        vHave.clear();
//        int nStep = 1;
//        while (pindex)
//        {
//            vHave.push_back(pindex->GetBlockHash());
//
//            // Exponentially larger steps back
//            for (int i = 0; pindex && i < nStep; i++)
//                pindex = pindex->pprev;
//            if (vHave.size() > 10)
//                nStep *= 2;
//        }
//        vHave.push_back(hashGenesisBlock);
//    }
//
//    CBlockIndex* GetBlockIndex()
//    {
//        // Find the first block the caller has in the main chain
//        foreach(const uint256& hash, vHave)
//        {
//            map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.find(hash);
//            if (mi != mapBlockIndex.end())
//            {
//                CBlockIndex* pindex = (*mi).second;
//                if (pindex->IsInMainChain())
//                    return pindex;
//            }
//        }
//        return pindexGenesisBlock;
//    }
//
//    uint256 GetBlockHash()
//    {
//        // Find the first block the caller has in the main chain
//        foreach(const uint256& hash, vHave)
//        {
//            map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.find(hash);
//            if (mi != mapBlockIndex.end())
//            {
//                CBlockIndex* pindex = (*mi).second;
//                if (pindex->IsInMainChain())
//                    return hash;
//            }
//        }
//        return hashGenesisBlock;
//    }
//
//    int GetHeight()
//    {
//        CBlockIndex* pindex = GetBlockIndex();
//        if (!pindex)
//            return 0;
//        return pindex->nHeight;
//    }
//};
//
//
//
//
//bool BitcoinMiner()
//{
//    printf("BitcoinMiner started\n");
//    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST);
//
//    CKey key;
//    key.MakeNewKey();
//    CBigNum bnExtraNonce = 0;
//    while (fGenerateBitcoins)
//    {
//        Sleep(50);
//        CheckForShutdown(3);
//        while (vNodes.empty())
//        {
//            Sleep(1000);
//            CheckForShutdown(3);
//        }
//
//        unsigned int nTransactionsUpdatedLast = nTransactionsUpdated;
//        CBlockIndex* pindexPrev = pindexBest;
//        unsigned int nBits = GetNextWorkRequired(pindexPrev);
//
//
//        //
//        // Create coinbase tx
//        //
//        CTransaction txNew;
//        txNew.vin.resize(1);
//        txNew.vin[0].prevout.SetNull();
//        txNew.vin[0].scriptSig << nBits << ++bnExtraNonce;
//        txNew.vout.resize(1);
//        txNew.vout[0].scriptPubKey << key.GetPubKey() << OP_CHECKSIG;
//
//
//        //
//        // Create new block
//        //
//        auto_ptr<CBlock> pblock(new CBlock());
//        if (!pblock.get())
//            return false;
//
//        // Add our coinbase tx as first transaction
//        pblock->vtx.push_back(txNew);
//
//        // Collect the latest transactions into the block
//        int64 nFees = 0;
//        CRITICAL_BLOCK(cs_main)
//        CRITICAL_BLOCK(cs_mapTransactions)
//        {
//            CTxDB txdb("r");
//            map<uint256, CTxIndex> mapTestPool;
//            vector<char> vfAlreadyAdded(mapTransactions.size());
//            bool fFoundSomething = true;
//            unsigned int nBlockSize = 0;
//            while (fFoundSomething && nBlockSize < MAX_SIZE/2)
//            {
//                fFoundSomething = false;
//                unsigned int n = 0;
//                for (map<uint256, CTransaction>::iterator mi = mapTransactions.begin(); mi != mapTransactions.end(); ++mi, ++n)
//                {
//                    if (vfAlreadyAdded[n])
//                        continue;
//                    CTransaction& tx = (*mi).second;
//                    if (tx.IsCoinBase() || !tx.IsFinal())
//                        continue;
//
//                    // Transaction fee requirements, mainly only needed for flood control
//                    // Under 10K (about 80 inputs) is free for first 100 transactions
//                    // Base rate is 0.01 per KB
//                    int64 nMinFee = tx.GetMinFee(pblock->vtx.size() < 100);
//
//                    map<uint256, CTxIndex> mapTestPoolTmp(mapTestPool);
//                    if (!tx.ConnectInputs(txdb, mapTestPoolTmp, CDiskTxPos(1,1,1), 0, nFees, false, true, nMinFee))
//                        continue;
//                    swap(mapTestPool, mapTestPoolTmp);
//
//                    pblock->vtx.push_back(tx);
//                    nBlockSize += ::GetSerializeSize(tx, SER_NETWORK);
//                    vfAlreadyAdded[n] = true;
//                    fFoundSomething = true;
//                }
//            }
//        }
//        pblock->nBits = nBits;
//        pblock->vtx[0].vout[0].nValue = pblock->GetBlockValue(nFees);
//        printf("\n\nRunning BitcoinMiner with %d transactions in block\n", pblock->vtx.size());
//
//
//        //
//        // Prebuild hash buffer
//        //
//        struct unnamed1
//        {
//            struct unnamed2
//            {
//                int nVersion;
//                uint256 hashPrevBlock;
//                uint256 hashMerkleRoot;
//                unsigned int nTime;
//                unsigned int nBits;
//                unsigned int nNonce;
//            }
//            block;
//            unsigned char pchPadding0[64];
//            uint256 hash1;
//            unsigned char pchPadding1[64];
//        }
//        tmp;
//
//        tmp.block.nVersion       = pblock->nVersion;
//        tmp.block.hashPrevBlock  = pblock->hashPrevBlock  = (pindexPrev ? pindexPrev->GetBlockHash() : 0);
//        tmp.block.hashMerkleRoot = pblock->hashMerkleRoot = pblock->BuildMerkleTree();
//        tmp.block.nTime          = pblock->nTime          = max((pindexPrev ? pindexPrev->GetMedianTimePast()+1 : 0), GetAdjustedTime());
//        tmp.block.nBits          = pblock->nBits          = nBits;
//        tmp.block.nNonce         = pblock->nNonce         = 1;
//
//        unsigned int nBlocks0 = FormatHashBlocks(&tmp.block, sizeof(tmp.block));
//        unsigned int nBlocks1 = FormatHashBlocks(&tmp.hash1, sizeof(tmp.hash1));
//
//
//        //
//        // Search
//        //
//        unsigned int nStart = GetTime();
//        uint256 hashTarget = CBigNum().SetCompact(pblock->nBits).getuint256();
//        uint256 hash;
//        loop
//        {
//            BlockSHA256(&tmp.block, nBlocks0, &tmp.hash1);
//            BlockSHA256(&tmp.hash1, nBlocks1, &hash);
//
//
//            if (hash <= hashTarget)
//            {
//                pblock->nNonce = tmp.block.nNonce;
//                assert(hash == pblock->GetHash());
//
//                    //// debug print
//                    printf("BitcoinMiner:\n");
//                    printf("proof-of-work found  \n  hash: %s  \ntarget: %s\n", hash.GetHex().c_str(), hashTarget.GetHex().c_str());
//                    pblock->print();
//
//                SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
//                CRITICAL_BLOCK(cs_main)
//                {
//                    // Save key
//                    if (!AddKey(key))
//                        return false;
//                    key.MakeNewKey();
//
//                    // Process this block the same as if we had received it from another node
//                    if (!ProcessBlock(NULL, pblock.release()))
//                        printf("ERROR in BitcoinMiner, ProcessBlock, block not accepted\n");
//                }
//                SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST);
//
//                Sleep(500);
//                break;
//            }
//
//            // Update nTime every few seconds
//            if ((++tmp.block.nNonce & 0x3ffff) == 0)
//            {
//                CheckForShutdown(3);
//                if (tmp.block.nNonce == 0)
//                    break;
//                if (pindexPrev != pindexBest)
//                    break;
//                if (nTransactionsUpdated != nTransactionsUpdatedLast && GetTime() - nStart > 60)
//                    break;
//                if (!fGenerateBitcoins)
//                    break;
//                tmp.block.nTime = pblock->nTime = max(pindexPrev->GetMedianTimePast()+1, GetAdjustedTime());
//            }
//        }
//    }
//
//    return true;
//}
//
//
#include "../test/test_btc.cpp"
int test_btc_main(int argc, char ** argv);

int main(int argc, char ** argv)
{
	int rc = test_btc_main(argc, argv);
	CDiskTxPos txpos;
	return rc;
}
