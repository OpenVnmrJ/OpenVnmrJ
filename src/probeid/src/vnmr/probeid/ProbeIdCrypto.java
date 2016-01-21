/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
package vnmr.probeid;

import java.security.*;

import javax.crypto.*;
import javax.crypto.spec.SecretKeySpec;

import java.io.*;

/**
 * A Probe ID cryptography factory.
 * 
 * Encryption is done in the client on the premise that everything
 * up to the client is untrusted, but the client itself is trusted.
 * @author dirk
 *
 */
public class ProbeIdCrypto implements ProbeId {
	private Key m_key = null;
	
	public OutputStream getBufferedWriter(OutputStream writer) {
		return new CipherOutputStream(writer, getCipher(Cipher.ENCRYPT_MODE));
	}

	public InputStream getBufferedReader(InputStream reader) {
		return new CipherInputStream(reader, getCipher(Cipher.DECRYPT_MODE));
	}

	Cipher getCipher(int mode) {
		Cipher cipher = null;
		try {
			String cipherType = System.getProperty(KEY_PROBEID_CIPHER);
			// TODO: change this to "if there is a key file, then use the
			// algorithm indicated by the key file".
			if (cipherType == null)
				cipher = Cipher.getInstance(DEFAULT_CIPHER);
			else if (cipherType.isEmpty())
				cipher = new NullCipher();
			else 
				cipher = Cipher.getInstance(cipherType);
			cipher.init(mode, m_key);
		} catch (Exception e) {
			ProbeIdIO.error("Cipher error "+e.getMessage());
			e.printStackTrace();
		}
		return cipher;
	}
	
	/**
	 * Construct a cryptographic ProbeId according to the specified
	 * encryption algorithm or one of the fall-back algorithms
	 * (to facilitate experimentation).
	 * @param algorithm
	 */
	ProbeIdCrypto(String algorithm) {
		String cipherType = getAlgorithm(algorithm);
		if (cipherType == null || cipherType.isEmpty()) {
			boolean doGenKey = true;    // assume a key has to be (re)generated
			File keyFile = getKeyFile();
			if (keyFile.exists()) {
				m_key = getKey();
				doGenKey = !m_key.getAlgorithm().equals(cipherType);
			}
			if (doGenKey) {
				m_key = genKey(algorithm);
				writeKey(m_key);
			}
		}
	}

	static File getKeyFile() {
		String keyFileName = System.getProperty(KEY_PROBEID_KEYFILE);
		if (keyFileName == null || keyFileName.isEmpty()) {
			keyFileName = DEFAULT_KEYFILE;
		}
		return new File(keyFileName);
	}

	/**
	 * Determine the key size based on system defaults
	 * @return key length in units of bits
	 */
	static int getKeySize() {
		String keySize = System.getProperty(ProbeId.KEY_PROBEID_KEYLENGTH);
		if (keySize == null)        // use system default
			return 0;
		else if (keySize.isEmpty()) // use ProbeId default
			return ProbeId.ENCRYPTION_STRENGTH;
		else                        // use user-defined
			return Integer.parseInt(keySize);
	}

	static String getAlgorithm(String algorithm) {
		String cipherType = algorithm;
		if (cipherType == null || cipherType.isEmpty())
			cipherType = System.getProperty(ProbeId.KEY_PROBEID_CIPHER);
		if (cipherType == null || cipherType.isEmpty()) 
			cipherType = ProbeId.DEFAULT_CIPHER;
		return cipherType;
	}
	
	/**
	 * Generate an encryption key based user, default, or system default
	 * @param algorithm
	 */
	static void writeKey(Key key) {
		try {
			byte[] wrapped = wrapKey(key);
			FileOutputStream out = new FileOutputStream(getKeyFile());
			DataOutputStream file = new DataOutputStream(out);
			file.writeInt(wrapped.length);
			file.write(wrapped);
			file.close();
		} catch (FileNotFoundException e) {
			ProbeIdIO.error("Wrap file \""+getKeyFile()+"\" not found");
		} catch (IOException e) {
			ProbeIdIO.error("I/O error writing file "+getKeyFile());
			e.printStackTrace();
		}
	}
	
	static Key genKey(String algorithm) {
		String cipherType = getAlgorithm(algorithm);
		SecretKey key = null;
		
		try {
			int keysize = getKeySize();
			KeyGenerator keygen = KeyGenerator.getInstance(cipherType);
			SecureRandom random = new SecureRandom();
			if (keysize > 0)
				keygen.init(keysize, random);
			else 
				keygen.init(random);
			key = keygen.generateKey();
		} catch (NoSuchAlgorithmException e) {
			ProbeIdIO.error("Invalid algorithm "+cipherType);
			e.printStackTrace();
		}
		return key;
	}
	
	static byte [] wrapKey(Key raw) {
		byte [] wrapped = null;
		try {
			Cipher cipher = Cipher.getInstance(ProbeIdWrapper.algorithm);
			Key wrapper = getWrapKey();
			cipher.init(Cipher.WRAP_MODE, wrapper);
			wrapped = cipher.wrap(raw);
		} catch (Exception e) {
			ProbeIdIO.error("Cipher error "+e.getMessage());
			e.printStackTrace();
		}
		return wrapped;
	}
	
	static Key getWrapKey() {
		String algorithm = ProbeIdWrapper.algorithm;
		return new SecretKeySpec(ProbeIdWrapper.key, algorithm);
	}
	
	static Key getKey() {
		Key key = null;
		try {
			FileInputStream in = new FileInputStream(getKeyFile());
			DataInputStream file = new DataInputStream(in);
			int lengthRaw = file.readInt();
			byte [] wrapped = new byte[lengthRaw];
			file.read(wrapped);
			Key wrapper = getWrapKey();
			Cipher cipher = Cipher.getInstance(ProbeIdWrapper.algorithm);
			cipher.init(Cipher.UNWRAP_MODE, wrapper);
			key = cipher.unwrap(wrapped, 
								ProbeIdWrapper.algorithm, 
								Cipher.SECRET_KEY);
			file.close();
		} catch (FileNotFoundException e) {
			ProbeIdIO.error("Could not find "+getKeyFile());
		} catch (IOException e) {
			ProbeIdIO.error("IO Error reading "+getKeyFile());
		} catch (NoSuchAlgorithmException e) {
			ProbeIdIO.error("Invalid algorithm "+ProbeIdWrapper.algorithm);
		} catch (NoSuchPaddingException e) {
			ProbeIdIO.error("Invalid encryption pad ");
		} catch (InvalidKeyException e) {
			ProbeIdIO.error("Invalid key");
		}
		return key;
	}
	
	private static void genWrapper(String bootStrapName, String algorithm) {
		Key wrapper = null;
		try {
			KeyGenerator wrapGen = KeyGenerator.getInstance(algorithm);
			wrapper = wrapGen.generateKey();
		} catch (NoSuchAlgorithmException e) {
			ProbeIdIO.error("Invalid wrapper algorithm "+algorithm);
		}
		
		byte [] encodedWrapper = wrapper.getEncoded();
		String byteString = "";
		for (int i=0; i<encodedWrapper.length; i++) { 
			byteString = byteString + " (byte) " + Integer.toString(encodedWrapper[i]) + ",";
			if (i % 5 == 4)
				byteString += "\n\t"; 
		}
		
		File bootFile = new File(bootStrapName);
		try {
			FileWriter boots = new FileWriter(bootFile);
			String keybooter = 
				"/*\n * This file was automatically generated with " +
				"\"ProbeIdCrypto -boot " + algorithm + "\"\n *\n" +
				" *\t DO NOT EDIT THIS FILE!\n */\n" +
				"package vnmr.apt;\n\n" + "class ProbeIdWrapper {\n" +
				"\tpublic static String algorithm = \"" + algorithm + "\";\n" +
				"\tpublic static byte[] key = {\n\t" + byteString + "\n\t};\n}\n"; 
			boots.write(keybooter);
			boots.close();
		} catch (FileNotFoundException e) {
			ProbeIdIO.error("Wrapper file \""+bootFile.getPath()+"\" not found");
		} catch (IOException e) {
			ProbeIdIO.error("I/O error writing wrapper file "+bootFile.getPath());
		}
	}

	/**
	 * Key Generator and Unit Tests
	 * @usage ProbeIdCrypto [-bootstrap] [algorithm]
	 * @param args
	 */
	public static void main(String[] args) {
		String algorithm = null;
		String bootStrapper = "src.generated/ProbeIdWrapper.java";
		boolean bootstrap = false;
		
		try {
			@SuppressWarnings("unused")
            SecretKeyFactory f = SecretKeyFactory.getInstance("PBEWithMD5AndDES");
		} catch (NoSuchAlgorithmException e) {
			System.err.println("Invalid algorithm "+e.getMessage());
			e.printStackTrace();
		}
		
		// extract algorithm
		for (String arg : args) {
			if (arg.startsWith("-")) {
				// generate the ProbeIdWrapper.java file
				if (arg.equals("-bootkey")) {
					bootstrap = true;
				}
			} else algorithm = arg;
		}

		if (bootstrap) {
			genWrapper(bootStrapper, algorithm);
			System.out.println(bootStrapper);
			return;
		}
		
		File keyFile = getKeyFile();
		keyFile.delete();
		
		for (Integer i=0; i<2; i++)	try {
			// TODO: consolidate this in a few test support functions
			// that take a message and encrypt it in a temporary file that
			// returns a buf[] of encrypted data.  Or a StringStream?
			
			// generate the encrypted message
			ProbeIdCrypto probeId = new ProbeIdCrypto(algorithm);
			File file = File.createTempFile("ProbeIdCryptoTest", ".bin");
			OutputStream output = new FileOutputStream(file);
			OutputStream writer = probeId.getBufferedWriter(output);
			String original = "hello world!";
			writer.write(original.getBytes());
			writer.close();
			
			// recover the encrypted message
			byte[] in = new byte[100]; 
			byte[] raw = new byte[100];
			InputStream input = new FileInputStream(file);
			InputStream decrypt = probeId.getBufferedReader(input);
			InputStream encrypted = new FileInputStream(file);
			@SuppressWarnings("unused") // retain for debugging
			int bytes_decrypted = decrypt.read(in);
			@SuppressWarnings("unused") // retain for debugging
			int bytes_read = encrypted.read(raw);
			String msg_in = new String(in).trim();
			String msg_raw = new String(raw).trim();
			//System.out.println(msg_in);
			decrypt.close();
			encrypted.close();
			assert !original.equals(msg_raw);
			assert original.equals(msg_in);
			System.out.println("Unit Test " + i.toString() + " passed");
		} catch (FileNotFoundException e) {
			e.printStackTrace();
		} catch (IOException e) {
			e.printStackTrace();
		}
	}
}
