#define OPENSSL_SUPPRESS_DEPRECATED
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <openssl/conf.h>
#include <openssl/err.h>
#include <openssl/hmac.h>

int gcm_encrypt(unsigned char *plaintext, int plaintext_len,
		unsigned char *aad, int aad_len,
		unsigned char *key,
		unsigned char *iv, int iv_len,
		unsigned char *ciphertext,
		unsigned char *tag)
{
	EVP_CIPHER_CTX *ctx;
	int len,ciphertext_len;

	// Initialize context
	ctx = EVP_CIPHER_CTX_new();
	if (!ctx) {
		return -1;
	}

	// Initialize encryption
	if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL) != 1) {
		EVP_CIPHER_CTX_free(ctx);
		return -2;
	}


	if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, iv_len, NULL) != 1) {
		EVP_CIPHER_CTX_free(ctx);	
		return -3;
	}

	// Initialize key and IV
	if (EVP_EncryptInit_ex(ctx, NULL, NULL, key, iv) != 1) {
		EVP_CIPHER_CTX_free(ctx);
		return -4;
	}

	// Provide aad data
	if (EVP_EncryptUpdate(ctx, NULL, &len, aad, aad_len) != 1) {
	EVP_CIPHER_CTX_free(ctx);
	return -5;
	}

	// Provide the message to be encrypted, and obtain the encrypted output
	if (EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len) != 1) {
		EVP_CIPHER_CTX_free(ctx);
		return -6;
	}
	ciphertext_len = len;

	// Finalize
	if (EVP_EncryptFinal_ex(ctx, ciphertext + len, &len) != 1) {
		EVP_CIPHER_CTX_free(ctx);
		return -7;
	}

	ciphertext_len += len;

	// Get the tag
	if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag) != 1) {
		EVP_CIPHER_CTX_free(ctx);
		return -8;
	}

	// Clean up
	EVP_CIPHER_CTX_free(ctx);
	return ciphertext_len;
}

int main(void)
{
	// Declare Variables
	struct sockaddr_in dest;
	char dest_ip[] = "10.9.0.5"; // IP of HostA
	int dest_port = 9090;
	char buffer[1024];

	// Create socket
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0) {
		printf("%s\n", "Socket creation error");
		return -1;
	}

	// Send connection request to server, be sure to set port and IP the same as server
	//side
	memset(&dest, 0, sizeof(struct sockaddr_in));
	dest.sin_family = AF_INET;
	dest.sin_addr.s_addr = inet_addr(dest_ip);
	dest.sin_port = htons(dest_port);
	int res = connect(sockfd, (struct sockaddr*)&dest, sizeof(struct sockaddr_in));
	if (res < 0) {
		printf("%s\n", "Connection error");
		return -2;
	}


	// Get input from the user:
	printf("Enter a message to send to the server: ");
	unsigned char user_input[1024];
	fgets(user_input, 1024, stdin);

	// Declare encryption variables
	unsigned char ciphertext[1024];
	unsigned char tag[16];
	int tag_len = 16;

	unsigned char key[32] = "01234567890123456789012345678901";
	unsigned char iv[12] = "012345678901";
	int iv_len = 12;

	unsigned char aad[] = "Using GCM Authentication";
	int aad_len = strlen((char*)aad);

	// Encrypt the message using GCM
	int ciphertext_len = gcm_encrypt(user_input, strlen(user_input), 
								aad, aad_len,
								key, iv, iv_len,
								ciphertext, tag);
									
	if (ciphertext_len < 0) {
		printf("%s\n", "Encryption error");
		return -3;
	} 
	printf("Created ciphertext: ");
	for (int i = 0; i < ciphertext_len; i++) {
		printf("%02x", ciphertext[i]);
	}
	printf("\n");
	printf("Authentication tag: ");
	for (int i = 0; i < 16; i++) {
		printf("%02x", tag[i]);
	}
	printf("\n");

	// Send the ciphertext to server:
	int s = send(sockfd, ciphertext, ciphertext_len, 0);
	if (s < 0) { 
		printf("%s\n", "Send error");
		return -4;
	}

	// Send the tag to the server
	s = send(sockfd, tag, tag_len, 0);
	if (s < 0) { 
		printf("%s\n", "Send error");
		return -4;
	}

		
	// Receive the server's response:
	memset(buffer, 0, sizeof(buffer));
	int rec = recv(sockfd, buffer, sizeof(buffer)-1, 0);
	if (rec < 0) {
		printf("%s\n", "Read error");
		return -5;
	}
	printf("Message from server: %s\n", buffer);

	// Close the socket
	close(sockfd);

	return 0;
}
