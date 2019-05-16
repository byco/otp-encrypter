# otp-encrypter
This C program encrypts and decrypts messages using a one-time pad-like implementation.

The user can type in plaintext that will be sent from client to the server, which encrypts the plaintext into ciphertext, using a randomly generated key. The user could also send decrypt ciphertext back into plaintext. The program functions using network sockets to send and receive bytes. Using child processes, it can handle five encryptions concurrently.

<h1>Instructions</h1>
You can run this on your own server. You will need to compile with <tt>compileall</tt>. You may need to <tt>chmod +x compileall</tt>

Then run the decrypter and encrypter files as background processes, picking two random port numbers.

<tt>otp_enc_d [portNumber1] &</tt>
<tt>otp_dec_d [portNumber2] &</tt>

First, write a plaintext file. This file will be used to encrypt.

You can then generate a key. Specify the length and stream it to a new text file.

<tt>keygen [key length] > [keyFile]</tt>

You're ready to encrypt now.

<tt>otp_enc [plaintextFile] [keyFile] [portNumber1] > [ciphertextFile]</tt>

You can decrypt the ciphertext using the key.

<tt>otp_dec [ciphertextFile] [keyFile] [portNumber2] > [newFile]</tt>
