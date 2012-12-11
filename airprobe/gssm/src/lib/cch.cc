// $Id: cch.cc,v 1.1.1.1 2007-06-01 04:26:57 jl Exp $

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#include <exception>
#include <stdexcept>

#include "burst_types.h"
#include "cch.h"

/*
 * GSM SACCH -- Slow Associated Control Channel
 *
 * These messages are encoded exactly the same as on the BCCH.
 * (Broadcast Control Channel.)
 *
 * 	Input: 184 bits
 * 	
 * 	1. Add parity and flushing bits.
 * 	2. Convolutional encode.
 * 	3. Interleave.
 * 	4. Map on bursts.
 */


/*
 * Parity (FIRE) for the GSM SACCH channel.
 *
 * 	g(x) = (x^23 + 1)(x^17 + x^3 + 1)
 * 	     = x^40 + x^26 + x^23 + x^17 + x^3 + 1
 */

static const unsigned char parity_polynomial[PARITY_SIZE + 1] = {
   1, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 1, 0,
   0, 1, 0, 0, 0, 0, 0, 1,
   0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 1, 0, 0,
   1
};

// remainder after dividing data polynomial by g(x)
static const unsigned char parity_remainder[PARITY_SIZE] = {
   1, 1, 1, 1, 1, 1, 1, 1,
   1, 1, 1, 1, 1, 1, 1, 1,
   1, 1, 1, 1, 1, 1, 1, 1,
   1, 1, 1, 1, 1, 1, 1, 1,
   1, 1, 1, 1, 1, 1, 1, 1
};


/*
static void parity_encode(unsigned char *d, unsigned char *p) {

	int i;
	unsigned char buf[DATA_BLOCK_SIZE + PARITY_SIZE], *q;

	memcpy(buf, d, DATA_BLOCK_SIZE);
	memset(buf + DATA_BLOCK_SIZE, 0, PARITY_SIZE);

	for(q = buf; q < buf + DATA_BLOCK_SIZE; q++)
		if(*q)
			for(i = 0; i < PARITY_SIZE + 1; i++)
				q[i] ^= parity_polynomial[i];
	for(i = 0; i < PARITY_SIZE; i++)
		p[i] = !buf[DATA_BLOCK_SIZE + i];
}
 */


static int parity_check(unsigned char *d) {

	unsigned int i;
	unsigned char buf[DATA_BLOCK_SIZE + PARITY_SIZE], *q;

	memcpy(buf, d, DATA_BLOCK_SIZE + PARITY_SIZE);

	for(q = buf; q < buf + DATA_BLOCK_SIZE; q++)
		if(*q)
			for(i = 0; i < PARITY_SIZE + 1; i++)
				q[i] ^= parity_polynomial[i];
	return memcmp(buf + DATA_BLOCK_SIZE, parity_remainder, PARITY_SIZE);
}


/*
 * Convolutional encoding and Viterbi decoding for the GSM SACCH channel.
 */

/*
 * Convolutional encoding:
 *
 *	G_0 = 1 + x^3 + x^4
 *	G_1 = 1 + x + x^3 + x^4
 *
 * i.e.,
 *
 *	c_{2k} = u_k + u_{k - 3} + u_{k - 4}
 *	c_{2k + 1} = u_k + u_{k - 1} + u_{k - 3} + u_{k - 4}
 */
#define K		5
#define MAX_ERROR	(2 * CONV_INPUT_SIZE + 1)


/*
 * Given the current state and input bit, what are the output bits?
 *
 * 	encode[current_state][input_bit]
 */
static const unsigned int encode[1 << (K - 1)][2] = {
	{0, 3}, {3, 0}, {3, 0}, {0, 3},
	{0, 3}, {3, 0}, {3, 0}, {0, 3},
	{1, 2}, {2, 1}, {2, 1}, {1, 2},
	{1, 2}, {2, 1}, {2, 1}, {1, 2}
};


/*
 * Given the current state and input bit, what is the next state?
 * 
 * 	next_state[current_state][input_bit]
 */
static const unsigned int next_state[1 << (K - 1)][2] = {
	{0, 8}, {0, 8}, {1, 9}, {1, 9},
	{2, 10}, {2, 10}, {3, 11}, {3, 11},
	{4, 12}, {4, 12}, {5, 13}, {5, 13},
	{6, 14}, {6, 14}, {7, 15}, {7, 15}
};


/*
 * Given the previous state and the current state, what input bit caused
 * the transition?  If it is impossible to transition between the two
 * states, the value is 2.
 *
 * 	prev_next_state[previous_state][current_state]
 */
static const unsigned int prev_next_state[1 << (K - 1)][1 << (K - 1)] = {
        { 0,  2,  2,  2,  2,  2,  2,  2,  1,  2,  2,  2,  2,  2,  2,  2},
        { 0,  2,  2,  2,  2,  2,  2,  2,  1,  2,  2,  2,  2,  2,  2,  2},
        { 2,  0,  2,  2,  2,  2,  2,  2,  2,  1,  2,  2,  2,  2,  2,  2},
        { 2,  0,  2,  2,  2,  2,  2,  2,  2,  1,  2,  2,  2,  2,  2,  2},
        { 2,  2,  0,  2,  2,  2,  2,  2,  2,  2,  1,  2,  2,  2,  2,  2},
        { 2,  2,  0,  2,  2,  2,  2,  2,  2,  2,  1,  2,  2,  2,  2,  2},
        { 2,  2,  2,  0,  2,  2,  2,  2,  2,  2,  2,  1,  2,  2,  2,  2},
        { 2,  2,  2,  0,  2,  2,  2,  2,  2,  2,  2,  1,  2,  2,  2,  2},
        { 2,  2,  2,  2,  0,  2,  2,  2,  2,  2,  2,  2,  1,  2,  2,  2},
        { 2,  2,  2,  2,  0,  2,  2,  2,  2,  2,  2,  2,  1,  2,  2,  2},
        { 2,  2,  2,  2,  2,  0,  2,  2,  2,  2,  2,  2,  2,  1,  2,  2},
        { 2,  2,  2,  2,  2,  0,  2,  2,  2,  2,  2,  2,  2,  1,  2,  2},
        { 2,  2,  2,  2,  2,  2,  0,  2,  2,  2,  2,  2,  2,  2,  1,  2},
        { 2,  2,  2,  2,  2,  2,  0,  2,  2,  2,  2,  2,  2,  2,  1,  2},
        { 2,  2,  2,  2,  2,  2,  2,  0,  2,  2,  2,  2,  2,  2,  2,  1},
        { 2,  2,  2,  2,  2,  2,  2,  0,  2,  2,  2,  2,  2,  2,  2,  1}
};


static inline unsigned int hamming_distance2(unsigned int w) {

	return (w & 1) + !!(w & 2);
}


/*
static void conv_encode(unsigned char *data, unsigned char *output) {

	unsigned int i, state = 0, o;

	// encode data
	for(i = 0; i < CONV_INPUT_SIZE; i++) {
		o = encode[state][data[i]];
		state = next_state[state][data[i]];
		*output++ = !!(o & 2);
		*output++ = o & 1;
	}
}
 */


static int conv_decode(unsigned char *output, unsigned char *data) {

	int i, t;
	unsigned int rdata, state, nstate, b, o, distance, accumulated_error,
	   min_state, min_error, cur_state;

	unsigned int ae[1 << (K - 1)];
	unsigned int nae[1 << (K - 1)]; // next accumulated error
	unsigned int state_history[1 << (K - 1)][CONV_INPUT_SIZE + 1];

	// initialize accumulated error, assume starting state is 0
	for(i = 0; i < (1 << (K - 1)); i++)
		ae[i] = nae[i] = MAX_ERROR;
	ae[0] = 0;

	// build trellis
	for(t = 0; t < CONV_INPUT_SIZE; t++) {

		// get received data symbol
		rdata = (data[2 * t] << 1) | data[2 * t + 1];

		// for each state
		for(state = 0; state < (1 << (K - 1)); state++) {

			// make sure this state is possible
			if(ae[state] >= MAX_ERROR)
				continue;

			// find all states we lead to
			for(b = 0; b < 2; b++) {

				// get next state given input bit b
				nstate = next_state[state][b];

				// find output for this transition
				o = encode[state][b];

				// calculate distance from received data
				distance = hamming_distance2(rdata ^ o);

				// choose surviving path
				accumulated_error = ae[state] + distance;
				if(accumulated_error < nae[nstate]) {

					// save error for surviving state
					nae[nstate] = accumulated_error;

					// update state history
					state_history[nstate][t + 1] = state;
				}
			}
		}
		
		// get accumulated error ready for next time slice
		for(i = 0; i < (1 << (K - 1)); i++) {
			ae[i] = nae[i];
			nae[i] = MAX_ERROR;
		}
	}

	// the final state is the state with the fewest errors
	min_state = (unsigned int)-1;
	min_error = MAX_ERROR;
	for(i = 0; i < (1 << (K - 1)); i++) {
		if(ae[i] < min_error) {
			min_state = i;
			min_error = ae[i];
		}
	}

	// trace the path
	cur_state = min_state;
	for(t = CONV_INPUT_SIZE; t >= 1; t--) {
		min_state = cur_state;
		cur_state = state_history[cur_state][t]; // get previous
		output[t - 1] = prev_next_state[cur_state][min_state];
	}

	// return the number of errors detected (hard-decision)
	return min_error;
}


/*
 * GSM SACCH interleaving and burst mapping
 *
 * Interleaving:
 *
 * Given 456 coded input bits, form 4 blocks of 114 bits:
 *
 * 	i(B, j) = c(n, k)	k = 0, ..., 455
 * 				n = 0, ..., N, N + 1, ...
 * 				B = B_0 + 4n + (k mod 4)
 * 				j = 2(49k mod 57) + ((k mod 8) div 4)
 *
 * Mapping on Burst:
 *
 * 	e(B, j) = i(B, j)
 * 	e(B, 59 + j) = i(B, 57 + j)	j = 0, ..., 56
 * 	e(B, 57) = h_l(B)
 * 	e(B, 58) = h_n(B)
 *
 * Where h_l(B) and h_n(B) are bits in burst B indicating flags.
 */

/*
static void interleave(unsigned char *data, unsigned char *iBLOCK) {

	int j, k, B;

	// for each bit in input data
	for(k = 0; k < CONV_SIZE; k++) {
		B = k % 4;
		j = 2 * ((49 * k) % 57) + ((k % 8) / 4);
		iBLOCK[B * iBLOCK_SIZE + j] = data[k];
	}
}
 */


static void decode_interleave(unsigned char *data, unsigned char *iBLOCK) {

	int j, k, B;

	for(k = 0; k < CONV_SIZE; k++) {
		B = k % 4;
		j = 2 * ((49 * k) % 57) + ((k % 8) / 4);
		data[k] = iBLOCK[B * iBLOCK_SIZE + j];
	}
}


/*
static void burstmap(unsigned char *iBLOCK, unsigned char *eBLOCK,
   unsigned char hl, unsigned char hn) {

	int j;

	for(j = 0; j < 57; j++) {
		eBLOCK[j] = iBLOCK[j];
		eBLOCK[j + 59] = iBLOCK[j + 57];
	}
	eBLOCK[57] = hl;
	eBLOCK[58] = hn;
}
 */


static void decode_burstmap(unsigned char *iBLOCK, unsigned char *eBLOCK,
   unsigned char *hl, unsigned char *hn) {

	int j;

	for(j = 0; j < 57; j++) {
		iBLOCK[j] = eBLOCK[j];
		iBLOCK[j + 57] = eBLOCK[j + 59];
	}
	*hl = eBLOCK[57];
	*hn = eBLOCK[58];
}


/*
 * Transmitted bits are sent least-significant first.
 */
static int compress_bits(unsigned char *dbuf, unsigned int dbuf_len,
   unsigned char *sbuf, unsigned int sbuf_len) {

	unsigned int i, j, c, pos = 0;

	if(dbuf_len < ((sbuf_len + 7) >> 3))
		return -1;

	for(i = 0; i < sbuf_len; i += 8) {
		for(j = 0, c = 0; (j < 8) && (i + j < sbuf_len); j++)
			c |= (!!sbuf[i + j]) << j;
		dbuf[pos++] = c & 0xff;
	}
	return pos;
}


int get_ns_l3_len(unsigned char *data, unsigned int datalen) {

	if((data[0] & 3) != 1) {
		fprintf(stderr, "error: get_ns_l3_len: pseudo-length reserved "
		   "bits bad (%2.2x)\n", data[0] & 3);
		return -1;
	}
	return (data[0] >> 2);
}


static unsigned char *decode_sacch(unsigned char *e0, unsigned char *e1,
   unsigned char *e2, unsigned char *e3, unsigned int *datalen) {

	int errors, len, data_size = (DATA_BLOCK_SIZE + 7) >> 3;
	unsigned char conv_data[CONV_SIZE], iBLOCK[BLOCKS][iBLOCK_SIZE],
	   hl, hn, decoded_data[PARITY_OUTPUT_SIZE], *data;

	if(!(data = (unsigned char *)malloc(data_size))) {
		throw std::runtime_error("error: decode_cch: malloc");
	}

	if(datalen)
		*datalen = 0;

	// unmap the bursts
	decode_burstmap(iBLOCK[0], e0, &hl, &hn); // XXX ignore stealing bits
	decode_burstmap(iBLOCK[1], e1, &hl, &hn);
	decode_burstmap(iBLOCK[2], e2, &hl, &hn);
	decode_burstmap(iBLOCK[3], e3, &hl, &hn);

	// remove interleave
	decode_interleave(conv_data, (unsigned char *)iBLOCK);

	// Viterbi decode
	errors = conv_decode(decoded_data, conv_data);

	// check parity
	if(errors || parity_check(decoded_data)) {
		// fprintf(stderr, "error: sacch: parity error (%d)\n", errors);
		return 0;
	}
	if((len = compress_bits(data, data_size, decoded_data,
	   DATA_BLOCK_SIZE)) < 0) {
		fprintf(stderr, "error: compress_bits\n");
		return 0;
	}
	if((unsigned int)len < data_size) {
		fprintf(stderr, "error: buf too small (%d < %d)\n",
		   sizeof(data), len);
		return 0;
	}

	if(datalen)
		*datalen = (unsigned int)len;
	return data;
}


/*
 * decode_cch
 *
 * 	Decode a "common" control channel.  Most control channels use
 * 	the same burst, interleave, Viterbi and parity configuration.
 * 	The documentation for the control channels defines SACCH first
 * 	and then just keeps referring to that.
 *
 * 	The current (investigated) list is as follows:
 *
 * 		BCCH Norm
 * 		BCCH Ext
 * 		PCH
 * 		AGCH
 * 		CBCH (SDCCH/4)
 * 		CBCH (SDCCH/8)
 * 		SDCCH/4
 * 		SACCH/C4
 * 		SDCCH/8
 * 		SACCH/C8
 *
 * 	We provide two functions, one for where all four bursts are
 * 	contiguous, and one where they aren't.
 */
unsigned char *decode_cch(unsigned char *e0, unsigned char *e1,
   unsigned char *e2, unsigned char *e3, unsigned int *datalen) {

	return decode_sacch(e0, e1, e2, e3, datalen);
}


unsigned char *decode_cch(unsigned char *e, unsigned int *datalen) {

	return decode_sacch(e, e + eBLOCK_SIZE, e + 2 * eBLOCK_SIZE,
	   e + 3 * eBLOCK_SIZE, datalen);
}
