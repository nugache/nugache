#pragma once

#include <windows.h>
#include "config.h"
#include "file.h"
#include "rand.h"
#include "registry.h"
extern "C"{
	#include "rsa/global.h"
	#include "rsa/rsaref.h"
	#include "rsa/rsa.h"
	#include "rsa/nn.h"
	#include "aes/aes.h"
	#include "rsa/md5.h"
}

class CFB
{
public:
	CFB();
	VOID SetKey(UCHAR Key[], UCHAR InitVect[], UINT KeySize);
	VOID SetKey(UCHAR Key[], UINT KeySize);
	BOOL KeySet(VOID);
	VOID ResetKey(VOID);
	UCHAR Crypt(const UCHAR Input);
	VOID Crypt(PUCHAR Buffer, UINT Length);

private:
	BOOL bKeySet;
	UCHAR State[BLOCK_SIZE];
	aes_ctx cx[1];
};

class MD5
{
public:
	MD5();
	VOID Update(PUCHAR Buffer, UINT Length);
	VOID Finalize(UCHAR Hash[16]);

private:
	MD5_CTX Context;
};

const UINT RSAPublicKeyMasterLen = 512;
extern UCHAR RSAPublicKeyMasterModulus[RSAPublicKeyMasterLen];

namespace RSA
{
	R_RSA_PUBLIC_KEY MakePublicKey(UINT Bits, const PBYTE Modulus);
	R_RSA_PUBLIC_KEY MakePublicKey(const R_RSA_PRIVATE_KEY & PrivateKey);
	UINT PrivateKeyBufferSize(VOID);
	VOID PrivateKeyToBuffer(const R_RSA_PRIVATE_KEY & PrivateKey, PCHAR Buffer);
	VOID BufferToPrivateKey(PCHAR Buffer, R_RSA_PRIVATE_KEY *PrivateKey);
	VOID CryptBuffer(UCHAR PassHash[16], PCHAR Buffer);
	VOID MakePublicKey(R_RSA_PUBLIC_KEY *PublicKey, const R_RSA_PRIVATE_KEY & PrivateKey);
	VOID StorePrivateKey(UCHAR PassHash[16], const R_RSA_PRIVATE_KEY & PrivateKey);
	VOID StorePrivateKeyMaster(UCHAR PassHash[16], const R_RSA_PRIVATE_KEY & PrivateKey);
	BOOL PrivateKeyStored(VOID);
	BOOL PrivateKeyMasterStored(VOID);
	VOID RetrievePrivateKey(UCHAR PassHash[16], R_RSA_PRIVATE_KEY *PrivateKey);
	VOID RetrievePrivateKeyMaster(UCHAR PassHash[16], R_RSA_PRIVATE_KEY *PrivateKey);
	VOID DeletePrivateKey(VOID);
	BOOL ImportPrivateKey(UCHAR PassHash[16], PCHAR FileName, R_RSA_PRIVATE_KEY *PrivateKey);
	VOID ExportPrivateKey(UCHAR PassHash[16], PCHAR FileName, const R_RSA_PRIVATE_KEY & PrivateKey);
	VOID ExportPublicKey(UCHAR PassHash[16], PCHAR FileName, const R_RSA_PUBLIC_KEY & PublicKey);
}

namespace Password
{
	BOOL Exists(VOID);
	BOOL TestHash(UCHAR Hash[16]);
	VOID SetHash(UCHAR Hash[16]);
	VOID RememberHash(UCHAR Hash[16]);
	VOID RetreiveHash(UCHAR Hash[16]);

	extern UCHAR StoredHash[16];
}