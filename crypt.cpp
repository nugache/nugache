#include "crypt.h"

CFB::CFB(){
	bKeySet = FALSE;
	for(UINT i = 0; i < sizeof(State); i++)
		State[i] = rand_r(0, 0xff);
}

VOID CFB::SetKey(UCHAR Key[], UCHAR InitVect[], UINT KeySize){
	for(UINT i = 0; i < KeySize; i++)
		CFB::State[i] = InitVect[i];
	aes_enc_key(Key, KeySize, cx);
	aes_enc_blk(State, State, cx);
	bKeySet = TRUE;
}

VOID CFB::SetKey(UCHAR Key[], UINT KeySize){
	UCHAR IV[32];
	memset(IV, 0, KeySize);
	SetKey(Key, IV, KeySize);
}

BOOL CFB::KeySet(VOID){
	return(bKeySet);
}

VOID CFB::ResetKey(VOID){
	bKeySet = FALSE;
}

UCHAR CFB::Crypt(const UCHAR Input){
	aes_enc_blk(State, State, cx);
	return State[0] ^ Input;
}

VOID CFB::Crypt(PUCHAR Buffer, UINT Length){
	for(UINT i = 0; i < Length; i++)
		Buffer[i] = Crypt(Buffer[i]);
}

MD5::MD5(){
	MD5Init(&Context);
}

VOID MD5::Update(PUCHAR Buffer, UINT Length){
	MD5Update(&Context, Buffer, Length);
}

VOID MD5::Finalize(UCHAR Hash[]){
	MD5Final(Hash, &Context);
}

UCHAR RSAPublicKeyMasterModulus[RSAPublicKeyMasterLen] =
{
	0xB2, 0x85, 0x67, 0xEC, 0x90, 0x74, 0x15, 0x8B, 0x4A, 0x99, 0x86, 0x26, 0xFF, 0x53, 0xD2, 0x2A, 
	0x6B, 0x12, 0x76, 0x8B, 0x89, 0xC9, 0x17, 0x72, 0xF4, 0x0D, 0x76, 0x90, 0x65, 0x78, 0x17, 0x5A, 
	0xF6, 0x96, 0x6B, 0x7B, 0xCB, 0x44, 0x13, 0x2C, 0xA5, 0xCC, 0x91, 0x7F, 0xFE, 0xF5, 0x0D, 0x39, 
	0x52, 0x73, 0x35, 0x17, 0x0B, 0x84, 0xC7, 0xFF, 0xBB, 0x14, 0xC3, 0xDE, 0x54, 0xFF, 0x65, 0x50, 
	0x52, 0xB5, 0x77, 0x15, 0xA4, 0x94, 0xA0, 0x23, 0x05, 0xF7, 0x6C, 0x67, 0x6F, 0x19, 0x6A, 0x7B, 
	0x54, 0xC8, 0x91, 0x38, 0xBB, 0x8C, 0xF5, 0x2C, 0x27, 0x0D, 0xE4, 0xF5, 0x4F, 0xBF, 0xC6, 0x86, 
	0x6F, 0x2C, 0x0E, 0x57, 0xE5, 0x63, 0x2E, 0x9B, 0x8E, 0x27, 0xDD, 0xA1, 0x9C, 0x3A, 0x1B, 0xB7, 
	0x1B, 0x79, 0x56, 0x2B, 0xE3, 0x2F, 0x35, 0x74, 0xFC, 0xDC, 0x8D, 0xA6, 0x8C, 0xB5, 0xB2, 0xE4, 
	0x8E, 0x73, 0x9D, 0x5A, 0x13, 0xAC, 0xBF, 0xA6, 0x77, 0x4F, 0xA4, 0x09, 0x1A, 0xCB, 0x21, 0x6E, 
	0xB3, 0xAE, 0x33, 0x19, 0xA8, 0xF3, 0xA5, 0x8A, 0xE2, 0xE9, 0xDF, 0xA4, 0xE4, 0x6C, 0xD2, 0x08, 
	0x35, 0xF7, 0x1B, 0x9E, 0x9D, 0x53, 0x6F, 0x28, 0x0D, 0x41, 0x33, 0x36, 0x26, 0xD3, 0x6C, 0x4D, 
	0xE5, 0xD9, 0x4D, 0x4E, 0x21, 0x12, 0x90, 0xA3, 0xF2, 0xDF, 0x00, 0x05, 0xF7, 0xFB, 0xDC, 0xD1, 
	0x41, 0x18, 0x76, 0x39, 0xA4, 0x15, 0x73, 0xDE, 0x82, 0x4B, 0xE8, 0x34, 0xBA, 0xED, 0x5D, 0xBB, 
	0x72, 0x30, 0x22, 0x6B, 0xDB, 0xCB, 0xAE, 0x7E, 0x39, 0x31, 0x11, 0x65, 0x1D, 0xD5, 0x28, 0x34, 
	0x85, 0x93, 0xA7, 0xCC, 0x45, 0x37, 0xE1, 0x1F, 0x9A, 0xD0, 0x6F, 0x95, 0xB1, 0xB7, 0x7F, 0xEE, 
	0x4D, 0x19, 0x2E, 0x8A, 0x4C, 0x27, 0x0A, 0x3F, 0x80, 0x70, 0x83, 0xD4, 0x70, 0x13, 0xE8, 0xF7, 
	0x35, 0xDE, 0x0F, 0x9D, 0x2C, 0xD7, 0x3B, 0xA3, 0xA0, 0x0B, 0x8B, 0x95, 0x6C, 0xEA, 0x35, 0xE6, 
	0xCB, 0x8B, 0x1B, 0x4F, 0x99, 0xA3, 0x69, 0xBA, 0x90, 0xB0, 0xDD, 0x21, 0x46, 0xC7, 0x45, 0xCD, 
	0xEE, 0x09, 0x55, 0x76, 0x80, 0x3E, 0xFC, 0xB4, 0xE2, 0xF1, 0x84, 0x94, 0xAF, 0x2B, 0x5B, 0x74, 
	0x3B, 0x42, 0xC1, 0x52, 0x6F, 0xDA, 0xF3, 0x53, 0xB7, 0xD7, 0x47, 0x0B, 0x81, 0xAF, 0x28, 0xA1, 
	0xDF, 0xE6, 0xFA, 0x68, 0xFE, 0xC8, 0x1D, 0xAE, 0xE3, 0x7F, 0xE4, 0x9D, 0x0C, 0x37, 0xF9, 0xA5, 
	0x7B, 0xFE, 0xF0, 0x65, 0x47, 0x35, 0x12, 0x18, 0x4E, 0x5A, 0x7B, 0x82, 0xB2, 0x5F, 0xEF, 0x2E, 
	0xE1, 0x68, 0xFC, 0x2E, 0x5F, 0xE1, 0x98, 0x12, 0x13, 0x0F, 0xE3, 0xEE, 0x66, 0x15, 0x90, 0xDB, 
	0xCF, 0xEE, 0x78, 0x97, 0xBF, 0x34, 0x6C, 0x38, 0x6E, 0x97, 0xB6, 0xC3, 0x80, 0x15, 0x10, 0x0C, 
	0xB5, 0xCC, 0xF5, 0xDE, 0x0C, 0x9F, 0x90, 0x8D, 0x4A, 0x6F, 0x45, 0x96, 0xB9, 0x86, 0xA8, 0xC6, 
	0xA1, 0x4B, 0xEA, 0x5E, 0xAD, 0x12, 0xCB, 0xD7, 0x73, 0x62, 0xE2, 0x08, 0x8D, 0x64, 0xD5, 0x83, 
	0x19, 0x3F, 0x24, 0x3E, 0x02, 0x1E, 0xD1, 0x31, 0x84, 0x16, 0xCF, 0x12, 0x6B, 0xE8, 0x48, 0x88, 
	0xEF, 0x04, 0x2B, 0x3A, 0xEE, 0x29, 0xE4, 0x6D, 0xE7, 0x98, 0xA3, 0x17, 0x9C, 0xF2, 0x49, 0x79, 
	0x53, 0xFA, 0x86, 0x9D, 0x13, 0x18, 0xFE, 0x47, 0x26, 0xC6, 0xD8, 0x69, 0x2A, 0x56, 0xCD, 0x94, 
	0xDA, 0x61, 0xA6, 0x49, 0xFC, 0xD3, 0x92, 0xD6, 0xE2, 0x3D, 0x74, 0xB3, 0x13, 0x41, 0x7E, 0xD0, 
	0x3D, 0x42, 0xC9, 0xBC, 0xC3, 0x4D, 0x43, 0x91, 0xFB, 0x7C, 0x34, 0x88, 0xA7, 0xDD, 0x8A, 0xE1, 
	0x94, 0xAA, 0x1F, 0x5C, 0x49, 0xC0, 0x9B, 0x78, 0x5E, 0x3D, 0x2E, 0x8C, 0x27, 0xC4, 0x5C, 0xF7, 
};

R_RSA_PUBLIC_KEY RSA::MakePublicKey(UINT Bits, const PBYTE Modulus){
	R_RSA_PUBLIC_KEY PublicKey;
	PublicKey.bits = Bits;
	memset(PublicKey.exponent, 0, sizeof(PublicKey.exponent));
	memset(PublicKey.modulus, 0, sizeof(PublicKey.modulus));
	NN_DIGIT NN = (NN_DIGIT)65537; // fermat4
	NN_Encode(PublicKey.exponent, MAX_RSA_MODULUS_LEN, &NN, 1);
	for(UINT i = 0; i < ((PublicKey.bits + 7) / 8); i++)
		PublicKey.modulus[i + (MAX_RSA_MODULUS_LEN - ((PublicKey.bits + 7) / 8))] = Modulus[i];

	return PublicKey;
}

R_RSA_PUBLIC_KEY RSA::MakePublicKey(const R_RSA_PRIVATE_KEY & PrivateKey){
	return MakePublicKey(PrivateKey.bits, (PBYTE)PrivateKey.modulus + (MAX_RSA_MODULUS_LEN - ((PrivateKey.bits + 7) / 8)));
}

VOID RSA::MakePublicKey(R_RSA_PUBLIC_KEY *PublicKey, const R_RSA_PRIVATE_KEY & PrivateKey){
	R_RSA_PUBLIC_KEY RSAPublicKey = MakePublicKey(PrivateKey);
	PublicKey->bits = RSAPublicKey.bits;
	memcpy(PublicKey->modulus, RSAPublicKey.modulus, sizeof(PublicKey->modulus));
	memcpy(PublicKey->exponent, RSAPublicKey.exponent, sizeof(PublicKey->exponent));
}

VOID RSA::StorePrivateKey(UCHAR PassHash[16], const R_RSA_PRIVATE_KEY & PrivateKey){
	cRegistry Reg(HKEY_CURRENT_USER, REG_ROOT);
	UINT Size = PrivateKeyBufferSize();
	PCHAR Buffer = new CHAR[Size];
	PrivateKeyToBuffer(PrivateKey, Buffer);
	CryptBuffer(PassHash, Buffer);
	Reg.SetBinary(REG_PRIVATEKEY, Buffer, Size);
	delete[] Buffer;
}

VOID RSA::StorePrivateKeyMaster(UCHAR PassHash[16], const R_RSA_PRIVATE_KEY & PrivateKey){
	cRegistry Reg(HKEY_CURRENT_USER, REG_ROOT);
	UINT Size = PrivateKeyBufferSize();
	PCHAR Buffer = new CHAR[Size];
	PrivateKeyToBuffer(PrivateKey, Buffer);
	CryptBuffer(PassHash, Buffer);
	Reg.SetBinary(REG_PRIVATEKEYMASTER, Buffer, Size);
	delete[] Buffer;
}

BOOL RSA::PrivateKeyStored(VOID){
	cRegistry Reg(HKEY_CURRENT_USER, REG_ROOT);
	if(Reg.Exists(REG_PRIVATEKEY))
		if(Reg.GetType(REG_PRIVATEKEY) == REG_BINARY)
			if(Reg.GetSize(REG_PRIVATEKEY) == PrivateKeyBufferSize())
				return TRUE;
	return FALSE;
}

BOOL RSA::PrivateKeyMasterStored(VOID){
	cRegistry Reg(HKEY_CURRENT_USER, REG_ROOT);
	if(Reg.Exists(REG_PRIVATEKEYMASTER))
		if(Reg.GetType(REG_PRIVATEKEYMASTER) == REG_BINARY)
			if(Reg.GetSize(REG_PRIVATEKEYMASTER) == PrivateKeyBufferSize())
				return TRUE;
	return FALSE;
}

VOID RSA::DeletePrivateKey(VOID){
	cRegistry Reg(HKEY_CURRENT_USER, REG_ROOT);
	Reg.DeleteValue(REG_PRIVATEKEY);
}

VOID RSA::RetrievePrivateKey(UCHAR PassHash[16], R_RSA_PRIVATE_KEY *PrivateKey){
	cRegistry Reg(HKEY_CURRENT_USER, REG_ROOT);
	DWORD Size = PrivateKeyBufferSize();
	PCHAR Buffer = new CHAR[Size];
	Reg.GetBinary(REG_PRIVATEKEY, Buffer, &Size);
	CryptBuffer(PassHash, Buffer);
	BufferToPrivateKey(Buffer, PrivateKey);
	delete[] Buffer;
}

VOID RSA::RetrievePrivateKeyMaster(UCHAR PassHash[16], R_RSA_PRIVATE_KEY *PrivateKey){
	cRegistry Reg(HKEY_CURRENT_USER, REG_ROOT);
	DWORD Size = PrivateKeyBufferSize();
	PCHAR Buffer = new CHAR[Size];
	Reg.GetBinary(REG_PRIVATEKEYMASTER, Buffer, &Size);
	CryptBuffer(PassHash, Buffer);
	BufferToPrivateKey(Buffer, PrivateKey);
	delete[] Buffer;
}

VOID RSA::CryptBuffer(UCHAR PassHash[16], PCHAR Buffer){
	CFB CFB;
	UCHAR Key[BLOCK_SIZE];
	for(UINT i = 0; i < sizeof(Key); i++)
		Key[i] = PassHash[i % sizeof(PassHash)];
	CFB.SetKey(Key, sizeof(Key));
	CFB.Crypt((PUCHAR)Buffer, PrivateKeyBufferSize());
}

UINT RSA::PrivateKeyBufferSize(VOID){
	R_RSA_PRIVATE_KEY PrivateKey;
	#define SizeOf(x) (sizeof(PrivateKey.x))
	return SizeOf(bits) + SizeOf(modulus) + SizeOf(publicExponent) + SizeOf(exponent) + SizeOf(prime) + SizeOf(primeExponent) + SizeOf(coefficient);
}

VOID RSA::BufferToPrivateKey(PCHAR Buffer, R_RSA_PRIVATE_KEY *PrivateKey){
	UINT Read = sizeof(PrivateKey->bits);
	#define ReadPart(x) memcpy(PrivateKey->x, Buffer + Read, sizeof(PrivateKey->x)); Read += sizeof(PrivateKey->x)
	memcpy(&PrivateKey->bits, Buffer, sizeof(PrivateKey->bits));
	ReadPart(modulus);
	ReadPart(publicExponent);
	ReadPart(exponent);
	ReadPart(prime);
	ReadPart(primeExponent);
	ReadPart(coefficient);
}

VOID RSA::PrivateKeyToBuffer(const R_RSA_PRIVATE_KEY & PrivateKey, PCHAR Buffer){
	UINT Written = sizeof(PrivateKey.bits);
	#define MemCpy(x) memcpy(Buffer + Written, PrivateKey.x, sizeof(PrivateKey.x)); Written += sizeof(PrivateKey.x)
	memcpy(Buffer, &PrivateKey.bits, sizeof(PrivateKey.bits));
	MemCpy(modulus);
	MemCpy(publicExponent);
	MemCpy(exponent);
	MemCpy(prime);
	MemCpy(primeExponent);
	MemCpy(coefficient);
}

BOOL RSA::ImportPrivateKey(UCHAR PassHash[16], PCHAR FileName, R_RSA_PRIVATE_KEY *PrivateKey){
	File File(FileName);
	UINT Size = PrivateKeyBufferSize();
	PCHAR Buffer = new CHAR[Size];
	for(UINT i = 0; i < Size; i++){
		BYTE Hex[2];
		File.Read(Hex, sizeof(Hex));
		PCHAR Stop;
		Buffer[i] = strtol((PCHAR)Hex, &Stop, 16);
	}
	CryptBuffer(PassHash, Buffer);
	BufferToPrivateKey(Buffer, PrivateKey);
	delete[] Buffer;
	if(PrivateKey->bits > MAX_RSA_MODULUS_BITS || PrivateKey->bits < MIN_RSA_MODULUS_BITS)
		return FALSE;
	if(PrivateKey->publicExponent[MAX_RSA_MODULUS_LEN - 1] != 1 || PrivateKey->publicExponent[MAX_RSA_MODULUS_LEN - 3] != 1)
		return FALSE;
	return TRUE;
}

VOID RSA::ExportPrivateKey(UCHAR PassHash[16], PCHAR FileName, const R_RSA_PRIVATE_KEY & PrivateKey){
	DeleteFile(FileName);
	File File(FileName);

	UINT Size = PrivateKeyBufferSize();
	PCHAR Buffer = new CHAR[Size];
	PrivateKeyToBuffer(PrivateKey, Buffer);
	CryptBuffer(PassHash, Buffer);

	for(UINT i = 0; i < Size; i++)
		File.Writef("%.2X", (BYTE)Buffer[i]);

	File.Close();
	delete[] Buffer;
}

VOID RSA::ExportPublicKey(UCHAR PassHash[16], PCHAR FileName, const R_RSA_PUBLIC_KEY & PublicKey){
	DeleteFile(FileName);
	File File(FileName);

	UINT Size = sizeof(PublicKey.bits) + sizeof(PublicKey.modulus);
	PCHAR Buffer = new CHAR[Size];
	memcpy(Buffer, &PublicKey.bits, sizeof(PublicKey.bits));
	memcpy(Buffer + sizeof(PublicKey.bits), &PublicKey.modulus, sizeof(PublicKey.modulus));

	for(UINT i = 0; i < Size; i++)
		File.Writef("%.2X", (BYTE)Buffer[i]);

	File.Close();
	delete[] Buffer;
}

UCHAR Password::StoredHash[16];

BOOL Password::Exists(VOID){
	cRegistry Reg(HKEY_CURRENT_USER, REG_ROOT);
	if(Reg.Exists(REG_PWORDENC))
		if(Reg.GetType(REG_PWORDENC) == REG_BINARY)
			if(Reg.GetSize(REG_PWORDENC) == BLOCK_SIZE)
				return TRUE;
	return FALSE;
}

BOOL Password::TestHash(UCHAR Hash[16]){
	UCHAR Key[BLOCK_SIZE];
	for(UINT i = 0; i < sizeof(Key); i++)
		Key[i] = Hash[i % sizeof(Hash)];
	UCHAR Key2[BLOCK_SIZE];
	cRegistry Reg(HKEY_CURRENT_USER, REG_ROOT);
	DWORD Size = sizeof(Key2);
	Reg.GetBinary(REG_PWORDENC, (PCHAR)Key2, &Size);
	CFB CFB;
	CFB.SetKey(Key, sizeof(Key));
	CFB.Crypt(Key, sizeof(Key));
	for(UINT i = 0; i < sizeof(Key); i++){
		if(Key[i] != Key2[i])
			return FALSE;
	}
	return TRUE;
}

VOID Password::SetHash(UCHAR Hash[16]){
	UCHAR Key[BLOCK_SIZE];
	for(UINT i = 0; i < sizeof(Key); i++)
		Key[i] = Hash[i % sizeof(Hash)];
	CFB CFB;
	CFB.SetKey(Key, sizeof(Key));
	CFB.Crypt(Key, sizeof(Key));
	cRegistry Reg(HKEY_CURRENT_USER, REG_ROOT);
	Reg.SetBinary(REG_PWORDENC, (PCHAR)Key, sizeof(Key));
}

VOID Password::RememberHash(UCHAR Hash[16]){
	memcpy(StoredHash, Hash, sizeof(StoredHash));
}

VOID Password::RetreiveHash(UCHAR Hash[16]){
	memcpy(Hash, StoredHash, sizeof(StoredHash));
}