#include "omnicore/test/utils_tx.h"

#include "omnicore/createpayload.h"
#include "omnicore/encoding.h"
#include "omnicore/omnicore.h"
#include "omnicore/rules.h"
#include "omnicore/script.h"
#include "omnicore/tx.h"

#include "base58.h"
#include "coins.h"
#include "primitives/transaction.h"
#include "script/script.h"
#include "script/standard.h"
#include "test/test_bitcoin.h"
#include "utilstrencodings.h"

#include <stdint.h>
#include <limits>
#include <vector>

#include <boost/test/unit_test.hpp>

using namespace mastercore;

BOOST_FIXTURE_TEST_SUITE(omnicore_parsing_d_tests, BasicTestingSetup)

/** Creates a dummy transaction with the given inputs and outputs. */
static CTransaction TxClassD(const std::vector<CTxOut>& txInputs, const std::vector<CTxOut>& txOuts)
{
    CMutableTransaction mutableTx;

    // Inputs:
    for (std::vector<CTxOut>::const_iterator it = txInputs.begin(); it != txInputs.end(); ++it)
    {
        const CTxOut& txOut = *it;

        // Create transaction for input:
        CMutableTransaction inputTx;
        unsigned int nOut = 0;
        inputTx.vout.push_back(txOut);
        CTransaction tx(inputTx);

        // Populate transaction cache:
        CCoinsModifier coins = view.ModifyCoins(tx.GetHash());

        if (nOut >= coins->vout.size()) {
            coins->vout.resize(nOut+1);
        }
        coins->vout[nOut].scriptPubKey = txOut.scriptPubKey;
        coins->vout[nOut].nValue = txOut.nValue;

        // Add input:
        CTxIn txIn(tx.GetHash(), nOut);
        mutableTx.vin.push_back(txIn);
    }

    for (std::vector<CTxOut>::const_iterator it = txOuts.begin(); it != txOuts.end(); ++it)
    {
        const CTxOut& txOut = *it;
        mutableTx.vout.push_back(txOut);
    }

    return CTransaction(mutableTx);
}

/** Helper to create a CTxOut object. */
static CTxOut createTxOut(int64_t amount, const std::string& dest)
{
    return CTxOut(amount, GetScriptForDestination(CBitcoinAddress(dest).Get()));
}

BOOST_AUTO_TEST_CASE(reference_identification)
{
    {
        int nBlock = ConsensusParams().NULLDATA_BLOCK;

        std::vector<CTxOut> txInputs;
        txInputs.push_back(createTxOut(5000000, "LVGU3Y1vaCNWum3gEMjDQX5GD2TVZm3QLp"));

        std::vector<CTxOut> txOutputs;
        txOutputs.push_back(OpReturn_SimpleSend());

        CTransaction dummyTx = TxClassD(txInputs, txOutputs);

        CMPTransaction metaTx;
        BOOST_CHECK(ParseTransaction(dummyTx, nBlock, 1, metaTx) == 0);
        BOOST_CHECK(metaTx.getReceiver().empty());
        BOOST_CHECK_EQUAL(metaTx.getFeePaid(), 5000000);
        BOOST_CHECK_EQUAL(metaTx.getSender(), "LVGU3Y1vaCNWum3gEMjDQX5GD2TVZm3QLp");
        BOOST_CHECK_EQUAL(metaTx.getPayload(), "000001904e");
    }
    {
        int nBlock = ConsensusParams().NULLDATA_BLOCK + 1000;

        std::vector<CTxOut> txInputs;
        txInputs.push_back(createTxOut(6000, "3NfRfUekDSzgSyohRro9jXD1AqDALN321P"));

        std::vector<CTxOut> txOutputs;
        txOutputs.push_back(OpReturn_SimpleSend());
        txOutputs.push_back(createTxOut(6000, "3QHw8qKf1vQkMnSVXarq7N4PYzz1G3mAK4"));

        CTransaction dummyTx = TxClassD(txInputs, txOutputs);

        CMPTransaction metaTx;
        BOOST_CHECK(ParseTransaction(dummyTx, nBlock, 1, metaTx) == 0);
        BOOST_CHECK_EQUAL(metaTx.getFeePaid(), 0);
        BOOST_CHECK_EQUAL(metaTx.getSender(), "3NfRfUekDSzgSyohRro9jXD1AqDALN321P");
        BOOST_CHECK_EQUAL(metaTx.getReceiver(), "3QHw8qKf1vQkMnSVXarq7N4PYzz1G3mAK4");
        BOOST_CHECK_EQUAL(metaTx.getPayload(), "000001904e");
    }
    {
        int nBlock = std::numeric_limits<int>::max();

        std::vector<CTxOut> txInputs;
        txInputs.push_back(createTxOut(80000, "LSN17u4hbViPj4DAevCDtdCXdFv6tGMvPR"));

        std::vector<CTxOut> txOutputs;
        txOutputs.push_back(OpReturn_SimpleSend());
        txOutputs.push_back(createTxOut(6000, "LSN17u4hbViPj4DAevCDtdCXdFv6tGMvPR"));

        CTransaction dummyTx = TxClassD(txInputs, txOutputs);

        CMPTransaction metaTx;
        BOOST_CHECK(ParseTransaction(dummyTx, nBlock, 1, metaTx) == 0);
        BOOST_CHECK_EQUAL(metaTx.getFeePaid(), 74000);
        BOOST_CHECK_EQUAL(metaTx.getSender(), "LSN17u4hbViPj4DAevCDtdCXdFv6tGMvPR");
        BOOST_CHECK_EQUAL(metaTx.getReceiver(), "LSN17u4hbViPj4DAevCDtdCXdFv6tGMvPR");
        BOOST_CHECK_EQUAL(metaTx.getPayload(), "000001904e");
    }
    {
        int nBlock = std::numeric_limits<int>::max();

        std::vector<CTxOut> txInputs;
        txInputs.push_back(createTxOut(80000, "LdyJrW2wPYbn4xMnN2iAp7SjdA4Ujn1MVb"));

        std::vector<CTxOut> txOutputs;
        txOutputs.push_back(OpReturn_SimpleSend());
        txOutputs.push_back(createTxOut(6000, "3NjfEthPHg6GEH9j7o4j4BGGZafBX2yw8j"));
        txOutputs.push_back(PayToPubKey_Unrelated());
        txOutputs.push_back(NonStandardOutput());
        txOutputs.push_back(createTxOut(6000, "37pwWHk1oFaWxVsnYKfGs7Lyt5yJEVomTH"));
        txOutputs.push_back(PayToBareMultisig_1of3());
        txOutputs.push_back(createTxOut(6000, "LdyJrW2wPYbn4xMnN2iAp7SjdA4Ujn1MVb"));

        CTransaction dummyTx = TxClassD(txInputs, txOutputs);

        CMPTransaction metaTx;
        BOOST_CHECK(ParseTransaction(dummyTx, nBlock, 1, metaTx) == 0);
        BOOST_CHECK_EQUAL(metaTx.getSender(), "LdyJrW2wPYbn4xMnN2iAp7SjdA4Ujn1MVb");
        BOOST_CHECK_EQUAL(metaTx.getReceiver(), "37pwWHk1oFaWxVsnYKfGs7Lyt5yJEVomTH");
    }
    {
        int nBlock = std::numeric_limits<int>::max();

        std::vector<CTxOut> txInputs;
        txInputs.push_back(createTxOut(55550, "35iqJySouevicrYzMhjKSsqokSGwGovGov"));

        std::vector<CTxOut> txOutputs;
        txOutputs.push_back(createTxOut(6000, "3NjfEthPHg6GEH9j7o4j4BGGZafBX2yw8j"));
        txOutputs.push_back(PayToPubKey_Unrelated());
        txOutputs.push_back(NonStandardOutput());
        txOutputs.push_back(createTxOut(6000, "35iqJySouevicrYzMhjKSsqokSGwGovGov"));
        txOutputs.push_back(createTxOut(6000, "35iqJySouevicrYzMhjKSsqokSGwGovGov"));
        txOutputs.push_back(OpReturn_SimpleSend());

        CTransaction dummyTx = TxClassD(txInputs, txOutputs);

        CMPTransaction metaTx;
        BOOST_CHECK(ParseTransaction(dummyTx, nBlock, 1, metaTx) == 0);
        BOOST_CHECK_EQUAL(metaTx.getSender(), "35iqJySouevicrYzMhjKSsqokSGwGovGov");
        BOOST_CHECK_EQUAL(metaTx.getReceiver(), "35iqJySouevicrYzMhjKSsqokSGwGovGov");
    }
}

BOOST_AUTO_TEST_CASE(empty_op_return)
{
    {
        int nBlock = std::numeric_limits<int>::max();

        std::vector<CTxOut> txInputs;
        txInputs.push_back(createTxOut(900000, "35iqJySouevicrYzMhjKSsqokSGwGovGov"));

        std::vector<CTxOut> txOutputs;
        txOutputs.push_back(OpReturn_PlainMarker());
        txOutputs.push_back(PayToPubKeyHash_Unrelated());

        CTransaction dummyTx = TxClassD(txInputs, txOutputs);

        CMPTransaction metaTx;
        BOOST_CHECK(ParseTransaction(dummyTx, nBlock, 1, metaTx) == 0);
        BOOST_CHECK(metaTx.getPayload().empty());
        BOOST_CHECK_EQUAL(metaTx.getSender(), "35iqJySouevicrYzMhjKSsqokSGwGovGov");
        // via PayToPubKeyHash_Unrelated:
        BOOST_CHECK_EQUAL(metaTx.getReceiver(), "LgHpnHW3j9qe6if3wTJ1JivB6dQ2iKTDTb");
    }
}


BOOST_AUTO_TEST_CASE(trimmed_op_return)
{
    {
        int nBlock = std::numeric_limits<int>::max();

        std::vector<CTxOut> txInputs;
        txInputs.push_back(createTxOut(100000, "3LzuqJs1deHYeFyJz5JXqrZXpuMk3GBEX2"));

        std::vector<CTxOut> txOutputs;

        std::vector<unsigned char> vchFiller(65535, 0x07);
        std::vector<unsigned char> vchPayload = GetOmMarker();
        vchPayload.insert(vchPayload.end(), vchFiller.begin(), vchFiller.end());

        // These will be trimmed:
        vchPayload.push_back(0x44);
        vchPayload.push_back(0x44);
        vchPayload.push_back(0x44);

        CScript scriptPubKey;
        scriptPubKey << OP_RETURN;
        scriptPubKey << vchPayload;
        CTxOut txOut = CTxOut(0, scriptPubKey);
        txOutputs.push_back(txOut);

        CTransaction dummyTx = TxClassD(txInputs, txOutputs);

        CMPTransaction metaTx;
        BOOST_CHECK(ParseTransaction(dummyTx, nBlock, 1, metaTx) == 0);
        BOOST_CHECK_EQUAL(metaTx.getSender(), "3LzuqJs1deHYeFyJz5JXqrZXpuMk3GBEX2");
        BOOST_CHECK_EQUAL(metaTx.getPayload(), HexStr(vchFiller.begin(), vchFiller.end()));
        BOOST_CHECK_EQUAL(metaTx.getPayload().size() / 2, 65535);
    }
}

BOOST_AUTO_TEST_CASE(multiple_op_return_short)
{
    {
        int nBlock = ConsensusParams().NULLDATA_BLOCK;

        std::vector<CTxOut> txInputs;
        txInputs.push_back(createTxOut(100000, "LLGhCyV4JLwPsE96fBRk7PGECmPeUyJytX"));

        std::vector<CTxOut> txOutputs;
        {
            CScript scriptPubKey;
            scriptPubKey << OP_RETURN << ParseHex("6f6c0000111122223333");
            CTxOut txOut = CTxOut(0, scriptPubKey);
            txOutputs.push_back(txOut);
        }
        {
            CScript scriptPubKey;
            scriptPubKey << OP_RETURN;
            CTxOut txOut = CTxOut(0, scriptPubKey);
            txOutputs.push_back(txOut);
        }
        {
            CScript scriptPubKey;
            scriptPubKey << OP_RETURN << ParseHex("6f6c0001000200030004");
            CTxOut txOut = CTxOut(0, scriptPubKey);
            txOutputs.push_back(txOut);
        }
        {
            CScript scriptPubKey;
            scriptPubKey << OP_RETURN << ParseHex("6f6c");
            CTxOut txOut = CTxOut(0, scriptPubKey);
            txOutputs.push_back(txOut);
        }
        CTransaction dummyTx = TxClassD(txInputs, txOutputs);

        CMPTransaction metaTx;
        BOOST_CHECK(ParseTransaction(dummyTx, nBlock, 1, metaTx) == 0);
        BOOST_CHECK_EQUAL(metaTx.getSender(), "LLGhCyV4JLwPsE96fBRk7PGECmPeUyJytX");
        BOOST_CHECK_EQUAL(metaTx.getPayload(), "00001111222233330001000200030004");
    }
}

BOOST_AUTO_TEST_CASE(multiple_op_return)
{
    {
        int nBlock = ConsensusParams().NULLDATA_BLOCK;

        std::vector<CTxOut> txInputs;
        txInputs.push_back(createTxOut(100000, "3LzuqJs1deHYeFyJz5JXqrZXpuMk3GBEX2"));

        std::vector<CTxOut> txOutputs;
        {
            CScript scriptPubKey;
            scriptPubKey << OP_RETURN << ParseHex("6f6c1222222222222222222222222223");
            CTxOut txOut = CTxOut(0, scriptPubKey);
            txOutputs.push_back(txOut);
        }
        {
            CScript scriptPubKey;
            scriptPubKey << OP_RETURN << ParseHex("6f6c4555555555555555555555555556");
            CTxOut txOut = CTxOut(5, scriptPubKey);
            txOutputs.push_back(txOut);
        }
        {
            CScript scriptPubKey;
            scriptPubKey << OP_RETURN << ParseHex("6f6c788888888889");
            CTxOut txOut = CTxOut(0, scriptPubKey);
            txOutputs.push_back(txOut);
        }
        {
            CScript scriptPubKey; // has no marker and will be ignored!
            scriptPubKey << OP_RETURN << ParseHex("4d756c686f6c6c616e64204472697665");
            CTxOut txOut = CTxOut(0, scriptPubKey);
            txOutputs.push_back(txOut);
        }
        {
            CScript scriptPubKey;
            scriptPubKey << OP_RETURN << ParseHex("6f6cffff111111111111111111111111"
                    "11111111111111111111111111111111111111111111111111111111111111"
                    "11111111111111111111111111111111111111111111111111111111111111"
                    "11111111111111111111111111111111111111111117");
            CTxOut txOut = CTxOut(0, scriptPubKey);
            txOutputs.push_back(txOut);
        }

        CTransaction dummyTx = TxClassD(txInputs, txOutputs);

        CMPTransaction metaTx;
        BOOST_CHECK(ParseTransaction(dummyTx, nBlock, 1, metaTx) == 0);
        BOOST_CHECK_EQUAL(metaTx.getSender(), "3LzuqJs1deHYeFyJz5JXqrZXpuMk3GBEX2");
        BOOST_CHECK_EQUAL(metaTx.getPayload(), "12222222222222222222222222234555555"
                "555555555555555555556788888888889ffff11111111111111111111111111111"
                "111111111111111111111111111111111111111111111111111111111111111111"
                "111111111111111111111111111111111111111111111111111111111111111111"
                "1111111111111111111111111111117");
    }
}

BOOST_AUTO_TEST_CASE(multiple_op_return_pushes)
{
    {
        int nBlock = std::numeric_limits<int>::max();

        std::vector<CTxOut> txInputs;
        txInputs.push_back(createTxOut(100000, "3LzuqJs1deHYeFyJz5JXqrZXpuMk3GBEX2"));
        txInputs.push_back(PayToBareMultisig_3of5());

        std::vector<CTxOut> txOutputs;
        txOutputs.push_back(OpReturn_SimpleSend());
        txOutputs.push_back(PayToScriptHash_Unrelated());
        txOutputs.push_back(OpReturn_MultiSimpleSend());

        CTransaction dummyTx = TxClassD(txInputs, txOutputs);

        CMPTransaction metaTx;
        BOOST_CHECK(ParseTransaction(dummyTx, nBlock, 1, metaTx) == 0);
        BOOST_CHECK_EQUAL(metaTx.getSender(), "3LzuqJs1deHYeFyJz5JXqrZXpuMk3GBEX2");
        BOOST_CHECK_EQUAL(metaTx.getPayload(),
                // OpReturn_SimpleSend (without marker):
                "000001904e"
                // OpReturn_MultiSimpleSend (without marker):
                "000001904e"
                "0062e907b15cbf27d5425399ebf6f0fb50ebb88f18"
                "000003904e"
                "05da59767e81f4b019fe9f5984dbaa4f61bf197967");
    }
    {
        int nBlock = ConsensusParams().NULLDATA_BLOCK;

        std::vector<CTxOut> txInputs;
        txInputs.push_back(createTxOut(100000, "3LzuqJs1deHYeFyJz5JXqrZXpuMk3GBEX2"));

        std::vector<CTxOut> txOutputs;
        {
            CScript scriptPubKey;
            scriptPubKey << OP_RETURN;
            scriptPubKey << ParseHex("6f6c000001904e");
            scriptPubKey << ParseHex("000003904e");
            CTxOut txOut = CTxOut(0, scriptPubKey);
            txOutputs.push_back(txOut);
        }

        CTransaction dummyTx = TxClassD(txInputs, txOutputs);

        CMPTransaction metaTx;
        BOOST_CHECK(ParseTransaction(dummyTx, nBlock, 1, metaTx) == 0);
        BOOST_CHECK_EQUAL(metaTx.getSender(), "3LzuqJs1deHYeFyJz5JXqrZXpuMk3GBEX2");
        BOOST_CHECK_EQUAL(metaTx.getPayload(),
                "000001904e000003904e");
    }
    {
        int nBlock = std::numeric_limits<int>::max();

        std::vector<CTxOut> txInputs;
        txInputs.push_back(createTxOut(100000, "3LzuqJs1deHYeFyJz5JXqrZXpuMk3GBEX2"));

        std::vector<CTxOut> txOutputs;
        {
          CScript scriptPubKey;
          scriptPubKey << OP_RETURN;
          scriptPubKey << ParseHex("6f6c");
          scriptPubKey << ParseHex("000001904e");
          CTxOut txOut = CTxOut(0, scriptPubKey);
          txOutputs.push_back(txOut);
        }

        CTransaction dummyTx = TxClassD(txInputs, txOutputs);

        CMPTransaction metaTx;
        BOOST_CHECK(ParseTransaction(dummyTx, nBlock, 1, metaTx) == 0);
        BOOST_CHECK_EQUAL(metaTx.getPayload(), "000001904e");
    }
    {
        /**
         * The following transaction is invalid, because the first pushed data
         * doesn't contain the class C marker.
         */
        int nBlock = ConsensusParams().NULLDATA_BLOCK;

        std::vector<CTxOut> txInputs;
        txInputs.push_back(createTxOut(100000, "3LzuqJs1deHYeFyJz5JXqrZXpuMk3GBEX2"));

        std::vector<CTxOut> txOutputs;
        {
            CScript scriptPubKey;
            scriptPubKey << OP_RETURN;
            scriptPubKey << ParseHex("6f");
            scriptPubKey << ParseHex("6c");
            scriptPubKey << ParseHex("000001904e");
            CTxOut txOut = CTxOut(0, scriptPubKey);
            txOutputs.push_back(txOut);
        }

        CTransaction dummyTx = TxClassD(txInputs, txOutputs);

        CMPTransaction metaTx;
        BOOST_CHECK(ParseTransaction(dummyTx, nBlock, 1, metaTx) != 0);
    }
}


BOOST_AUTO_TEST_SUITE_END()
