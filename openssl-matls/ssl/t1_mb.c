
/*
 * Copyright 1995-2016 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the OpenSSL license (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

#include <stdio.h>
#include <stdlib.h>

#include "ssl_locl.h"
#include "tls1.h"
#include "logs.h"
#include "matls.h"

#include <openssl/objects.h>
#include <openssl/ssl.h>
#include <openssl/evp.h>
#include <openssl/ec.h>
#include <openssl/sha.h>
#include <openssl/bn.h>

#define SSL_CURVE_SECP256R1 23
#define SECP256r1_PUBKEY_LENGTH    64;
#define SECRET_LENGTH 32
#define CLIENT 0
#define SERVER 1

int idx;

#define PRINTK(msg, arg1, arg2) \
  fprintf(stderr, "[matls] %s: %s (%d bytes) \n", __func__, msg, arg2); \
  for (idx=0;idx<arg2;idx++) \
  { \
    if (idx % 10 == 0) \
      fprintf(stderr, "\n"); \
    fprintf(stderr, "%02X ", arg1[idx]); \
  } \
  fprintf(stderr, "\n");

int make_keypair(struct keypair **pair, EC_GROUP *group, BN_CTX *ctx) {
  MA_LOG("make keypair");
  BIGNUM *n = BN_new();
  EC_GROUP_get_order(group, n, ctx);

  (*pair) = (struct keypair *)malloc(sizeof(struct keypair));
  (*pair)->pri = BN_new();
  (*pair)->pub = EC_POINT_new(group);

  BN_rand_range((*pair)->pri, n); //private key
  EC_POINT_mul(group, (*pair)->pub, (*pair)->pri, NULL, NULL, ctx); //public key
  BIGNUM *x, *y;
  x = BN_new();
  y = BN_new();
  EC_POINT_get_affine_coordinates_GFp(group, (*pair)->pub, x, y, ctx);

  MA_LOG("end make keypair");
  return 1;
}

int char_to_pub(unsigned char *input, int key_length, EC_POINT *pubkey, EC_GROUP *group, BN_CTX *ctx)
{
    int klen = (key_length - 1)/ 2;
    unsigned char *xstr = (unsigned char *)malloc(klen); //klen+1?
    unsigned char *ystr = (unsigned char *)malloc(klen); //klen+1?

    BIGNUM *x, *y;
    
    memcpy(xstr, input + 1, klen);
    memcpy(ystr, input + klen + 1, klen);

    x = BN_new();
    y = BN_new();

    BN_bin2bn(xstr, klen, x);
    BN_bin2bn(ystr, klen, y);

    EC_POINT_set_affine_coordinates_GFp(group, pubkey, x, y, ctx);

    free(xstr);
    free(ystr);

    BN_free(x);
    BN_free(y);

    return 1;
}

int pub_to_char(EC_POINT *secret, unsigned char **secret_str, int *slen, EC_GROUP *group, BN_CTX *ctx)
{
  int key_bytes, ret;
  unsigned char *xstr, *ystr;

  if (EC_GROUP_get_curve_name(group) == NID_X9_62_prime256v1)
    key_bytes = 256 / 8;
  else
    return -1;

	*slen = 2 * key_bytes + 1;
    (*secret_str) = (unsigned char *)malloc(*slen);

	BIGNUM *x = BN_new();
	BIGNUM *y = BN_new();

	EC_POINT_get_affine_coordinates_GFp(group, secret, x, y, ctx);

  xstr = (unsigned char *)malloc(key_bytes);
  ystr = (unsigned char *)malloc(key_bytes);
	ret = BN_bn2bin(x, xstr);
	ret = BN_bn2bin(y, ystr);

	BN_free(x);
	BN_free(y);

  memset((*secret_str), 0x04, 1);
	memcpy((*secret_str) + 1, xstr, key_bytes);
	memcpy((*secret_str) + key_bytes + 1, ystr, key_bytes);

	OPENSSL_free(xstr);
	OPENSSL_free(ystr);
	
	return 1;
}


int handle_parse_errors() {
    //SSLerr(SSL_F_SSL_PARSE_CLIENTHELLO_MB_EXT, SSL_R_MB_ENCODING_ERR);
    //*al = SSL_AD_ILLEGAL_PARAMETER;
    printf("Error\n");
    return 0;
}

/* Add the client's mb */
int ssl_add_clienthello_mb_ext(SSL *s, unsigned char *p, int *len,
        int maxlen)
{
    MA_LOG("adding clienthello mb");

    int group_id;
    int num_keys;
    EC_GROUP *group;
    BN_CTX *ctx;
    unsigned char *pub_str;
    int pub_length, plen;
    int ext_len;
    struct keypair *keypair;

    MA_LOG("before if");

    if (p)
    {
      MA_LOG("Before waiting the message");
      while (!(s->pair && (s->pair->extension_from_clnt_msg > 0))) { printf(""); }
      MA_LOG("The client side pair has the extension message");
      memcpy(&(s->mb_info), &(s->pair->mb_info), sizeof(struct mb_st));
      group_id = s->mb_info.group_id;
      MA_LOG1d("group_id is set to", group_id);
      ctx = BN_CTX_new();

      switch(group_id)
      {
      case SSL_CURVE_SECP256R1:
        group = EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1);
        plen = pub_length = 2 * 256 / 8 + 1;
      default:
        group = EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1);
        plen = pub_length = 2 * 256 / 8 + 1;
      }

      MA_LOG1d("before memcpy", s->pair->extension_from_clnt_msg_len);
      memcpy(p, s->pair->extension_from_clnt_msg, s->pair->extension_from_clnt_msg_len);
      free(s->pair->extension_from_clnt_msg);

      MA_LOG("after free");

      MA_LOG1p("s->lock", s->lock);
      while (*(s->lock)) { printf(""); }
      *(s->lock) = 1;
      if (!(s->mb_info.keypair))
      {
        if (s->pair && s->pair->mb_info.keypair)
          s->mb_info.keypair = s->pair->mb_info.keypair;
        else
        {
          MA_LOG("before keypair");
          make_keypair(&keypair, group, ctx);
          MA_LOG("after keypair");
          s->mb_info.keypair = keypair;
          if (s->pair)
            s->pair->mb_info.keypair = keypair;
        }
      }
      *(s->lock) = 0;
      MA_LOG1d("lock value", *(s->lock));

      MA_LOG1p("keypair->pub", s->mb_info.keypair->pub);
      pub_to_char(s->mb_info.keypair->pub, &pub_str, &pub_length, group, ctx);
      MA_LOG1d("the length of public key", pub_length);

      n2s(p, ext_len);
      p -= 2;
      MA_LOG1d("ext_len before", ext_len);
      ext_len = ext_len + TYPE_LENGTH + META_LENGTH + pub_length;
      MA_LOG1d("ext_len after", ext_len);
      s2n(ext_len, p);
      p += 2; // Group ID
      num_keys = *p;
      MA_LOG1d("before number of keys", (int)*p);
      *p = num_keys + 1;
      MA_LOG1d("after number of keys", (int)*p);
      p++;
      p += s->pair->extension_from_clnt_msg_len - 5;

      if (s->server_side)
      {
        *p = TYPE_SERVER_SIDE;
        p++;
        *len = s->pair->extension_from_clnt_msg_len + TYPE_LENGTH + META_LENGTH + pub_length + META_LENGTH + s->proof_length;
      }
      else
      {
        *p = TYPE_CLIENT_SIDE;
        p++;
        *len = s->pair->extension_from_clnt_msg_len + TYPE_LENGTH + META_LENGTH + pub_length;
      }
      s2n(pub_length, p);
      MA_LOG1d("Added Public Key Length", pub_length);
      memcpy(p, pub_str, pub_length);
      p += pub_length;

      MA_LOG1d("s->pair->extension_from_clnt_msg_len", s->pair->extension_from_clnt_msg_len);
      MA_LOG1d("length of client hello extension", *len);
    }

    return 1;
}

/*
 * Parse the client's mb and abort if it's not right
 */
// This is the function for parsing the ClientHello message from the client
// The purpose is to check the intention of the client
// Input: SSL object, Extension Packet, Alert
// Output: 1 for Success, 0 for Failure
int ssl_parse_clienthello_mb_ext(SSL *s, unsigned char *d, int len, int *al)
{
  MA_LOG("Parse client hello");
    // SSL_F_SSL_PARSE_CLIENTHELLO_MB_EXT

    unsigned char *p;
    int i, slen, klen, nk, plen;  // klen: key length, nk: number of keys, plen: EC point length
    unsigned char *secret_str, *peer_str;
    struct keypair *keypair;
    EC_GROUP *group;
    BIGNUM *x, *y;
    EC_POINT *secret, *peer_pub;
    BN_CTX *ctx;

    MA_LOG1d("Read the mb length from the extension packet", len);
    /* Parse the length byte */
    if(len < 1)
    {
        return handle_parse_errors();
    }

    MA_LOG1d("Length", len);

    if (s->middlebox)
    {
      MA_LOG("Copy this extension message to my SSL struct (not to pair)");
      s->extension_from_clnt_msg = (unsigned char *)malloc(len);
      MA_LOG1p("after malloc", s->extension_from_clnt_msg);
      memcpy(s->extension_from_clnt_msg, d, len);
      MA_LOG1d("after memcpy", len);
      s->extension_from_clnt_msg_len = len;
    }
    p = d;
    int ext_len;
    n2s(p, ext_len);
    MA_LOG1d("Received Extension Length", ext_len);

    /* message: group_id(2bytes) + num_keys(1byte) + (key length(1byte) and key value) list */

    n2s(p,s->mb_info.group_id);

    MA_LOG1d("Group ID", s->mb_info.group_id);

    switch(s->mb_info.group_id)
    {
    case SSL_CURVE_SECP256R1:
      group = EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1);
      plen = 2 * 256 / 8 + 1;
    default:
      group = EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1);
      plen = 2 * 256 / 8 + 1;
    }

    /* Check num_keys */
    nk = s->mb_info.num_keys = *(p++);
    MA_LOG1d("Number of Keys (nk)", nk);

    if (s->middlebox) // middlebox, index 0: client->server, index 1: server->client
    {
      nk = 2;
      s->mb_info.key_length = (int *)calloc(2, sizeof(int));
      s->mb_info.secret = (unsigned char **)calloc(2, sizeof(unsigned char *));
      s->mb_info.mac_array = (unsigned char **)calloc(2, sizeof(unsigned char *));
    }
    else
    {
      s->mb_info.key_length = (int *)calloc(nk, sizeof(int));
      s->mb_info.secret = (unsigned char **)calloc(nk, sizeof(unsigned char *));
      s->mb_info.mac_array = (unsigned char **)calloc(nk, sizeof(unsigned char *));
    }

    for (i=0; i<nk; i++)
    {
      s->mb_info.mac_array[i] = (unsigned char *)malloc(SSL_MAX_GLOBAL_MAC_KEY_LENGTH);
    }
    MA_LOG1d("Number of Keys (nk, revised?)", nk);

    if(nk < 1)
    {
        return handle_parse_errors();
    }

    ctx = BN_CTX_new();
    x = BN_new();
    y = BN_new();

    if (s->middlebox)
    {
      while ((s->lock) && *(s->lock)) { printf(""); }
      *(s->lock) = 1;
      if (!(s->mb_info.keypair))
      {
        if (s->pair && s->pair->mb_info.keypair)
          s->mb_info.keypair = s->pair->mb_info.keypair;
        else
        {
          make_keypair(&keypair, group, ctx);
          s->mb_info.keypair = keypair;
          if (s->pair)
            s->pair->mb_info.keypair = keypair;
        }
      }
      *(s->lock) = 0;
    }
    else
    {
      make_keypair(&keypair, group, ctx);
      s->mb_info.keypair = keypair;
    }

    int end;
    if (s->middlebox)
      end = 1;
    else
      end = nk;

    for (i=0; i<end; i++)
    {
      secret = EC_POINT_new(group);

      if (*p != TYPE_CLIENT_SIDE)
      {
        MA_LOG1d("Wrong Type", *p);
      }

      p++;
      n2s(p, klen);

      s->mb_info.key_length[i] = klen;
      peer_str = (unsigned char *)malloc(klen);

      if (i == CLIENT)
      {
        s->mb_info.random[CLIENT] = (unsigned char *)malloc(klen);
        memcpy(s->mb_info.random[CLIENT], p, klen);
      }

      memcpy(peer_str, p, klen);
      p += klen;

      peer_pub = EC_POINT_new(group);
      char_to_pub(peer_str, klen, peer_pub, group, ctx);
      EC_POINT_mul(group, secret, NULL, peer_pub, keypair->pri, ctx);
      EC_POINT_get_affine_coordinates_GFp(group, secret, x, y, ctx);
      secret_str = (unsigned char *)malloc((klen-1)/2);
      BN_bn2bin(x, secret_str);
      s->mb_info.secret[i] = secret_str;

      free(peer_str);
      EC_POINT_free(secret);
      EC_POINT_free(peer_pub);
    }

    EC_GROUP_free(group);
    BN_CTX_free(ctx);
    s->mb_enabled = 1; // Enable the mb mode

    MA_LOG("MB Extension is enabled");

    return 1;
}

/* Add the server's mb */
// Add the mb extension in the ServerHello message
// If p is 0, it returns the length of the added message in the len
// Input: SSL object, buffer, length to be stored, maximum length
// Output: 1 for Success, 0 for Failure
int ssl_add_serverhello_mb_ext(SSL *s, unsigned char *p, int *len,
        int maxlen)
{
    // group_id (2 bytes) + num_keys (1 byte) + pubkey_len (1 byte) + pubkey (pubkey_len bytes)
  MA_LOG("adding serverhello mb");

	int group_id = s->mb_info.group_id;
	int num_keys = 1;
	EC_GROUP *group;
	BN_CTX *ctx;
	unsigned char *pub_str;
	int i, pub_length, plen, ext_len;
  struct keypair *keypair;

  switch(group_id)
  {
  case SSL_CURVE_SECP256R1:
    group = EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1);
    plen = pub_length = 2 * 256 / 8 + 1;
  default:
    group = EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1);
    plen = pub_length = 2 * 256 / 8 + 1;
  }

  ctx = BN_CTX_new();

  if (p) {
    MA_LOG1p("s->lock", s->lock);
    while (*(s->lock)) { printf(""); }
    *(s->lock) = 1;
    if (!(s->mb_info.keypair))
    {
      if (s->pair && s->pair->mb_info.keypair)
        s->mb_info.keypair = s->pair->mb_info.keypair;
      else
      {
        MA_LOG("before keypair");
        make_keypair(&keypair, group, ctx);
        MA_LOG("after keypair");
        s->mb_info.keypair = keypair;
        if (s->pair)
          s->pair->mb_info.keypair = keypair;
      }
    }
    *(s->lock) = 0;
    MA_LOG1d("lock value", *(s->lock));

    MA_LOG1p("keypair->pub", s->mb_info.keypair->pub);
    pub_to_char(s->mb_info.keypair->pub, &pub_str, &pub_length, group, ctx);
    MA_LOG1d("the length of public key", pub_length);

    if (s->middlebox)
    {
      int tmp1;
      MA_LOG("Before waiting the message");
      while (!(s->pair && (s->pair->extension_from_srvr_msg_len > 0))) { printf(""); }
      MA_LOG("The server side pair has the extension message");

      MA_LOG1d("before memcpy", s->pair->extension_from_srvr_msg_len);
      memcpy(p, s->pair->extension_from_srvr_msg, s->pair->extension_from_srvr_msg_len);
      free(s->pair->extension_from_srvr_msg);
      MA_LOG("after free");
      n2s(p, ext_len);
      MA_LOG1d("ext_len", ext_len);
      p -= 2;

	    if (s->server_side)
	    {
		    ext_len = ext_len + TYPE_LENGTH + META_LENGTH + pub_length + META_LENGTH + s->proof_length;
	    }
	    else
	    {
      	ext_len = ext_len + TYPE_LENGTH + META_LENGTH + pub_length;
	    }
	  
      s2n(ext_len, p);

      n2s(p, tmp1);
      MA_LOG1d("Received Group ID", tmp1);

      num_keys = *p;
      *p = num_keys + 1;
      MA_LOG1d("num keys after", (*p));
      p++;
      p += s->pair->extension_from_srvr_msg_len - 5;

	    if (s->server_side)
	    {
		    *p = TYPE_SERVER_SIDE;
		    p++;
		    *len = s->pair->extension_from_srvr_msg_len + TYPE_LENGTH + META_LENGTH + pub_length + META_LENGTH + s->proof_length;
	    }
	    else
	    {
		    *p = TYPE_CLIENT_SIDE;
		    p++;
		    *len = s->pair->extension_from_srvr_msg_len + TYPE_LENGTH + META_LENGTH + pub_length;
	    }

      s2n(pub_length, p);
      memcpy(p, pub_str, pub_length);
	    p += pub_length;

	    if (s->server_side)
	    {
		    s2n(s->proof_length, p);
		    memcpy(p, s->proof, s->proof_length);
	    }
    }
    else
    {
      ext_len = META_LENGTH + 1 + TYPE_LENGTH + META_LENGTH + pub_length;
      s2n(ext_len, p);
	    s2n(group_id, p);
	    *(p++) = num_keys;
	    *(p++) = TYPE_SERVER;
	    s2n(pub_length, p); //pubkey_len
	    memcpy(p, pub_str, pub_length); //pubkey
      p += pub_length;
      *len = META_LENGTH + META_LENGTH + 1 + TYPE_LENGTH + META_LENGTH + pub_length;
	  }

    SSL_register_id(s);
    PRINTK("Identifier", s->id->data, s->id->length);

    PRINTK("MB Pubkey", pub_str, pub_length);
    unsigned char *tmp = (unsigned char *)malloc(SECRET_LENGTH);

    if (s->middlebox)
    {
      SSL *tssl;
      for (i=0; i<2; i++)
      {
        if (i==0)
          MA_LOG("Client");
        else
          MA_LOG("Server");

        while (!s->mb_info.secret[i]) { printf(""); }
        memcpy(tmp, s->mb_info.secret[i], SECRET_LENGTH);

        t1_prf(TLS_MD_ACCOUNTABILITY_KEY_CONST, TLS_MD_ACCOUNTABILITY_KEY_CONST_SIZE,
                s->mb_info.random[SERVER], s->mb_info.key_length[SERVER],
                s->mb_info.random[CLIENT], s->mb_info.key_length[CLIENT],
                NULL, 0, NULL, 0,
                s->mb_info.secret[i], SECRET_LENGTH,
                s->mb_info.mac_array[i], SSL_MAX_ACCOUNTABILITY_KEY_LENGTH); //LENGTH: 32

        PRINTK("Server Random", s->mb_info.random[SERVER], s->mb_info.key_length[SERVER]);
        PRINTK("Client Random", s->mb_info.random[CLIENT], s->mb_info.key_length[CLIENT]);
        PRINTK("Secret", tmp, SECRET_LENGTH);
        PRINTK("Global MAC", s->mb_info.mac_array[i], SSL_MAX_ACCOUNTABILITY_KEY_LENGTH);
      }
    }
    else
    {
      for (i=0; i<s->mb_info.num_keys; i++)
      {
        memcpy(tmp, s->mb_info.secret[i], SECRET_LENGTH);

        t1_prf(TLS_MD_ACCOUNTABILITY_KEY_CONST, TLS_MD_ACCOUNTABILITY_KEY_CONST_SIZE,
                pub_str, pub_length,
                s->mb_info.random[CLIENT], s->mb_info.key_length[CLIENT],
                NULL, 0, NULL, 0,
                s->mb_info.secret[i], SECRET_LENGTH,
                s->mb_info.mac_array[i], SSL_MAX_ACCOUNTABILITY_KEY_LENGTH); //LENGTH: 32

        PRINTK("Server Random", pub_str, pub_length);
        PRINTK("Client Random", s->mb_info.random[CLIENT], s->mb_info.key_length[CLIENT]);
        PRINTK("Secret", tmp, SECRET_LENGTH);
        PRINTK("Global MAC", s->mb_info.mac_array[i], SSL_MAX_ACCOUNTABILITY_KEY_LENGTH);
      }
    }
  }

  MA_LOG("Set the length for the extension matls");

  MA_LOG1d("Complete Setting the length for the extension", *len);

  EC_GROUP_free(group);
  BN_CTX_free(ctx);

  return 1;
}

/*
 * Parse the server's mb and abort if it's not right
 */
int ssl_parse_serverhello_mb_ext(SSL *s, unsigned char *d, int size, int *al)
{
  unsigned char *p;
  unsigned char *secret_str, *peer_str;
  struct keypair *keypair;
  int plen, klen, ext_len, group_id, num_keys, type;
  EC_GROUP *group;
  BIGNUM *x, *y;
  EC_POINT *secret, *peer_pub;
  BN_CTX *ctx;

  if (size < 0)
  {
    return handle_parse_errors();
  }

  keypair = s->pair->mb_info.keypair;
//  s->extension_from_srvr_msg = (unsigned char *)malloc(size);
//  memcpy(s->extension_from_srvr_msg, d, size);
//  s->extension_from_srvr_msg_len = size;

  p = d;
  n2s(p, ext_len);
  MA_LOG1d("ext_len", ext_len);
  n2s(p, group_id);
  MA_LOG1d("Received Group ID", group_id);

  num_keys = *(p++);

  switch(s->pair->mb_info.group_id)
  {
    case SSL_CURVE_SECP256R1:
      group = EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1);
      plen = 2 * 256 / 8 + 1;
      break;
    default:
      group = EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1);
      plen = 2 * 256 / 8 + 1;
  }

  ctx = BN_CTX_new();
  x = BN_new();
  y = BN_new();

  secret = EC_POINT_new(group);
  type = *(p++);

  MA_LOG1d("Received Type", type);
  n2s(p, klen);
  MA_LOG1d("Received Server Key Length", klen);
  s->pair->mb_info.key_length[SERVER] = klen;

  peer_str = (unsigned char *)malloc(klen);
  s->pair->mb_info.random[SERVER] = (unsigned char *)malloc(klen);
  memcpy(peer_str, p, klen);
  PRINTK("Server DH Public", peer_str, klen);
  memcpy(s->pair->mb_info.random[SERVER], p, klen);
  p += klen;

  peer_pub = EC_POINT_new(group);
  char_to_pub(peer_str, klen, peer_pub, group, ctx);
  EC_POINT_mul(group, secret, NULL, peer_pub, keypair->pri, ctx);
  EC_POINT_get_affine_coordinates_GFp(group, secret, x, y, ctx);
  secret_str = (unsigned char *)malloc((klen-1)/2);
  BN_bn2bin(x, secret_str);
  s->pair->mb_info.secret[SERVER] = secret_str;

  s->extension_from_srvr_msg = (unsigned char *)malloc(size);
  memcpy(s->extension_from_srvr_msg, d, size);
  s->extension_from_srvr_msg_len = size;

  free(peer_str);
  EC_POINT_free(secret);
  EC_POINT_free(peer_pub);

  EC_GROUP_free(group);
  BN_CTX_free(ctx);

  return 1;
}
