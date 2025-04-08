#pragma once

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include "emp-tool/io/io_channel.h"
using std::string;


#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

namespace emp {

class TLSNetIO: public IOChannel<TLSNetIO> { public:
	bool is_server;
	int mysocket = -1;
	int consocket = -1;
	// FILE * stream = nullptr;
	char * buffer = nullptr;
	bool has_sent = false;
	string addr;
	int port;
	SSL_CTX *ctx;
	SSL *ssl;
	BIO *buf_bio;

	TLSNetIO(const char * address, int port, std::string trusted_cert_path, bool quiet = false) {
		if (port <0 || port > 65535) {
			throw std::runtime_error("Invalid port number!");
		}

		this->port = port;
		is_server = false;

		// client-specific
		ctx = SSL_CTX_new(TLS_client_method());
		if (ctx == nullptr) {
			ERR_print_errors_fp(stderr);
			exit(1);
		}

		SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);
		if (SSL_CTX_load_verify_file(ctx, trusted_cert_path.c_str()) <= 0) {
			ERR_print_errors_fp(stderr);
			exit(1);
		}

		addr = string(address);

		struct sockaddr_in dest;
		memset(&dest, 0, sizeof(dest));
		dest.sin_family = AF_INET;
		dest.sin_addr.s_addr = inet_addr(address);
		dest.sin_port = htons(port);

		while(1) {
			consocket = socket(AF_INET, SOCK_STREAM, 0);

			if (connect(consocket, (struct sockaddr *)&dest, sizeof(struct sockaddr)) == 0) {
				break;
			}

			close(consocket);
			usleep(1000);
		}

		// wrap consocket in BIO
		// BIO *raw_bio = BIO_new(BIO_s_socket());
		// BIO_set_fd(raw_bio, consocket, BIO_NOCLOSE); // TODO maybe BIO_CLOSE

		// set up SSL
		if ((ssl = SSL_new(ctx)) == NULL) {
			ERR_print_errors_fp(stderr);
			// BIO_free(raw_bio);
                        throw std::runtime_error("Error creating SSL handle for new connection");
		}
		if (!SSL_set_fd(ssl, consocket)) {
			ERR_print_errors_fp(stderr);
			exit(EXIT_FAILURE);
		}
		// SSL_set_bio(ssl, raw_bio, raw_bio);

		// perform handshake
		if (SSL_connect(ssl) <= 0) {
			ERR_print_errors_fp(stderr);
			SSL_free(ssl);
                        throw std::runtime_error("Error performing SSL handshake with server");
		}

		// wrap SSL in BIO
		BIO *ssl_bio = BIO_new(BIO_f_ssl());
		BIO_set_ssl(ssl_bio, ssl, BIO_NOCLOSE);

		// general
		set_nodelay();
		// stream = fdopen(consocket, "wb+");
		// buffer = new char[NETWORK_BUFFER_SIZE];
		// memset(buffer, 0, NETWORK_BUFFER_SIZE);
		// setvbuf(stream, buffer, _IOFBF, NETWORK_BUFFER_SIZE);

		buf_bio = BIO_new(BIO_f_buffer());
		BIO_set_buffer_size(buf_bio, NETWORK_BUFFER_SIZE);
		BIO_push(buf_bio, ssl_bio);

		if(!quiet)
			std::cout << "connected\n";
	}

	TLSNetIO(int port, std::string certificate_chain_file, std::string private_key_file, bool quiet = false) {
		if (port <0 || port > 65535) {
			throw std::runtime_error("Invalid port number!");
		}

		this->port = port;
		is_server = true;

		// server-specific
		ctx = SSL_CTX_new(TLS_server_method());
		if (ctx == nullptr) {
			ERR_print_errors_fp(stderr);
			exit(1);
		}

		if (SSL_CTX_use_certificate_chain_file(ctx, certificate_chain_file.c_str()) <= 0) {
			SSL_CTX_free(ctx);
			ERR_print_errors_fp(stderr);
			perror("Failed to load the server certificate chain file");
			exit(1);
		}

		if (SSL_CTX_use_PrivateKey_file(ctx, private_key_file.c_str(), SSL_FILETYPE_PEM) <= 0) {
			SSL_CTX_free(ctx);
			ERR_print_errors_fp(stderr);
			perror("Error loading the server private key file, "
			       "possible key/cert mismatch???");
		}

		SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);

		struct sockaddr_in dest;
		struct sockaddr_in serv;
		socklen_t socksize = sizeof(struct sockaddr_in);
		memset(&serv, 0, sizeof(serv));
		serv.sin_family = AF_INET;
		serv.sin_addr.s_addr = htonl(INADDR_ANY); /* set our address to any interface */
		serv.sin_port = htons(port);           /* set the server port number */
		mysocket = socket(AF_INET, SOCK_STREAM, 0);
		int reuse = 1;
		setsockopt(mysocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse));
		if(bind(mysocket, (struct sockaddr *)&serv, sizeof(struct sockaddr)) < 0) {
			perror("error: bind");
			exit(1);
		}
		if(listen(mysocket, 1) < 0) {
			perror("error: listen");
			exit(1);
		}
		consocket = accept(mysocket, (struct sockaddr *)&dest, &socksize);
		// close(mysocket);

		// wrap consocket in BIO
		// BIO *raw_bio = BIO_new(BIO_s_socket());
		// BIO_set_fd(raw_bio, consocket, BIO_NOCLOSE); // TODO maybe BIO_CLOSE

		// set up SSL
		if ((ssl = SSL_new(ctx)) == NULL) {
			ERR_print_errors_fp(stderr);
			// BIO_free(raw_bio);
                        throw std::runtime_error("Error creating SSL handle for new connection");
		}
		if (!SSL_set_fd(ssl, consocket)) {
			ERR_print_errors_fp(stderr);
			exit(EXIT_FAILURE);
		}
		// SSL_set_bio(ssl, raw_bio, raw_bio);

		// perform handshake
		if (SSL_accept(ssl) <= 0) {
			ERR_print_errors_fp(stderr);
			SSL_free(ssl);
                        throw std::runtime_error("Error performing SSL handshake with client");
		}

		// wrap SSL in BIO
		BIO *ssl_bio = BIO_new(BIO_f_ssl());
		BIO_set_ssl(ssl_bio, ssl, BIO_NOCLOSE);

		// general
		set_nodelay();
		// stream = fdopen(consocket, "wb+");
		// buffer = new char[NETWORK_BUFFER_SIZE];
		// memset(buffer, 0, NETWORK_BUFFER_SIZE);
		// setvbuf(stream, buffer, _IOFBF, NETWORK_BUFFER_SIZE);

		buf_bio = BIO_new(BIO_f_buffer());
		BIO_set_buffer_size(buf_bio, NETWORK_BUFFER_SIZE);
		BIO_push(buf_bio, ssl_bio);

		if(!quiet)
			std::cout << "connected\n";
	}

	void sync() {
		int tmp = 0;
		if(is_server) {
			send_data_internal(&tmp, 1);
			recv_data_internal(&tmp, 1);
		} else {
			recv_data_internal(&tmp, 1);
			send_data_internal(&tmp, 1);
			flush();
		}
	}

	~TLSNetIO(){
		flush();
		// fclose(stream);
		BIO_free(buf_bio);
		delete[] buffer;
	}

	void set_nodelay() {
		const int one=1;
		setsockopt(consocket,IPPROTO_TCP,TCP_NODELAY,&one,sizeof(one));
	}

	void set_delay() {
		const int zero = 0;
		setsockopt(consocket,IPPROTO_TCP,TCP_NODELAY,&zero,sizeof(zero));
	}

	void flush() {
		BIO_flush(buf_bio);
		// fflush(stream);
	}

	void send_data_internal(const void * data, size_t len) {
		size_t sent = 0;
		while(sent < len) {
			// size_t res = fwrite(sent + (char*)data, 1, len - sent, stream);
			size_t res = BIO_write(buf_bio, sent + (char*)data, len - sent);
			if (res > 0)
				sent+=res;
			else {
				error("net_send_data\n");
			}
		}
		has_sent = true;
	}

	void recv_data_internal(void  * data, size_t len) {
		if(has_sent) {
			// fflush(stream);
			BIO_flush(buf_bio);
		}
		has_sent = false;
		size_t sent = 0;
		while(sent < len) {
			// size_t res = fread(sent + (char*)data, 1, len - sent, stream);
			size_t res = BIO_read(buf_bio, sent + (char*)data, len - sent);
			if (res > 0)
				sent += res;
			else {
				error("net_recv_data\n");
			}
		}
	}
};

}
