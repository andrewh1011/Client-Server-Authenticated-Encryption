#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>

int gcm_decrypt(unsigned char *ciphertext, int ciphertext_len,
		unsigned char *aad, int aad_len,
		unsigned char *tag,
		unsigned char *key,
		unsigned char *iv, int iv_len,
		unsigned char *plaintext)
{
	EVP_CIPHER_CTX *ctx;
	int len, plaintext_len, ret;

	// Initialize context
	ctx = EVP_CIPHER_CTX_new();
	if (!ctx) {
		return -2;
	}

	// Initialize encryption
	if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL) != 1) {
		EVP_CIPHER_CTX_free(ctx);
		return -3;
	}


	// Set IV length
	if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, iv_len, NULL) != 1) {
		EVP_CIPHER_CTX_free(ctx);
		return -4;
	}

	// Initialize key and IV
	if (EVP_DecryptInit_ex(ctx, NULL, NULL, key, iv) != 1) { 
		EVP_CIPHER_CTX_free(ctx);
		return -5;
	}

	// Provide aad data
	if (EVP_DecryptUpdate(ctx, NULL, &len, aad, aad_len) != 1) {
		EVP_CIPHER_CTX_free(ctx);
		return -6;
	}

	// Provide the message to be encrypted, and obtain the encrypted output
	if (EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len) != 1) {
		EVP_CIPHER_CTX_free(ctx);
		return -7;
	}
	plaintext_len = len;

	// Set tag value
	if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, tag) != 1) {
		EVP_CIPHER_CTX_free(ctx);
		return -8;
	}

	// Finalize
	ret = EVP_DecryptFinal_ex(ctx, plaintext + len, &len);

	// Clean up
	EVP_CIPHER_CTX_free(ctx);

	if(ret > 0) {
		// Success
		plaintext_len += len;
		return plaintext_len;
	} else {
		// Verify failed
		return -1;
	}
}

int main(void)
{
	// Declare variables
	struct sockaddr_in my_addr, client_addr;
	char server_ip[] = "10.9.0.5"; //IP address of HostA
	int server_port = 9090;

	// Create socket
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0) {
		printf("%s\n", "Socket creation error");
		return -1;
	}

	// Server information
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(server_port);
	my_addr.sin_addr.s_addr = inet_addr(server_ip);

	// Bind to the set port and ip
	int res = bind(sockfd, (struct sockaddr*)&my_addr, sizeof(struct sockaddr_in));
	if (res < 0) {
		printf("%s\n", "Bind error");
		return -2;
	}
	printf("Done with binding with IP: %s, Port: %d\n", server_ip, server_port);

	// Listen for clients
	res = listen(sockfd, 5);
	if (res < 0) {
		printf("%s\n", "Listen error");
		return -3;
	}

	// Accept an incoming connection
	int client_length = sizeof(client_addr);
	int newsockfd = accept(sockfd, (struct sockaddr*) &client_addr, &client_length);
	if (newsockfd < 0) {
		printf("%s\n", "Accept error");
		return -4;
	}


	// Receive client's message
	unsigned char message_buffer[1024];
	memset(message_buffer, 0, sizeof(message_buffer));
	int message_len = recv(newsockfd, message_buffer, sizeof(message_buffer), 0);
	if (message_len < 0) {
		printf("%s\n", "Read error");
		return -5;
	}
	printf("Raw message from client: ");
	for (int i = 0; i < strlen(message_buffer); i++) {
	printf("%02x", message_buffer[i]);
	}
	printf("\n");

	// Receive client's tag
	unsigned char tag_buffer[16];
	int tag_len = 16;
	memset(tag_buffer, 0, tag_len);
	int tag = recv(newsockfd, tag_buffer, tag_len, 0);
	if (tag < 0) {
		printf("%s\n", "Read error");
		return -5;
	}
	printf("Authentication tag: ");
	for (int i = 0; i < tag_len; i++) {
		printf("%02x", tag_buffer[i]);
	}
	printf("\n");

	// Setup decryption variables
	unsigned char plaintext[1024];
	unsigned char key[32] = "01234567890123456789012345678901";
	unsigned char iv[12] = "012345678901";
	int iv_len = 12;

	unsigned char aad[] = "Using GCM Authentication";
	int aad_len = strlen((char*)aad);

	// Verify and decrypt
	int plaintext_len = gcm_decrypt(message_buffer, message_len, 
								aad, aad_len,
								tag_buffer,
								key, iv, iv_len,
								plaintext);
		
	if (plaintext_len == -1) {
		// Decrpytion function returns -1 if verification failed
		printf("%s\n", "VERIFICATION FAILED");
		return -6;
	} else if (plaintext_len < -1) {
		printf("%s\n", "Decryption error");
		return -7;
	}

	// Verification AND decryption were successful
	plaintext[plaintext_len] = '\0';
	printf("Decrypted plaintext: %s\n", (char*)plaintext);

	// Respond to client
	char server_message[] = "Hello client!";
	int s = send(newsockfd, server_message, sizeof(server_message), 0);
	if (s < 0) {
		printf("%s\n", "Send error");
		return -8;
	}

	// Close the sockets
	close(sockfd);
	close(newsockfd);

	return 0;
}

