// Purpose: Header file for the C++ ICE encryption class.
//			Taken from public domain code, as written by Matthew Kwan - July 1996
//			http://www.darkside.com.au/ice/

#ifndef _IceKey_H
#define _IceKey_H

/*
The IceKey class is used for encrypting and decrypting 64-bit blocks of data 
with the ICE (Information Concealment Engine) encryption algorithm. 

The constructor creates a new IceKey object that can be used to encrypt and decrypt data. 
The level of encryption determines the size of the key, and hence its speed. 
Level 0 uses the Thin-ICE variant, which is an 8-round cipher taking an 8-byte key. 
This is the fastest option, and is generally considered to be at least as secure as DES, 
although it is not yet certain whether it is as secure as its key size. 

For levels n greater than zero, a 16n-round cipher is used, taking 8n-byte keys. 
Although not as fast as level 0, these are very very secure. 

Before an IceKey can be used to encrypt data, its key schedule must be set with the set() member function. 
The length of the key required is determined by the level, as described above. 

The member functions encrypt() and decrypt() encrypt and decrypt respectively data 
in blocks of eight chracters, using the specified key. 

Two functions keySize() and blockSize() are provided 
which return the key and block size respectively, measured in bytes. 
The key size is determined by the level, while the block size is always 8. 

The destructor zeroes out and frees up all memory associated with the key. 
*/

class IceSubkey;

class IceKey {
    public:
	IceKey (int n);
	~IceKey ();

	void		set (const unsigned char *key);

	void		encrypt (const unsigned char *plaintext,
					unsigned char *ciphertext) const;

	void		decrypt (const unsigned char *ciphertext,
					unsigned char *plaintext) const;

	int		keySize () const;

	int		blockSize () const;

    private:
	void		scheduleBuild (unsigned short *k, int n,
							const int *keyrot);

	int		_size;
	int		_rounds;
	IceSubkey	*_keysched;
};

#endif
